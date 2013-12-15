# ParaViewWeb architecture

## Introduction

ParaViewWeb is a framework used to leverage the power of VTK and ParaView on the Web in an interactive manner. ParaViewWeb allow the user to perform heavy 3D visualization and processing within a Web browser by relying on a remote processing server for the rendering and/or the processing. In case of small 3D geometry, ParaViewWeb can send the geometry to the client to allow local rendering using WebGL.

ParaViewWeb started around the beguinning of 2010 and greatly evolved since then. This document will briefly go through the historical architecture in order to highlight the major improvement done and how the deployment has been greatly improved.

## Architecture evolution

When ParaViewWeb started, WebGL and WebSocket were just hope for a better future.
At that time basic HTML was the only option and everything has to be defined.

The solution found at that time to interconnect a WebServer with our C++ backend was a JMS brocker. The WebServer had the role to interface the HTTP client with the JMS backend. The following picture illustrate all the technologies and layer involved in that setup.

{@img images/PVWeb-old.png Multi-user setup}

On top of that we made another design choice where all the ParaView proxies were wrapped into JavaScript object. That gives us a great flexibility in term of JavaScript code as anything could be achieved directly from the client. But on the other hand that was producing a lot of unecessary communication. To overcome, that issue we allowed the user to wrapped external python module. This allowed a cleaner design and better performances by pre-defining the set of methods that are needed by the Web client on the server side.

When we finally upgrade the framework, we went to a new route and had a very precise set of requirements:

- Need to be easy to use and deploy.
- No more external dependencies difficult to build.
- Leverage the technologies that were not available when we started.
- Easy to secure.
- Force the user for best practice

### Simplicity

The new architecture of ParaViewWeb allow ad-hoc usage of the service with the distributed binaries. How simple is that?

The following picture illustrate how a single user can start and interact with a local ParaViewWeb instance.

{@img images/PVWeb-singleuser.png Single user setup}

In that case, ParaViewWeb is a single Python script that could be executed by the provided python interpretor. The script will be responsible to start a web server and listen to a given port.

The following command line illustrate how to trigger such server:

    $ ./bin/pvpython lib/paraview-4.1/site-packages/paraview/web/pv_web_visualizer.py  \
                --content ./share/paraview-4.1/www                                     \
                --data-dir /path-to-share/                                             \
                --port 8080 &

Obviously such setup does not allow several users to connect to a remote server and build their own visualization independantly from each other. To support such deployment, a multi-user setup is needed.

### Multi-user setup

In order to support in a transparent manner several users connecting concurrently on different visualization session, the server must provide a single entry point where to connect as well as a mechanism to start new visualization session on demand.

The figure below illustrate such setup where Apache is use as a front-end to deliver the static content as well as forwarding the WebSocket communication to the appropriate backend visualization session. Moreover, a __launcher__ process is used to dynamically start the pvpython process for the visualization session.

{@img images/PVWeb-multiusers.png Multi-user setup}

Even if that setup is more complex than the ad-hoc one, it still remain practicle for small institution.

### Technologies and principles

The current implementation prevent the client from having a direct access to the ParaView proxy. Only what has been defined as an exposed API is exposed.