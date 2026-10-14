#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <errno.h>
#include "bioampexgpillClib.h"

// the C library returns -errno; turn that into a proper OSError
static PyObject *raise_oserror(int rc){
    errno = -rc;
    return PyErr_SetFromErrno(PyExc_OSError);
}

// build array.array(typecode, bytes) from a bytes object
static PyObject *make_array(const char *typecode, PyObject *bytes){
    PyObject *mod = PyImport_ImportModule("array");
    if (!mod) return NULL;
    PyObject *arr = PyObject_CallMethod(mod, "array", "sO", typecode, bytes);
    Py_DECREF(mod);
    return arr;
}

static PyObject *core_open(PyObject *self, PyObject *args){
    const char *device;
    int addr;
    if (!PyArg_ParseTuple(args, "si", &device, &addr)) return NULL;

    int fd;
    Py_BEGIN_ALLOW_THREADS
    fd = bioampexgpillclib_init(device, addr);
    Py_END_ALLOW_THREADS

    if (fd < 0) return raise_oserror(fd);
    return PyLong_FromLong(fd);
}

static PyObject *core_read_raw(PyObject *self, PyObject *args){
    int fd;
    if (!PyArg_ParseTuple(args, "i", &fd)) return NULL;

    int16_t raw;
    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = bioampexgpillclib_read_raw(fd, &raw);
    Py_END_ALLOW_THREADS

    if (rc < 0) return raise_oserror(rc);
    return PyLong_FromLong(raw);
}

static PyObject *core_counts_to_voltage(PyObject *self, PyObject *args){
    int raw;
    if (!PyArg_ParseTuple(args, "i", &raw)) return NULL;
    if (raw < INT16_MIN || raw > INT16_MAX) {
        PyErr_SetString(PyExc_ValueError, "raw counts must fit in a signed 16 bit integer");
        return NULL;
    }
    return PyFloat_FromDouble(bioampexgpillclib_counts_to_voltage((int16_t)raw));
}

// read_block(fd, n, rate_hz, volts) -> array('h') of counts, or array('d') of volts
static PyObject *core_read_block(PyObject *self, PyObject *args){
    int fd, volts;
    Py_ssize_t n;
    double rate_hz;
    if (!PyArg_ParseTuple(args, "indp", &fd, &n, &rate_hz, &volts)) return NULL;

    if (n < 0 || n > PY_SSIZE_T_MAX / (Py_ssize_t)sizeof(double)) {
        PyErr_SetString(PyExc_ValueError, "n out of range");
        return NULL;
    }
    if (rate_hz < 0) {
        PyErr_SetString(PyExc_ValueError, "rate_hz must be >= 0");
        return NULL;
    }

    // read straight into the bytes object's buffer; nobody else has a reference yet,
    // so it's safe to fill without holding the GIL
    PyObject *raw_bytes = PyBytes_FromStringAndSize(NULL, n * (Py_ssize_t)sizeof(int16_t));
    if (!raw_bytes) return NULL;
    int16_t *raw = (int16_t *)PyBytes_AS_STRING(raw_bytes);

    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = bioampexgpillclib_read_block(fd, raw, (size_t)n, rate_hz);
    Py_END_ALLOW_THREADS

    if (rc < 0) {
        Py_DECREF(raw_bytes);
        return raise_oserror(rc);
    }

    if (!volts) {
        PyObject *arr = make_array("h", raw_bytes);
        Py_DECREF(raw_bytes);
        return arr;
    }

    PyObject *volt_bytes = PyBytes_FromStringAndSize(NULL, n * (Py_ssize_t)sizeof(double));
    if (!volt_bytes) {
        Py_DECREF(raw_bytes);
        return NULL;
    }
    double *out = (double *)PyBytes_AS_STRING(volt_bytes);
    for (Py_ssize_t i = 0; i < n; i++) out[i] = bioampexgpillclib_counts_to_voltage(raw[i]);

    PyObject *arr = make_array("d", volt_bytes);
    Py_DECREF(raw_bytes);
    Py_DECREF(volt_bytes);
    return arr;
}

static PyObject *core_close(PyObject *self, PyObject *args){
    int fd;
    if (!PyArg_ParseTuple(args, "i", &fd)) return NULL;

    int rc = bioampexgpillclib_close(fd);
    if (rc < 0) return raise_oserror(rc);
    Py_RETURN_NONE;
}

static PyMethodDef methods[] = {
    {"open", core_open, METH_VARARGS, "open(device, addr) -> fd"},
    {"read_raw", core_read_raw, METH_VARARGS, "read_raw(fd) -> int"},
    {"read_block", core_read_block, METH_VARARGS, "read_block(fd, n, rate_hz, volts) -> array"},
    {"counts_to_voltage", core_counts_to_voltage, METH_VARARGS, "counts_to_voltage(raw) -> float"},
    {"close", core_close, METH_VARARGS, "close(fd)"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT, "_core", "C core of bioampexgpillclib", -1, methods
};

PyMODINIT_FUNC PyInit__core(void){
    return PyModule_Create(&module);
}
