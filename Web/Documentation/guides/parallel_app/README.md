# Parallel Visualization Setup

## Introduction

As opposed to VTK-Web, ParaViewWeb can run on a distributed environment using MPI.
ParaViewWeb can also use composite rendering for a large dataset.
In order to use ParaViewWeb in a distributed context, such as regular ParaView,
you will need to start a remote server with several processes using MPI.
Then, you will need to provide the proper host and port information to the
visualization application that you plan to use.

In the set of applications provided within ParaView, one of them illustrates
how to accomplish this. The aim of the documentation is to properly walk you
through all the required configuration and deployment steps.

## The Parallel Web application

The Parallel application allows the user to pick a setting that he or she wants to use
for an existing Web application.
Examples settings include the number of processes that should be used to run
the server and the file that should be pre-loaded.
In addition, the application can be extended to customize the way the server is launched, as well as
what the client should do once the server is loaded.

The following picture illustrates what the application looks like.

{@img images/Parallel-WebApp.png The ParaViewWeb parallel application }

## How it works

The Parallel application simply uses an intermediate launcher to process
extra information such as the application name and the number of nodes.
Based on those options, the intermediate launcher will start pvserver using
MPI and then connect the client to that distributed server using the appropriate
python web server.

Then, on the client side, the page will reload the targeted web application using a new URL. This web application has additional parameters, such as the file name to load and the session that should be used to connect to the previously started pvserver instance.

## How to deploy

### Extend Multi-Session configuration

In this example, we will assume that you are using the JettySessionManager
to launch the ParaViewWeb process.
In that case, you will need to extend your existing configuration to register
a "launcher" application, which is simply the script that is provided later as a reference.

    pw.launcher.cmd=./launcher.sh PORT CLIENT RESOURCES FILE SECRET
    pw.launcher.cmd.run.dir=/path-that-contains-launcher-script
    pw.launcher.cmd.map=PORT:getPort|SECRET:secret|CLIENT:client|FILE:file|RESOURCES:resources

In addition, we need to make sure the extended key that we are using will be provided to
the client. In order to do so, you will need to extend the property __pw.session.public.fields__ with the following set of keys: [file, (any other that you may have added yourself) ]

    pw.session.public.fields=id,sessionURL,name,description,sessionManagerURL,application,idleTimeout,startTime,secret,file

Here is the content of the launcher.sh bash script:

    #!/bin/bash

    # Configuration paths
    PV_WEB_SERVER_PATH=/paraview-build-tree/lib/site-packages/paraview/web
    PV_BUILD_PATH=/.../FIXME/paraview-build-tree
    DATA_DIR=/.../FIXME/ParaViewData/Data

    PVPYTHON=$PV_BUILD_PATH/bin/pvpython
    PVSERVER=$PV_BUILD_PATH/bin/pvserver

    # expect PORT CLIENT RESOURCE FILE
    usage(){
        echo "Usage: $0 portNumber client-app-name resources file-to-load"
        echo "   portNumber: The port on which the ParaViewWeb app should listen"
        echo "   client-app-name: [loader, visualizer]"
        echo "   resource: Number of nodes to run on"
        echo "   file-to-load: File that should be pre-loaded is any"
        echo "   secret: Password used to authenticate user"
        exit 1
    }

    # Check if we got all the command line arguments we need
    if [[ $# -ne 5 ]]
    then
        usage
    fi

    # Grab the command line arguments into variables
    HTTPport=$1
    client=$2
    resource=$3
    file=$4
    secret=$5
    server=$PV_WEB_SERVER_PATH/pv_web_file_loader.py

    # Find the server script to use
    if [ $client = 'FileViewer' ]
    then server=$PV_WEB_SERVER_PATH/pv_web_file_loader.py
    fi
    if [ $client = 'Visualizer' ]
    then server=$PV_WEB_SERVER_PATH/pv_web_visualizer.py
    fi

    # Find available MPI port
    MPIport=11111
    for MPIportSearch in {11111..11211}
    do
      # Linux : result=`netstat -vatn | grep ":$port " | wc -l`
      # OSX   : result=`netstat -vatn | grep ".$port " | wc -l`
      result=`netstat -vatn | grep ".$MPIportSearch " | wc -l`
      if [ $result == 0 ]
      then
          MPIport=$MPIportSearch
          break
      fi
    done

    # Run the MPI server
    echo mpirun -n $resource $PVSERVER --server-port=$MPIport &
    mpirun -n $resource $PVSERVER --server-port=$MPIport &

    # Wait for the server to start and run the client
    sleep 1
    echo $PVPYTHON -dr $server -f --ds-host localhost --ds-port $MPIport --port $HTTPport --data-dir $DATA_DIR --authKey $secret
    $PVPYTHON -dr $server -f --ds-host localhost --ds-port $MPIport --port $HTTPport --data-dir $DATA_DIR --authKey $secret

## Customize the values of the form

The application automatically downloads a JSON file for each field.
In order to present other default options, you will need to edit
the JSON files located in the www/apps/Parallel/ directory.

Here is the information for those files:

__client.json__ provide the list of possible application that the user can choose from.

    {
        "FileViewer" : {
            "label": "File Loader"
        },
        "Visualizer" : {
            "label": "Generic Visualizer"
        }
    }

__file.json__ provide the list of files that can be loaded.

    {
        "None" : {
            "label": "None"
        },
        "can.ex2": {
            "label" : "can.ex2"
        },
        "disk_out_ref.ex2": {
            "label" : "disk_out_ref.ex2"
        },
        "bot2.wrl": {
            "label" : "bot2.wrl"
        },
        "bunny.ply": {
            "label" : "bunny.ply"
        },
        "cow.vtp": {
            "label" : "cow.vtp"
        },
        "disk_out_ref.ex2": {
            "label" : "disk_out_ref.ex2"
        },
        "disk_out_ref.ex2": {
            "label" : "disk_out_ref.ex2"
        },
        "subdir/": {
            "label" : "sub-directory",
            "children": {
                "subdir/wavelet": {
                    "label": "Wavelet"
                },
                "subdir/subdir": {
                    "label" : "another directory",
                    "children": {
                        "subdir/subdir/subdir": {
                            "label": "one more dir" ,
                            "children" : {
                                "/home/seb/Data/visit": {
                                    "label" : "Visit",
                                    "children": {
                                        "the-path-you-want": {
                                            "label": "Wavelet"
                                        },
                                        "the-path-you-want/yo": {
                                            "label" : "YO",
                                            "children": {
                                                "the-path-you-want/yo/ttt": {
                                                    "label": "tttt"
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

__resource.json__ provide the number of nodes we can run on.

    {
        "1" : {
            "label": "1 Node"
        },
        "2" : {
            "label": "2 Nodes"
        },
        "5" : {
            "label": "5 Nodes"
        },
        "10" : {
            "label": "10 Nodes"
        }
    }