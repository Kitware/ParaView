# Launching Examples Supported "Out-of-the-box"

## Introduction

As discussed in the [Multi-User Setup](index.html#!/guide/multi_user_setup) guide, there are three components required to deploy ParaViewWeb for multiple concurrent users.  Those components are a front end, a launcher, and ParaViewWeb itself.  Quite simple, yes?  And yet because there is so much flexibility designed into the system components, there is a nearly infinite number of possibile deployment configurations that can be achieved out-of-the-box.

We provide two possibilities for the launcher component, which are described in the [Python Launcher](index.html#!/guide/python_launcher) and [Jetty Launcher](index.html#!/guide/jetty_session_manager) guides.  These are designed to be very flexible, while supporting the typical "showcase" deployment.  It is very important to note, however, that these launchers are just two possible implementations of the [Launcher RESTful API](index.html#!/guide/launcher_api).  Neither of these launchers requires user to authenticate themselves before requesting resources, and neither supports limiting the number of compute hours for a given user, just to name two limitations.  In any real-world deployment of ParaViewWeb, for example in any HPC system, it is expected that launching of ParaViewWeb jobs will need to be done in a custom manner.  All HPC systems come with their own job scheduling frameworks, which will typically need to be used to start processes on the system.

Having gotten that caveat out of the way, this document will describe several deployment configurations that can be achieved with an Apache front end along with either the Python or Jetty launcher.  In order to make the examples more concrete, the relevant configuration lines of the Apache virtual host as well as the launcher (for both Python and Jetty, where applicable) are given.

As a final note before we dive into some examples, we want to mention that Kitware is always available to consult on real-world ParaViewWeb deployments where specialized or custom launching is involved.

## First Example: One machine

{@img pvw-deploy-opt-1.png}

This is the simplest deployment option, where Apache, the launcher, and the ParaViewWeb servers all run on the same machine.  As mentioned in the introduction of this guide, only the relevant configuration lines are shown here.  See other guides for complete configuration examples, for instance, the [Ubuntu 14.04 LTS](index.html#!/guide/ubuntu_14_04), [Apache as a front end](index.html#!/guide/apache_front_end), [Python Launcher](index.html#!/guide/python_launcher), or [Jetty Launcher](index.html#!/guide/jetty_session_manager) guides.

__Apache virtual host__

    <VirtualHost *:80>
        ServerName   host1.example.com
        ...
        # Handle launcher forwarding
        ProxyPass /paraview http://localhost:8080/paraview

        # Handle WebSocket forwarding
        RewriteEngine On
        RewriteMap  session-to-port  txt:<MAPPING-FILE-DIR>/proxy.txt
        RewriteCond %{QUERY_STRING}  ^sessionId=(.*)$               [NC]
        RewriteRule ^/proxy.*$       ws://${session-to-port:%1}/ws  [P]
    </VirtualHost>

__Python launcher config__

    {
        "resources": [ {"host": "localhost", "port_range": [9001, 9003] } ],
        ...
        "configuration": {
          "host": "localhost",
          "port": 8080,
          "sessionURL": "ws://host1.example.com/proxy?sessionId=${id}",
          "proxy_file": "<MAPPING-FILE-DIR>/proxy.txt"
        },
        ...
        "apps": {
          ...,
          "visualizer": {
            "cmd": [
              "${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_visualizer.py",
              "--port", "${port}", "--data-dir", "${dataDir}", "-f", "--authKey", "${secret}"
            ],
            "ready_line" : "Starting factory"
          },
          ...
        }
    }

__Jetty launcher config__

    # Web setup
    pw.web.port=8080

    # ==================================================
    # Process command: pipeline_manager.py | pipeline
    # ==================================================
    pw.pipeline.cmd=./bin/pvpython -dr ./lib/paraview-4.1/site-packages/paraview/web/pv_web_visualizer.py -f --data-dir <DATA-DIR> --port PORT --authKey SECRET
    pw.pipeline.cmd.run.dir=<PARAVIEW-INSTALL-DIRECTORY>
    pw.pipeline.cmd.map=PORT:getPort|SECRET:secret

    # Resources informations
    pw.resources=localhost:9001-9003

    # Factory
    pw.factory.proxy.adapter=com.kitware.paraviewweb.external.ApacheModRewriteMapFileConnectionAdapter

    # External configurations
    pw.factory.proxy.adapter.file=<MAPPING-FILE-DIR>/proxy.txt

    # Session URL
    pw.factory.session.url.generator.pattern=ws://host1.example.com/proxy?sessionId=SESSION_ID

## Second Example: Two machines

{@img pvw-deploy-opt-2.png}

In this example, Apache sits on one machine, while the launcher and the ParaViewWeb processes run on a second machine.  For Apache and the launcher to communicate through the mapping file, some sort of shared network drive is required.

__Apache virtual host__

    <VirtualHost *:80>
        ServerName   host1.example.com
        ...
        # Handle launcher forwarding
        ProxyPass /paraview http://host2.example.com:8080/paraview

        # Handle WebSocket forwarding
        RewriteEngine On
        RewriteMap  session-to-port  txt:<MAPPING-FILE-DIR>/proxy.txt
        RewriteCond %{QUERY_STRING}  ^sessionId=(.*)$               [NC]
        RewriteRule ^/proxy.*$       ws://${session-to-port:%1}/ws  [P]
    </VirtualHost>

__Python launcher config__

    {
        "resources": [ {"host": "host2.example.com", "port_range": [9001, 9003] } ],
        ...
        "configuration": {
          "host": "localhost",
          "port": 8080,
          "sessionURL": "ws://${host}/proxy?sessionId=${id}",
          "proxy_file": "<MAPPING-FILE-DIR>/proxy.txt"
        },
        ...
        "apps": {
          ...,
          "visualizer": {
            "cmd": [
              "${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_visualizer.py",
              "--port", "${port}", "--data-dir", "${dataDir}", "-f", "--authKey", "${secret}"
            ],
            "ready_line" : "Starting factory"
          },
          ...
        }
    }

__Jetty launcher config__

    # Web setup
    pw.web.port=8080

    # ==================================================
    # Process command: pipeline_manager.py | pipeline
    # ==================================================
    pw.pipeline.cmd=./bin/pvpython -dr ./lib/paraview-4.1/site-packages/paraview/web/pv_web_visualizer.py -f --data-dir <DATA-DIR> --port PORT --authKey SECRET
    pw.pipeline.cmd.run.dir=<PARAVIEW-INSTALL-DIRECTORY>
    pw.pipeline.cmd.map=PORT:getPort|SECRET:secret

    # Resources informations
    pw.resources=localhost:9001-9003

    # Factory
    pw.factory.proxy.adapter=com.kitware.paraviewweb.external.ApacheModRewriteMapFileConnectionAdapter

    # External configurations
    pw.factory.proxy.adapter.file=<MAPPING-FILE-DIR>/proxy.txt

    # Session URL
    pw.factory.session.url.generator.pattern=ws://host2.example.com/proxy?sessionId=SESSION_ID

## Third Example: Many machines

{@img pvw-deploy-opt-3.png}

In this example, Apache sits on one machine, the launcher runs on a second machine, and the ParaViewWeb processes are allowed to run on two machines distinct from each other and the first two.  Again in this scenario, for Apache and the launcher to communicate through the mapping file, some sort of shared network drive is required.

In this situation, the launcher cannot directly start processes on the target ParaViewWeb machines without some intermediary step.  To illustrate one way around this problem, we provide an example shell script which will ssh to the target machine, and then start the process.  The launcher will then run this shell script, passing appropriate arguments the script will need.

The Jetty launcher is not capable of handling multiple target hosts in this way, so only the Python launcher configuration is given.

__Apache virtual host__

    <VirtualHost *:80>
        ServerName   host1.example.com
        ...
        # Handle launcher forwarding
        ProxyPass /paraview http://host2.example.com:8080/paraview

        # Handle WebSocket forwarding
        RewriteEngine On
        RewriteMap  session-to-port  txt:<MAPPING-FILE-DIR>/proxy.txt
        RewriteCond %{QUERY_STRING}  ^sessionId=(.*)$               [NC]
        RewriteRule ^/proxy.*$       ws://${session-to-port:%1}/ws  [P]
    </VirtualHost>

__Python launcher config__

    {
        "resources": [ {"host": "host3.example.com", "port_range": [9001, 9003] },
                       {"host": "host4.example.com", "port_range": [9001, 9003] } ],
        ...
        "configuration": {
          "host": "localhost",
          "port": 8080,
          "sessionURL": "ws://${host}/proxy?sessionId=${id}",
          "proxy_file": "<MAPPING-FILE-DIR>/proxy.txt"
        },
        ...
        "apps": {
          ...,
          "visualizer": {
            "cmd": [
              "customScript.sh", "${host}", "pvw-user", ${port}", "${python_path}/paraview/web/pv_web_visualizer.py", "${dataDir}"
            ]
          },
          ...
        }
    }

__customScript.sh__

    ###
    ### Script which will secure shell to remote machine, then launch
    ### pvweb visualizer process with specific port and data dir.
    ###

    ### Grab the command line arguments into variables
    hostname=$1
    username=$2
    port=$3
    app=$4
    datadir=$5

    ssh $username@$hostname <<EOF
    export DISPLAY=:0.0
    export PATH=/opt/python-2.7.3/bin:\$PATH
    export LD_LIBRARY_PATH=/opt/python-2.7.3/lib

    "/home/pvw-user/projects/ParaView/build/bin/pvpython" "-dr" "/home/pvw-user/projects/ParaView/build/lib/site-packages/${app}" "--data-dir" "${datadir}" "--port" "${port}"
    EOF

In the above example, we made many simplifying assumptions in order to keep the configuration and script concise while still illustrating the important concepts.
