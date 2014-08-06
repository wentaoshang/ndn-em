ndn-em
======

An NDN network emulator. Similar to ndnSIM but fully compatible with real NDN applications and ndn-cxx library. It emulates both physical network and NDN forwarder and talks to real NDN applications using Unix domain socket interface.


Getting started
---------------

The emulator is implemented and tested on Mac OS X 10.9 platform. To compile the source code, you need to install boost library and ndn-cxx library. The source code of ndn-cxx library can be found at https://github.com/named-data/ndn-cxx.

After you installed the dependencies, simply go to the root folder of the source code and run the following commands:

    ./waf configure
    ./waf

The the emulator program 'ndnem' will be generated and stored in ./build/ folder.

To run the emulator and other test applications, you need to configure your local computer with basic NDN security parameters, which include an identity and corresponding identity certificate. The easiest way is to generate a self-signed certificate for your own identity. (Note that this identity is not signed by any authorities on the NDN testbed and will not be trusted by other NDN hosts unless that host is configured to disable identity verification.)

Suppose your identity name is "/ndnem/user/alice" (it can be any hierarchical name), run the following commands to generate your identity and self-signed certificate:

    ndnsec-keygen /ndnem/user/alice | ndnsec-install-cert -

Then dump the certificate and store it into /usr/local/etc/ndn/keys folder so that ndn-cxx library can find it:

    sudo mkdir -p /usr/local/etc/ndn/keys
    ndnsec-cert-dump -i /ndnem/user/alice > default.ndncert
    sudo mv default.ndncert /usr/local/etc/ndn/keys/default.ndncert

When you are running ndnem, OS X may ask you to authorize the ndnem program to access the keychain when the program tries to get the keychain for the first time. Simply allow the access so that the emulator can use your identity to sign the Data packets generated during the prefix registration process.

Refer to NFD and ndn-cxx documents for more information about RIB management protocol.

You can try to run the test cases inside ./scenarios folder. For example, the following command starts the emulator with the "3-swithces" topology:

    ./build/ndnem ./scenario/3-swithces/nodes.txt ./scenarios/3-switches/links.txt

Then you can connect the simple consumer to node 0 and let the consumer resend Interest packet every 4000 ms:

    ./build/simple-consumer 4000 /tmp/node0

And then connect the simple producer to node 5:

    ./build/simple-producer /tmp/node5
