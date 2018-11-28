This adds SSH support in the pqServerConfiguration XML file.

Instead of relying to complex .pvsc file, it is now possible to use the following tags to configure connection through SSH:

* `<SSHCommand exec=...>` instead of `<Command exec=...> ` so the command will be executed through SSH
* `<SSHConfig user="user">`, child of `<SSHCommand>` with an optional argument to set the SSH user
* `<Terminal exec=/path/to/term/>`, child of `<SSHConfig>` with an optional terminal executable argument. When this tag is set, the SSH command will be executed through on a new terminal
* `<SSH exec=/path/to/ssh/>`, child of `<SSHConfig>` with an ssh executable argument. When this tag is set and the executable is specified, a specific ssh executable will be used
* `<Askpass/>`, child of `<SSHConfig>`, so an askpass program is used. Make sure to set SSH_ASKPASS and DISPLAY before using this. Incompatible with `<Terminal>` tag, only on Linux.
* `<PortForwarding local="port">`, child of `<SSHConfig>` with an optional local port argument, this is the biggest change. This allow to set up port forwarding through SSH tunneling. If no local port is defined, the server port will be used.

When `PortForwarding` is used, it is completely invisible to the user, remote host and port are correct and not related to the SSH tunneling. In order to inform the user that the communication between client and server are secured, the server icon in the pipeline browser is slightly different.

Example of a simple configuration file:
```XML
<Servers>
  <Server name="SimpleSshServer" configuration="" resource="cs://127.0.0.1:11111">
    <CommandStartup>
      <SSHCommand exec="/home/login/pv/startpvserver.sh" timeout="0" delay="5">
        <SSHConfig user="login">
          <Terminal/>
        </SSHConfig>
        <Arguments>
          <Argument value="$PV_SERVER_PORT$"/>
        </Arguments>
      </SSHCommand>
    </CommandStartup>
  </Server>
</Servers>
```

Also, the new environment variable `PV_SSH_PF_SERVER_PORT` should be set when performing reverse connection with  port forwarding.

In order to be able to use it you need:
 * On Linux: any terminal emulator, ssh (default on most distributions) and ssh_askpass if needed
 * On Windows: Preferably an installed Putty (plink), alternatively, windows 10 spring update SSH client, and cmd.exe (default)
 * On MacOS: Terminal.app and ssh (both default)
