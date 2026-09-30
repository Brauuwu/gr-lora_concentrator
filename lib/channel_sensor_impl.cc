#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <cmath>
#include <iostream>

#include "channel_sensor_impl.h"

namespace gr {
namespace lora_concentrator {

channel_sensor::sptr
channel_sensor::make(int channel_index,
                     uint32_t channel_freq,
                     uint32_t samp_rate,
                     float energy_threshold_db,
                     int window_size_ms)
{
    return gnuradio::get_initial_sptr(
        new channel_sensor_impl(channel_index, channel_freq, samp_rate, 
                                energy_threshold_db, window_size_ms)
    );
}

channel_sensor_impl::channel_sensor_impl(int channel_index,
                                         uint32_t channel_freq,
                                         uint32_t samp_rate,
                                         float energy_threshold_db,
                                         int window_size_ms)
    : gr::sync_block("channel_sensor",
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     gr::io_signature::make(0, 0, 0)),
      m_channel_index(channel_index),
      m_channel_freq(channel_freq),
      m_samp_rate(samp_rate),
      m_energy_threshold_db(energy_threshold_db),
      m_window_size_ms(window_size_ms),
      m_sample_count(0),
      m_energy_buffer(0.0),
      m_total_measurements(0),
      m_active_count(0),
      m_current_rssi_db(-120.0f),
      m_peak_rssi_db(-120.0f),
      m_is_active(false)
{
    m_window_samples = (uint32_t)((samp_rate * window_size_ms) / 1000.0);
    if (m_window_samples < 1) m_window_samples = samp_rate / 10; // Default 100ms
    
    message_port_register_out(pmt::mp("sensing"));
}

channel_sensor_impl::~channel_sensor_impl()
{
}

void channel_sensor_impl::perform_measurement()
{
    if (m_sample_count == 0) return;
    
    double avg_power = m_energy_buffer / m_sample_count;
    
    if (avg_power > 0) {
        m_current_rssi_db = 10.0f * std::log10(avg_power) - 30.0f; // Approx dBm
    } else {
        m_current_rssi_db = -120.0f;
    }
    
    if (m_current_rssi_db > m_peak_rssi_db) {
        m_peak_rssi_db = m_current_rssi_db;
    }
    
    m_is_active = (m_current_rssi_db > m_energy_threshold_db);
    
    m_total_measurements++;
    if (m_is_active) {
        m_active_count++;
    }
    
    double duty_cycle = 0.0;
    if (m_total_measurements > 0) {
        duty_cycle = (double)m_active_count / m_total_measurements;
    }
    
    // Publish PMT report
    pmt::pmt_t msg = pmt::make_dict();
    msg = pmt::dict_add(msg, pmt::intern("channel_index"), pmt::from_long(m_channel_index));
    msg = pmt::dict_add(msg, pmt::intern("frequency"), pmt::from_long(m_channel_freq));
    msg = pmt::dict_add(msg, pmt::intern("rssi_db"), pmt::from_double(m_current_rssi_db));
    msg = pmt::dict_add(msg, pmt::intern("active"), pmt::from_bool(m_is_active));
    msg = pmt::dict_add(msg, pmt::intern("duty_cycle"), pmt::from_double(duty_cycle));
    msg = pmt::dict_add(msg, pmt::intern("peak_rssi_db"), pmt::from_double(m_peak_rssi_db));
    
    message_port_pub(pmt::mp("sensing"), msg);
    
    // Reset buffer
    m_energy_buffer = 0.0;
    m_sample_count = 0;
}

int channel_sensor_impl::work(int noutput_items,
                              gr_vector_const_void_star& input_items,
                              gr_vector_void_star& output_items)
{
    const gr_complex *in = (const gr_complex *) input_items[0];

    for (int i = 0; i < noutput_items; i++) {
        m_energy_buffer += std::norm(in[i]);
        m_sample_count++;
        
        if (m_sample_count >= m_window_samples) {
            perform_measurement();
        }
    }

    // We consumed all input items
    return noutput_items;
}

} /* namespace lora_concentrator */
} /* namespace gr */
