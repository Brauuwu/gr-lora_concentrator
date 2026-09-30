include(GrPybind)

list(APPEND lora_concentrator_python_files
    lora_concentrator_python.cc
    bind_metadata_extractor.cc
    bind_mqtt_sink.cc
    bind_mqtt_source.cc
    bind_channel_sensor.cc
    bind_mqtt_variable.cc
)

set(lora_concentrator_python_files "${lora_concentrator_python_files}")

GR_PYBIND_MAKE_OOT(lora_concentrator
    "../.."
    "lora_concentrator_python"
    "${lora_concentrator_python_files}"
    "gnuradio-lora_concentrator"
)
