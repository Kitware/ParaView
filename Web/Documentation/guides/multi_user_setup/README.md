# Multi-user Setup Guide

## Introduction

This document describes the three components required to deploy ParaViewWeb in a multi-user environment.  These three components are depicted in the following image, and include 1) a front end to act as a single point of entry for all clients, 2) a launcher which can start a new visualization process for each client who requests one, and 3) ParaViewWeb itself.

{@img pvw-3comp-resize.png The three components of multi-user ParaViewWeb}

The shortest path to getting all these components set up and working together is the recommended approach, using Ubuntu 14.04 LTS, Apache >= 2.4.7, and the Python launcher that is included with ParaViewWeb.  This approach is discussed in a separate guide, [Ubuntu 14.04 LTS](index.html#!/guide/ubuntu_14_04).  Note that although the Ubuntu 14.04 LTS guide is tailored specifically to that operating system, several parts of the guide are applicable to any operating system.  The generally applicable sections include configuration of the Apache virtual host as well as the setup of the launcher.

However, the recommended approach is not by any means the only one.  The rest of this document is dedicated to describing the three main components in a bit more detail, and providing pointers to relevant in-depth documentation in each section.  While there is really only one option for the ParaViewWeb component (that is ParaViewWeb itself), there are many technologies which can serve as a front end component, and the same is true for the launcher component.

## Front End

The point of the front end is to serve static content as well as to do network forwarding, so that there is a single point of entry through which all clients communicate.  The front end must be able to forward certain requests to the launcher when a new client wants to start a new visualization session, and then it must be able to learn from the launcher how to map subsequent session requests from that client to the port where the clients visualization session is listening.

While we recommend Apache (a version >= 2.4.7) as a front end component, there are really any number of potential options here.  In the past we provided a [Jetty front end](index.html#!/guide/jetty_session_manager), and may in the future provide specific support for other front ends such as NGINX or crossbar.io.

We have provided a detailed guide on using [Apache as a front end](index.html#!/guide/apache_front_end), including how to build a suitable copy if your system does not come with one.

## Launcher

The launcher component (sometimes previously called the "Session Manager") is responsible for launching a ParaViewWeb process for each user who requests one, and also for communicating the session ID and an associated port number to the front end component.  This allows the front end component to know how to forward future requests from each client to the correct port where that clients visualization session is listening.

Currently ParaViewWeb includes our recommended [Python launcher](index.html#!/guide/python_launcher) which is based on Twisted.  However, in the past we provided a [Jetty launcher](index.html#!/guide/jetty_session_manager) (which also served as a front end).  Also, many other approaches for launching visualization processes have been developed for specific applications and specific deployments.  Please see the [Laucher RESTful API](index.html#!/guide/launcher_api) guide for information about the API that should be implemented by any launcher.

## ParaViewWeb

ParaViewWeb is simply ParaView, compiled with the Python option turned on.  Any machine where it is to be run must either have a suitable graphics environment, or else ParaView must also be compiled with OSMesa support so that it can do offscreen (software) rendering.

There is some basic information about getting up and running with ParaViewWeb in the [Quick start](index.html#!/guide/quick_start) guide.

### Rendering environment

Perhaps the first task to be undertaken is to decide whether you want hardware or software rendering.  There is a great deal of information available throughout these guides regarding how to do either task.

#### Software rendering

For instructions on how to compile ParaView for software rendering, see [Offscreen OSMesa ParaViewWeb](index.html#!/guide/os_mesa).  The ParaView wiki [section](http://www.paraview.org/Wiki/ParaView_And_Mesa_3D) on Mesa 3D also has a lot of information on this topic.

#### Hardware rendering

For instructions on how to set up a hardware rendering environment, there are also many sources of information.  Again, the ParaView [wiki](http://www.paraview.org/Wiki/ParaView) is one such source.  We have also compiled instructions for setting up an environment suitable for hardware rendering on several kinds of Amazon EC2 AMI images running on g2.2xlarge instances.  That information can be found in the [EC2 Graphics Setup](index.html#!/guide/graphics_on_ec2_g2) guide.

### Server configuration

Even the most basic ParaViewWeb server application comes out-of-the-box with a lot of configurable options, currently available as command line arguments.  You can learn about some of those options, as well as get more paraViewWeb details, in the [Introduction for developers](index.html#!/guide/developer_howto) guide.
