import unittest

import lazy

class MemoizerTests(unittest.TestCase):
    def test_new(self):
        lazy.Memoizer(str)

        with self.assertRaises(TypeError):
            lazy.Memoizer("asdf")

        with self.assertRaises(TypeError):
            lazy.Memoizer(abc=123)

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

    def test_delete(self):
        created = 0
        deleted = 0

        class Item:
            def __init__(self):
                nonlocal created
                created += 1

            def __del__(self):
                nonlocal deleted
                deleted += 1

        def make_item(_):
            return Item()

        m = lazy.Memoizer(make_item)

        m[1]
        m[2]
        m[3]

        self.assertEqual(created, 3)

        del m

        self.assertEqual(deleted, 3)

    def test_len(self):
        def plus_two(x):
            return x + 2

        m = lazy.Memoizer(plus_two)

        m[1]
        m[2]
        m[3]

        self.assertEqual(len(m), 3)

    def test_delitem(self):
        def plus_two(x):
            return x + 2

        m = lazy.Memoizer(plus_two)

        m[1]
        m[2]
        m[3]

        del m[2]

        self.assertEqual(len(m), 2)

    def test_resize(self):
        def plus_two(x):
            return x + 2

        m = lazy.Memoizer(plus_two)

        # Current implementation detail:
        # INITIAL_SIZE is 128

        for i in range(128):
            m[i]
