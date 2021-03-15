#include <Python.h>
#include <structmember.h>

typedef void (* Callback)(char * ptr, PyObject * obj);

struct Attribute {
    int count;
    int size;
    Callback packer;
    int offset;
};

struct Packer {
    int attribute;
    int offset;
    int size;
    Callback packer;
};

struct Builder {
    PyObject_HEAD
    Builder * parent;
    int packer_count;
    Packer * packer_array;
    int attribute_count;
    Attribute * attribute_array;
    int vertex_stride;
    int vertex_size;
    int vertex_count;
    int vertex_limit;
    char * base;
};

PyTypeObject * Builder_type;

uint16_t half_float(double value) {
    union {
        float f;
        uint32_t x;
    };
    f = (float)value;
    return (x >> 16 & 0xc000) | (x >> 13 & 0x3cf0) | (x >> 13 & 0x03ff);
}

void pack_float(char * ptr, PyObject * obj) {
    *(float *)ptr = (float)PyFloat_AsDouble(obj);
}

void pack_hfloat(char * ptr, PyObject * obj) {
    *(uint16_t *)ptr = half_float(PyFloat_AsDouble(obj));
}

void pack_int(char * ptr, PyObject * obj) {
    *(int32_t *)ptr = PyLong_AsLong(obj);
}

void pack_uint(char * ptr, PyObject * obj) {
    *(uint32_t *)ptr = PyLong_AsUnsignedLong(obj);
}

void pack_byte(char * ptr, PyObject * obj) {
    *(uint8_t *)ptr = (uint8_t)PyLong_AsUnsignedLong(obj);
}

Attribute get_attribute(PyObject * name) {
    const char * s = PyUnicode_AsUTF8(name);
    int count = 0;
    while ('0' <= *s && *s <= '9') {
        count = count * 10 + *s - '0';
        s++;
    }
    switch (*s) {
        case 'f': return {count, 4, pack_float};
        case 'h': return {count, 2, pack_hfloat};
        case 'i': return {count, 4, pack_int};
        case 'u': return {count, 4, pack_uint};
        case 'b': return {count, 1, pack_byte};
    }
    PyErr_Format(PyExc_ValueError, "format");
    return {};
}

Builder * meth_builder(PyObject * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"vertex_format", "size", NULL};

    PyObject * vertex_format;
    int vertex_limit;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!I", keywords, &PyUnicode_Type, &vertex_format, &vertex_limit)) {
        return NULL;
    }

    PyObject * format_list = PyObject_CallMethod(vertex_format, "split", NULL);

    int vertex_size = 0;
    int packer_count = 0;

    int attribute_count = (int)PyList_Size(format_list);
    Attribute * attribute_array = (Attribute *)malloc(sizeof(Attribute) * attribute_count);

    for (int i = 0; i < attribute_count; ++i) {
        attribute_array[i] = get_attribute(PyList_GetItem(format_list, i));
        vertex_size += attribute_array[i].size * attribute_array[i].count;
        packer_count += attribute_array[i].count;
    }

    Packer * packer_array = (Packer *)malloc(sizeof(Packer) * packer_count);

    for (int i = 0, k = 0, offset = 0; i < attribute_count; ++i) {
        attribute_array[i].offset = offset;
        for (int j = 0; j < attribute_array[i].count; ++j) {
            packer_array[k++] = {i, offset, attribute_array[i].size, attribute_array[i].packer};
            offset += attribute_array[i].size;
        }
    }

    char * base = (char *)malloc(vertex_size * vertex_limit);
    memset(base, 0, vertex_size * vertex_limit);

    Builder * res = PyObject_New(Builder, Builder_type);
    res->parent = NULL;
    res->packer_count = packer_count;
    res->packer_array = packer_array;
    res->attribute_count = packer_count;
    res->attribute_array = attribute_array;
    res->vertex_limit = vertex_limit;
    res->vertex_stride = vertex_size;
    res->vertex_size = vertex_size;
    res->vertex_count = 0;
    res->base = base;
    return res;
}

Builder * Builder_meth_subset(Builder * self, PyObject ** args, Py_ssize_t nargs) {
    int vertex_size = 0;
    int packer_count = 0;

    int attribute_count = (int)nargs;
    Attribute * attribute_array = (Attribute *)malloc(sizeof(Attribute) * attribute_count);

    for (int i = 0; i < attribute_count; ++i) {
        int idx = PyLong_AsLong(args[i]);
        attribute_array[i] = self->attribute_array[idx];
        vertex_size += attribute_array[i].size * attribute_array[i].count;
        packer_count += attribute_array[i].count;
    }

    Packer * packer_array = (Packer *)malloc(sizeof(Packer) * packer_count);

    for (int i = 0, k = 0; i < attribute_count; ++i) {
        for (int j = 0; j < attribute_array[i].count; ++j) {
            int offset = attribute_array[i].offset + attribute_array[i].size * j;
            packer_array[k++] = {i, offset, attribute_array[i].size, attribute_array[i].packer};
        }
    }

    Py_INCREF(self);

    Builder * res = PyObject_New(Builder, Builder_type);
    res->parent = self;
    res->packer_count = packer_count;
    res->packer_array = packer_array;
    res->attribute_count = packer_count;
    res->attribute_array = attribute_array;
    res->vertex_size = vertex_size;
    res->vertex_limit = self->vertex_limit;
    res->vertex_stride = self->vertex_stride;
    res->vertex_size = vertex_size;
    res->vertex_count = 0;
    res->base = self->base;
    return res;
}

void run_packer(Builder * self, PyObject ** args, int nargs, int & index, char * dst) {
    if (index + nargs > self->packer_count) {
        PyErr_Format(PyExc_ValueError, "too many arguments");
        return;
    }
    for (int i = 0; i < nargs; ++i) {
        if (Py_TYPE(args[i])->tp_flags & (Py_TPFLAGS_LIST_SUBCLASS | Py_TPFLAGS_TUPLE_SUBCLASS)) {
            run_packer(self, PySequence_Fast_ITEMS(args[i]), (int)PySequence_Fast_GET_SIZE(args[i]), index, dst);
        } else {
            self->packer_array[index].packer(dst + self->packer_array[index].offset, args[i]);
            index += 1;
        }
    }
}

PyObject * Builder_meth_vertex(Builder * self, PyObject ** args, Py_ssize_t nargs) {
    int index = 0;
    char * dst = self->base + self->vertex_stride * self->vertex_count;
    run_packer(self, args, (int)nargs, index, dst);

    if (PyErr_Occurred()) {
        return NULL;
    }

    if (index != self->packer_count) {
        PyErr_Format(PyExc_ValueError, "too few arguments");
        return NULL;
    }

    if (self->parent && self->parent->vertex_count < self->vertex_count) {
        self->parent->vertex_count = self->vertex_count;
    }

    self->vertex_count += 1;
    Py_RETURN_NONE;
}

PyObject * Builder_meth_write(Builder * self, PyObject * arg) {
    Py_buffer view = {};
    PyObject_GetBuffer(arg, &view, PyBUF_SIMPLE);
    int count = (int)(view.len / self->vertex_size);

    if (view.len % self->vertex_size) {
        PyErr_Format(PyExc_ValueError, "underflow");
        PyBuffer_Release(&view);
        return NULL;
    }

    if (self->vertex_count + count > self->vertex_limit) {
        PyErr_Format(PyExc_ValueError, "overflow");
        PyBuffer_Release(&view);
        return NULL;
    }

    char * dst = self->base + self->vertex_stride * self->vertex_count;
    self->vertex_count += count;

    if (!self->parent) {
        memcpy(dst, view.buf, self->vertex_stride * count);
    } else {
        if (self->parent->vertex_count < self->vertex_count) {
            self->parent->vertex_count = self->vertex_count;
        }
        char * src = (char *)view.buf;
        for (int k = 0; k < count; ++k) {
            for (int i = 0; i < self->packer_count; ++i) {
                memcpy(dst + self->packer_array[i].offset, src, self->packer_array[i].size);
                src += self->packer_array[i].size;
            }
            dst += self->parent->vertex_stride;
        }
    }

    PyBuffer_Release(&view);
    Py_RETURN_NONE;
}

PyObject * Builder_meth_read(Builder * self) {
    if (!self->parent) {
        return PyBytes_FromStringAndSize(self->base, self->vertex_stride * self->vertex_count);
    } else {
        PyObject * res = PyBytes_FromStringAndSize(NULL, self->vertex_size * self->parent->vertex_count);
        char * dst = PyBytes_AsString(res);
        char * src = self->base;
        for (int k = 0; k < self->parent->vertex_count; ++k) {
            for (int i = 0; i < self->packer_count; ++i) {
                memcpy(dst, src + self->packer_array[i].offset, self->packer_array[i].size);
                dst += self->packer_array[i].size;
            }
            src += self->parent->vertex_stride;
        }
        return res;
    }
}

PyObject * Builder_meth_seek(Builder * self, PyObject * arg) {
    int offset = PyLong_AsLong(arg);
    if (PyErr_Occurred() || offset < 0 || offset >= self->vertex_limit) {
        PyErr_Format(PyExc_ValueError, "offset");
        return NULL;
    }
    self->vertex_count = offset;
    Py_RETURN_NONE;
}

void Builder_dealloc(PyObject * self) {
    Py_TYPE(self)->tp_free(self);
}

PyMemberDef Builder_members[] = {
    {"offset", T_INT, offsetof(Builder, vertex_count), NULL},
    {},
};

PyMethodDef Builder_methods[] = {
    {"subset", (PyCFunction)Builder_meth_subset, METH_FASTCALL, NULL},
    {"vertex", (PyCFunction)Builder_meth_vertex, METH_FASTCALL, NULL},
    {"write", (PyCFunction)Builder_meth_write, METH_O, NULL},
    {"read", (PyCFunction)Builder_meth_read, METH_NOARGS, NULL},
    {},
};

PyType_Slot Builder_slots[] = {
    {Py_tp_members, Builder_members},
    {Py_tp_methods, Builder_methods},
    {Py_tp_dealloc, (void *)Builder_dealloc},
    {},
};

PyMethodDef module_methods[] = {
    {"builder", (PyCFunction)meth_builder, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyType_Spec Builder_spec = {"vertex_builder.Builder", sizeof(Builder), 0, Py_TPFLAGS_DEFAULT, Builder_slots};

PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "vertex_builder", NULL, -1, module_methods};

extern "C" PyObject * PyInit_vertex_builder() {
    PyObject * module = PyModule_Create(&module_def);
    Builder_type = (PyTypeObject *)PyType_FromSpec(&Builder_spec);
    return module;
}
