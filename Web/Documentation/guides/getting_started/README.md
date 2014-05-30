# ParaViewWeb quick start

ParaViewWeb is a collection of components that enable the use of ParaView's visualization and data analysis capabilities within Web applications.
Using the latest HTML 5.0 based technologies, such as WebSocket and WebGL, ParaViewWeb enables communication with a ParaView server running on a remote visualization node or cluster using a light-weight JavaScript API. Using this API, Web applications can easily embed interactive 3D visualization components. Application developers can write simple Python scripts to extend the server capabilities to do things such as create custom visualization pipelines.
ParaViewWeb makes it possible to extend web-based scientific workflows to easily visualize and analyze datasets.
More samples and tutorials are forthcoming. In the meantime, one can access the JavaScript and Python API documentation on ParaView's website.

This documentation will focus on how to test ParaViewWeb locally using the easiest path. In order to perform real deployment, you can refer to [these other documentations, which explain how to make it run on the "EC2 Amazon Web Service"](index.html#!/guide/paraviewweb_on_aws_ec2).

## Getting the software

In order to have ParaViewWeb working on your system, you will need to either build or download it.
In either case, you will need ParaView 4.1 or newer.
To download Paraview, you can follow the [link](http://www.paraview.org/paraview/resources/software.php "Official ParaView Download page").

## Getting some data

You can download the VTK dataset with the following [link](http://www.paraview.org/files/v4.1/ParaViewData-v4.1.0-RC1.zip).
You should unzip the dataset somewhere on your disk. Once this is done, you should replace the __--data-dir__ option with the appropriate absolute path.

    --data-dir /path-to-share/

## How does it work

In order to run a visualization session with ParaViewWeb, you will need to run a Python script that will act as a web server.
Here is the set of command lines that can be run for each platform when the binaries are used.

__Windows__

    $ unzip ParaView-4.1.0-RC1-Windows-64bit.zip
    $ cd ParaView-4.1.0-RC1-Windows-64bit\bin
    $ pvpython.exe ..\lib\paraview-4.1\site-packages\paraview\web\pv_web_visualizer.py  \
                --content ..\share\paraview-4.1\www                                     \
                --data-dir .\path-to-share                                              \
                --port 8080
    -
    => Then, open a browser to the following URL http://localhost:8080/apps/Visualizer

__Linux__

As an example, we used the 4.1-RC1. However, depending on what you have downloaded, you should update the path and file name.

    $ tar xvzf ParaView-4.1.0-RC1-Linux-64bit.tar.gz
    $ cd ParaView-4.1.0-RC1-Linux-64bit
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

In order to play with the Web visualizer, you can either click on the __"+"__ icon of the pipeline browser to add a source or you can load a file by clicking on the __"folder"__ icon.
Then, further interaction can be executed, as is shown in the video available [here](index.html#!/video/WebVisualizer).

## Simple install

We've developed a Python script that will download for you the binaries and some data so you can have a local setup of ParaViewWeb in no time.
That script can be downloaded [here](guides/getting_started/data/pvw-setup).
This will give you a start script that will use to launcher to serve the static web pages and will automatically start a new ParaView process
for each user.

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
