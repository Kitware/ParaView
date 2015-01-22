# How to use Apache as front-end (DEPRECATED)

## Introduction

Since Apache does not by default support web sockets, it is not the most
straightforward approach for a ParaViewWeb deployment. However, many users
require or employ such a front-end application. Therefore, this document provides a step-by-step
guide on how to setup such an environment.

__Caution__: This installation focus on old Apache setup. The prefered setup should rely on Apache 2.4+ like described in the EC2 guide.

## Components overview

A web application that relies on ParaViewWeb is composed of two to three
components, depending on the type of deployment and desired usage.

The two main components are:

1. The web socket server that is used to control the ParaView engine.
2. Some static content that contains the JavaScript and web pages. This content
composes the web application that needs to be delivered via standard HTTP.

In such a setup, the ParaView process acts as the web server and only a shared
visualization can be achieved. In order to support multiple clients and,
therefore, concurrent visualizations, a third component is needed. This component will act
as a session manager by starting and stopping the ParaView sessions and
communication forwarder for the web socket.

The following image illustrates what a multi-user deployment could look like
behind an Apache front-end, with a Java web server handling the ParaView processes.

{@img images/ParaViewWeb-Apache.png Alt text}

## Installation

### ParaView

ParaView needs to be built from source in order to support the latest ParaViewWeb
features. This section will assume a Unix-based environment. It will detail the
command lines needed for the configuration, build, and installation of the ParaView
components for a ParaViewWeb deployment.

    $ cd ParaViewWeb
    $ mkdir ParaView
    $ cd ParaView
    $ mkdir build install
    $ git clone git://paraview.org/stage/ParaView.git src
    $ cd src
    $ git submodule update --init
    $ cd ../build
    $ ccmake ../src

        PARAVIEW_ENABLE_PYTHON      ON
        PARAVIEW_BUILD_QT_GUI       OFF
        CMAKE_INSTALL_PREFIX        /.../ParaView/install

    $ make
    $ make install

### ParaViewWeb

In this example, ParaViewWeb is directly embedded inside the ParaView repository. Building ParaView will create a web directory that should be served by Apache.
In the Apache configuration, we use the following path '/home/pvw/ParaViewWeb/www'.
If you would like to continue using this path, you should execute the following command line.

    $ cp -r /home/pvw/ParaViewWeb/build/www /home/pvw/ParaViewWeb/www

### Session Manager

ParaViewWeb comes with several external projects to support various types of deployment.
One ParaViewWeb deployment involves a web front-end that accommodates concurrent
ParaViewWeb visualization sessions.
This session manager uses the Jetty libraries to provide a fully functional
web server inside an embedded application. More information can be found on the
[Jetty Sessions Manager](index.html#!/guide/jetty_session_manager) and from the developer
team, and the executable can be downloaded [here](http://paraview.org/files/dependencies/ParaViewWeb/).

Configuration file for the session manager executable: (pw-config.properties):

    # Web setup
    pw.web.port=9000
    pw.web.content.dir=

    # Process logs
    pw.logging.dir=/tmp/pw-logs

    # ==================================================
    # Process command: data_prober.py      | data_prober
    # ==================================================
    pw.data_prober.cmd=./bin/pvpython -dr ../src/Web/Python/data_prober.py --data-dir /Data --port PORT -f --authKey SECRET
    pw.data_prober.cmd.run.dir=/.../paraview-build/
    pw.data_prober.cmd.map=PORT:getPort|SECRET:secret

    # ==================================================
    # Process command: file_loader.py      | loader
    # ==================================================
    pw.loader.cmd=./bin/pvpython -dr ../src/Web/Python/file_loader.py --data-dir /Data --port PORT -f --authKey SECRET
    pw.loader.cmd.run.dir=/.../paraview-build/
    pw.loader.cmd.map=PORT:getPort|SECRET:secret

    # ==================================================
    # Process command: pipeline_manager.py | pipeline
    # ==================================================
    pw.pipeline.cmd=./bin/pvpython -dr ../src/Web/Python/pipeline_manager.py --data-dir /Data --port PORT -f --authKey SECRET
    pw.pipeline.cmd.run.dir=/.../paraview-build/
    pw.pipeline.cmd.map=PORT:getPort|SECRET:secret

    # Resources informations
    pw.resources=localhost:9001-9100

    # Factory
    pw.factory.proxy.adapter=com.kitware.paraviewweb.external.JsonFileProxyConnectionAdapter
    pw.factory.session.url.generator=com.kitware.paraviewweb.external.GenericSessionURLGenerator
    pw.factory.resource.manager=com.kitware.paraviewweb.external.SimpleResourceManager
    pw.factory.visualization.launcher=com.kitware.paraviewweb.external.ProcessLauncher
    pw.factory.websocket.proxy=com.kitware.paraviewweb.external.SimpleWebSocketProxyManager
    pw.factory.session.manager=com.kitware.paraviewweb.external.MemorySessionManager

    # External configurations
    pw.factory.proxy.adapter.file=/home/pvw/proxy/session.map
    pw.factory.session.url.generator.pattern=ws://localhost/proxy?sessionId=SESSION_ID

    pw.process.launcher.wait.keyword=Starting factory
    pw.process.launcher.wait.timeout=10000

    pw.session.public.fields=id,sessionURL,name,description,sessionManagerURL,application,idleTimeout,startTime,file

Shell script used to start the session manager

    export DISPLAY=:0.0
    java -jar JettySessionManager-Server-1.0.jar /.../pw-config.properties

### Python 2.7 ###

The websocket proxy requires Python 2.7 with the following packages installed:

#### 1. AutobahnPython ####

Download the [zipped file](http://pypi.python.org/pypi/autobahn).

    $ unzip autobahn-0.5.9.zip
    $ cd autobahn-0.5.9
    $ python setup.py build
    $ sudo python setup.py install

#### 2. pywebsockets ####

Download the [tarball](http://code.google.com/p/pywebsocket/downloads).

    $ tar xfz mod_pywebsocket-0.7.8.tar.gz
    $ cd pywebsocket-0.7.8/src/
    $ python setup.py build
    $ sudo python setup.py install

### Apache ###

To proxy websocket traffic through Apache, a custom proxy is required. The proxy
can be download [here](guides/apache_setup/data/ApacheWebsocketProxy.tgz).

    $ tar xfz ApacheWebsocketProxy.tgz

The following modules need to be installed and configured to enable the proxy code.

#### mod_python ####

mod_pywebsocket is used by the websocket proxy to accept incoming websocket
connection. It requires mod_python.

    $ sudo apt-get install libapache2-mod-python

#### mod_pywebsocket ####

To enable the mod_pywebsocket module, add the following lines to /etc/apache2/apache2.conf

    PythonPath "sys.path+['/usr/local/lib/python2.7/dist-packages/mod_pywebsocket']"
    PythonOption mod_pywebsocket.handler_root <path_to_apache_websocket_proxy_code>
    PythonHeaderParserHandler mod_pywebsocket.headerparserhandler

#### Configuring the proxy ####

The following entries need to be added to the /etc/apache2/sites-available/pvw configuration file.

    AddHandler mod_python .py
    PythonHandler mod_python.publisher

The configuration for the proxy is held in proxy.json. It looks like this:

    {
      "loggingConfiguration": "/home/pvw/proxy/logging.json",
      "connectionReaper": {
          "reapInterval": 300,
          "connectionTimeout": 3600
      },
      "sessionMappingFile": "/home/pvw/proxy/session.map"
    }

* *loggingConfigurations* - This is the path to the JSON file containing the Python logging configuration.
* *connectionReaper.reapInterval* - This is the interval, in seconds, at which the connection reaper is run.
* *connectionReaper.connectionTimeout* - This is the length of time a connection can remain inactive before the connection will be cleaned up.
* *sessionMappingFile* - This is the session mapping file produced by the session manager. It maps session IDs to connection endpoints.
    The proxy uses this to know where to route sessions. See the Session manager configuration for details.

Place the proxy.json configuration file in the home directory of the user utilized
to run apache, which is usually /var/www

#### Virtual host configuring ####

The virtual host should be the following:

    <VirtualHost *:80>
        ServerName  your.paraviewweb.hostname
        ServerAdmin your@email.com

        # Static content directory (js, html, png, css)
        DocumentRoot /home/pvw/ParaViewWeb/www

        # Jetty Session Manager
        ProxyPass        /paraview  http://localhost:9000/paraview
        ProxyPassReverse /paraview  http://localhost:9000/paraview

        # Proxy web socket to ParaViewWeb processes
        <Directory /home/pvw/ParaViewWeb/www/websockets>
                Options Indexes FollowSymLinks MultiViews
                AllowOverride None
                Order allow,deny
                allow from all
                AddHandler mod_python .py
                PythonHandler mod_python.publisher
                PythonDebug On
        </Directory>

        # Log management
        LogLevel warn
        ErrorLog  /home/pvw/ParaViewWeb/logs/pvw-error.log`
        CustomLog /home/pvw/ParaViewWeb/logs/pvw-access.log combined
    </VirtualHost>
