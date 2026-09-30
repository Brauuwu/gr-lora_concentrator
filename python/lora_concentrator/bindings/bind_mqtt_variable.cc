#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <gnuradio/lora_concentrator/mqtt_variable.h>

namespace py = pybind11;

void bind_mqtt_variable(py::module& m)
{
    using mqtt_variable = ::gr::lora_concentrator::mqtt_variable;

    py::class_<mqtt_variable, 
               gr::block,
               gr::basic_block,
               std::shared_ptr<mqtt_variable>>(m, "mqtt_variable")
        .def(py::init(&mqtt_variable::make),
             py::arg("broker_host"),
             py::arg("broker_port"),
             py::arg("topic"),
             "Creates an MQTT variable block.")
        .def("set_callback", &mqtt_variable::set_callback,
             py::arg("cb"),
             "Sets the python callback function to trigger on MQTT message.");
}
