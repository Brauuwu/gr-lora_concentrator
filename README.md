# gr-lora_concentrator

A GNU Radio Out-Of-Tree (OOT) module designed to act as a LoRa Concentrator / Gateway. This module is built as an extension to [gr-lora_sdr](https://github.com/tapparelj/gr-lora_sdr) and adds real-world gateway capabilities including spectrum sensing, metadata extraction, MQTT bridging, and real-time remote configuration (e.g., dynamic CFO compensation).

## Overview

While `gr-lora_sdr` provides excellent physical layer (PHY) demodulation for LoRa, deploying an SDR as a true IoT gateway requires additional higher-level logic. `gr-lora_concentrator` bridges this gap by providing C++ blocks that seamlessly connect GNU Radio flowgraphs to IoT dashboards like Node-RED via MQTT.

### Key Features
* **Metadata Extraction**: Extracts SNR, RSSI, CFO (Carrier Frequency Offset), STO, and CRC validity from `gr-lora_sdr` stream tags.
* **Spectrum Sensing**: Calculates channel duty cycle and active energy (RSSI) over configurable measurement windows.
* **MQTT Publisher (Sink)**: Publishes received LoRa payloads (in HEX/ASCII) along with signal metadata to an MQTT broker.
* **MQTT Subscriber (Source)**: Subscribes to MQTT topics to receive downlink LoRa transmission requests and configuration updates.
* **MQTT Variable**: A standalone GRC variable block that subscribes to MQTT. This allows real-time tuning of flowgraph parameters (like CFO offset or TX Amplitude) directly from a web dashboard without restarting GNU Radio.

## Dependencies

* **GNU Radio 3.10+**
* **gr-lora_sdr** (must be installed and working)
* **Eclipse Paho MQTT C Client** (`libpaho-mqtt-dev` or `paho-mqtt-c`)
* **Pybind11** (for Python bindings)
* **nlohmann-json** (optional, for C++ JSON parsing)

## Installation (Ubuntu / Debian)

1. **Install Paho MQTT C Library**:
```bash
sudo apt-get update
sudo apt-get install libpaho-mqtt-dev
```

2. **Build and Install `gr-lora_concentrator`**:
```bash
git clone https://github.com/Brauuwu/gr-lora_concentrator.git
cd gr-lora_concentrator
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

## Available GRC Blocks

Once installed, the following blocks will appear in GNU Radio Companion under the `[lora_concentrator]` category:

1. **Metadata Extractor**: Taps into the baseband stream and `frame_info` tags to output a PMT dictionary of metadata.
2. **Channel Sensor**: Monitors a baseband channel for activity thresholding and duty cycle reporting.
3. **MQTT Publisher**: Converts PMT messages (payloads & metadata) into JSON and publishes them to MQTT topics (e.g., `lora/gw-001/rx/0/7/data`). Supports TLS and Last Will & Testament (LWT).
4. **MQTT Subscriber**: Listens for JSON transmission requests on MQTT (e.g., `lora/gw-001/tx/request`) and outputs PMT dictionaries to drive the TX chain.
5. **MQTT Variable**: Acts exactly like a standard GRC `Variable` block, but its value is updated dynamically via MQTT messages. Perfect for real-time CFO (Carrier Frequency Offset) tuning.

## Typical Integration with Node-RED

This module was heavily designed to interact with a Node-RED dashboard.
By using the **MQTT Variable** block for `cfo_offset` and `tx_amp`, you can use UI sliders in Node-RED to:
1. Dynamically tune the SDR's Center Frequency to compensate for LO drift (Clock Offset) against commercial LoRa nodes.
2. Dynamically reduce the TX Baseband Amplitude to prevent DAC clipping on SDRs like ADALM PLUTO.

### Example Node-RED MQTT Topics
* Publish `cfo_offset` to: `lora/gw/config/cfo_offset`
* Publish `tx_amp` to: `lora/gw/config/tx_amp`
* Receive data on: `lora/+/rx/+/+/data`

## License
This project is licensed under the GPLv3 License - see the LICENSE file for details.
