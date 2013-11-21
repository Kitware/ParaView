# Python Process Launcher

## Introduction

When deploying ParaViewWeb for multiple users, you will need a launcher module that
will start a new visualization process for each user that request one.
This task could be achieved by the JettySessionManager, but we wanted to provide
a simple answer without requirering any external component that is not already
available within the ParaView binaries.
Hence we build a Python based process launcher that follow the ParaViewWeb RESTful API
for launching new visualization process.
This document will first explain the expected RESTful API for launching a ParaViewWeb
process and then will focus on how to customize the service for a real deployment.

## Process launcher RESTful API

VTKWeb/ParaViewWeb come with a JavaScript library which allow the user to
trigger new process on the server side in a configurable manner.

The following code example illustrate what can be done on the client side and
we will explain what should be expected by the server.

    var config = {
       'sessionManagerURL' : 'http://localhost:8080/paraview',
       'application': 'loader',
       'key1': 'value1',
       'key2': 'value2',
       ...
       'keyN': 'valueN'
    }
    vtkWeb.start( config, function(connection){
       // Success callback
    }, function(code,reason){
       // Error callback
    });

The following client will trigger a __POST__ request on __http://localhost:8080/paraview__
with the given __config__ object as payload. As a response the server should return the
same __config__ object with additional keys such as:

- __sessionURL__: contains the WebSocket URL where the client should connect to in order
to connect to the newly started process. (ws://localhost:8080/proxy?id=2354623546)
- __secret__: contains the password that should be used to authenticate the client on the WebSocket connection.
- __id__: contains the session ID that can be used to query the launcher in order to retreive the full connection information.

In case of a 2 step connection, a client may want to trigger a __GET__ request on
__http://localhost:8080/paraview/${sessionID}__ in order to get the full __config__
object illustred earlier.

Then, the launcher should also be capable of stopping a running process by triggering
a __DELETE__ request on __http://localhost:8080/paraview/${sessionID}__ which will also
returned the same __config__ object illustred earlier.

### RESTful Cheatsheet

| URL                       | HTTP Method | Upload content | Download content |
|:-------------------------:|:-----------:|:--------------:|:----------------:|
| http://host:port/endpoint | __POST__    | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value'} | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value', __id__: '234523', __sessionURL__: 'ws://host:port/proxy?id=234523'} |
| http://host:port/endpoint/234523 | __GET__    | - | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value'} | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value', __id__: '234523', __sessionURL__: 'ws://host:port/proxy?id=234523'} |
| http://host:port/endpoint/234523 | __DELETE__    | - | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value'} | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value', __id__: '234523', __sessionURL__: 'ws://host:port/proxy?id=234523sdfg2weg'} |

## Python Launcher

### Configuration

The launcher server will rely on a configuration file that will provides all the required information
for the service to run. The following listing illustrate what that file could contains.

__launcher.config__

    {
      ## ===============================
      ## General launcher configuration
      ## ===============================

      "configuration": {
        "host" : "localhost",
        "port" : 8080,
        "endpoint": "paraview",                   # SessionManager Endpoint
        "content": "/.../www",                    # Optional: Directory shared over HTTP
        "proxy_file" : "/.../proxy-mapping.txt",  # Proxy-Mapping file for Apache
        "sessionURL" : "ws://${host}:${port}/ws", # ws url used by the client to connect to the started process
        "timeout" : 5,                            # Wait time in second after process start
        "log_dir" : "/.../viz-logs",              # Directory for log files
        "fields" : ["file", "host", "port"]       # List of fields that should be send back to client
      },

      ## ===============================
      ## Resources list for applications
      ## ===============================

      "resources" : [ { "host" : "localhost", "port_range" : [9001, 9003] } ],

      ## ===============================
      ## Set of properties for cmd line
      ## ===============================

      "properties" : {
        "build_dir" : "/.../build",
        "python_exec" : "/.../build/bin/vtkpython",
        "WWW" : "/.../build/www",
        "source_dir": "/.../src"
      },

      ## ===============================
      ## Application list with cmd line
      ## ===============================

      "apps" : {
        "cone" : {
          "cmd" : [
            "${python_exec}", "${build_dir}/Wrapping/Python/vtk/web/vtk_web_cone.py", "--content", "${WWW}", "--port", "$port", "-f"
              ],
          "ready_line" : "Starting factory"
        },
        "test" : {
          "cmd" : [
            "${python_exec}", "${build_dir}/PhylogeneticTree/server/vtk_web_phylogenetic_tree.py", "--content", "${WWW}" ],
          "ready_line" : "Starting factory"
        },
        "launcher" : {
          "cmd" : [
            "/home/kitware/launcher.sh", "${host}", "${port}", "${node}", "${app}", "${user}", "${password}", "${secret}" ],
          "ready_line" : "Good to go"
        }
      }
    }


In order to run that service, you will need to execute the following command line.

    $ cd ParaView/build
    $ ./bin/pvpython lib/site-package/vtk/web/launcher.py launcher.config

or inside VTK

    $ cd VTK/build
    $ ./bin/vtkpython Wrapping/Python/vtk/web/launcher.py launcher.config

Then once the service receive a POST request it will trigger a new command line which will have its output redirected to __/tmp/pw-log/${session_id}.log__ where the __${session_id}__ will be a unique generated string that will ID the given process.

For example, if the client send the given JSON payload

    {
    'sessionManagerURL': 'http://localhost:8080/paraview',
    'application': 'launcher',
    'node': 1024,
    'secret': 'katglrt54#%dfg',
    'user': 'sebastien.jourdain',
    'password': 'ousdfbdxldfgh',
    'app': 'Visualizer'
    }

The server will respond something like

    {
    'sessionManagerURL': 'http://localhost:8080/paraview',
    'application': 'launcher',
    'node': 1024,
    'secret': 'katglrt54#%dfg',
    'user': 'sebastien.jourdain',
    'password': 'ousdfbdxldfgh',
    'app': 'Visualizer',
    'id': '2345634574567',
    'sessionURL': 'ws://localhost:8080/proxy/2345634574567'
    }

After triggering something that will have the same effect as that command line

    $ '/home/kitware/launcher.sh' 'localhost' 9001 '1024' 'Visualizer' 'sebastien.jourdain' 'ousdfbdxldfgh' 'katglrt54#%dfg' > /tmp/pw-logs/2345634574567.log