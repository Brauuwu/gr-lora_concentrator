#include <gnuradio/io_signature.h>
#include <iostream>
#include <chrono>

#include "mqtt_variable_impl.h"

namespace gr {
namespace lora_concentrator {

mqtt_variable::sptr
mqtt_variable::make(const std::string& broker_host,
                    int broker_port,
                    const std::string& topic)
{
    return gnuradio::get_initial_sptr(
        new mqtt_variable_impl(broker_host, broker_port, topic)
    );
}

mqtt_variable_impl::mqtt_variable_impl(const std::string& broker_host,
                                       int broker_port,
                                       const std::string& topic)
    : gr::block("mqtt_variable",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      m_broker_host(broker_host),
      m_broker_port(broker_port),
      m_topic(topic),
      m_connected(false),
      m_callback(nullptr)
{
    // Generate a somewhat unique client ID
    auto now = std::chrono::system_clock::now().time_since_epoch();
    long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    m_client_id = "gr_var_" + std::to_string(timestamp);

#ifdef HAVE_PAHO_MQTT
    std::string uri = "tcp://" + broker_host + ":" + std::to_string(broker_port);
    MQTTClient_create(&m_client, uri.c_str(), m_client_id.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(m_client, this, on_connection_lost, on_message_arrived, NULL);
    connect();
#else
    std::cerr << "[WARNING] gr-lora_concentrator compiled without Paho MQTT. MQTT variable is a stub." << std::endl;
#endif
}

mqtt_variable_impl::~mqtt_variable_impl()
{
#ifdef HAVE_PAHO_MQTT
    disconnect();
    MQTTClient_destroy(&m_client);
#endif
}

void mqtt_variable_impl::set_callback(std::function<void(std::string)> cb)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = cb;
}

#ifdef HAVE_PAHO_MQTT
void mqtt_variable_impl::connect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connected) return;

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 60;
    conn_opts.cleansession = 1;

    int rc = MQTTClient_connect(m_client, &conn_opts);
    if (rc == MQTTCLIENT_SUCCESS) {
        m_connected = true;
        MQTTClient_subscribe(m_client, m_topic.c_str(), 1);
        std::cout << "[MQTT Variable] Subscribed to " << m_topic << std::endl;
    } else {
        std::cerr << "[MQTT Variable] Failed to connect, rc " << rc << std::endl;
    }
}

void mqtt_variable_impl::disconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected) return;
    
    MQTTClient_disconnect(m_client, 10000);
    m_connected = false;
}

void mqtt_variable_impl::on_connection_lost(void* context, char* cause)
{
    mqtt_variable_impl* self = static_cast<mqtt_variable_impl*>(context);
    self->m_connected = false;
    std::cerr << "[MQTT Variable] Connection lost: " << (cause ? cause : "unknown") << std::endl;
    self->connect();
}

int mqtt_variable_impl::on_message_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message)
{
    mqtt_variable_impl* self = static_cast<mqtt_variable_impl*>(context);
    
    std::string payload((char*)message->payload, message->payloadlen);
    
    // Call the python callback if registered
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        if (self->m_callback) {
            // NOTE: Python callback needs GIL handling if called from C++ thread, 
            // but Pybind11 automatically acquires GIL for std::function if bound properly.
            // However, to be perfectly safe when C++ thread calls into Python:
            pybind11::gil_scoped_acquire acquire;
            try {
                self->m_callback(payload);
            } catch (const std::exception& e) {
                std::cerr << "[MQTT Variable] Python callback error: " << e.what() << std::endl;
            }
        }
    }
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}
#endif

} /* namespace lora_concentrator */
} /* namespace gr */
