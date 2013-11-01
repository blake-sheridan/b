import unittest

from b.operator import ExceptionRaiser

class ExceptionRaiserTests(unittest.TestCase):
    def test_init_class(self):
        ExceptionRaiser(AttributeError)

    def test_init_instance(self):
        ExceptionRaiser(AttributeError('asdf'))

    def test_call_class(self):
        class Expecting(Exception):
            pass

        raise_it = ExceptionRaiser(Expecting)

        with self.assertRaises(Expecting):
            raise_it()

    def test_call_instance(self):
        e1 = AttributeError('hai')

        raise_it = ExceptionRaiser(e1)

        try:
            raise_it()
        except Exception as e2:
            self.assertIs(e1, e2)
        else:
            self.fail("Not raised")
