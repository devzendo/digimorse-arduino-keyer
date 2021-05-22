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

(C) 2020-2021 Matt J. Gumbley
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

Protocol
--------
The keyer uses the Arduino's Serial over USB protocol, so can be checked
with the Arduino IDE's serial console.

Events
======
Tapping the Morse key or paddle causes timing events to be emitted. The
first tap will emit a 'start of keying' code, an ASCII S.

As you key, at the end of the key down, a 'press' code will be emitted.
This is a '+' followed by a 16-bit big-engian unsigned binary number
giving the duration of the press in milliseconds.

When you next press the key, a 'release' code will be emitted. This is a
'-' followed by the duration of the release in milliseconds, as for '+'.

The keyer has a timeout of one second. Currently not configurable. One
second after the last event, an 'end of keying' event will be emitted.
This is an ASCII 'E'.

Commands
========
In addition to events from the keyer, commands can be issued to change
several settings, and to enquire of device status, version, etc.

All commands must start with a '>' (greater than) then the command letter,
with any value following immediately, then a return.

```
>V<return>         Display version info.
>!RESET!<return>   Reset to all defaults.

```
(more commands to be implemented in time....)

Protocol Summary
================

'S'       Start of keying.
'+' HH MM Press for HHMM milliseconds.
'-' HH MM Release for HHMM milliseconds.
'E'       End of keying.

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
directory. The only additional components are wires and an appropriate connector
for your Morse key / paddle, e.g. a 3.5mm stereo jack socket.

For a Morse key, the tip goes to Arduino pin D5.
For a paddle, dit goes to D5, dah goes to D4. (maybe, haven't tested that yet!)

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
(C) 2020-2021 Matt Gumbley, DevZendo.org


Acknowledgements
----------------
This project would not have been possible without the hard work and inspiration of many individuals.

Notably, thanks to:

SCoop - Simple Cooperative Scheduler for Arduino and Teensy ARM and AVR
https://github.com/fabriceo/SCoop


Bibliography
------------
TBC


