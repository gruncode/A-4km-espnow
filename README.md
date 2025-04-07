This project demonstrates an esp32-s3 communication system using the ESP-NOW protocol combined with the principle of spread spectrum by doing frequency hopping over 3 wifi channels. 
Suitable for sending simple commands e.g on/off, or some basic data, and generaly where a latency of eg a sec is not a problem.
In te code example a simple alarm is broadcast.

Can reach 4km line-of-sight with 2db antennas. Can go to 1.5km with paper antennas line-of-sight.
Distance reached depends also largely on interference and good earthing on the systems.
Enable espnow Long Distance in menuconfig.

The same code is flashed to both transmitter and receiver devices.
Devices determine their role based on their MAC address, which has to be entered in the code manually.

Communication Flow:

Transmitter (TX) Operation:
Cycles through WiFi channels (1→6→13) spending 1000ms on each.
Broadcasts "ALARM!" messages every 5ms while on each channel.
Counts and prints the send messages cumulatively across all channels.

Receiver (RX) Operation:
Rapidly hops between channels (every 20ms) to catch transmissions.
Counts received messages (if received at all) cumulatively across all channels every 3 seconds.

You can play with timing parameters (TX_CHANNEL_DURATION, RX_CHANNEL_DURATION, etc.)

Can also enable espnow power save (periodic sleep wakeup) but performance is degraded.

compiled with espidf v5.3.2 and python3.9
