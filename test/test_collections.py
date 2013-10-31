import unittest

from b.collections import IdentityDict

class ConstantHash:
    def __eq__(self, other):
        return True

    def __hash__(self):
        return 22

class IdentityDictTests(unittest.TestCase):
    def test_sanity(self):
        d = IdentityDict()

        x = ConstantHash()
        y = ConstantHash()

        d[x] = 2
        d[y] = 3

        self.assertEqual(d[x], 2)
        self.assertEqual(d[y], 3)

        hash_d = dict()

        hash_d[x] = 2
        hash_d[y] = 3

        self.assertEqual(len(hash_d), 1)
        self.assertEqual(len(d), 2)

    def test_contains(self):
        d = IdentityDict()

        x = ConstantHash()
        y = ConstantHash()

        d[x] = 2
        d[y] = 3

        self.assertIn(x, d)
        self.assertIn(y, d)
        self.assertNotIn(ConstantHash(), d)

    def test_iter(self):
        LENGTH = 5

        d = IdentityDict()

        for i in range(LENGTH):
            d[ConstantHash()] = i

        count = 0
        for key in d:
            count += 1
            self.assertIsInstance(key, ConstantHash)

        self.assertEqual(count, LENGTH)

    def test_get(self):
        d = IdentityDict()

        key = ConstantHash()

        self.assertIs(d.get(key), None)

        d[key] = 5

        self.assertEqual(d.get(key), 5)

        self.assertEqual(d.get(ConstantHash(), 6), 6)

    def test_keys_iter(self):
        LENGTH = 5

        d = IdentityDict()

        for i in range(LENGTH):
            d[ConstantHash()] = i

        # Same code as __iter__, really,
        # both here and in the implementation.

        count = 0
        for key in d.keys():
            count += 1
            self.assertIsInstance(key, ConstantHash)

        self.assertEqual(count, LENGTH)

    def test_items_iter(self):
        LENGTH = 5

        d = IdentityDict()

        reverse_map = {}

        for i in range(LENGTH):
            key = ConstantHash()

            d[key] = i
            reverse_map[i] = key

        count = 0
        for key, value in d.items():
            count += 1

            self.assertIs(reverse_map[value], key)

        self.assertEqual(count, LENGTH)

    def test_values_iter(self):
        LENGTH = 5

        d = IdentityDict()

        reverse_map = {}

        for i in range(LENGTH):
            key = ConstantHash()

            d[key] = i
            reverse_map[i] = key

        count = 0
        for value in d.values():
            count += 1

            del reverse_map[value] # Failure if this KeyErrors

        self.assertEqual(count, LENGTH)

    def test_clear(self):
        LENGTH = 5

        deletions = 0

        class Tracked:
            def __del__(self):
                nonlocal deletions
                deletions += 1

        d = IdentityDict()

        for i in range(LENGTH):
            d[Tracked()] = i

        self.assertEqual(deletions, 0)
        self.assertEqual(len(d), LENGTH)

        d.clear()

        self.assertEqual(deletions, LENGTH)
        self.assertEqual(len(d), 0)

    def test_pop(self):
        LENGTH = 5

        d = IdentityDict()
        keys = []

        for i in range(LENGTH):
            key = ConstantHash()

            d[key] = i
            keys.append(key)

        self.assertEqual(d.pop(keys[2]), 2)
        self.assertEqual(d.pop(keys[4]), 4)

        self.assertEqual(len(d), LENGTH - 2)

        self.assertEqual(d.pop('not in there, bro', 'but ok'), 'but ok')

        with self.assertRaises(KeyError):
            d.pop('no fallback!!!!!')

    def test_resize(self):
        d = IdentityDict()

        # Current implementation detail:
        # INITIAL_SIZE = 16
        # resize at ~2/3

        for i in range(30):
            d[ConstantHash()] = i

        # Random code snippet which
        # cause core dump...
        for _ in d.items():
            pass
