# Jetty Session Manager as front-end

## Introduction

The Jetty session manager enables you to easily deploy ParaViewWeb for multiple users
with a very easy and less intrusive setup. This document covers the different
steps necessary to obtain the component, configure it, and run it to allow access to
your ParaViewWeb back-end application.

## Role of that server/service

The session manager has to manage the ParaViewWeb visualization session and
forward the communication to the appropriate remote or local visualization process.

The following schema illustrates this setup:

{@img images/ParaViewWeb-multiuser.png Standard Multi-user setup }

The Jetty session manager, in its embedded version, does not provide any
authentication or user-specific security. However, such features can easily be added
in a custom environment.

## Download

The first step is to download the server by using this
[link](http://paraview.org/files/dependencies/ParaViewWeb/JettySessionManager-Server-1.0.jar)
or by using the following command line.

    $ wget http://paraview.org/files/dependencies/ParaViewWeb/JettySessionManager-Server-1.0.jar

## Configuration

In order to customize and configure the Jetty session manager web server, you
will need to create a configuration file, named pw-config.properties, as follows:

    # Web setup
    pw.web.port=9000
    pw.web.content.dir=/.../paraview-build/www

    # Process logs
    pw.logging.dir=/tmp/pw-logs

    # ==================================================
    # Process command: data_prober.py      | data_prober
    # ==================================================
    pw.data_prober.cmd=./bin/pvpython ../src/Web/Python/data_prober.py --data-dir /Data --port PORT -f --authKey SECRET
    pw.data_prober.cmd.run.dir=/.../paraview-build/
    pw.data_prober.cmd.map=PORT:getPort|SECRET:secret

    # ==================================================
    # Process command: file_loader.py      | loader
    # ==================================================
    pw.loader.cmd=./bin/pvpython ../src/Web/Python/file_loader.py --data-dir /Data --port PORT -f --authKey SECRET
    pw.loader.cmd.run.dir=/.../paraview-build/
    pw.loader.cmd.map=PORT:getPort|SECRET:secret

    # ==================================================
    # Process command: pipeline_manager.py | pipeline
    # ==================================================
    pw.pipeline.cmd=./bin/pvpython ../src/Web/Python/pipeline_manager.py --data-dir /Data --port PORT -f --authKey SECRET
    pw.pipeline.cmd.run.dir=/.../paraview-build/
    pw.pipeline.cmd.map=PORT:getPort|SECRET:secret

    # Resources informations
    pw.resources=localhost:9001-9100

    # Factory
    pw.factory.proxy.adapter=
    pw.factory.session.url.generator=com.kitware.paraviewweb.external.GenericSessionURLGenerator
    pw.factory.resource.manager=com.kitware.paraviewweb.external.SimpleResourceManager
    pw.factory.visualization.launcher=com.kitware.paraviewweb.external.ProcessLauncher
    pw.factory.websocket.proxy=com.kitware.paraviewweb.external.SimpleWebSocketProxyManager
    pw.factory.session.manager=com.kitware.paraviewweb.external.MemorySessionManager

    # External configurations
    pw.factory.proxy.adapter.file=

    # CAUTION: The ws port should match the server port
    pw.factory.session.url.generator.pattern=ws://localhost:9000/paraview/SESSION_ID

    pw.process.launcher.wait.keyword=Starting factory
    pw.process.launcher.wait.timeout=10000

    pw.session.public.fields=id,sessionURL,name,description,sessionManagerURL,application,idleTimeout,startTime,file
    pw.session.max=10

The following section of the configuration file will determine your setup, as well as what
type of application you will deploy.

    # Process commands
    pw.XXX.cmd=
    pw.XXX.cmd.run.dir=/tmp/
    pw.XXX.cmd.map=PORT:getPort|HOST:getHost

The XXX is whatever the client (JavaScript inside your web browser) is requesting for the 'application' field in its start request.

    paraview.start({
           sessionManagerURL: location.protocol + "//" + location.hostname + ":" + location.port + "/paraview",
           application: 'loader'
       },
       okCallback,
       errorCallback
    );

## Running

Once you have a valid configuration file, execute the following command line to
run the server.

    $ java -jar JettySessionManager-Server-1.0.jar /path_to_your_config_file/pw-config.properties

To generate a default configuration file, you can execute the following command line.

    $ java -jar JettySessionManager-Server-1.0.jar > default-config.properties

or just read the output of the following command line.

    $ java -jar JettySessionManager-Server-1.0.jar
