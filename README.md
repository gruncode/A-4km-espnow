This project demonstrates esp32-s3 communication system using ESP-NOW protocol combined with principle of spread spectrum i.e frequency hopping over 3 channels. 
Suitable for sending simple commands on off, or some data where latency of eg a sec is not a problem.
In this example a simple alarm is broadcast.

Can reach 4km line-of-sight with 2db antennas. Can go to 1.5km with paper antennas line-of-sight.
Distance reached depends also largely on interference and good earthing on the systems.
Enable espnow Long Distance in menuconfig.

The same code is flashed to both transmitter and receiver devices
Devices determine their role based on their MAC address

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
