#ifndef INCLUDED_LORA_CONCENTRATOR_MQTT_SOURCE_H
#define INCLUDED_LORA_CONCENTRATOR_MQTT_SOURCE_H

#include <gnuradio/lora_concentrator/api.h>
#include <gnuradio/block.h>
#include <string>
#include <cstdint>

namespace gr {
namespace lora_concentrator {

/*!
 * \brief MQTT subscriber source block for downlink TX requests.
 *
 * Subscribes to MQTT topics and outputs received messages
 * via message ports for the LoRa TX chain.
 *
 * Message ports (output):
 *   - "tx_request"    : downlink TX request (PMT dict with payload + params)
 *   - "config_update" : runtime configuration update (PMT dict)
 *
 * Subscribes to:
 *   - lora/{gateway_id}/tx/request  (downlink messages)
 *   - lora/{gateway_id}/config      (runtime config)
 *
 * TX request JSON format:
 * {
 *   "frequency": 869525000,
 *   "sf": 12,
 *   "bw": 125000,
 *   "cr": 1,
 *   "payload_hex": "48656C6C6F",
 *   "tx_power": 14
 * }
 */
class LORA_CONCENTRATOR_API mqtt_source : virtual public gr::block
{
public:
    typedef std::shared_ptr<mqtt_source> sptr;

    /*!
     * \brief Create a new MQTT source block.
     *
     * \param broker_host    MQTT broker hostname
     * \param broker_port    MQTT broker port
     * \param client_id      MQTT client ID
     * \param gateway_id     Gateway identifier (used in topic paths)
     * \param username       MQTT username
     * \param password       MQTT password
     * \param qos            MQTT QoS level
     * \param tls_enabled    Enable TLS/SSL
     * \param ca_cert_path   Path to CA certificate
     */
    static sptr make(const std::string& broker_host,
                     int broker_port,
                     const std::string& client_id,
                     const std::string& gateway_id,
                     const std::string& username = "",
                     const std::string& password = "",
                     int qos = 1,
                     bool tls_enabled = false,
                     const std::string& ca_cert_path = "");
};

} // namespace lora_concentrator
} // namespace gr

#endif /* INCLUDED_LORA_CONCENTRATOR_MQTT_SOURCE_H */
