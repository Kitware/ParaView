## Timeout Connection Improvements

* Deprecration of current attribute `timeout` on the XML for the **server command** and replacing it with `process_wait`

This parameter let you set how long the client will wait for the "exec" content to be run in a process, in seconds.

It was previously:
```
  <Server name="SimpleServer" configuration="" resource="cs://localhost:11111">
    <CommandStartup>
      <Command exec="path/to/server.sh" timeout="3" delay="5"/>
    </CommandStartup>
  </Server>
```

It should now be:
```
  <Server name="SimpleServer" configuration="" resource="cs://localhost:11111">
    <CommandStartup>
      <Command exec="path/to/server.sh" process_wait="3" delay="5"/>
    </CommandStartup>
  </Server>
```

* Adding a new `timeout` attribute on the XML of the **server** and connect it to the timeout spinbox in the UI

This new parameter let you control how long will the client try to connect to the server, in seconds.
Default is 60 for standard connection and infinite for reverse connection.
You can now write:
```
  <Server name="SimpleServer" configuration="" resource="cs://localhost:11111" timeout="15">
    <CommandStartup>
      <Command exec="path/to/server.sh"/>
    </CommandStartup>
  </Server>
```

* Add support for connection abort in all cases (except catalyst). Before that it was only limited to reverse connection.
* Add support for connection retry in case of timeout on connection.
* Add support for timeout with reverse connections, client side only.
