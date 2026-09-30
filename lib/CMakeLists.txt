########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

list(APPEND lora_concentrator_sources
    metadata_extractor_impl.cc
    channel_sensor_impl.cc
    mqtt_sink_impl.cc
    mqtt_source_impl.cc
    mqtt_variable_impl.cc
)

set(lora_concentrator_sources "${lora_concentrator_sources}")

add_library(gnuradio-lora_concentrator SHARED ${lora_concentrator_sources})
target_link_libraries(gnuradio-lora_concentrator 
    gnuradio::gnuradio-runtime 
    gnuradio::gnuradio-blocks
    Volk::volk
)

if(PAHO_MQTT_FOUND)
    target_compile_definitions(gnuradio-lora_concentrator PRIVATE HAVE_PAHO_MQTT)
    target_link_libraries(gnuradio-lora_concentrator ${PAHO_MQTT_C_LIBRARY})
endif()

target_include_directories(gnuradio-lora_concentrator
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    PUBLIC $<INSTALL_INTERFACE:include>
)

set_target_properties(gnuradio-lora_concentrator PROPERTIES DEFINE_SYMBOL "gnuradio_lora_concentrator_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-lora_concentrator PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-lora_concentrator)

install(TARGETS gnuradio-lora_concentrator
    EXPORT lora_concentrator-export
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)
