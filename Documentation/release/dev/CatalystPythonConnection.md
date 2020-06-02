## ParaView catalyst live python API

ParaView now provides a python API to create live visualization with Catalyst.
The new `live` module includes the following method:
* `ConnectToCatalyst` to initiate communication with a running Catalyst server
* `ExtractCatalystData` to extract data from catalyst to client.
* `PauseCatalyst` to pause the simulation
* `ProcessServerNotifications` to look for catalyst notifications.

You can find an example of use in ParaView sources `Examples/PythonClient/catalystLiveVisualizer.py`
