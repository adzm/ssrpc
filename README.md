ssrpc - SSRP (SQL Server Resolution Protocol) Client
====================================================
Find Microsoft SQL Server instances via hostname or broadcast using SQL Browser

Usage
-----

- `ssrpc` will enumerate network interfaces and broadcast over (ipv4) udp
- `ssrpc hostname` will resolve hostname and send the request over udp via ipv4 or ipv6

Details
-------

ssrpc will send a single-byte UDP packet either to a specific host or broadcast to port 1434 (the SQL Browser port). The SQL Browser will respond with a list of instances and their details. Multicast for ipv6 is not yet supported.

Every instance will at least report the name of the instance, the version of MSSQL, and details of any enabled protocols including the TCP port and the named pipe path.

Broadcasting will wait for responses for 2 seconds before timing out. Unicast will wait 1 second.

`ERRORLEVEL` will be set to 0 on success, non-zero on failure.

Sample Output
-------------

```
Detecting network interfaces...
  interface 192.168.1.234 | ~255.255.255.0 = 192.168.1.255
Broadcasting...
  packet sent to 192.168.1.255:1434
Listening...

NEPTUNE\LARISSA
  ver: 10.50.6000.34
  np:  \\NEPTUNE\pipe\MSSQL$LARISSA\sql\query
  tcp: 51297
  ip:  192.168.1.234
NEPTUNE\TRITON
  ver: 11.0.5058.0
  np:  \\NEPTUNE\pipe\MSSQL$TRITON\sql\query
  tcp: 1534
  ip:  192.168.1.234

MARS\PHOBOS
  ver: 9.00.4035.00
  np:  \\MARS\pipe\MSSQL$PHOBOS\sql\query
  tcp: 50266
  ip:  192.168.1.235
MARS\DEIMOS
  ver: 10.50.2500.0
  ip:  192.168.1.235

Everything's shiny cap'n!
```

References
----------

For full specification see [MC-SQLR] at https://msdn.microsoft.com/en-us/library/cc219703.aspx
