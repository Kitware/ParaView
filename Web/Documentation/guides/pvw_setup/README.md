# ParaViewWeb Setup Guide

## Introduction

The [Quick start](index.html#!/guide/quick_start) guide has information about the fastest way to get ParaViewWeb and try it out.  This guide, however, contains pointers to other guides describing all the ways of obtaining and running ParaViewWeb, including sections on downloading release versions, getting and building from source, as well as setup, running, and deployment of ParaViewWeb.

{@img PVW_Logo_128_txt.png}

## Obtaining ParaViewWeb

This section will cover the ways of obtaining a copy of ParaView, which contains ParaViewWeb.

### Download a binary installer

Follow this [link](http://www.paraview.org/paraview/resources/software.php "Official ParaView Download page") to go to the downloads page.  Choose the type of download, and your target system, then click the "Download" button.

### Get the source

There are several ways of obtaining the source code, you can either download it from the downloads page, or you can check it out using `git`.

#### Source code from download site

Go to the downloads page, linked above, and choose "ParaView Source Files" as the download type.

#### Source code directly from repository

If you have `git` installed on your system, you can simply type:

    $ mkdir -p ~/projects/ParaView
    $ cd ~/projects/ParaView
    $ git clone http://paraview.org/ParaView.git src

This will clone the source tree into a subfolder called `src`.

## Configuring and building ParaViewWeb

If you chose to obtain ParaView source code, then you will want to build it yourself.  __Please Note:__ If you are compiling ParaView because you want to run ParaViewWeb, you must compile with the CMake variable `PARAVIEW_ENABLE_PYTHON` turned `ON`.  Otherwise, you will not be able to run any ParaViewWeb servers.

There is a wealth of information regarding compiling ParaView available on the [ParaView Wiki](http://www.paraview.org/Wiki/ParaView), as well as in the [Configure and build ParaView](index.html#!/guide/configure_and_build) and [Offscreen OSMesa ParaViewWeb](index.html#!/guide/os_mesa) guides.

## Running ParaViewWeb

Once you have successfully compiled ParaView (or if you have downloaded a release binary), you are ready to run ParaViewWeb.  Both the [Quick start](index.html#!/guide/quick_start) as well the [Introduction for developers](index.html#!/guide/developer_howto) guides have more information about running ParaViewWeb.

## Deploying ParaViewWeb

When it comes time to move from running ParaViewWeb server processes by hand to an automated multi-user deployment, see the [Multi-User Setup](index.html#!/guide/multi_user_setup) guide for the background information on deployment.  Also, the [Ubuntu 14.04 LTS](index.html#!/guide/ubuntu_14_04) and [Amazon EC2 AMI Instance](index.html#!/guide/paraviewweb_on_aws_ec2) guides have detailed information about deploying on some specific systems.  In addition, the [Launching Examples](index.html#!/guide/launching_examples) guide has some examples of deployments achievable with the [Python Launcher](index.html#!/guide/python_launcher) and the [Jetty Launcher](index.html#!/guide/jetty_session_manager).
