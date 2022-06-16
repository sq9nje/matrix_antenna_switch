# Matrix Antenna Switch

This is a 6x2 antenna switch which allows to connect two TRXes to up to 6 antennas. The device currently consists of two boards: tha main switchong board and an add-on coomunications module. The communications module allows the switch to be controlled remotely via RS-485.

![Main switching module](/images/antenna_switch.png?raw=true "Main switching module")

## Parameters
The switch provides decent isolation between the 2 TRX ports.
- better than 75dB below 10MHz
- better than 65db below 30MHz
- better than 50dB below 50MHz

![Isolation graph](/images/6x2_iso.png?raw=true "Isolation graph")

## Control Software

A python app that provides a WebUI to control the switch is provided. The app reqiures that a serial-to-RS485 or USB-to-RS485 converter is connected to the machine it is running on (in our set up this is usually a Raspberry Pi somewhere in the shack floor ;) )