digimorse-arduino-keyer
=======================
This is an Arduino Nano-based Morse key/paddle <-> USB Serial interface and simple keyer, for use in the DigiMorse project.

It has been built using the Arduino IDE v1.6.4 on Windows 10, and uploaded to a third-party Arduino Nano, using the CH340 USB Serial chip.

https://www.amazon.co.uk/gp/product/B072BMYZ18/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1

https://www.elegoo.com/products/elegoo-nano-v3-0

I'd normally use a genuine Arduino, which I'd recommend, but had this to hand.
It's £10.99 for a pack of 3 in Nov 2020. The genuine one is £21.60 (for one). 

Use of this variant requires downloading their CH340 USB driver.

It is part of the [DigiMorse Project](https://devzendo.github.io/digimorse).

(C) 2020 Matt J. Gumbley
matt.gumbley@devzendo.org
@mattgumbley @devzendo
http://devzendo.github.io/digimorse


Status
------
Project started September 2020. Status: initial investigations, feasiblilty,
thoughts.

In active development:
* Building the keyer.

Roadmap
-------
First release:
* Sends on/off keying data to the host computer.
* Can sense switches of key or paddle.
* Straight key signalling only (no keyer).

Second release:
* Accepts commands, implements keyer.
* Sidetone generated on-board?

Release Notes
-------------
0.0.1 First Release (work in progress)
* Created repository!

 

Uploading to your Nano
----------------------
Full documentation to follow.. but in the meantime...

Load digimorse-arduino-keyer.ino into the Arduino IDE.
Connect your Arduino Nano via USB.
Choose the model and port.
Upload...

In the IDE's Serial Monitor, send a V followed by return. You should see some
version information returned.

Wire up the Nano as shown in the schematic / wiring diagram in the Docs
directory.

Connect a Morse key / paddle. Tapping the key / paddle should yield single bytes
on the Serial Monitor.

Source directory structure
--------------------------
The source is split into the following directories:

Libraries - third-party libraries I've used in this project

Docs - pinouts and schematic diagrams.

This directory - the main code, and SCoop library.



License
-------
This code is released under the Apache 2.0 License: http://www.apache.org/licenses/LICENSE-2.0.html.
(C) 2020 Matt Gumbley, DevZendo.org


Acknowledgements
----------------
This project would not have been possible without the hard work and inspiration of many individuals.

Notably, thanks to:

SCoop - Simple Cooperative Scheduler for Arduino and Teensy ARM and AVR
https://github.com/fabriceo/SCoop


Bibliography
------------
TBC


