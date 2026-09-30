#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <gnuradio/lora_concentrator/mqtt_sink.h>

namespace py = pybind11;

void bind_mqtt_sink(py::module& m)
{
    using mqtt_sink = ::gr::lora_concentrator::mqtt_sink;

    py::class_<mqtt_sink, 
               gr::block,
               gr::basic_block,
               std::shared_ptr<mqtt_sink>>(m, "mqtt_sink")
        .def(py::init(&mqtt_sink::make),
             py::arg("broker_host"),
             py::arg("broker_port"),
             py::arg("client_id"),
             py::arg("gateway_id"),
             py::arg("username") = "",
             py::arg("password") = "",
             py::arg("qos") = 1,
             py::arg("tls_enabled") = false,
             py::arg("ca_cert_path") = "",
             "Creates an MQTT sink block.");
}
