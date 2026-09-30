#include <pybind11/pybind11.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace py = pybind11;

// Forward declarations of bind functions
void bind_metadata_extractor(py::module& m);
void bind_mqtt_sink(py::module& m);
void bind_mqtt_source(py::module& m);
void bind_channel_sensor(py::module& m);
void bind_mqtt_variable(py::module& m);

// We need to provide a C-API pointer to numpy
// so that pybind11 can use numpy types if needed.
#if PY_MAJOR_VERSION >= 3
static void* init_numpy()
{
    import_array();
    return NULL;
}
#else
static void init_numpy()
{
    import_array();
}
#endif

PYBIND11_MODULE(lora_concentrator_python, m)
{
    init_numpy();

    // Module docstring
    m.doc() = "Python bindings for lora_concentrator C++ blocks";

    // Call all the individual bind functions
    bind_metadata_extractor(m);
    bind_mqtt_sink(m);
    bind_mqtt_source(m);
    bind_channel_sensor(m);
    bind_mqtt_variable(m);
}
