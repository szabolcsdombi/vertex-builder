from setuptools import Extension, setup

ext = Extension(
    name='vertex_builder',
    sources=['./vertex_builder.cpp'],
    define_macros=[
        ('PY_SSIZE_T_CLEAN', None),
    ],
)

setup(
    name='vertex_builder',
    version='0.1.0',
    ext_modules=[ext],
)
