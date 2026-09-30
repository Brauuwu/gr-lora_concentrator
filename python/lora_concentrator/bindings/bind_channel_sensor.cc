#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <gnuradio/lora_concentrator/channel_sensor.h>

namespace py = pybind11;

void bind_channel_sensor(py::module& m)
{
    using channel_sensor = ::gr::lora_concentrator::channel_sensor;

    py::class_<channel_sensor, 
               gr::sync_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<channel_sensor>>(m, "channel_sensor")
        .def(py::init(&channel_sensor::make),
             py::arg("channel_index"),
             py::arg("channel_freq"),
             py::arg("samp_rate"),
             py::arg("energy_threshold_db") = -100.0f,
             py::arg("window_size_ms") = 100,
             "Creates a channel_sensor block.");
}
