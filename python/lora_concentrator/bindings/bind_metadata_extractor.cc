#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <gnuradio/lora_concentrator/metadata_extractor.h>

namespace py = pybind11;

void bind_metadata_extractor(py::module& m)
{
    using metadata_extractor = ::gr::lora_concentrator::metadata_extractor;

    py::class_<metadata_extractor, 
               gr::sync_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<metadata_extractor>>(m, "metadata_extractor")
        .def(py::init(&metadata_extractor::make),
             py::arg("channel_index"),
             py::arg("channel_freq"),
             py::arg("sf"),
             py::arg("bw"),
             py::arg("gateway_id"),
             "Creates a metadata_extractor block.");
}
