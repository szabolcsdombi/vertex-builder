# vertex-builder

```sh
pip install vertex-builder
```

- [Documentation](https://vertex-builder.readthedocs.io/)
- [vertex-builder on Github](https://github.com/cprogrammer1994/vertex-builder)
- [vertex-builder on PyPI](https://pypi.org/project/vertex-builder/)

## Example

```py
import vertex_builder

builder = vertex_builder.builder('2f 2i', 10)
builder.vertex(1.0, 2.0, 3, 4)

vertex_data = builder.read()
```
