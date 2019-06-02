import os
from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

pyext_path = os.path.dirname(os.path.abspath(__file__))
src_path = os.path.join(pyext_path, '../src/')

pyx_srcs = [
    'frame_queue.pyx'
]

extensions = [
    Extension("frame_queue",
        sources=pyx_srcs,
        libraries=['coda'],
        include_dirs=[src_path])
]

setup(
    ext_modules = cythonize(extensions, language_level="3")
)

