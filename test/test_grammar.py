import unittest

from b.grammar import Parser

class ParserTests(unittest.TestCase):
    def test_parse(self):
        p = Parser()

        p.parse('123 "things"')

        raise NotImplementedError
