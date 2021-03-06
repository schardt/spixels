16 LED strip Raspberry Pi Adapter
=================================

Adapter to connect up to 16 SPI-type LED strips (e.g. WS2801, LPD6803 or APA102).
The software to control these SPI outputs can be found in the `MultiSPI` class
in this project.

The output pins contain GND, CLK and Data. Also, 5V is given which can power
shorter LED strips or is useful for active termination of longer wires.

Pinout of each of the connectors:

GND | CLK | DATA | +5V
---:|:---:|:----:|:---

(GND is pointing towards the Rasbperry Pi GPIO header).

### Features

* 16 data outputs for LED strips including GND and 5V. All outputs receive data
  in parallel.
* Input power (5V) operates level shifter as well as the Raspberry Pi; no
  separate USB supply needed.
* Fused 5V outputs on connectors to prevent accidents.
* The pins of the I²C, UART and system-SPI busses (including both CS-lines)
  of the Raspberry Pi are not occupied by the data lines. They are broken out
  and allow connecting other common peripherals.

The CLK is shared, but each connector gets a dedicated buffered signal.

The 5V output, accessible on each of the 16 connectors, is fused, so that
accidental shorts when messing with the LED strip wires in the field do not
cause harm.

If you don't need 5V at all (using an external power supply), don't add a fuse.
For smaller power use (e.g. active termination), use a 1A fuse.
The outputs are beefy enough that they even could support a couple of
shorter 5V LED strips directly, in that case use up to 20A fuse. However,
generally it is advisable to separately power the strips;
20A only powers 300 LEDs or so.

If there is no fuse, then the 5V connectors are floating, but note that all
5V pins on that rail are connected with each other: if you use multiple
external power supplies, only connect GND, not 5V to prevent balancing currents.

### BOM
|Count | Type                        | Package      | Info
|-----:|-----------------------------|--------------|------------
|    4 | 74HCT245                    | TSSOP-20     | HCT or AHCT _not_ HC.
|    4 | 100nF ceramic capacitor     | 0805         | One per HCT245
|    1 | 22µF ceramic capacitor, 6.3V| 1206         | Stabilizing Pi Power
|    1 | 20x2 female pin socket      | 0.1" raster  | to connect to Pi GPIO
|   16 | 4x1 pin header              | 0.1" raster  | to connect to LED strips
|    1 | Fuse holder                 | for 20mm fuse| Optional (see text)

The board is also [shared on OSH Park][oshpark-pcb] (not affiliated).

![](../img/pi-adapter-pcb.png)

[oshpark-pcb]: https://oshpark.com/shared_projects/BTdhLFc3
