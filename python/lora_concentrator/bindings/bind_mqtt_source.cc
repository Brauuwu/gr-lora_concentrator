#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <gnuradio/lora_concentrator/mqtt_source.h>

namespace py = pybind11;

void bind_mqtt_source(py::module& m)
{
    using mqtt_source = ::gr::lora_concentrator::mqtt_source;

    py::class_<mqtt_source, 
               gr::block,
               gr::basic_block,
               std::shared_ptr<mqtt_source>>(m, "mqtt_source")
        .def(py::init(&mqtt_source::make),
             py::arg("broker_host"),
             py::arg("broker_port"),
             py::arg("client_id"),
             py::arg("gateway_id"),
             py::arg("username") = "",
             py::arg("password") = "",
             py::arg("qos") = 1,
             py::arg("tls_enabled") = false,
             py::arg("ca_cert_path") = "",
             "Creates an MQTT source block.");
}
