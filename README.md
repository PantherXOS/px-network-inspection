# px-network-inspection

PantherX Network Inspection

To build on ubuntu:

```
mkdir ./bin
cd bin/
cmake ..
make
```


To test:

`./px-network-inspection -f JSNO -o ./output`


To run on Pantherx, install the package version `0.0.11`.
Conditions:

* It works with openvpn default configuration: tune-based interface, default route, command-line openvpn execution.
* It only show the information about the pirmary route.
* User should not diable the tun interface.
* It does not account for irregular configuration route-table.
* It works for IPv4.
* It does not provide any firewall information.
* It does not provide any IPtabple information.
* It does not provide any bluetooth information.

The output is given in `JSON` format. It contains some records for each adapter and route. The pos field shows the relative position regarding to the internet.
We assume that public internet has 'pos=0', the physical adapter has `pos=1`, and the virtual adapter (vpn) has `pos=2`. The following is a sample output.

```
{ "primary": [ { "pos": 0, "adapter": "PUBLIC", "method": "NONE", "type": "display", "ip4": "37.59.236.227", "ip6": "", "dns": "", "gateway": "", "status": "ACTIVE" }, { "pos": 1, "adapter": "wlo1", "method": "WIFI", "type": "physical", "ip4": "192.168.0.13", "ip6": "", "dns": "", "gateway": "192.168.0.1", "status": "ACTIVE", "essid": "dlink_DWR-932_59DC" }, { "pos": 2, "adapter": "tun0", "method": "OPENVPN", "type": "virtual", "ip4": "172.16.100.93", "ip6": "", "dns": "", "gateway": "37.59.236.227", "status": "ACTIVE", "profile": "client_sinap" } ] }
```

