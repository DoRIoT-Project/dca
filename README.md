# DoRIoT Data Collection Agent

This agent for embedded devices will collect information regarding QoS, CPU/RAM capacity, and load.
It offers this information through a file system to the DoRIoT VM and through CoAP to optimization and/or broker entities.

## Getting Started

Clone this repository somewhere, and set your `RIOTBASE` properly.

The examples build as normal RIOT applications, but need to be configured with `make menuconfig` before building.

	export RIOTBASE=foo/bar/baz    # if in doubt, check out RIOT version 21.07
	make BOARD=native menuconfig   # just save the default config to get started
	make BOARD=native
	make BOARD=native term

## Using the Data Collection Agent in your RIOT Project

In your RIOT application's `Makefile`, settle `EXTERNAL_MODULE_DIRS` properly.
Refer to the examples, or to the "exernal modules" section in the RIOT docs.

	EXTERNAL_MODULE_DIRS += <your external module folder (containing doriot_dca directory)>
	USEMODULE += doriot_dca

## The Database

The database is a read-only key/value storage with hierarchical keys.
Keys are similar to a file path, e.g. `/board/mcu`.
Values are typed.
Data types are either `int32t`, `float`, or strings.

To see a database dump, use the shell command `dcadump`.

## DCA Shell

The database can be accessed via the shell for debugging purposes.

Various commands are available:

- `dcaq` enables you to query the value of a single key, e.g., `dcaq board/mcu`.

- `dcadump` prints a complete dump of the database.

- `dcalat` and `dcatp` are used to trigger QoS measurements, see below.

## CoAP

The database can be accessed via CoAP by issuing a GET request to the URI `coap://<ipv6addr>/dca`.
Refer to the coap example, which shows a minimal application that exposes the database via CoAP.

An example query could be:

	coap-client -mGET -t0 coap://[fe80::2c60:daff:fef2:d242%tapbr0]/dca/runtime/ps/idle/pid

which returns the PID of the idle process.

Beware that security instruments are not yet implemented, but will include capability tokens (with [LCap](https://code.ovgu.de/doriot/wp4/lcap)) and transport encryption in the future, so that information access can restricted to trusted users.

## Network QoS Measurements

When networking is enabled, the `dcalat` and `dcatp` commands can be used to measure QoS parameters to all known neighbors.

Neighbors must have been discovered first, e.g., via a ping, so that they are known.
When a communication was once established, the neighbor should show up under the respective `netif` device.
After issuing the above commands, the QoS should show up as well.

The database uses the gnrc neighbor cache (nib) to find neighbors.
lwip is not supported at the moment.

## Tested Boards

- `native`
- `nucleo-f429zi`
- `esp32-wroom-32`
- `nrf52840dk`
- `nrf52840dongle`

Tested with *RIOT version 21.07*
