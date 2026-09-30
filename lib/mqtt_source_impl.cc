#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "mqtt_source_impl.h"

namespace gr {
namespace lora_concentrator {

mqtt_source::sptr
mqtt_source::make(const std::string& broker_host,
                  int broker_port,
                  const std::string& client_id,
                  const std::string& gateway_id,
                  const std::string& username,
                  const std::string& password,
                  int qos,
                  bool tls_enabled,
                  const std::string& ca_cert_path)
{
    return gnuradio::get_initial_sptr(
        new mqtt_source_impl(broker_host, broker_port, client_id, gateway_id,
                             username, password, qos, tls_enabled, ca_cert_path)
    );
}

mqtt_source_impl::mqtt_source_impl(const std::string& broker_host,
                                   int broker_port,
                                   const std::string& client_id,
                                   const std::string& gateway_id,
                                   const std::string& username,
                                   const std::string& password,
                                   int qos,
                                   bool tls_enabled,
                                   const std::string& ca_cert_path)
    : gr::block("mqtt_source",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      m_broker_host(broker_host),
      m_broker_port(broker_port),
      m_client_id(client_id),
      m_gateway_id(gateway_id),
      m_username(username),
      m_password(password),
      m_qos(qos),
      m_tls_enabled(tls_enabled),
      m_ca_cert_path(ca_cert_path),
      m_connected(false)
{
    // Register message ports
    message_port_register_out(pmt::mp("tx_request"));
    message_port_register_out(pmt::mp("config_update"));

#ifdef HAVE_PAHO_MQTT
    std::string uri = (tls_enabled ? "ssl://" : "tcp://") + broker_host + ":" + std::to_string(broker_port);
    MQTTClient_create(&m_client, uri.c_str(), client_id.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(m_client, this, on_connection_lost, on_message_arrived, NULL);
    connect();
#else
    std::cerr << "[WARNING] gr-lora_concentrator compiled without Paho MQTT. MQTT source is a stub." << std::endl;
#endif
}

mqtt_source_impl::~mqtt_source_impl()
{
#ifdef HAVE_PAHO_MQTT
    disconnect();
    MQTTClient_destroy(&m_client);
#endif
}

#ifdef HAVE_PAHO_MQTT
void mqtt_source_impl::connect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connected) return;

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 60;
    conn_opts.cleansession = 1;

    if (!m_username.empty()) {
        conn_opts.username = m_username.c_str();
        if (!m_password.empty()) {
            conn_opts.password = m_password.c_str();
        }
    }

    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    if (m_tls_enabled) {
        if (!m_ca_cert_path.empty()) {
            ssl_opts.trustStore = m_ca_cert_path.c_str();
        }
        conn_opts.ssl = &ssl_opts;
    }

    int rc = MQTTClient_connect(m_client, &conn_opts);
    if (rc == MQTTCLIENT_SUCCESS) {
        m_connected = true;
        
        // Subscribe to downlink topics
        std::string tx_topic = "lora/" + m_gateway_id + "/tx/request";
        std::string config_topic = "lora/" + m_gateway_id + "/config";
        
        MQTTClient_subscribe(m_client, tx_topic.c_str(), m_qos);
        MQTTClient_subscribe(m_client, config_topic.c_str(), m_qos);
        
        std::cout << "[MQTT Source] Connected and subscribed to " << tx_topic << std::endl;
    } else {
        std::cerr << "Failed to connect to MQTT broker, return code " << rc << std::endl;
    }
}

void mqtt_source_impl::disconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected) return;
    
    MQTTClient_disconnect(m_client, 10000);
    m_connected = false;
}

void mqtt_source_impl::on_connection_lost(void* context, char* cause)
{
    mqtt_source_impl* self = static_cast<mqtt_source_impl*>(context);
    self->m_connected = false;
    std::cerr << "MQTT connection lost: " << (cause ? cause : "unknown") << std::endl;
    self->connect();
}

int mqtt_source_impl::on_message_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message)
{
    mqtt_source_impl* self = static_cast<mqtt_source_impl*>(context);
    
    std::string topic(topicName);
    std::string payload((char*)message->payload, message->payloadlen);
    
    self->process_message(topic, payload);
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

// Naive JSON extractor for a specific key
std::string extract_json_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos += search.length();
    
    // Skip spaces
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.length() || json[pos] != '\"') return ""; // Not a string
    pos++; // Skip opening quote
    
    size_t end_pos = pos;
    while (end_pos < json.length() && json[end_pos] != '\"') end_pos++;
    
    if (end_pos >= json.length()) return "";
    
    return json.substr(pos, end_pos - pos);
}

long extract_json_long(const std::string& json, const std::string& key, long default_val) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    
    pos += search.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.length()) return default_val;
    
    try {
        return std::stol(json.substr(pos));
    } catch (...) {
        return default_val;
    }
}

void mqtt_source_impl::process_message(const std::string& topic, const std::string& payload)
{
    std::string tx_topic = "lora/" + m_gateway_id + "/tx/request";
    
    if (topic == tx_topic) {
        // Parse basic TX request
        std::string payload_hex = extract_json_string(payload, "payload_hex");
        long freq = extract_json_long(payload, "frequency", 868100000);
        long sf = extract_json_long(payload, "sf", 12);
        long bw = extract_json_long(payload, "bw", 125000);
        long tx_power = extract_json_long(payload, "tx_power", 14);
        
        if (payload_hex.empty()) {
            std::cerr << "[MQTT Source] TX request missing payload_hex" << std::endl;
            return;
        }
        
        // Convert hex to string
        std::string payload_str;
        for (size_t i = 0; i < payload_hex.length(); i += 2) {
            std::string byteString = payload_hex.substr(i, 2);
            char byte = (char) strtol(byteString.c_str(), NULL, 16);
            payload_str += byte;
        }
        
        // Create PMT message for TX chain
        pmt::pmt_t msg = pmt::make_dict();
        msg = pmt::dict_add(msg, pmt::intern("frequency"), pmt::from_long(freq));
        msg = pmt::dict_add(msg, pmt::intern("sf"), pmt::from_long(sf));
        msg = pmt::dict_add(msg, pmt::intern("bw"), pmt::from_long(bw));
        msg = pmt::dict_add(msg, pmt::intern("tx_power"), pmt::from_long(tx_power));
        msg = pmt::dict_add(msg, pmt::intern("payload"), pmt::intern(payload_str));
        
        message_port_pub(pmt::mp("tx_request"), msg);
        
    } else {
        // Assume it's a config update
        pmt::pmt_t msg = pmt::make_dict();
        msg = pmt::dict_add(msg, pmt::intern("raw_json"), pmt::intern(payload));
        message_port_pub(pmt::mp("config_update"), msg);
    }
}
#endif

} /* namespace lora_concentrator */
} /* namespace gr */
