#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "mqtt_sink_impl.h"

namespace gr {
namespace lora_concentrator {

mqtt_sink::sptr
mqtt_sink::make(const std::string& broker_host,
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
        new mqtt_sink_impl(broker_host, broker_port, client_id, gateway_id,
                           username, password, qos, tls_enabled, ca_cert_path)
    );
}

mqtt_sink_impl::mqtt_sink_impl(const std::string& broker_host,
                               int broker_port,
                               const std::string& client_id,
                               const std::string& gateway_id,
                               const std::string& username,
                               const std::string& password,
                               int qos,
                               bool tls_enabled,
                               const std::string& ca_cert_path)
    : gr::block("mqtt_sink",
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
      m_connected(false),
      m_pending_metadata(pmt::PMT_NIL)
{
    // Register message ports
    message_port_register_in(pmt::mp("rx_payload"));
    set_msg_handler(pmt::mp("rx_payload"),
                    [this](pmt::pmt_t msg) { this->handle_rx_payload(msg); });

    message_port_register_in(pmt::mp("rx_metadata"));
    set_msg_handler(pmt::mp("rx_metadata"),
                    [this](pmt::pmt_t msg) { this->handle_rx_metadata(msg); });

    message_port_register_in(pmt::mp("sensing"));
    set_msg_handler(pmt::mp("sensing"),
                    [this](pmt::pmt_t msg) { this->handle_sensing(msg); });

#ifdef HAVE_PAHO_MQTT
    std::string uri = (tls_enabled ? "ssl://" : "tcp://") + broker_host + ":" + std::to_string(broker_port);
    MQTTClient_create(&m_client, uri.c_str(), client_id.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(m_client, this, on_connection_lost, on_message_arrived, on_delivery_complete);
    connect();
#else
    std::cerr << "[WARNING] gr-lora_concentrator compiled without Paho MQTT. MQTT sink is a stub." << std::endl;
#endif
}

mqtt_sink_impl::~mqtt_sink_impl()
{
#ifdef HAVE_PAHO_MQTT
    disconnect();
    MQTTClient_destroy(&m_client);
#endif
}

#ifdef HAVE_PAHO_MQTT
void mqtt_sink_impl::connect()
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

    // Last Will and Testament (LWT)
    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    std::string will_topic = "lora/" + m_gateway_id + "/status";
    std::string will_payload = "{\"gateway_id\":\"" + m_gateway_id + "\",\"status\":\"offline\"}";
    will_opts.topicName = will_topic.c_str();
    will_opts.message = will_payload.c_str();
    will_opts.retained = 1;
    will_opts.qos = m_qos;
    conn_opts.will = &will_opts;

    int rc = MQTTClient_connect(m_client, &conn_opts);
    if (rc == MQTTCLIENT_SUCCESS) {
        m_connected = true;
        // Publish online status
        std::string online_payload = "{\"gateway_id\":\"" + m_gateway_id + "\",\"status\":\"online\"}";
        publish_message(will_topic, online_payload, true);
    } else {
        std::cerr << "Failed to connect to MQTT broker, return code " << rc << std::endl;
    }
}

void mqtt_sink_impl::disconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected) return;
    
    // Publish offline status gracefully before disconnecting
    std::string status_topic = "lora/" + m_gateway_id + "/status";
    std::string offline_payload = "{\"gateway_id\":\"" + m_gateway_id + "\",\"status\":\"offline\"}";
    publish_message(status_topic, offline_payload, true);
    
    MQTTClient_disconnect(m_client, 10000);
    m_connected = false;
}

void mqtt_sink_impl::publish_message(const std::string& topic, const std::string& payload, bool retain)
{
    if (!m_connected) return;

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = (void*)payload.c_str();
    pubmsg.payloadlen = (int)payload.length();
    pubmsg.qos = m_qos;
    pubmsg.retained = retain ? 1 : 0;
    
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(m_client, topic.c_str(), &pubmsg, &token);
}

void mqtt_sink_impl::on_connection_lost(void* context, char* cause)
{
    mqtt_sink_impl* self = static_cast<mqtt_sink_impl*>(context);
    self->m_connected = false;
    std::cerr << "MQTT connection lost: " << (cause ? cause : "unknown") << std::endl;
    // Basic auto-reconnect attempt
    self->connect();
}

int mqtt_sink_impl::on_message_arrived(void*, char* topicName, int, MQTTClient_message* message)
{
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void mqtt_sink_impl::on_delivery_complete(void*, MQTTClient_deliveryToken)
{
    // Do nothing
}
#endif

// Simple recursive PMT dict to JSON string converter
std::string mqtt_sink_impl::pmt_dict_to_json(pmt::pmt_t dict)
{
    if (!pmt::is_dict(dict)) return "{}";
    
    std::stringstream ss;
    ss << "{";
    
    pmt::pmt_t keys = pmt::dict_keys(dict);
    size_t len = pmt::length(keys);
    
    for (size_t i = 0; i < len; i++) {
        pmt::pmt_t k = pmt::nth(i, keys);
        pmt::pmt_t v = pmt::dict_ref(dict, k, pmt::PMT_NIL);
        
        if (i > 0) ss << ",";
        
        // Key
        ss << "\"" << pmt::symbol_to_string(k) << "\":";
        
        // Value
        if (pmt::is_bool(v)) {
            ss << (pmt::to_bool(v) ? "true" : "false");
        } else if (pmt::is_integer(v)) {
            ss << pmt::to_long(v);
        } else if (pmt::is_uint64(v)) {
            ss << pmt::to_uint64(v);
        } else if (pmt::is_real(v)) {
            ss << pmt::to_double(v);
        } else if (pmt::is_symbol(v)) {
            ss << "\"" << pmt::symbol_to_string(v) << "\"";
        } else if (pmt::is_dict(v)) {
            ss << pmt_dict_to_json(v); // Recursive
        } else {
            ss << "\"unknown\"";
        }
    }
    
    ss << "}";
    return ss.str();
}

void mqtt_sink_impl::handle_rx_metadata(pmt::pmt_t msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pending_metadata = msg;
    
    // Also publish metadata-only topic
#ifdef HAVE_PAHO_MQTT
    if (pmt::is_dict(msg)) {
        pmt::pmt_t err = pmt::intern("error");
        pmt::pmt_t ch_val = pmt::dict_ref(msg, pmt::intern("channel"), err);
        pmt::pmt_t sf_val = pmt::dict_ref(msg, pmt::intern("sf"), err);
        
        if (!pmt::eq(ch_val, err) && !pmt::eq(sf_val, err)) {
            std::string topic = "lora/" + m_gateway_id + "/rx/" + 
                                std::to_string(pmt::to_long(ch_val)) + "/" + 
                                std::to_string(pmt::to_long(sf_val)) + "/metadata";
            
            publish_message(topic, pmt_dict_to_json(msg));
        }
    }
#endif
}

void mqtt_sink_impl::handle_rx_payload(pmt::pmt_t msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
#ifdef HAVE_PAHO_MQTT
    if (!m_connected) return;
    
    std::string payload_str = "";
    
    if (pmt::is_symbol(msg)) {
        payload_str = pmt::symbol_to_string(msg);
    } else if (pmt::is_u8vector(msg)) {
        size_t len;
        const uint8_t* elements = pmt::u8vector_elements(msg, len);
        payload_str = std::string((const char*)elements, len);
    } else if (pmt::is_blob(msg)) {
        payload_str = std::string((const char*)pmt::blob_data(msg), pmt::blob_length(msg));
    }
    
    // Convert to hex
    std::stringstream hex_ss;
    for (unsigned char c : payload_str) {
        hex_ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    std::string payload_hex = hex_ss.str();
    
    // Create combined JSON
    std::stringstream json_ss;
    json_ss << "{";
    json_ss << "\"gateway_id\":\"" << m_gateway_id << "\",";
    json_ss << "\"payload_hex\":\"" << payload_hex << "\",";
    
    // Escape ascii payload (basic)
    std::string safe_ascii = "";
    for (char c : payload_str) {
        if (c >= 32 && c <= 126 && c != '"' && c != '\\') safe_ascii += c;
        else safe_ascii += '.';
    }
    json_ss << "\"payload_ascii\":\"" << safe_ascii << "\",";
    json_ss << "\"size\":" << payload_str.length();
    
    // Append metadata if available
    int channel = 0;
    int sf = 7;
    
    if (pmt::is_dict(m_pending_metadata)) {
        json_ss << ",\"metadata\":" << pmt_dict_to_json(pmt::dict_ref(m_pending_metadata, pmt::intern("metadata"), pmt::PMT_NIL));
        
        pmt::pmt_t err = pmt::intern("error");
        pmt::pmt_t ch_val = pmt::dict_ref(m_pending_metadata, pmt::intern("channel"), err);
        pmt::pmt_t sf_val = pmt::dict_ref(m_pending_metadata, pmt::intern("sf"), err);
        
        if (!pmt::eq(ch_val, err)) channel = pmt::to_long(ch_val);
        if (!pmt::eq(sf_val, err)) sf = pmt::to_long(sf_val);
    }
    
    json_ss << "}";
    
    // Publish
    std::string topic = "lora/" + m_gateway_id + "/rx/" + 
                        std::to_string(channel) + "/" + 
                        std::to_string(sf) + "/data";
                        
    publish_message(topic, json_ss.str());
    
    // Clear pending metadata
    m_pending_metadata = pmt::PMT_NIL;
#endif
}

void mqtt_sink_impl::handle_sensing(pmt::pmt_t msg)
{
#ifdef HAVE_PAHO_MQTT
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected) return;
    
    if (pmt::is_dict(msg)) {
        std::string topic = "lora/" + m_gateway_id + "/sensing";
        publish_message(topic, pmt_dict_to_json(msg));
    }
#endif
}

} /* namespace lora_concentrator */
} /* namespace gr */
