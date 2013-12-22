# ParaViewWeb architecture

## Introduction

ParaViewWeb is a framework used to leverage the power of VTK and ParaView on the Web in an interactive manner. ParaViewWeb allows the user to perform heavy 3D visualization and processing within a Web browser by relying on a remote processing server for rendering and/or processing. In cases of small 3D geometry, ParaViewWeb can send the geometry to the client to allow local rendering using WebGL.

ParaViewWeb started around the beginning of 2010. It has greatly evolved since then. This document will briefly go through the historical architecture in order to highlight major improvements in areas including deployment.

## Architecture evolution

When ParaViewWeb was started, WebGL and WebSocket were still in very early stages of development. At that time, basic HTML was the only option and everything had to be defined.

The solution found at that time to interconnect a WebServer with our C++ backend was a JMS brocker. The WebServer’s role was to interface the HTTP client with the JMS backend. The following picture illustrates all the technologies and layers involved in that setup.

{@img images/PVWeb-old.png Multi-user setup}

On top of that, we made another design choice where all the ParaView proxies were wrapped into a JavaScript object. This gave us great flexibility in terms of JavaScript code, as anything could be achieved directly from the client. On the other hand, it was producing a lot of unnecessary communication. To overcome this issue, we allowed the user to wrap an external python module. By pre-defining the set of methods that are needed by the Web client on the server side, it provided for a cleaner design, as well as better performance.

When we finally upgraded the ParaViewWeb, we took a new route and had a very precise set of requirements:

- It needs to be easy to use and deploy.
- It should not have external dependencies that are difficult to build.
- It should leverage the technologies that were not available when we started.
- It should be easy to secure.
- Best practices should be enforced by the software.

### Simplicity

The new architecture of ParaViewWeb allows ad-hoc usage of the service with the distributed binaries. How simple is that?

The following picture illustrates how a single user can begin interacting with a local ParaViewWeb instance.

{@img images/PVWeb-singleuser.png Single user setup}

In this case, ParaViewWeb is a single Python script that could be executed by the provided python interpreter. The script will be responsible for starting a web server and listening to a given port.

The following command line illustrates how to trigger such a server:

    $ ./bin/pvpython lib/paraview-4.1/site-packages/paraview/web/pv_web_visualizer.py  \
                --content ./share/paraview-4.1/www                                     \
                --data-dir /path-to-share/                                             \
                --port 8080 &

This setup does not allow several users to connect to a remote server and build their own visualizations independently from each other. To support such deployment, a multi-user setup is needed.

### Multi-user setup

In order to support in a transparent manner the connection of several users on different visualization sessions, the server must provide a single entry point to establish a connection, as well as a mechanism to start a new visualization session on demand.

The figure below illustrates such a setup where Apache is used as a front-end application to deliver the static content, as well as to forward the WebSocket communication to the appropriate back-end visualization session. Moreover, a __launcher__ process is used to dynamically start the pvpython process for the visualization session.

{@img images/PVWeb-multiusers.png Multi-user setup}

Even if that setup is more complex than the ad-hoc one, it still remains practical for small institutions.

### Technologies and principles

The current implementation prevents the client from having direct access to the ParaView proxy. The exposed API is limited to what the user have chosen to expose.
