# ParaViewWeb quick start

ParaViewWeb is a collection of components that enables the use of ParaView's visualization and data analysis capabilities within Web applications.
Using the latest HTML 5.0 based technologies, such as WebSocket, and WebGL, ParaViewWeb enables communiation with a ParaView server runnning on a remote visualization node or cluster using a light-weight JavaScript API. Using this API, Web applications can easily embed interactive 3D visualization components. Application developers can write simple Python scripts to extend the server capabilities including creating custom visualization pipelines.
ParaViewWeb makes it possible to extend web-based scientific workflows with ability to visualizate and analyze datasets easily.
More samples and tutorials are forthcoming. In the mean time, one can access the JavaScript and Python API documentation on ParaView website.

This documentation will focus on how to test it locally using the easiest path for that. In order to perform real deployment you can refer to [these other documentation which explain how to make it run on the "EC2 Amazon Web Service"](index.html#!/guide/paraviewweb_on_aws_ec2).

## Getting the software

In order to get ParaViewWeb working on your system, you will need to either build or download it.
In either case you will need ParaView 4.1 or newer.
To download it, you can follow that [link](http://www.paraview.org/paraview/resources/software.php "Official ParaView Download page").

## Getting some data

You can download VTK dataset with the following [link](http://www.paraview.org/files/v4.1/ParaViewData-v4.1.0-RC1.zip).
The you should unzip it somewhere on your disk. Once that's done, you should replace the __--data-dir__ option with the appropriate absolute path.

    --data-dir /path-to-share/

## How does it work

In order to run a visualization session with ParaViewWeb, you will need to run a Python script that will act as a web server.
Here is the set of command line that can be ran for each platform when the binaries are used.

__Windows__

    $ unzip ParaView-4.1.0-RC1-Windows-64bit.zip
    $ cd ParaView-4.1.0-RC1-Windows-64bit\bin
    $ pvpython.exe ..\lib\paraview-4.1\site-packages\paraview\web\pv_web_visualizer.py  \
                --content ..\share\paraview-4.1\www                                     \
                --data-dir .\path-to-share                                              \
                --port 8080
    -
    => Then open a browser to the following URL http://localhost:8080/apps/Visualizer

__Linux__

As example we took the 4.1-RC1 but depending on what you've downloaded, you should update path and file name.

    $ tar xvzf ParaView-4.1.0-RC1-Linux-64bit.tar.gz
    $ cd ParaView-4.1.0-RC1-Linux-64bit
    $ ./bin/pvpython lib/paraview-4.1/site-packages/paraview/web/pv_web_visualizer.py  \
                --content ./share/paraview-4.1/www                                     \
                --data-dir /path-to-share/                                             \
                --port 8080 &
    -
    => Then open a browser to the following URL http://localhost:8080/apps/Visualizer


__Mac OS X__ (For Mac you need to download the Python 2.7 binaries)

Please copy the paraview.app inside /Applications

    $ cd /Applications/paraview.app/Contents
    $ ./bin/pvpython Python/paraview/web/pv_web_visualizer.py  \
               --content www                                   \
               --data-dir /path-to-share/                      \
               --port 8080 &
    $ open http://localhost:8080/apps/Visualizer

### Server arguments

Server arguments are explain in the server files (pv_web_*.py) or online inside the Python documentation.

- [Web visualizer](http://www.paraview.org/ParaView3/Doc/Nightly/www/py-doc/paraview.web.pv_web_visualizer.html)
- [File loader](http://www.paraview.org/ParaView3/Doc/Nightly/www/py-doc/paraview.web.paraview.web.pv_web_file_loader.html)
- [Data prober](http://www.paraview.org/ParaView3/Doc/Nightly/www/py-doc/paraview.web.pv_web_data_prober.html)

### Interacting with the web visualizer

In order to play with the Web visualizer, you can either click on the __"+"__ icon of the pipeline browser to add a source or you can load a file by clicking on the __"folder"__ icon.
Then further interaction can be performed like shown in the video available [here](index.html#!/video/WebVisualizer).