How to write configuration files
================================

Basic idea
----------

The NDN emulator uses XML-formatted configuration files to describe network scenarios. Those files use pre-defined tags to specify the configuration parameters. The entire configuration file is wrapped inside a single root element using `Config` tag. Inside the root element there are three sections:

- `Links`, which describes individual LANs inside the network;
- `Nodes`, which describes individual NDN forwards inside the network;
- `Matrices`, which describes the connectivity information between different pairs of nodes on each link, using the adjacency matrix data structure.

We will explain how to write each section in the rest of this tutorial.

"Links" section
---------------

The `Links` section contains one or more `Link` elements. For now, there is only one mandatory attribute for links: the link id, which is a mnemonic name for the individual LANs. More attributes (such as link transmission rate) will be added later.

Here is an example of the `Links` section that defines a single LAN called "homenet0":

```xml
<Links>
  <Link>
    <Id>homenet0</Id>
  </Link>
</Links>
```

"Nodes" section
---------------

The `Nodes` section contains one or more `Node` elements. Each node requires the following attributes:

- Id: the mnemonic name of the node.
- Path: the Unix domain socket path which the NDN applications can connect to.
- CacheLimit: the size of the cache on the node (in bytes). This attribute is optional. If not specified, the default value is 100 KB.
- Devices: each node needs at least one network device to connect to some link. The `Devices` element contains one or more `Device` elements. Each device needs the following mandatory attributes:
  - DeviceId: the node-local mnemonic name of the device (e.g., eth0). It only has to be unique within each node.
  - LinkId: the id of the link which the device is attached to. The id must have appeared in the `Links` section.
- Routes: optional attribute to provide static routes for each node. The `Routes` element contains one or more `Route` elements. Each route has the following attributes:
  - Prefix: the URL-formatted NDN prefix.
  - Interface: the id of the network device where the matched Interests should be forwarded.
  - Nexthop: the id of the node on the same link who should receive the packet. This attribute is optional. If not specified, the default action is to broadcast to all nodes. If present, the id of the nexthop node must also appear in the `Nodes` section, but it may refer to nodes that are defined after the current node.

Here is an example of the `Nodes` section that defines a node called "n0" with a network device called "wn0" that is attached to "homenet0". It also has a default route to "wn0" and the nexthop is node "n1", which should be defined later in the configuration. If "n1" is not defined, the emulator will reject the configuration and throw an error.

```xml
<Nodes>
  <Node>
    <Id>n0</Id>
    <Path>/tmp/node0</Path>
    <Devices>
      <Device>
        <DeviceId>wn0</DeviceId>
        <LinkId>homenet0</LinkId>
      </Device>
    </Devices>
    <Routes>
      <Route>
        <Prefix>/</Prefix>
        <Interface>wn0</Interface>
        <Nexthop>n1</Nexthop>
      </Route>
    </Routes>
  </Node>
  ... (there also needs to be configurations for node n1)
</Nodes>
```

"Matrices" section
------------------

The `Matrices` section contains one or more `Matrix` elements. Each `Matrix` element needs to have the following attributes:

- LinkId: the id of the link that the connectivity matrix describes.
- Connections: contains one or more `Connection` elements. Each element describes the directed connectivity information between two nodes on the same link. It has the following attributes:
  - From: the id of the source node
  - To: the id of the destination node
  - LossRate: the packet loss rate during transmission

Here is an example of `Matrices` section that describes the connectivity between two nodes:

```xml
<Matrices>
  <Matrix>
    <LinkId>homenet0</LinkId>
    <Connections>
      <Connection>
        <From>n0</From>
        <To>n1</To>
        <LossRate>0.5</LossRate>
      </Connection>
      <Connection>
        <From>n1</From>
        <To>n0</To>
        <LossRate>0.5</LossRate>
      </Connection>
    </Connections>
  </Matrix>
</Matrices>
```

See ./scenarios/ folder for more examples of the configuration files.