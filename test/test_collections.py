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

    def test_items(self):
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
