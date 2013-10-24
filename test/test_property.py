import unittest

import lazy

class A:
    def __init__(self, x):
        self.x = x

    @lazy.property
    def plus_two(self):
        """add two!"""
        self.x += 2
        return self.x

    @lazy.property
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
