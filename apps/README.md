Test Apps Usage
===============

This folder contains a few NDN applications that are used to test the functionality of the emulator.
Those applications provide a simple command line interface that takes in basic parameters.
All of them are programmed against ndn-cxx library and will be compiled together with the emulator.
This file explains the functionality of those applications and their usage.

Simple producer
---------------

The simple producer answers incoming Interest with Data packets.
It takes in two parameters: the NDN name under which it publishes the data,
and the Unix socket path to which it connects. The producer will append the current timestamp
to the end of the name specified by the command line every time it receives an request.
The `FreshnessPeriod` of all Data packets is set to 10 sec.

Example:

```
simple-producer /test/app /tmp/node0
```

Simple consumer
---------------

The simple consumer sends Interest periodically to a specific NDN name.
It takes in three parameters: the periodic requenst interval in milliseconds, the NDN name to which it sends the Interest,
and the Unix socket path to which it connects. If the request interval is less than or equal to zero,
the consumer will send only one Interest and exit immediately upon receiving the Data or a timeout notification.

Example:

```
simple-consumer 4000 /test/app /tmp/node1
```

Dummy sensor
------------

The dummy sensor emulates the behavior of a physical low-end sensor that generates raw data points every 5 seconds.
It maintains an internal sequence number that gets incremented by 1 every time it generates new data.
It does not hold any historical data. Any request to the data with a different sequence number from its current internal
number will be dropped silently.
It supports three communication modes: `POLL`, `PUSH`, and `NOTIFY`, which must match the communication mode of the repo.
The sensor itself can be reached via the prefix `/wsn/thermometer`.
This program is designed to run on top of [wsn-simple] (https://github.com/wentaoshang/ndn-em/tree/master/scenarios/wsn-simple)
and [wsn-complex] (https://github.com/wentaoshang/ndn-em/tree/master/scenarios/wsn-complex) scenarios.

Example:

```
dummy-sensor poll /tmp/node0
```

In-memory repo
--------------

The in-memory repo talks to the dummy sensor, pulls data out of the sensor and packages them as new NDN Data packets.
The Data packets are published under the name `/wsn/data/temperatur/{seqNum}`
and stored in a `std::map` (indexed by the sequence number).
The repo itself can be reached via the prefix `/wsn/repo`.
It supports three communication modes: `POLL`, `PUSH`, and `NOTIFY`, which must match the communication mode of the sensor.
This program is designed to run on top of [wsn-simple] (https://github.com/wentaoshang/ndn-em/tree/master/scenarios/wsn-simple)
and [wsn-complex] (https://github.com/wentaoshang/ndn-em/tree/master/scenarios/wsn-complex) scenarios.

Example:

```
in-memory-repo poll /tmp/node3
```
