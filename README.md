
This project demonstrates a robust esp32-s3 communication system using ESP-NOW protocol combined with principle of spread spectrum i.e frequency hopping over 3 channels. 
The implementation TX a simple alarm broadcast system that works over 4km distances.

4km line-of-sight with 2db antennas. Can go to 1.5km with paper antennas line-of-sight.

How It Works
Core Features
Unified Codebase: The same code is flashed to both transmitter and receiver devices
Automatic Role Detection: Devices determine their role based on their MAC address
Channel Hopping: Communicates across WiFi channels 1, 6, and 13 to improve reliability
Long Range: Optimized for long-distance communication (4km+ tested)

Communication Flow

Transmitter (TX) Operation:
Cycles through WiFi channels (1→6→13) spending 1000ms on each
Broadcasts "ALARM!" messages every 5ms while on each channel
Logs transmission statistics for each channel

Receiver (RX) Operation:
Rapidly hops between channels (every 20ms) to catch transmissions
Counts received messages across all channels
Reports reception statistics every 3 seconds

How to Use This Code

Hardware Requirements:
Two esp32-s3 devices (any variant)
Antennas 2db at least for 4km

Setup:
Identify the MAC addresses of both ESP devices using:
Edit the RX_MAC and TX_MAC constants in the code with your devices' addresses
Build and flash the same code to both devices using ESP-IDF

Operation:
The devices will automatically determine their role based on their MAC address
The TX device will begin broadcasting alarm messages
The RX device will begin scanning channels and reporting reception statistics
Monitor debug output via serial console to see transmission/reception statistics

Customization:
Adjust timing parameters (TX_CHANNEL_DURATION, RX_CHANNEL_DURATION, etc.) for different environments
Modify the message content in alarm_tx_task() function as needed

Performance Notes
The channel hopping strategy maximizes the chance of successful communication over long distances
Balanced power consumption vs. reliability with tunable parameters
Default configuration tested successfully over 4km line-of-sight
