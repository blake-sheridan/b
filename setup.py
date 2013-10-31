#!/usr/local/bin/python3

from distutils.core import setup, Extension

setup(
    name = 'lazy',
    version = '1.0',
    description = 'Lazy',
    ext_modules = [
        Extension(
            name = 'b._collections',
            sources = [
                'src/collections.c',
            ],
        ),
        Extension(
            name = 'b._types',
            depends = [
                'include/memoizer.h',
                ],
            include_dirs = [
                'include',
            ],
            sources = [
                'src/memoizer.c',
                'src/types.c',
            ],
        ),
    ],
)
