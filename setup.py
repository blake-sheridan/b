#!/usr/local/bin/python3

from distutils.core import setup, Extension

setup(
    name = 'lazy',
    version = '1.0',
    description = 'Lazy',
    ext_modules = [
        Extension(
            name = '_lazy',
            sources = [
                'src/module.c',
                'src/property.c',
            ],
        )
    ],
)
