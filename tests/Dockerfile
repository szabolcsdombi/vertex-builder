FROM python:3.9.2
RUN python3 -m pip install pytest==6.2.2 sphinx==3.5.1 sphinx_rtd_theme==0.5.1

WORKDIR /build
COPY docs /build/docs
COPY setup.py README.md LICENSE vertex_builder.cpp /build/
RUN python3 setup.py sdist
RUN python3 setup.py build_sphinx
RUN python3 -m pip wheel dist/vertex_builder-*.tar.gz
RUN python3 -m pip install vertex_builder-*.whl

WORKDIR /tests
COPY tests /tests
CMD python3 -m pytest
