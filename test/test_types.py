import unittest

from b.types import lazyproperty, StructSequence

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

class StructSequenceMetaTests(unittest.TestCase):
    def test_new_noop(self):
        class A(StructSequence):
            pass

    def test_new(self):
        class A(StructSequence):
            x = __(str)
            y = __(str)

    def test_len(self):
        class A(StructSequence):
            x = __(str)
            y = __(str)

        self.assertEqual(len(A), 2)

    @unittest.expectedFailure
    def test_getitem(self):
        class A(StructSequence):
            x = __(str)
            y = __(str)

        self.assertIs(A[0], A.x)
        self.assertIs(A[1], A.y)

    @unittest.expectedFailure
    def test_iter(self):
        class A(StructSequence):
            x = __(str)
            y = __(str)

        for index, field in enumerate(A):
            if index == 0:
                self.assertIs(field, A.x)
            elif index == 1:
                self.assertIs(field, A.y)
            else:
                self.fail()

class StructSequenceFieldTests(unittest.TestCase):
    def test_doc(self):
        class A(StructSequence):
            x = __(str, doc='Stuff about x')
            y = __(str)

        self.assertEqual(A.x.__doc__, 'Stuff about x')
        self.assertEqual(A.y.__doc__, None)

    def test_name(self):
        class A(StructSequence):
            x = __(str, doc='Stuff about x')
            y = __(str)

        self.assertEqual(A.x.__name__, 'x')
        self.assertEqual(A.y.__name__, 'y')

    def test_qualname(self):
        class A(StructSequence):
            x = __(str, doc='Stuff about x')
            y = __(str)

        self.assertEqual(A.x.__qualname__, 'StructSequenceFieldTests.test_qualname.<locals>.A.x')
        self.assertEqual(A.y.__qualname__, 'StructSequenceFieldTests.test_qualname.<locals>.A.y')

    def test_get(self):
        class A(StructSequence):
            x = __(str, doc='Stuff about x')
            y = __(str)

        a = A("Hello", "world!")

        self.assertEqual(a.x, "Hello")
        self.assertEqual(a.y, "world!")

class StructSequenceTests(unittest.TestCase):
    def test_new(self):
        class A(StructSequence):
            x = __(str, doc='Stuff about x')
            y = __(str)

        a = A("Hello", "world!")

    def test_getitem(self):
        class A(StructSequence):
            x = __(str, doc='Stuff about x')
            y = __(str)

        a = A("Hello", "world!")

        self.assertEqual(a[0], "Hello")
        self.assertEqual(a[1], "world!")

    def test_len(self):
        class A(StructSequence):
            x = __(str, doc='Stuff about x')
            y = __(str)

        a = A("Hello", "world!")

        self.assertEqual(len(a), 2)

    def test_iter(self):
        class A(StructSequence):
            x = __(str, doc='Stuff about x')
            y = __(str)

        a = A("Hello", "world!")

        x, y = a

        self.assertEqual(x, "Hello")
        self.assertEqual(y, "world!")
