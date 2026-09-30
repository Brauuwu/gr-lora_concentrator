#ifndef INCLUDED_LORA_CONCENTRATOR_MQTT_VARIABLE_H
#define INCLUDED_LORA_CONCENTRATOR_MQTT_VARIABLE_H

#include <gnuradio/lora_concentrator/api.h>
#include <gnuradio/block.h>
#include <string>
#include <functional>

namespace gr {
namespace lora_concentrator {

/*!
 * \brief MQTT Variable block.
 *
 * Subscribes to a specific MQTT topic. When a message arrives, 
 * it triggers a Python callback. This is used in GRC to dynamically
 * update flowgraph variables (similar to QT GUI Entry/Slider) via MQTT.
 */
class LORA_CONCENTRATOR_API mqtt_variable : virtual public gr::block
{
public:
    typedef std::shared_ptr<mqtt_variable> sptr;

    /*!
     * \brief Create a new MQTT variable block.
     *
     * \param broker_host  MQTT broker hostname
     * \param broker_port  MQTT broker port
     * \param topic        MQTT topic to subscribe to
     */
    static sptr make(const std::string& broker_host,
                     int broker_port,
                     const std::string& topic);

    /*!
     * \brief Set the callback function to be called when a message arrives.
     * \param cb The callback function taking a string argument.
     */
    virtual void set_callback(std::function<void(std::string)> cb) = 0;
};

} // namespace lora_concentrator
} // namespace gr

#endif /* INCLUDED_LORA_CONCENTRATOR_MQTT_VARIABLE_H */
