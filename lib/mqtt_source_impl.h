#ifndef INCLUDED_LORA_CONCENTRATOR_MQTT_SOURCE_IMPL_H
#define INCLUDED_LORA_CONCENTRATOR_MQTT_SOURCE_IMPL_H

#include <gnuradio/lora_concentrator/mqtt_source.h>
#include <mutex>
#include <thread>
#include <string>

#ifdef HAVE_PAHO_MQTT
#include <MQTTClient.h>
#endif

namespace gr {
namespace lora_concentrator {

class mqtt_source_impl : public mqtt_source
{
private:
    std::string m_broker_host;
    int m_broker_port;
    std::string m_client_id;
    std::string m_gateway_id;
    std::string m_username;
    std::string m_password;
    int m_qos;
    bool m_tls_enabled;
    std::string m_ca_cert_path;

    bool m_connected;
    std::mutex m_mutex;

#ifdef HAVE_PAHO_MQTT
    MQTTClient m_client;
    
    // Callbacks
    static void on_connection_lost(void* context, char* cause);
    static int on_message_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message);
    
    void connect();
    void disconnect();
    void process_message(const std::string& topic, const std::string& payload);
#endif

public:
    mqtt_source_impl(const std::string& broker_host,
                     int broker_port,
                     const std::string& client_id,
                     const std::string& gateway_id,
                     const std::string& username,
                     const std::string& password,
                     int qos,
                     bool tls_enabled,
                     const std::string& ca_cert_path);
    ~mqtt_source_impl();
};

} // namespace lora_concentrator
} // namespace gr

#endif /* INCLUDED_LORA_CONCENTRATOR_MQTT_SOURCE_IMPL_H */
