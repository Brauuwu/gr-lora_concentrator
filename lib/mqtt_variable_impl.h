#ifndef INCLUDED_LORA_CONCENTRATOR_MQTT_VARIABLE_IMPL_H
#define INCLUDED_LORA_CONCENTRATOR_MQTT_VARIABLE_IMPL_H

#include <gnuradio/lora_concentrator/mqtt_variable.h>
#include <mutex>

#ifdef HAVE_PAHO_MQTT
#include <MQTTClient.h>
#endif

namespace gr {
namespace lora_concentrator {

class mqtt_variable_impl : public mqtt_variable
{
private:
    std::string m_broker_host;
    int m_broker_port;
    std::string m_topic;
    std::string m_client_id;
    bool m_connected;
    std::mutex m_mutex;
    
    std::function<void(std::string)> m_callback;

#ifdef HAVE_PAHO_MQTT
    MQTTClient m_client;
    
    static void on_connection_lost(void* context, char* cause);
    static int on_message_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message);
    
    void connect();
    void disconnect();
#endif

public:
    mqtt_variable_impl(const std::string& broker_host,
                       int broker_port,
                       const std::string& topic);
    ~mqtt_variable_impl();
    
    void set_callback(std::function<void(std::string)> cb) override;
    
    // Not a stream block, so work() just returns 0
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) { return 0; }
};

} // namespace lora_concentrator
} // namespace gr

#endif /* INCLUDED_LORA_CONCENTRATOR_MQTT_VARIABLE_IMPL_H */
