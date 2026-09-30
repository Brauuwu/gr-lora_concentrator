#ifndef INCLUDED_LORA_CONCENTRATOR_METADATA_EXTRACTOR_H
#define INCLUDED_LORA_CONCENTRATOR_METADATA_EXTRACTOR_H

#include <gnuradio/lora_concentrator/api.h>
#include <gnuradio/sync_block.h>
#include <string>
#include <cstdint>

namespace gr {
namespace lora_concentrator {

/*!
 * \brief Extracts LoRa frame metadata (SNR, RSSI, CFO, STO, SFO)
 *        from stream tags and publishes them as PMT messages.
 *
 * This block taps into the gr-lora_sdr demodulation chain and
 * extracts signal quality metrics from frame_info tags placed
 * by the frame_sync and crc_verif blocks.
 *
 * Input:  complex IQ samples (pass-through)
 * Output: complex IQ samples (unchanged)
 * Message ports:
 *   - "metadata" (out): PMT dict with extracted metadata per frame
 *   - "sync_log" (in): receives sync log data from frame_sync
 *
 * Extracted metadata:
 *   - snr:       Signal-to-Noise Ratio (dB)
 *   - rssi:      Received Signal Strength Indicator (dBm)
 *   - cfo_int:   Carrier Frequency Offset, integer part (bins)
 *   - cfo_frac:  Carrier Frequency Offset, fractional part
 *   - cfo_hz:    CFO converted to Hz
 *   - sto_int:   Symbol Timing Offset, integer part (samples)
 *   - sto_frac:  Symbol Timing Offset, fractional part
 *   - sfo:       Sampling Frequency Offset
 *   - crc_valid: CRC check result (bool)
 *   - frame_cnt: Frame counter
 */
class LORA_CONCENTRATOR_API metadata_extractor : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<metadata_extractor> sptr;

    /*!
     * \brief Create a new metadata_extractor block.
     *
     * \param channel_index  Channel index number
     * \param channel_freq   Channel center frequency (Hz)
     * \param sf             Spreading factor
     * \param bw             Bandwidth (Hz)
     * \param gateway_id     Gateway identifier string
     */
    static sptr make(int channel_index,
                     uint32_t channel_freq,
                     uint8_t sf,
                     uint32_t bw,
                     const std::string& gateway_id);
};

} // namespace lora_concentrator
} // namespace gr

#endif /* INCLUDED_LORA_CONCENTRATOR_METADATA_EXTRACTOR_H */
