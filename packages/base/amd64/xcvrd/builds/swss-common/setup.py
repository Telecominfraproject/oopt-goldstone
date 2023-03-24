#!/usr/bin/env python

from distutils.core import setup, Extension

setup(name='swsscommon',
    version='1.0.0',
    description='SWSS common Python Wrapper',
    author='Shuotian Cheng',
    author_email='shuche@microsoft.com',
    url='https://github.com/Azure/sonic-swss-common',
    ext_modules=[Extension('_swsscommon', ['swsscommon.i'],
                             swig_opts=['-c++', '-I../common'],
                             include_dirs=['../common'],
                             libraries=['swsscommon'])],
    py_modules=['swsscommon'],
)
