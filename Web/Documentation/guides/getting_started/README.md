# ParaViewWeb quick start

## Introduction

ParaViewWeb is a collection of components that enable the use of ParaView's visualization and data analysis capabilities within Web applications.  Using the latest HTML 5.0 based technologies, such as WebSockets and WebGL, ParaViewWeb enables communication with a ParaView server running on a remote visualization node or cluster using a light-weight JavaScript API. Using this API, Web applications can easily embed interactive 3D visualization components. Application developers can write simple Python scripts to extend the server capabilities to do things such as create custom visualization pipelines.  ParaViewWeb makes it possible to extend web-based scientific workflows to easily visualize and analyze datasets.

More samples and tutorials are forthcoming.  In the meantime, one can access the JavaScript and Python API documentation on ParaView's website.

This documentation will focus on how to test ParaViewWeb locally using the easiest path.  In order to perform real deployments, you can refer to the [Ubuntu 14.04 LTS](index.html#!/guide/ubuntu_14_04) or [Amazon EC2 AMI instance](index.html#!/guide/paraviewweb_on_aws_ec2) guides.

## Getting the software

Simply stated, ParaViewWeb is just ParaView with Python turned on.  To get started with ParaViewWeb, you either need to download a binary release of ParaView, or else you can get the source and build it yourself.  In either case, you will need ParaView 4.1 or newer.  To download Paraview, you can follow this [link](http://www.paraview.org/paraview/resources/software.php "Official ParaView Download page").

To get more information about building ParaView yourself, please see the [Configure and build ParaView](index.html#!/guide/configure_and_build) guide.  Also, this ParaView wiki [section](http://www.paraview.org/Wiki/ParaView#Compile.2FInstall) has a lot of information on compiling/building ParaView.

## Getting some data

You can download the VTK dataset with the following [link](http://www.paraview.org/download/).  When you get there choose "Data, Documentation, and Tutorials" under "Type of Download".  You should unzip the dataset somewhere on your disk.  Once this is done, you should replace the __--data-dir__ option with the appropriate absolute path.

    --data-dir /path-to-share/

## How does it work

In order to run a visualization session with ParaViewWeb, you will need to run a Python script that will act as a web server.  Here is the set of command lines that can be run for each platform when the binaries are used.  As of the time of this writing, the latest stable release is ParaView 4.1, and the following instructions assume you have downloaded this binary release version of the software.  If you have downloaded a different version, you will need to update the commands appropriately.

__Windows__

    $ unzip ParaView-4.1.0-Windows-64bit.exe
    $ cd ParaView-4.1.0-Windows-64bit\bin
    $ pvpython.exe ..\lib\paraview-4.1\site-packages\paraview\web\pv_web_visualizer.py  \
                --content ..\share\paraview-4.1\www                                     \
                --data-dir .\path-to-share                                              \
                --port 8080
    -
    => Then, open a browser to the following URL http://localhost:8080/apps/Visualizer

__Linux__

    $ tar xvzf ParaView-4.1.0-Linux-64bit-glibc-2.3.6.tar.gz
    $ cd ParaView-4.1.0-Linux-64bit
    $ ./bin/pvpython lib/paraview-4.1/site-packages/paraview/web/pv_web_visualizer.py  \
                --content ./share/paraview-4.1/www                                     \
                --data-dir /path-to-share/                                             \
                --port 8080 &
    -
    => Then, open a browser to the following URL http://localhost:8080/apps/Visualizer


__Mac OS X__ (For Mac you need to download the Python 2.7 binaries)

Please copy the paraview.app inside /Applications

    $ cd /Applications/paraview.app/Contents
    $ ./bin/pvpython Python/paraview/web/pv_web_visualizer.py  \
               --content www                                   \
               --data-dir /path-to-share/                      \
               --port 8080 &
    $ open http://localhost:8080/apps/Visualizer

### Server arguments

Server arguments are explained in the server files (pv_web_*.py), as well as online inside the Python documentation.

- [Web visualizer](http://www.paraview.org/ParaView3/Doc/Nightly/www/py-doc/paraview.web.pv_web_visualizer.html) => http://localhost:8080/apps/Visualizer
- [File loader](http://www.paraview.org/ParaView3/Doc/Nightly/www/py-doc/paraview.web.pv_web_file_loader.html) => http://localhost:8080/apps/FileViewer
- [Data prober](http://www.paraview.org/ParaView3/Doc/Nightly/www/py-doc/paraview.web.pv_web_data_prober.html) => http://localhost:8080/apps/DataProber

http://localhost:8080/apps/LiveArticles and http://localhost:8080/apps/Parallel require a special configuration inside a session manager.

### Interacting with the web visualizer

In order to play with the Web visualizer, you can either click on the __"+"__ icon of the pipeline browser to add a source or you can load a file by clicking on the __"folder"__ icon.  Then, further interaction can be executed, as is shown in the video available [here](index.html#!/video/WebVisualizer).

## Simple install

We've developed a Python script that will download for you the binaries and some data so you can have a local setup of ParaViewWeb in no time.  That script can be downloaded [here](http://www.paraview.org/gitweb?p=ParaViewSuperbuild.git;a=blob_plain;f=Scripts/pvw-setup.py;hb=HEAD).  This will give you a start script that will use to launcher to serve the static web pages and will automatically start a new ParaView process for each user.

Here is an example on how to use that script and its output (note the use of quotes around url in `curl` command):

    $ cd /Users/seb/Desktop
    $ mkdir pvw-osx
    $ cd pvw-osx
    $ curl -o pvw-setup.py "http://www.paraview.org/gitweb?p=ParaViewSuperbuild.git;a=blob_plain;f=Scripts/pvw-setup.py;hb=HEAD"
    $ python pvw-setup.py

    Is ParaViewWeb install path correct? (/Users/seb/Desktop/pvw-osx) yes/no/quit: y

    Installing ParaViewWeb inside: /Users/seb/Desktop/pvw-osx

    Which system? [osx, linux32, linux64, win32, win64, all]: osx

    Downloading: /Users/seb/Desktop/pvw-osx/download/ParaView-4.1.0-Darwin-64bit-Lion-Python27.dmg Bytes: 89125150
      89125150  [100.00%]
    => Unpack ParaView
    => Unpack data
       Unzip [..]ata-v4.1/Baseline/WebTesting/ParaView/image_sphere_part_known_good.jpg
    => Unpack Web
       Unzip [..]-doc/extjs/resources/themes/images/default/util/splitter/mini-left.gif
    => Clean web directory
    => Configure local instance

    To start ParaViewWeb web server just run:
          /Users/seb/Desktop/pvw-osx/bin/start.sh

    And go in your Web browser (Safari, Chrome, Firefox) to:
          http://localhost:8080/
