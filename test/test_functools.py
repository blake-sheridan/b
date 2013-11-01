import unittest

from b.functools import identity, noop

class IdentityTests(unittest.TestCase):
    def test_identity(self):
        a = object()

        self.assertIs(identity(a), a)

class NoopTests(unittest.TestCase):
    def test_noop(self):
        noop()
        noop(1, 2, 3)
        noop(abc='xyz')
