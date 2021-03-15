import struct
import vertex_builder


def test_subset_write():
    builder = vertex_builder.builder('2f 2i', 10)
    assert builder.read() == b''

    chunk_1 = struct.pack('2f', 1.0, 2.0)
    chunk_2 = struct.pack('2i', 3, 4)
    chunk_3 = struct.pack('2f', -1.0, 0.0)
    chunk_4 = struct.pack('2i', 1024, -256)

    subset_1 = builder.subset(0)
    subset_2 = builder.subset(1)

    subset_1.write(chunk_1 + chunk_3)
    subset_2.write(chunk_2 + chunk_4)

    assert builder.read() == chunk_1 + chunk_2 + chunk_3 + chunk_4
