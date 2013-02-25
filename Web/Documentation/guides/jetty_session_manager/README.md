# Jetty Session Manager as front-end

## Introduction

The Jetty session manager enables you easily deploy ParaViewWeb for multiple users
with a very easy and less intrusive setup. This document covers the different
steps to get the component, configure it, and run it to allow access to
your ParaViewWeb backend.

## Role of that server/service

The session manager has to manage the ParaViewWeb visualization session and
forward the communication to the appropriate remote or local visualization process.

The following schema illustrates this setup:

{@img images/ParaViewWeb-multiuser.png Standard Multi-user setup }

The Jetty session manager, in its embedded version, does not provide any
authentication or user-specific security, but such features can easily be added
in a custom environment.

## Download

The first step is to to download the server, either through this
[link](guides/jetty_session_manager/data/JettySessionManager-Server-1.0.jar)
or with the following command line.

    $ wget http://pvw.kitware.com/guides/jetty_session_manager/data/JettySessionManager-Server-1.0.jar

## Configuration

In order to customize and configure the Jetty session manager web server, you
will need to create a configuration file as follows, named pw-config.properties.

    # Web setup
    pw.web.port=9000
    pw.web.content.dir=

    # Process logs
    pw.logging.dir=/tmp/pw-logs

    # Process commands
    pw.default.cmd=
    pw.default.cmd.run.dir=/tmp/
    pw.default.cmd.map=PORT:getPort|HOST:getHost

    # Resources informations
    pw.resources=localhost:9001-9100

    # Factory
    pw.factory.proxy.adapter=
    pw.factory.wamp.url.generator=com.kitware.paraviewweb.external.GenericWampURLGenerator
    pw.factory.resource.manager=com.kitware.paraviewweb.external.SimpleResourceManager
    pw.factory.visualization.launcher=com.kitware.paraviewweb.external.ProcessLauncher
    pw.factory.websocket.proxy=com.kitware.paraviewweb.external.SimpleWebSocketProxyManager
    pw.factory.session.manager=com.kitware.paraviewweb.external.MemorySessionManager

    # External configurations
    pw.factory.proxy.adapter.file=
    pw.factory.wamp.url.generator.pattern=ws://localhost:9000/SESSION_ID

    pw.process.launcher.wait.keyword=Starting factory
    pw.process.launcher.wait.timeout=10000

    pw.session.public.fields=id,sessionURL,name,description,host,port,url,application,idleTimeout,startTime
    pw.session.max=10

The following section of the configuration file will determine your setup and what
type of application you will deploy.

    # Process commands
    pw.XXX.cmd=
    pw.XXX.cmd.run.dir=/tmp/
    pw.XXX.cmd.map=PORT:getPort|HOST:getHost

The extended code below supports three types of applications (cone, can, loader):

    # Process commands
    pw.cone.cmd.run.dir=/.../ParaViewWeb/web-server/server
    pw.cone.cmd=/.../ParaViewWeb/ParaView/build/bin/pvpython simple_server.py --content=/.../ParaViewWeb/web-content/apps/cone --port=PORT
    pw.cone.cmd.map=PORT:getPort

    pw.can.cmd.run.dir=/.../ParaViewWeb/web-server/server
    pw.can.cmd=/.../ParaViewWeb/ParaView/build/bin/pvpython simple_server.py --content=/.../ParaViewWeb/web-content/apps/can --port=PORT --module=file_loader --file-to-load=/.../ParaViewData/Data/can.ex2
    pw.can.cmd.map=PORT:getPort

    pw.loader.cmd.run.dir=/.../ParaViewWeb/web-server/server
    pw.loader.cmd=/.../ParaViewWeb/ParaView/build/bin/pvpython simple_server.py --content=/.../ParaViewWeb/web-content/apps/loader --port=PORT --module=file_loader --path-to-list=/.../ParaViewData/Data
    pw.loader.cmd.map=PORT:getPort

## Running

Once you have a valid configuration file, execute the following command line to
run the server.

    $ java -jar JettySessionManager-Server-1.0.jar /path_to_your_config_file/pw-config.properties
