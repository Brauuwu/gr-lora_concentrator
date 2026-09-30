#ifndef INCLUDED_LORA_CONCENTRATOR_CHANNEL_SENSOR_H
#define INCLUDED_LORA_CONCENTRATOR_CHANNEL_SENSOR_H

#include <gnuradio/lora_concentrator/api.h>
#include <gnuradio/sync_block.h>
#include <string>
#include <cstdint>

namespace gr {
namespace lora_concentrator {

/*!
 * \brief Channel energy sensor for LoRa spectrum sensing.
 *
 * Performs energy detection on a single LoRa channel to measure:
 *   - RSSI (average signal power in dBm)
 *   - Activity detection (energy above threshold)
 *   - Duty cycle (fraction of time channel is active)
 *   - Peak RSSI tracking
 *
 * Input:  complex IQ samples (consumed, not passed through)
 * Output: none (sink block)
 * Message ports:
 *   - "sensing" (out): PMT dict with sensing report per measurement window
 *
 * Output message format (PMT dict):
 *   - channel_index: int
 *   - frequency: long (Hz)
 *   - rssi_db: double
 *   - active: bool
 *   - duty_cycle: double (0.0 - 1.0)
 *   - peak_rssi_db: double
 */
class LORA_CONCENTRATOR_API channel_sensor : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<channel_sensor> sptr;

    /*!
     * \brief Create a new channel sensor block.
     *
     * \param channel_index      Channel index number
     * \param channel_freq       Channel center frequency (Hz)
     * \param samp_rate          Sample rate (Hz)
     * \param energy_threshold_db Energy threshold for activity detection (dBm)
     * \param window_size_ms     Measurement window size (milliseconds)
     */
    static sptr make(int channel_index,
                     uint32_t channel_freq,
                     uint32_t samp_rate,
                     float energy_threshold_db = -100.0f,
                     int window_size_ms = 100);
};

} // namespace lora_concentrator
} // namespace gr

#endif /* INCLUDED_LORA_CONCENTRATOR_CHANNEL_SENSOR_H */
