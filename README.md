RFID Base Station on Raspberry Pi
=================================

## How to run it

Ensure that that Raspberry Pi is correctly plugged in and the 5 LEDs on the board is on.

Login to Raspberry Pi via SSH using PuTTY or cygwin.

```
cd rfid-base-station
screen
(press Enter twice)
cd merger
nodejs merger.js
(Ctrl+A Ctrl+D to exit screen)
screen
(press Enter twice)
cd rx
./main 50 -m -n your-mobile-number >rx.log
(Ctrl+A Ctrl+D to exit screen)
(Ctrl+D to exit SSH session)

```

Note: If you want to send warnings to multiple mobiles, separate them with comma (",").

## How to check receive log

Login to Raspberry Pi.

Goto the receiver directory (you can use TAB to autocomplete the directory name):
```cd rfid-base-station/rx```

Show last 10 lines of receive log and watch it:
```tail -f rx.log```

Show last 100 lines of receive log and watch it:
```tail -f rx.log -n 100```

Once you have finished watching the log, you can type ```Ctrl+C```.

Another approach is to use ```less``` utility:

Show last 1000 lines of receive log.
```tail rx.log -n 1000 | less```

Use ```Ctrl+U```, ```Ctrl+D```, ```j```, ```k``` to scroll up and down, type ```q``` to exit (Ctrl+C does not work here).


# Module Description

## rx

Run on both master and slave devices.

Receive from NRF24L01 RF chip and send to master device.

## merger

Run on master device.

* Receive from two ```rx``` devices.
* Merge packets into events of RFID coming in or out.
* Send the events to central server.
