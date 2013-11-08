import unittest

from b.types import lazyproperty

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
