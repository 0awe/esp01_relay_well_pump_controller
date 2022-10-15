# esp01_relay_well_pump_controller

ESP01 (ESP8266) and ESP-01S v1.0 relay board setup to control a large 7.5HP agricultural well pump.

The relay board has been modified to use GPIO3, instead of GPIO0 (default), to prevent the relay from
being trigger on startup:

https://github.com/IOT-MCU/ESP-01S-Relay-v4.0/issues/1#issuecomment-574480241

Futhermore, the relay board has been modified by adding a jumper between Vcc (pin 8) and CH_EN (pin 4).
DO NOT REMOVE R2, you'll need it for the GPIO3 mod mentioned above.

https://www.youtube.com/watch?v=OCWvhxCiyw4

Little more details on the setup:

1. The ESP01 module connects to a wireless network that's routing all traffic to a Raspberry Pi connected to an Android smartphone.
2. The cellular connection is naturally double NAT'd by the cellular provider, therefore, a Raspberry Pi is keeping a constant reverse SSH tunnel to an externally accessible ISP connection (also on RPi).
3. The web interface is running on the Lighttpd utilizing a virtual proxy from the SSH tunnel.
