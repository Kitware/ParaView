# Jetty Session Manager as launcher and/or front-end

## Introduction

The Jetty session manager enables you to easily deploy ParaViewWeb for multiple users.  It can be used a just a launcher running behind a separate front end, or it can be used as both a launcher and a front end.  This document covers the steps necessary to obtain, configure, and run it to allow access to your ParaViewWeb back-end application.

## Role of the Jetty Launcher

As just a launcher, this service is responsible for responding to user requests for new ParaViewWeb visualization sessions.  As a launcher and a front end together, it is additionally responsible for forwarding communications from clients to the appropriate remote or local visualization processes.

The following schema illustrates this setup:

{@img images/ParaViewWeb-multiuser.png Standard Multi-user setup }

The Jetty session manager, in its embedded version, does not provide any authentication or user-specific security. However, such features can easily be added in a custom environment.

## Download

The first step is to download the server by using this [link](http://paraview.org/files/dependencies/ParaViewWeb/JettySessionManager-Server-1.0.jar) or by using the following command line.

    $ wget http://paraview.org/files/dependencies/ParaViewWeb/JettySessionManager-Server-1.0.jar

## Configuration as launcher + front end

This section discusses the configuration of the Jetty launcher in such a way that it behaves as both a launcher and a front end.  In order to customize and configure the Jetty session manager web server, you will need to create a configuration file, named pw-config.properties, as follows:

    # Web setup
    pw.web.port=9000
    pw.web.content.dir=/.../paraview-build/www

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

## Configuration as launcher only (for Apache front end)

This section discusses configuring the Jetty launcher when it is to be used as only a launcher, and Apache is used as the front end.  Following is an example configuration file for use by the Jetty launcher.

    # ===================================================
    # Expect a configuration file as the argument
    #
    #  $ java -jar file.jar <config_file_path>.
    #
    # The content of that file should look like this
    # ===================================================

    # Web setup
    pw.web.port=9000
    pw.web.content.dir=/var/www/pvweb-deploy/www

    # Process logs
    pw.logging.dir=/home/ec2-user/ParaView/launcher/logs

    # ==================================================
    # Process command: data_prober.py      | data_prober
    # ==================================================
    pw.data_prober.cmd=./bin/pvpython -dr ./lib/paraview-4.1/site-packages/paraview/web/pv_web_data_prober.py -f --data-dir /home/ec2-user/ParaView/ParaViewData/Data --port PORT --authKey SECRET
    pw.data_prober.cmd.run.dir=/home/ec2-user/ParaView/ParaView-4.1.0-RC1-Linux-64bit
    pw.data_prober.cmd.map=PORT:getPort|SECRET:secret

    # ==================================================
    # Process command: file_loader.py      | loader
    # ==================================================
    pw.loader.cmd=./bin/pvpython -dr ./lib/paraview-4.1/site-packages/paraview/web/pv_web_file_loader.py -f --data-dir /home/ec2-user/ParaView/ParaViewData/Data --port PORT --authKey SECRET
    pw.loader.cmd.run.dir=/home/ec2-user/ParaView/ParaView-4.1.0-RC1-Linux-64bit
    pw.loader.cmd.map=PORT:getPort|SECRET:secret

    # ==================================================
    # Process command: pipeline_manager.py | pipeline
    # ==================================================
    pw.pipeline.cmd=./bin/pvpython -dr ./lib/paraview-4.1/site-packages/paraview/web/pv_web_visualizer.py -f --data-dir /home/ec2-user/ParaView/ParaViewData/Data --port PORT --authKey SECRET
    pw.pipeline.cmd.run.dir=/home/ec2-user/ParaView/ParaView-4.1.0-RC1-Linux-64bit
    pw.pipeline.cmd.map=PORT:getPort|SECRET:secret

    # Resources informations
    pw.resources=localhost:9001-9100

    # Factory
    pw.factory.proxy.adapter=com.kitware.paraviewweb.external.ApacheModRewriteMapFileConnectionAdapter
    pw.factory.session.url.generator=com.kitware.paraviewweb.external.GenericSessionURLGenerator
    pw.factory.resource.manager=com.kitware.paraviewweb.external.SimpleResourceManager
    pw.factory.visualization.launcher=com.kitware.paraviewweb.external.ProcessLauncher
    pw.factory.websocket.proxy=com.kitware.paraviewweb.external.SimpleWebSocketProxyManager
    pw.factory.session.manager=com.kitware.paraviewweb.external.MemorySessionManager

    # External configurations
    pw.factory.proxy.adapter.file=/opt/apache-2.4.7/pv-mapping-file/mapping.txt

    # CAUTION: The ws port should match the server port
    # For Jetty websocket forwarder use: ws://localhost:9000/paraview/SESSION_ID
    # For Apache front-end use         : ws://localhost/proxy?sessionId=SESSION_ID
    pw.factory.session.url.generator.pattern=ws://ec2-XXX-XXX-XXX-XXX.compute-1.amazonaws.com/proxy?sessionId=SESSION_ID

    # Timeout for websocket proxy in milliseconds
    pw.factory.websocket.proxy.timeout=300000

    pw.process.launcher.wait.keyword=Starting factory
    pw.process.launcher.wait.timeout=10000

    pw.session.public.fields=id,sessionURL,name,description,sessionManagerURL,application,idleTimeout,startTime,file
    pw.session.max=10

    # ===================================================

In particular, note the use of `pw.factory.proxy.adapter` and `pw.factory.proxy.adapter.file` variables, which were empty in the previous configuration.  Also note that the `pw.factory.session.url.generator.pattern` variable has a different form when using Apache as a front end as opposed to using the Jetty launcher as the front end.

## Running the Jetty launcher

    $ cd /home/ec2-user/ParaView/launcher
    $ java -jar JettySessionManager-Server-1.1.jar jetty-config.txt &

Once you have a valid configuration file, execute the following command line to run the server.

    $ java -jar JettySessionManager-Server-1.0.jar /path_to_your_config_file/pw-config.properties

To generate a default configuration file, you can execute the following command line.

    $ java -jar JettySessionManager-Server-1.0.jar > default-config.properties

or just read the output of the following command line.

    $ java -jar JettySessionManager-Server-1.0.jar
