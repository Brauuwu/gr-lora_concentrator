#ifndef INCLUDED_LORA_CONCENTRATOR_METADATA_EXTRACTOR_IMPL_H
#define INCLUDED_LORA_CONCENTRATOR_METADATA_EXTRACTOR_IMPL_H

#include <gnuradio/lora_concentrator/metadata_extractor.h>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace gr {
namespace lora_concentrator {

class metadata_extractor_impl : public metadata_extractor
{
private:
    int m_channel_index;
    uint32_t m_channel_freq;
    uint8_t m_sf;
    uint32_t m_bw;
    std::string m_gateway_id;

    // Frame tracking
    uint64_t m_frame_count;

    // Current frame metadata accumulation
    float m_snr;
    float m_rssi;
    int m_cfo_int;
    float m_cfo_frac;
    float m_sto;
    float m_sfo;
    bool m_crc_valid;
    bool m_has_sync_data;

    // RSSI estimation
    double m_signal_energy_acc;
    uint64_t m_signal_sample_count;

    std::mutex m_mutex;

    // Message handlers
    void handle_sync_log(pmt::pmt_t msg);

    // Internal helpers
    float estimate_rssi();
    pmt::pmt_t build_metadata_pmt();

public:
    metadata_extractor_impl(int channel_index,
                            uint32_t channel_freq,
                            uint8_t sf,
                            uint32_t bw,
                            const std::string& gateway_id);
    ~metadata_extractor_impl();

    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace lora_concentrator
} // namespace gr

#endif /* INCLUDED_LORA_CONCENTRATOR_METADATA_EXTRACTOR_IMPL_H */
