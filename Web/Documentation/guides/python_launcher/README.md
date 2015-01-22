# How to use the Python Launcher

## Introduction

When deploying ParaViewWeb for multiple users, you will need a launcher module that will start a new visualization process for each user that requests one.  This task could be achieved by the Jetty launcher (see the [Jetty Launcher](index.html#!/guide/jetty_session_manager) guide for details). However, we wanted to provide a simple answer that did not require the use of an external component that is not already available within the ParaView binaries.  Hence, we built a Python based process launcher that follows the ParaViewWeb RESTful API for launching a new visualization process (see the [Launcher RESTful API](index.html#!/guide/launcher_api) guide for details about the API itself.

Because the Python launcher is included in ParaViewWeb and requires no extra software to get up and running, it is the recommended approach to launching ParaViewWeb visualization sessions.  This document covers how to configure the Python launcher for use with a front end, including specific details on getting it working with Apache.

## Role of the launcher

The launcher has several specific tasks, all with the common goal of allowing clients to request visualization sessions and then interact with them.  The launcher needs to respond to requests for new sessions by:

1. starting a new process for the user
1. generating a unique key for that session
1. communicating that unique key back to both the client and the front end so that the front end knows how to forward subsequent session requests to the correct visualiation process

## Configuration of the launcher

The launcher provides a great deal of flexibility, which can be leveraged by customizing its configuration file.  In this guide, as in the [Apache as a front end](index.html#!/guide/apache_front_end) guide, we assume you are using Apache as the front end, and we also assume the directory you have chosen for the mapping file is `<MAPPING-FILE-DIR>`.  The Following is an example configuration file:

__launcher.config__

    {
      ## ===============================
      ## General launcher configuration
      ## ===============================

      "configuration": {
        "host" : "localhost",                           # name of network interface to bind for launcher webserver
        "port" : 8080,                                  # port to bind for launcher webserver
        "endpoint": "paraview",                         # SessionManager Endpoint
        "content": "/.../www",                          # Optional: Directory shared over HTTP
        "proxy_file" : "<MAPPING-FILE-DIR>/proxy.txt",  # Proxy-Mapping file for Apache
        "sessionURL" : "ws://${host}:${port}/ws",       # ws url used by the client to connect to started process
        "timeout" : 5,                                  # Wait time in second after process start
        "log_dir" : "/.../viz-logs",                    # Directory for log files
        "upload_dir" : "/.../uploadDirectory",          # Start file upload server on same port as launcher
        "fields" : ["file", "host", "port", "updir"]    # Fields not listed are filtered from response
      },

      ## ===============================
      ## Resources list for applications
      ## ===============================

      "resources" : [ { "host" : "localhost", "port_range" : [9001, 9003] } ],

      ## ===============================
      ## Set of properties for cmd line
      ## ===============================

      "properties" : {
        "build_dir" : "/.../build",
        "python_exec" : "/.../build/bin/vtkpython",
        "WWW" : "/.../build/www",
        "source_dir": "/.../src"
      },

      ## ===============================
      ## Application list with cmd line
      ## ===============================

      "apps": {
        "pipeline": {
          "cmd": [
            "${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_visualizer.py",
            "--port", "${port}", "--data-dir", "${dataDir}", "-f", "--authKey", "${secret}"
          ],
          "ready_line" : "Starting factory"
        },
        "visualizer": {
            "cmd": [
                "${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_visualizer.py",
                "--port", "${port}", "--data-dir", "${dataDir}", "-f", "--authKey", "${secret}"
            ],
            "ready_line" : "Starting factory"
        },
        "loader": {
            "cmd": [
                "${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_file_loader.py",
                "--port", "${port}", "--data-dir", "${dataDir}", "-f", "--authKey", "${secret}"
            ],
            "ready_line" : "Starting factory"
        },
        "data_prober": {
            "cmd": [
                "${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_data_prober.py",
                "--port", "${port}", "--data-dir", "${dataDir}", "-f", "--authKey", "${secret}"
            ],
            "ready_line" : "Starting factory"
        }
        }
    }


In order to run the Python launcher service, you will need to execute the following command line.

    $ cd ParaView/build
    $ ./bin/pvpython lib/site-package/vtk/web/launcher.py launcher.config

or inside VTK

    $ cd VTK/build
    $ ./bin/vtkpython Wrapping/Python/vtk/web/launcher.py launcher.config


 It is important to take special note a few of the details:

- The `apps` section gives the command lines required to start any of the visualization processes the launcher will be capable of starting.  We recommend always providing the `-dr` argument to `pvpython` so that it never tries to load any saved preferences it might find.

- The `configuration` section gives several important values:
  - `sessionURL` gives the url needed to communicate with the visualization processes.  In the case of the Apache front end, Apache will recognize this url and re-write it so that Apache websocket forwarding can send the packets to the correct running process on the back end.  This `sessionURL` value needs to match what is expected by the Apache RewriteRule (given in the Apache virtual host configuration).  See the [Apache as a front end](index.html#!/guide/apache_front_end) guide for details.
  - `proxy_file` gives the location of the mapping file that the launcher and front end will use to communicate about which session ids get mapped to which port.  The location of this file must concur with the Apache RewriteMap value (given in the Apache virtual host configuration).

- `sessionData` is designed to allow you to specify that certain arbitrary key/value pairs should be included in the data returned to the client upon successful creation of the session.  In the example launcher config file above, we have added a key called `updir` with a value of `/Home` to the `sessionData` segment.  This will cause the launcher to perform normal substitutions on the on the value (in this case, none are needed as the value is simply `/Home`), and then include `"updir": "/Home"` in the response when sessions are successfully created for clients.
  - Note, however, that the in the `configuration` section of the launcher config file, the `fields` key denotes a list of keys that will be used as a filter when deciding which data should be returned by the server in response to the client's JSON payload.  In other words, if you want a key/value pair returned to the client when the session has been successfully started, make sure to include the key name of this desired pair in the `fields` list of the `configuration` section.  In the example launcher config above, therefore, we have added the key `updir` to the `fields` filter list so that the `"updir":"/Home"` key pair will actually be returned to the client, instead of filtered out.

- `resources` is a list of host/port-range combinations that the launcher will keep track of and assign to incoming client requests.  The launcher will take the hosts and associated port ranges for each host and create a master list of available "resources" it can assign to clients.  For example if `resources` are given as follows:

    $ "resources" : [ { "host" : "host1.example.com", "port_range" : [9001, 9003] },
                      { "host" : "host2.example.com", "port_range" : [9001, 9003] } ],

then the launcher will build the following list of "resources":

    [
        ["host1.example.com", 9001],
        ["host1.example.com", 9002],
        ["host1.example.com", 9003],
        ["host2.example.com", 9001],
        ["host2.example.com", 9002],
        ["host2.example.com", 9003]
    ]

When a request comes in from a client, then the launcher will choose the first available resource, and use it to fulfill the request.  The host and port the launcher chooses will be available as `${host}` and `${port}` in both the `sessionURL` configuration variable, as well as in any of the `application` command lines.  When the process associated to the client request eventually ends, the [host, port] combination will again become available for future requests.  Until such time, that particular [host, port] combination, or "resource", is considered by the launcher to be unavailable.

## Optional Upload Server

The launcher can now start an optional file upload server to handle multipart POST requests on the same server and port as the launcher itself, as in the launcher config file example, above.  By specifying the `upload_dir` property in the `configuration` section of the launcher config file, you can tell the launcher to start the file upload server in such a way that uploaded files will get written in the path given by the value of the `upload_dir` property.  In order to use the upload server, you would then issue a POST request with a multipart file attachment to the url `http://launcher-host:launcher-port/upload`, replacing `launcher-host` and `launcher-port` with the correct values.

## Python launcher as a front end

It is worth mentioning briefly that the Python launcher can also be used as a front end.  Specifying a path for the `content` key of the `configuration` section of the laucher config json will cause the Python launcher to serve static content from listed directory.  The Python launcher can not, however, do websocket forwarding/tunnelling, so in this situation, clients would need direct access to the host/port combinations where the visualization processes are running.  Then, the `sessionURL` would take this form:

    $ "sessionURL" : "ws://${host}:${port}/ws",

rather than this one:

    $ "sessionURL": "ws://YOUR_HOST_NAME_TO_REPLACE/proxy?sessionId=${id}",
