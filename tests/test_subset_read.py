import struct
import vertex_builder


def test_subset():
    builder = vertex_builder.builder('2f 2i', 10)
    assert builder.read() == b''

    vertex_1 = struct.pack('2f2i', 1.0, 2.0, 3, 4)
    vertex_2 = struct.pack('2f2i', -1.0, 0.0, 1024, -256)

    builder.write(vertex_1 + vertex_2)

    subset = builder.subset(0)
    assert struct.unpack('4f', subset.read()) == (1.0, 2.0, -1.0, 0.0)

    subset = builder.subset(1)
    assert struct.unpack('4i', subset.read()) == (3, 4, 1024, -256)


def test_subset_permutation():
    builder = vertex_builder.builder('2f 2i', 10)
    assert builder.read() == b''

    vertex_1 = struct.pack('2f2i', 1.0, 2.0, 3, 4)
    vertex_2 = struct.pack('2f2i', -1.0, 0.0, 1024, -256)

    builder.write(vertex_1 + vertex_2)

    subset = builder.subset(1, 0)
    assert subset.read() == struct.pack('2i2f2i2f', 3, 4, 1.0, 2.0, 1024, -256, -1.0, 0.0)
