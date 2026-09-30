#ifndef INCLUDED_LORA_CONCENTRATOR_MQTT_SINK_H
#define INCLUDED_LORA_CONCENTRATOR_MQTT_SINK_H

#include <gnuradio/lora_concentrator/api.h>
#include <gnuradio/block.h>
#include <string>
#include <cstdint>

namespace gr {
namespace lora_concentrator {

/*!
 * \brief MQTT publisher sink block.
 *
 * Receives decoded LoRa payload and metadata via message ports
 * and publishes them to an MQTT broker as JSON messages.
 *
 * Message ports (input):
 *   - "rx_payload"   : received payload (PMT string or u8vector)
 *   - "rx_metadata"  : frame metadata (PMT dict)
 *   - "sensing"      : channel sensing data (PMT dict)
 *
 * MQTT Topics published:
 *   - lora/{gateway_id}/rx/{channel}/{sf}/data      (frame + metadata)
 *   - lora/{gateway_id}/rx/{channel}/{sf}/metadata   (metadata only)
 *   - lora/{gateway_id}/sensing                      (channel sensing)
 *   - lora/{gateway_id}/status                       (gateway status, retained)
 *   - lora/{gateway_id}/tx/ack                       (TX acknowledgment)
 *
 * Features:
 *   - Thread-safe publish queue
 *   - TLS/SSL support
 *   - Last Will & Testament (LWT) for offline detection
 *   - Auto-reconnection
 */
class LORA_CONCENTRATOR_API mqtt_sink : virtual public gr::block
{
public:
    typedef std::shared_ptr<mqtt_sink> sptr;

    /*!
     * \brief Create a new MQTT sink block.
     *
     * \param broker_host    MQTT broker hostname
     * \param broker_port    MQTT broker port
     * \param client_id      MQTT client ID
     * \param gateway_id     Gateway identifier (used in topic paths)
     * \param username       MQTT username (empty for anonymous)
     * \param password       MQTT password
     * \param qos            MQTT QoS level (0, 1, or 2)
     * \param tls_enabled    Enable TLS/SSL
     * \param ca_cert_path   Path to CA certificate file
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

#endif /* INCLUDED_LORA_CONCENTRATOR_MQTT_SINK_H */
