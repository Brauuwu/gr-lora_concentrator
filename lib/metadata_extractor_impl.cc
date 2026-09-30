#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <cmath>
#include <iostream>

#include "metadata_extractor_impl.h"

namespace gr {
namespace lora_concentrator {

metadata_extractor::sptr
metadata_extractor::make(int channel_index,
                         uint32_t channel_freq,
                         uint8_t sf,
                         uint32_t bw,
                         const std::string& gateway_id)
{
    return gnuradio::get_initial_sptr(
        new metadata_extractor_impl(channel_index, channel_freq, sf, bw, gateway_id)
    );
}

metadata_extractor_impl::metadata_extractor_impl(int channel_index,
                                                 uint32_t channel_freq,
                                                 uint8_t sf,
                                                 uint32_t bw,
                                                 const std::string& gateway_id)
    : gr::sync_block("metadata_extractor",
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     gr::io_signature::make(1, 1, sizeof(gr_complex))),
      m_channel_index(channel_index),
      m_channel_freq(channel_freq),
      m_sf(sf),
      m_bw(bw),
      m_gateway_id(gateway_id),
      m_frame_count(0),
      m_snr(0.0f),
      m_rssi(-120.0f),
      m_cfo_int(0),
      m_cfo_frac(0.0f),
      m_sto(0.0f),
      m_sfo(0.0f),
      m_crc_valid(false),
      m_has_sync_data(false),
      m_signal_energy_acc(0.0),
      m_signal_sample_count(0)
{
    // Register message ports
    message_port_register_in(pmt::mp("sync_log"));
    set_msg_handler(pmt::mp("sync_log"),
                    [this](pmt::pmt_t msg) { this->handle_sync_log(msg); });

    message_port_register_out(pmt::mp("metadata"));
}

metadata_extractor_impl::~metadata_extractor_impl()
{
}

void metadata_extractor_impl::handle_sync_log(pmt::pmt_t msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (pmt::is_dict(msg)) {
        pmt::pmt_t err = pmt::intern("error");
        
        pmt::pmt_t val_snr = pmt::dict_ref(msg, pmt::intern("snr"), err);
        if (!pmt::eq(val_snr, err) && pmt::is_real(val_snr)) {
            m_snr = pmt::to_double(val_snr);
        }
        
        pmt::pmt_t val_cfo = pmt::dict_ref(msg, pmt::intern("cfo"), err);
        if (!pmt::eq(val_cfo, err) && pmt::is_real(val_cfo)) {
            // In sync_log, cfo is total, but we'll extract it from tags later 
            // if available. Here we just store it.
            float cfo_total = pmt::to_double(val_cfo);
            m_cfo_int = std::round(cfo_total);
            m_cfo_frac = cfo_total - m_cfo_int;
        }
        
        pmt::pmt_t val_sto = pmt::dict_ref(msg, pmt::intern("sto"), err);
        if (!pmt::eq(val_sto, err) && pmt::is_real(val_sto)) {
            m_sto = pmt::to_double(val_sto);
        }
        
        pmt::pmt_t val_sfo = pmt::dict_ref(msg, pmt::intern("sfo"), err);
        if (!pmt::eq(val_sfo, err) && pmt::is_real(val_sfo)) {
            m_sfo = pmt::to_double(val_sfo);
        }
        
        m_has_sync_data = true;
    }
}

float metadata_extractor_impl::estimate_rssi()
{
    if (m_signal_sample_count == 0) return -120.0f;
    
    double avg_power = m_signal_energy_acc / m_signal_sample_count;
    float rssi_dbm = -120.0f;
    
    if (avg_power > 0) {
        // Rough calibration: power to dBm
        rssi_dbm = 10.0f * std::log10(avg_power) - 30.0f; 
    }
    
    // Reset accumulator
    m_signal_energy_acc = 0.0;
    m_signal_sample_count = 0;
    
    return rssi_dbm;
}

pmt::pmt_t metadata_extractor_impl::build_metadata_pmt()
{
    m_frame_count++;
    m_rssi = estimate_rssi();
    
    pmt::pmt_t meta = pmt::make_dict();
    
    // Core info
    meta = pmt::dict_add(meta, pmt::intern("gateway_id"), pmt::intern(m_gateway_id));
    meta = pmt::dict_add(meta, pmt::intern("channel"), pmt::from_long(m_channel_index));
    meta = pmt::dict_add(meta, pmt::intern("frequency"), pmt::from_long(m_channel_freq));
    meta = pmt::dict_add(meta, pmt::intern("sf"), pmt::from_long(m_sf));
    meta = pmt::dict_add(meta, pmt::intern("bw"), pmt::from_long(m_bw));
    meta = pmt::dict_add(meta, pmt::intern("frame_count"), pmt::from_uint64(m_frame_count));
    
    // Quality metadata dict
    pmt::pmt_t q = pmt::make_dict();
    q = pmt::dict_add(q, pmt::intern("snr"), pmt::from_double(m_snr));
    q = pmt::dict_add(q, pmt::intern("rssi"), pmt::from_double(m_rssi));
    
    // CFO
    q = pmt::dict_add(q, pmt::intern("cfo_int"), pmt::from_long(m_cfo_int));
    q = pmt::dict_add(q, pmt::intern("cfo_frac"), pmt::from_double(m_cfo_frac));
    
    // Convert CFO to Hz: cfo_total * bw / 2^sf
    float cfo_total = m_cfo_int + m_cfo_frac;
    float n_bins = 1 << m_sf;
    float cfo_hz = cfo_total * m_bw / n_bins;
    q = pmt::dict_add(q, pmt::intern("cfo_hz"), pmt::from_double(cfo_hz));
    
    // STO & SFO
    q = pmt::dict_add(q, pmt::intern("sto"), pmt::from_double(m_sto));
    q = pmt::dict_add(q, pmt::intern("sfo"), pmt::from_double(m_sfo));
    
    q = pmt::dict_add(q, pmt::intern("crc_valid"), pmt::from_bool(m_crc_valid));
    
    meta = pmt::dict_add(meta, pmt::intern("metadata"), q);
    
    return meta;
}

int metadata_extractor_impl::work(int noutput_items,
                                  gr_vector_const_void_star& input_items,
                                  gr_vector_void_star& output_items)
{
    const gr_complex *in = (const gr_complex *) input_items[0];
    gr_complex *out = (gr_complex *) output_items[0];

    // Pass through samples
    std::copy(in, in + noutput_items, out);
    
    // Accumulate energy for RSSI
    double energy = 0.0;
    for (int i = 0; i < noutput_items; i++) {
        energy += std::norm(in[i]); // mag squared
    }
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_signal_energy_acc += energy;
        m_signal_sample_count += noutput_items;
    }

    // Process tags
    std::vector<gr::tag_t> tags;
    get_tags_in_window(tags, 0, 0, noutput_items, pmt::intern("frame_info"));
    
    for (const auto& tag : tags) {
        if (pmt::is_dict(tag.value)) {
            pmt::pmt_t err = pmt::intern("error");
            
            // Check for header tag (starts the frame)
            pmt::pmt_t val_is_header = pmt::dict_ref(tag.value, pmt::intern("is_header"), err);
            if (!pmt::eq(val_is_header, err) && pmt::is_bool(val_is_header)) {
                bool is_header = pmt::to_bool(val_is_header);
                
                std::lock_guard<std::mutex> lock(m_mutex);
                
                if (is_header) {
                    // Start of frame, extract CFO from tag
                    pmt::pmt_t val_cfo_int = pmt::dict_ref(tag.value, pmt::intern("cfo_int"), err);
                    if (!pmt::eq(val_cfo_int, err) && pmt::is_integer(val_cfo_int)) {
                        m_cfo_int = pmt::to_long(val_cfo_int);
                    }
                    
                    pmt::pmt_t val_cfo_frac = pmt::dict_ref(tag.value, pmt::intern("cfo_frac"), err);
                    if (!pmt::eq(val_cfo_frac, err) && pmt::is_real(val_cfo_frac)) {
                        m_cfo_frac = pmt::to_double(val_cfo_frac);
                    }
                }
            }
            
            // Check for CRC tag (ends the frame)
            pmt::pmt_t val_crc = pmt::dict_ref(tag.value, pmt::intern("crc_valid"), err);
            if (!pmt::eq(val_crc, err) && pmt::is_bool(val_crc)) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_crc_valid = pmt::to_bool(val_crc);
                
                // End of frame, build and publish metadata
                pmt::pmt_t meta = build_metadata_pmt();
                message_port_pub(pmt::mp("metadata"), meta);
                
                // Reset flag for next frame
                m_has_sync_data = false;
            }
        }
    }

    return noutput_items;
}

} /* namespace lora_concentrator */
} /* namespace gr */
