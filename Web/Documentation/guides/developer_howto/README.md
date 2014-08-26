# Developer introduction to ParaViewWeb

## Introduction

ParaViewWeb is a framework which expose VTK or ParaView processing
and visualization capabilities through Web protocols.
This allow the creation of Web Server or Web Sites that generate
interactive 3D rendering as well as interactive processing
capabilities.
ParaViewWeb is distributed along the ParaView binaries which
allow local testing or deployment without any code compilation.

Let's look at the components that are used within that framework
and see how they interact in order to build the ParaViewWeb framework.

## Server side of ParaViewWeb

### Thirdparty

Python Thirdparty libraries:

1. __Autobahn__: WAMP implementation using WebSocket and Long Polling.
2. __Twisted__: Web server used by autobahn to expose its WebSocket resource.
3. __Six__: Used by Autobahn to handle Python 2 and 3.
4. __ZopeInterface__: Library used by Twisted to provide abstract interface.

### Components

ParaViewWeb components:

1. server.py
2. protocols.py
3. vtkWebApplication / vtkPVWebApplication
4. Your server application (i.e.: pv_web_visualizer.py, vtk_web_cone.py...)

#### server.py

The __server__ module provides helper functions that will register standard
arguments and handle them when the __start_webserver__ method will be called.
This main function will configure and use Twisted to start a Web server on a
given port. The protocol argument of that function will be use to create
a WebSocket endpoint.

The following arguments are used to configure that web server:

1. __--port__ (default:8080): Which port to start the Web Server on and listen to.
2. __--content__ (optional): If provided, the web server will serve the content of the provided directory.
3. __--authKey__ (default: vtkweb-secret): Key that should be used to authenticate the client with the server.
4. __--timeout__ (default: 300 seconds): Timeout that will stop the server/process when the last connected user leave or if nobody connects.
5. __--debug__: Will enable debug output on the server side.
6. __--nosignalhandlers__: Will prevent default signal handling when the server needs to be started within a thread.

#### protocols.py

The __protocols__ module provides a set of implementation of vtkWebProtocol classes
that can be use to build your server application. Some common classes
are the one that handle mouse interaction and rendering.

- ParaView
  - ParaViewWebMouseHandler
  - ParaViewWebViewPortImageDelivery
  - ParaViewWebViewPortGeometryDelivery
  - ...
- VTK
  - vtkWebMouseHandler
  - vtkWebViewPortImageDelivery
  - vtkWebViewPortGeometryDelivery
  - ...

All available protocols for ParaView are listed [here](http://www.paraview.org/ParaView3/Doc/Nightly/www/js-doc/index.html#!/api).

#### vtkWebApplication / vtkPVWebApplication

The __vtkWebApplication__ is a C++ class which is use inside the rendering
protocols listed earlier. That class provides easy to use and
optimized method to capture images from VTK and ParaView views
as well as convinient mouse event handling.

#### Your server application

This part is generaly a Python module that contains a __main__ section which will
trigger the starting of a Web Server. In that module, you usually find an internal
class that list all the existing protocols that you want to use and expose along
the ones you want to add yourself.

A good example is either _{VTK_SRC}/Web/Applications/Cone/server/vtk_web_cone.py_ or _{PV_SRC}/Web/Applications/Visualizer/server/pv_web_visualizer.py_.

## Launcher

The launcher is nothing more than a WebServer that expose a RESTful API that is used to
run a specific command line base on a specific request with some pattern replacement.

This is used to dynamically start a new visualization session at the request of a Web
Client which will then allow each client to work independently from each other.

## Client side

As a framework VTK/ParaView Web provides a set of JavaScript libraries
that rely on JQuery to ease its usage.

A typical ParaViewWeb or vtkWeb web page will look like that:

    <!DOCTYPE html>
    <html>
        <body class="page" onbeforeunload="stop()" onunload="stop()">
            <div class="viewport-container">
            </div>

            <script src="../../lib/core/vtkweb-loader-min.js" load="core-min"></script>
            <script type="text/javascript">
                var config = {
                    sessionManagerURL: vtkWeb.properties.sessionManagerURL,
                    application: "cone"
                },
                stop = vtkWeb.NoOp,
                start = function(connection) {
                    // Create viewport
                    var viewport = vtkWeb.createViewport({session:connection.session});
                    viewport.bind(".viewport-container");

                    // Handle window resize
                    $(window).resize(function() {
                        if(viewport) {
                            viewport.render();
                        }
                    }).trigger('resize');

                    // Update stop method to use the connection
                    stop = function() {
                        connection.session.call('application.exit');
                    }
                };

                // Try to launch the Viz process
                vtkWeb.smartConnect(config, start, function(code,reason){
                    alert(reason);
                });
            </script>
        </body>
    </html>

This will first try to request a new visualization session using the Launcher
RESTful API, and will fall back to a direct WebSocket connection using the
current host and port information in case of lack of launcher.

The Client initialization step can be decomposed as follow:

1. vtkweb-loader-min.js is used to automate the loading of ParaViewWeb JavaScript libraries and dependencies. This will load JQuery, Autobahn.js and all vtkWeb code base.
2. We define a __start__ method that will be called onced a connection to the server side has been establised. The connection provided as argument will give us an handle on the session which will allow
us to make Remote Procedure Call (RPC) on the server side.
3. Then we try to connect to the remote server using the convinient method vtkWeb.smartConnect.
4. In the __start__ method, we override the __stop__ function which will be automatically called if the user quit his browser or close its tab. And we attach to the ".viewport-container" `<div>` an interactive 3D renderer.
