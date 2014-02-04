#!/usr/bin/env python3

import os.path

ROOT = os.path.abspath(os.path.join(__file__, '..', '..'))
DATA = os.path.join(ROOT, 'data')

HEADER = os.path.join(ROOT, 'include', 'unicode.h')

def main(path=HEADER):
    white_space  = PropertyList('White_Space')
    xid_start    = PropertyList('XID_Start', derived=True)
    xid_continue = PropertyList('XID_Continue', derived=True)

    xid_continue_only = xid_continue - xid_start

    with open(path, 'w') as f:
        white_space.write_macro(f)

        f.write('\n\n')

        xid_start.write_macro(f)

        f.write('\n\n')

        xid_continue_only.write_macro(f, ignore={ord(c) for c in '0123456789'})

class PropertyList:
    def __init__(self, name, *, derived:bool=False):
        codes = []

        if derived:
            path = os.path.join(DATA, 'DerivedCoreProperties-3.2.0.txt')
        else:
            path = os.path.join(DATA, 'PropList-3.2.0.txt')

        with open(path, 'r') as f:
            for line in f:
                if not line:
                    continue

                if line[0] == '#':
                    continue

                try:
                    (code, _, property_name, _) = line.split(None, 3)
                except ValueError:
                    continue

                if property_name != name:
                    continue

                if len(code) < 6:
                    codes.append(int(code, 16))
                else:
                    (lower, upper) = code.split('..')

                    codes.extend(range(int(lower, 16), int(upper, 16) + 1))

        self.codes = codes
        self.name = name

    def write_macro(self, f, *, ignore=()):
        f.write('#define CASE__')
        f.write(self.name.upper())
        f.write(' \\\n')

        first = True
        for code in self.codes:
            if code in ignore:
                continue

            if first:
                first = False
            else:
                f.write(': \\\n')

            f.write('    case ')
            f.write(hex(code))

    def __sub__(self, other) -> 'PropertyList':
        codes = set(self.codes)
        codes.difference_update(other.codes)
        codes = sorted(codes)

        r = object.__new__(self.__class__)
        r.codes = codes
        r.name = self.name + '__EXCLUDING__' + other.name
        return r


if __name__ == '__main__':
    main()
