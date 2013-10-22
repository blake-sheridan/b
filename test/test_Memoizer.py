import unittest

import lazy

class Memoizer(unittest.TestCase):
    def test_getitem(self):
        count = 0

        def plus_two(x):
            nonlocal count
            count += 1
            return x + 2

        m = lazy.Memoizer(plus_two)

        self.assertEqual(m[5], 7)
        self.assertEqual(m[5], 7)
        self.assertEqual(count, 1)
