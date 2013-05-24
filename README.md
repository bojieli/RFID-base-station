RFID Base Station on Raspberry Pi
=================================

rx
--

Run on both master and slave devices.

Receive from NRF24L01 RF chip and send to master device.

merger
------

Run on master device.

* Receive from two ```rx``` devices.
* Merge packets into events of RFID coming in or out.
* Send the events to central server.
