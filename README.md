# arduino-radio-control-27mhz

## Arduino-based transmitter for controlling 27 MHz RC toys using TX-2/RX-2 or TX-6B/RX-6B chipsets

## Connection Method

To use an Arduino as a replacement transmitter, connect it to the RF front-end of the original remote as follows:

* Arduino signal pin → DATA OUT of the TX-2 / TX-6B encoder before the RF stage

* Arduino power-control pin → PC (Power Control Output) of the original encoder. This pin powers the RF stage during transmission. Provide the same voltage level that the original chip outputs (typically 2.9–3.3 V). You may connect directly or through a voltage divider if required.

* GND (Arduino) → GND of the transmitter board. All grounds must be common.

This setup allows the Arduino to fully control the RF modulator and transmit correctly encoded command packets without removing the original circuitry.

![RC](cheme.png)
