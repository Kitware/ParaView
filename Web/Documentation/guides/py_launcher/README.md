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

## Process launching RESTful API

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
    'url'        : 'http://localhost:8080/paraview',# Endpoint for the RESTful API
    'log'        : '/tmp/pw-logs',                  # Directory that will contains all the process output
    'map_file'   : '/var/www/proxy.map',            # Text file for the websocket proxy module (Apache 2.4+)
    'proxy_url'  : 'ws://localhost:8080/proxy/$id', # URL returned for sessionURL where $ID is replaced
    'wait_line'  : ['Starting factory', 'Ready'],   # The launcher only returns once a line contains that output or timeout
    'timeout'    : 10000,                           # The timeout value in seconds if no valid wait_line found
    'max_session': 10,                              # Prevent the start of two many concurrent process
    'resources'  : [ {'host': 'localhost', port: [9001, 9100]} ] # Resource range
    'applications': {
       'loader': {
          'cmd': ['$python_exec', '$server_base_path/pv_web_file_loader.py', '--data-dir', '$data_dir',
                  '--authKey', '$super_secret'],
          'dir': '/tmp'
       },
       'pipeline': {
          'cmd': ['$python_exec', '$server_base_path/pv_web_visualizer.py', '--data-dir', '$data_dir',
                  '--authKey', '$super_secret'],
          'dir': '/tmp'
       },
       'launcher': {
          'cmd': ['$launcher_exec', '$host', '$port', '$node', '$app', '$user', '$password', '$secret'],
          'dir': '/tmp'
       }
    },
    'properties': {
       'python_exec': '/home/kitware/ParaView/bin/pvpython',
       'launcher_exec': '/home/kitware/launcher.sh',
       'data_dir': '/home/kitware/ParaViewData/Data',
       'super_secret': 'paraviewweb-secret',
       'server_base_path': '/home/kitware/ParaView/lib/site-package/paraview/web'
    }
    }

In order to run that service, you will need to execute the following command line.

    $ cd ParaView
    $ ./bin/pvpython lib/site-package/paraview/web/launcher.py launcher.config

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