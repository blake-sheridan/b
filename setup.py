#!/usr/local/bin/python3

from distutils.core import setup, Extension

setup(
    name = 'lazy',
    version = '1.0',
    description = 'Lazy',
    ext_modules = [
        Extension(
            name = '_lazy',
            include_dirs = [
                'include',
            ],
            sources = [
                'src/__init__.c',
                'src/lazy.c',
            ],
        )
    ],
)
