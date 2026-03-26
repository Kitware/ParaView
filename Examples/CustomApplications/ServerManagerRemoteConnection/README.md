## Simple ParaView remote connection example

This example aims to show how to establish a remote connection to pvserver using the ParaView C++ API without using the Qt code.

To run the example, first start a pvserver, then run the example and give the URL with the `--url` argument:
```bash
./ServerManagerRemoteConnectionExample --url=cs://host:port
```
