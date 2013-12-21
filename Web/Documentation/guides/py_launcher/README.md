# Python Process Launcher

## Introduction

When deploying ParaViewWeb for multiple users, you will need a launcher module that
will start a new visualization process for each user that requests one.
This task could be achieved by the JettySessionManager. However, we wanted to provide
a simple answer that did not require the use of an external component that is not already
available within the ParaView binaries.
Hence, we built a Python based process launcher that follows the ParaViewWeb RESTful API
for launching a new visualization process.
This document will first explain the expected RESTful API for launching a ParaViewWeb
process. It will then focus on how to customize the service for a real deployment.

## Process launcher RESTful API

VTKWeb/ParaViewWeb come with a JavaScript library, which allows the user to
trigger a new process on the server side in a configurable manner.

The following code example illustrates what can be done on the client side, and
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
with the given __config__ object as payload. As a response, the server should return the
same __config__ object with additional keys such as:

- __sessionURL__: contains the WebSocket URL where the client should connect to in order
to connect to the newly started process. (ws://localhost:8080/proxy?id=2354623546)
- __secret__: contains the password that should be used to authenticate the client on the WebSocket connection.
- __id__: contains the session ID that can be used to query the launcher in order to retrieve the full connection information.

In the case of a two-step connection, a client may want to trigger a __GET__ request on
__http://localhost:8080/paraview/${sessionID}__ in order to get the full __config__
object illustrated earlier.

The launcher should now also be capable of stopping a running process by triggering
a __DELETE__ request on __http://localhost:8080/paraview/${sessionID}__. This will
return the same __config__ object illustrated earlier.

### RESTful Cheat sheet

| URL                       | HTTP Method | Upload content | Download content |
|:-------------------------:|:-----------:|:--------------:|:----------------:|
| http://host:port/endpoint | __POST__    | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value'} | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value', __id__: '234523', __sessionURL__: 'ws://host:port/proxy?id=234523'} |
| http://host:port/endpoint/234523 | __GET__    | - | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value'} | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value', __id__: '234523', __sessionURL__: 'ws://host:port/proxy?id=234523'} |
| http://host:port/endpoint/234523 | __DELETE__    | - | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value'} | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value', __id__: '234523', __sessionURL__: 'ws://host:port/proxy?id=234523sdfg2weg'} |

## Python Launcher

### Configuration

The launcher server will rely on a configuration file that will provide all of the information required
for the service to run. The following listing illustrates what that file could contain.

__launcher.config__

    {
      ## ===============================
      ## General launcher configuration
      ## ===============================

      "configuration": {
        "host" : "localhost",
        "port" : 8080,
        "endpoint": "paraview",                       # SessionManager Endpoint
        "content": "/.../www",                        # Optional: Directory shared over HTTP
        "proxy_file" : "/.../proxy-mapping.txt",      # Proxy-Mapping file for Apache
        "sessionURL" : "ws://${host}:${port}/ws",     # ws url used by the client to connect to started process
        "timeout" : 5,                                # Wait time in second after process start
        "log_dir" : "/.../viz-logs",                  # Directory for log files
        "upload_dir" : "/.../uploadDirectory",        # Start file upload server on same port as launcher
        "fields" : ["file", "host", "port", "updir"]  # Fields not listed are filtered from response
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

Then, once the service receives a POST request, it will trigger a new command line, which will have its output redirected to __/tmp/pw-log/${session_id}.log__ where the __${session_id}__ will be a unique generated string that will ID the given process.

For example, if the client sends the given JSON payload

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

A couple of notes about some elements in the configuration file:

The "sessionData" object is designed to allow you to specify that certain arbitrary key/value pairs should be included in the data returned to the client upon successful creation of the session.  In the example launcher config file above, we have added a key called "updir" with a value of "/Home" to the "sessionData" segment.  This will cause the launcher to perform normal substitutions on the on the value (in this case, none are needed as the value is simply "/Home"), and then include "updir": "/Home" in the response when sessions are successfully created for clients.

A caveat of this, however, is that the in the "configuration" section of the launcher config file, the "fields" key denotes a list of keys that will be used as a filter when deciding which data should be returned by the server in response to the client's JSON payload.  In other words, if you want a key/value pair returned to the client when the session has been successfully started, make sure to include the key name of this desired pair in the "fields" list of the "configuration" section.  In the example launcher config above, therefore, we have added the key "updir" to the "fields" filter list so that the "updir":"/Home" key pair will actually be returned to the client, instead of filtered out.

The launcher can now start an optional file upload server to handle multipart POST requests on the same server and port as the launcher itself, as in the launcher config file example, above.  By specifying an "upload_dir" property in the "configuration" section of the launcher config file, you can tell the launcher to start the file upload server in such a way that uploaded files will get written in the path given by the value of the "upload_dir" property.  In order to use the upload server, you would then issue a POST request with a multipart file attachment to the url "http://<launcher-host>:<launcher-port>/upload".  Of course you must actually replace <launcher-host> and <launcher-port> with the correct values.
