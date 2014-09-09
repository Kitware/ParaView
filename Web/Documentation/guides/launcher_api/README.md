# Launcher API Documentation

## Introduction

This document will cover the expected RESTful API for launching a ParaViewWeb process.  For details on specific launcher implementations, see either the [Python Launcher](index.html#!/guide/python_launcher) guide or the [Jetty Launcher](index.html#!/guide/jetty_session_manager) guide.  Also, see the [Launching Examples](index.html#!/guide/launching_examples) guide for descriptions of some of the deployment configurations possible with either of these provided launchers.

## Role of the launcher

The launcher has a well-defined set of responsibilities:

1. listen for incoming client requests for new visualization process,
1. find an available "resource" from list it maintains (a "resource" is simply a [host, port] combination),
1. launch the requested command-line,
1. wait for that command-line process to be ready and when it is ready, note the session-id, host, and port in the mapping file,
1. and finally, respond to the client with the proper information (including at least the sessionURL where the client can reach the running visualization process)

## Process launcher RESTful API

VTKWeb/ParaViewWeb come with a JavaScript library, which allows the user to trigger a new process on the server side in a configurable manner.

The following code example illustrates what can be done on the client side, and we will explain what should be expected by the server.

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

The preceeding client code will trigger a __POST__ request on __http://localhost:8080/paraview__ with the given __config__ object as payload.  In response, the server will first start the application specified in the config __application__ value, and then it should  return the initial __config__ object with additional keys such as:

- __sessionURL__: contains the WebSocket URL where the client should connect to in order
to connect to the newly started process. (ws://localhost:8080/proxy?id=2354623546)
- __secret__: contains the password that should be used to authenticate the client on the WebSocket connection.
- __id__: contains the session ID that can be used to query the launcher in order to retrieve the full connection information.

In the case of a two-step connection, a client may want to trigger a __GET__ request on __http://localhost:8080/paraview/${sessionID}__ in order to get the full __config__ object illustrated earlier.

The launcher should now also be capable of stopping a running process by triggering a __DELETE__ request on __http://localhost:8080/paraview/${sessionID}__. This will return the same __config__ object illustrated earlier.

### RESTful Cheat sheet

| URL                       | HTTP Method | Upload content | Download content |
|:-------------------------:|:-----------:|:--------------:|:----------------:|
| http://host:port/endpoint | __POST__    | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value'} | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value', __id__: '234523', __sessionURL__: 'ws://host:port/proxy?id=234523'} |
| http://host:port/endpoint/234523 | __GET__    | - | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value'} | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value', __id__: '234523', __sessionURL__: 'ws://host:port/proxy?id=234523'} |
| http://host:port/endpoint/234523 | __DELETE__    | - | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value'} | { __sessionManagerURL__: 'http://host:port/endpoint', __application__: 'value', __id__: '234523', __sessionURL__: 'ws://host:port/proxy?id=234523sdfg2weg'} |

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

The server should first run the command line associated with the application `launcher` (see the [Python Launcher](index.html#!/guide/python_launcher) and [Jetty Launcher](index.html#!/guide/jetty_session_manager) guides for information about how the application commands lines are configured).  The server running the command line in this case would have the same effect as if you manually ran:

    $ '/home/kitware/launcher.sh' 'localhost' 9001 '1024' 'Visualizer' 'sebastien.jourdain' 'ousdfbdxldfgh' 'katglrt54#%dfg' > /tmp/pw-logs/2345634574567.log

Once the command line is running, the server should respond something like:

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
