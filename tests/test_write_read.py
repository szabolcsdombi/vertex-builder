import struct
import vertex_builder


def test_write_read():
    builder = vertex_builder.builder('2f 2i', 10)
    assert builder.read() == b''

    vertex_1 = struct.pack('2f2i', 1.0, 2.0, 3, 4)
    vertex_2 = struct.pack('2f2i', -1.0, 0.0, 1024, -256)

    builder.write(vertex_1)
    assert builder.read() == vertex_1

    builder.write(vertex_2)
    assert builder.read() == vertex_1 + vertex_2


def test_vertex_read():
    builder = vertex_builder.builder('2f 2i', 10)
    assert builder.read() == b''

    vertex_1 = struct.pack('2f2i', 1.0, 2.0, 3, 4)
    vertex_2 = struct.pack('2f2i', -1.0, 0.0, 1024, -256)

    builder.vertex(1.0, 2.0, 3, 4)
    assert builder.read() == vertex_1

    builder.vertex(-1.0, 0.0, 1024, -256)
    assert builder.read() == vertex_1 + vertex_2
