import os
import yaml
from gnuradio import gr, filter, blocks
import gnuradio.lora_sdr as lora_sdr
import gnuradio.lora_concentrator as lora_concentrator

class ConcentratorApp(gr.top_block):
    """
    Main Concentrator flowgraph using the C++ OOT blocks.
    """
    def __init__(self, config_path):
        gr.top_block.__init__(self, "LoRa SDR Concentrator", catch_exceptions=True)
        
        # Load config
        with open(config_path, 'r') as f:
            self.config = yaml.safe_load(f)
            
        self.gateway_id = self.config.get("gateway", {}).get("id", "gw-001")
        
        # 1. Source configuration
        sdr_cfg = self.config.get("sdr", {})
        self.samp_rate = sdr_cfg.get("sample_rate", 1000000)
        self.center_freq = sdr_cfg.get("center_freq", 868000000)
        sdr_type = sdr_cfg.get("type", "soapy").lower()
        sdr_gain = sdr_cfg.get("gain", 30)
        
        try:
            if sdr_type == "uhd":
                from gnuradio import uhd
                uhd_args = sdr_cfg.get("uhd_args", "")
                self.source = uhd.usrp_source(
                    ",".join(("", uhd_args)),
                    uhd.stream_args(cpu_format="fc32", args="", channels=list(range(1))),
                )
                self.source.set_samp_rate(self.samp_rate)
                self.source.set_center_freq(self.center_freq, 0)
                self.source.set_gain(sdr_gain, 0)
                self.source.set_antenna(sdr_cfg.get("antenna", "TX/RX"), 0)
                
            elif sdr_type == "osmosdr":
                from gnuradio import osmosdr
                self.source = osmosdr.source(args="numrecv=1")
                self.source.set_sample_rate(self.samp_rate)
                self.source.set_center_freq(self.center_freq, 0)
                self.source.set_freq_corr(0, 0)
                self.source.set_dc_offset_mode(2, 0)
                self.source.set_iq_balance_mode(2, 0)
                self.source.set_gain_mode(False, 0)
                self.source.set_gain(sdr_gain, 0)
                self.source.set_antenna(sdr_cfg.get("antenna", ""), 0)
                
            else:
                # Default to SoapySDR for "rtlsdr", "hackrf", "limesdr", "soapy"
                from gnuradio import soapy
                driver = sdr_type if sdr_type != "soapy" else sdr_cfg.get("soapy_driver", "")
                dev_args = f"driver={driver}" if driver else ""
                soapy_args = sdr_cfg.get("soapy_args", "")
                if soapy_args:
                    dev_args += f",{soapy_args}"
                    
                self.source = soapy.source(dev_args, "fc32", 1, "", "", [""], 
                                         [self.center_freq], [self.samp_rate], [sdr_gain])
                if sdr_cfg.get("antenna"):
                    self.source.set_antenna(0, sdr_cfg.get("antenna"))
                    
        except ImportError as e:
            print(f"Error loading SDR driver '{sdr_type}': {e}")
            print("Using null_source for testing.")
            self.source = blocks.null_source(gr.sizeof_gr_complex)
            
        # 2. MQTT Sink (Publisher)
        mqtt_cfg = self.config.get("mqtt", {})
        self.mqtt_sink = lora_concentrator.mqtt_sink(
            mqtt_cfg.get("broker_host", "localhost"),
            mqtt_cfg.get("broker_port", 1883),
            mqtt_cfg.get("client_id", "gw-pub"),
            self.gateway_id,
            mqtt_cfg.get("username", ""),
            mqtt_cfg.get("password", ""),
            mqtt_cfg.get("qos", 1),
            mqtt_cfg.get("tls_enabled", False),
            ""
        )
        
        # 3. Channels
        plan = self.config.get("frequency_plan", "EU868")
        ch_list = self.config.get("plans", {}).get(plan, {}).get("channels", [])
        
        if not ch_list:
            # Default EU868 channels
            ch_list = [{"freq": 868100000}, {"freq": 868300000}, {"freq": 868500000}]
            
        sfs = self.config.get("lora_rx", {}).get("spreading_factors", [7,8,9,10,11,12])
        channel_rate = 250000
        decimation = int(self.samp_rate / channel_rate)
        
        # We need to keep references to blocks to prevent garbage collection
        self.blocks = []
        
        for i, ch in enumerate(ch_list):
            freq = ch["freq"]
            freq_offset = freq - self.center_freq
            
            # Channelizer filter
            taps = filter.firdes.low_pass(1.0, self.samp_rate, 125000/2 * 1.2, 125000*0.2)
            xlating = filter.freq_xlating_fir_filter_ccc(decimation, taps, freq_offset, self.samp_rate)
            self.blocks.append(xlating)
            self.connect(self.source, xlating)
            
            # Channel Sensor
            sensor = lora_concentrator.channel_sensor(i, freq, channel_rate, -100.0, 100)
            self.blocks.append(sensor)
            self.connect(xlating, sensor)
            self.msg_connect((sensor, "sensing"), (self.mqtt_sink, "sensing"))
            
            # Receivers for each SF
            for sf in sfs:
                # Demodulation chain (gr-lora_sdr)
                frame_sync = lora_sdr.frame_sync(int(freq), 125000, sf, False, [0x34], int(channel_rate/125000), 8)
                fft_demod = lora_sdr.fft_demod(True, True)
                gray_mapping = lora_sdr.gray_mapping(True)
                deinterleaver = lora_sdr.deinterleaver(True)
                hamming_dec = lora_sdr.hamming_dec(True)
                header_decoder = lora_sdr.header_decoder(False, 1, 255, True, 2, True)
                dewhitening = lora_sdr.dewhitening()
                crc_verif = lora_sdr.crc_verif(True, False)
                
                # Metadata Extractor (C++ OOT)
                meta_ext = lora_concentrator.metadata_extractor(i, freq, sf, 125000, self.gateway_id)
                
                # Connections
                self.connect(xlating, frame_sync, fft_demod, gray_mapping, deinterleaver, hamming_dec, header_decoder, dewhitening, crc_verif)
                self.connect(xlating, meta_ext) # Metadata extractor taps into the input stream for RSSI
                
                # Message connections
                self.msg_connect((header_decoder, "frame_info"), (frame_sync, "frame_info"))
                
                # We need a Python block to translate frame_sync float port to PMT for metadata_extractor
                # For simplicity in this demo script, we assume the C++ block handles this or we just skip it
                # In a full implementation, we'd wrap the SyncLogExtractor logic.
                
                # Publish
                self.msg_connect((crc_verif, "msg"), (self.mqtt_sink, "rx_payload"))
                self.msg_connect((meta_ext, "metadata"), (self.mqtt_sink, "rx_metadata"))
                
                self.blocks.extend([frame_sync, fft_demod, gray_mapping, deinterleaver, hamming_dec, header_decoder, dewhitening, crc_verif, meta_ext])

if __name__ == '__main__':
    import sys
    config_file = "config/default_config.yaml"
    if len(sys.argv) > 1:
        config_file = sys.argv[1]
        
    print(f"Starting Concentrator with config: {config_file}")
    tb = ConcentratorApp(config_file)
    tb.start()
    print("Running... Press Enter to quit.")
    try:
        input()
    except EOFError:
        pass
    tb.stop()
    tb.wait()
