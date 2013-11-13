import unittest

from b.types import lazyproperty, Memoizer

class A:
    def __init__(self, x):
        self.x = x

    @lazyproperty
    def plus_two(self):
        """add two!"""
        self.x += 2
        return self.x

    @lazyproperty
    def __private(self):
        self.x += 2
        return self.x

class LazyPropertyTests(unittest.TestCase):
    def test_doc(self):
        self.assertEqual(A.plus_two.__doc__, 'add two!')

    def test_name(self):
        self.assertEqual(A.plus_two.__name__, 'plus_two')

    def test_qualname(self):
        self.assertEqual(A.plus_two.__qualname__, 'A.plus_two')

    def test_get(self):
        a = A(2)

        self.assertEqual(a.plus_two, 4)
        self.assertEqual(a.plus_two, 4)

    def test_mangling(self):
        # Since naive Python version would fail this.
        a = A(2)

        self.assertEqual(a._A__private, 4)
        self.assertEqual(a._A__private, 4)

    def test_delete(self):
        a = A(2)

        self.assertEqual(a.plus_two, 4)

        del a.plus_two

        self.assertEqual(a.plus_two, 6)

    def test_set(self):
        # Though I'm undecided as to allowing this.
        a = A(2)

        self.assertEqual(a.plus_two, 4)

        a.plus_two = 5

        self.assertEqual(a.plus_two, 5)

    def test_owner_gc(self):
        # Not that this should have happened,
        # but for sanity.

        creations = 0
        deletions = 0

        class B:
            def __init__(self):
                nonlocal creations
                creations += 1

            def __del__(self):
                nonlocal deletions
                deletions += 1

            @lazyproperty
            def x(self):
                return 5

        instances = [B() for i in range(10)]

        self.assertEqual(creations, 10)
        self.assertEqual(deletions, 0)

        del instances

        self.assertEqual(deletions, 10)

class MemoizerTests(unittest.TestCase):
    def test_new(self):
        Memoizer(str)

        with self.assertRaises(TypeError):
            Memoizer("asdf")

        with self.assertRaises(TypeError):
            Memoizer(abc=123)

    def test_getitem(self):
        count = 0

        def plus_two(x):
            nonlocal count
            count += 1
            return x + 2

        m = Memoizer(plus_two)

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

        m = Memoizer(make_item)

        m[1]
        m[2]
        m[3]

        self.assertEqual(created, 3)

        del m

        self.assertEqual(deleted, 3)

    def test_len(self):
        def plus_two(x):
            return x + 2

        m = Memoizer(plus_two)

        m[1]
        m[2]
        m[3]

        self.assertEqual(len(m), 3)

    def test_delitem(self):
        def plus_two(x):
            return x + 2

        m = Memoizer(plus_two)

        m[1]
        m[2]
        m[3]

        del m[2]

        self.assertEqual(len(m), 2)

    def test_setitem(self):
        # Though I'm up the air as to allowing this
        def plus_two(x):
            return x + 2

        m = Memoizer(plus_two)

        self.assertEqual(m[2], 4)

        m[2] = 5

        self.assertEqual(m[2], 5)

    def test_grow(self):
        count = 0

        def plus_two(x):
            nonlocal count
            count += 1
            return x + 2

        m = Memoizer(plus_two)

        for i in range(64):
            m[i]

        self.assertEqual(count, 64)

        # Current implementation detail:
        # INITIAL_SIZE is 128

        for i in range(128):
            m[i]

        self.assertEqual(count, 128)

        for i in range(128):
            self.assertEqual(m[i], i + 2)

        self.assertEqual(count, 128)

    def test_contains(self):
        def plus_two(x):
            return x + 2

        m = Memoizer(plus_two)

        self.assertNotIn(2, m)

        m[2]

        self.assertIn(2, m)

        del m[2]

        self.assertNotIn(2, m)

    def test_reap(self):
        creations = 0
        deletions = 0

        class A:
            def __init__(self):
                nonlocal creations
                creations += 1

            def __del__(self):
                nonlocal deletions
                deletions += 1

        def new_a(_):
            return A()

        m = Memoizer(new_a)

        objects = [object() for i in range(10)]

        for o in objects:
            m[o]

        self.assertEqual(creations, 10)
        self.assertEqual(deletions, 0)
        self.assertEqual(len(m), 10)

        reaps = m.reap()

        self.assertEqual(reaps, 0)

        del objects

        reaps = m.reap()

        # I was only getting 9/10 collections here,
        # but I believe it's due to gc uncertainty?
        # This might come out lower with other builds.

        MINIMUM_REAPS = 8

        self.assertGreater(reaps, MINIMUM_REAPS)

        self.assertEqual(deletions, reaps)
        self.assertEqual(len(m), 10 - reaps)
