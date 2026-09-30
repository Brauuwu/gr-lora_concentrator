#ifndef INCLUDED_LORA_CONCENTRATOR_CHANNEL_SENSOR_IMPL_H
#define INCLUDED_LORA_CONCENTRATOR_CHANNEL_SENSOR_IMPL_H

#include <gnuradio/lora_concentrator/channel_sensor.h>
#include <chrono>

namespace gr {
namespace lora_concentrator {

class channel_sensor_impl : public channel_sensor
{
private:
    int m_channel_index;
    uint32_t m_channel_freq;
    uint32_t m_samp_rate;
    float m_energy_threshold_db;
    int m_window_size_ms;

    uint32_t m_window_samples;
    uint32_t m_sample_count;
    double m_energy_buffer;

    // Stats
    uint64_t m_total_measurements;
    uint64_t m_active_count;
    float m_current_rssi_db;
    float m_peak_rssi_db;
    bool m_is_active;

    void perform_measurement();

public:
    channel_sensor_impl(int channel_index,
                        uint32_t channel_freq,
                        uint32_t samp_rate,
                        float energy_threshold_db,
                        int window_size_ms);
    ~channel_sensor_impl();

    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace lora_concentrator
} // namespace gr

#endif /* INCLUDED_LORA_CONCENTRATOR_CHANNEL_SENSOR_IMPL_H */
