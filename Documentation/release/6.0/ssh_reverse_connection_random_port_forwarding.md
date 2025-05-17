## SSHCommand support random port forwarding port

SSHCommand port forwarding mechanism has been improved to now support
random port forwarding port, thanks to the `PV_SSH_PF_SERVER_PORT` variable.

Example syntax for a reverse connection server with a random forwarded port between 8080 and 88888:

```
<Server name="case18" resource="csrc://gateway:11115">
  <CommandStartup>
    <Options>
      <Option name="PV_SSH_PF_SERVER_PORT" label="Forwarding Port" readonly="true">
        <Range type="int" min="8080" max="8888" default="random" />
      </Option>
    </Options>
    <SSHCommand exec="/path/to/submit_script_pvserver.sh" delay="5">
      <SSHConfig user="user">
        <Terminal/>
        <PortForwarding/>
      </SSHConfig>
      <Arguments>
        <Argument value="--reverse-connection"/>
        <Argument value="--client-host=gateway"/>
        <Argument value="--server-port=$PV_SERVER_PORT$"/>
      </Arguments>
    </SSHCommand>
  </CommandStartup>
</Server>
```
