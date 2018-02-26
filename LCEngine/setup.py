from distutils.core import setup
from distutils.extension import Extension

from Cython.Build import cythonize

setup(
    ext_modules = cythonize([Extension("LCEngineV1", ["LCEngineV1.pyx"], libraries=["irina"])])
)
