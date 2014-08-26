# How to set up an EC2 AMI instance to run ParaViewWeb

## Notice

To reduce the amount of content duplication in these guides, most of this document has been re-organized into other guides.  We have left its outline in place here, with pointers to relevant guides in each section.

## Introduction

This document describes how to set up a clean EC2 instance, running Amazon Linux AMI, and launched on a g2.2xlarge instance.  The configuration described will be the following:

1. Deploy the latest nightly binary version of ParaView
2. Use a recent version of Apache, which now supports web sockets
3. Use the Jetty session manager to launch python processes

In this document, we will refer to the DNS name of our EC2 host as

    ec2-XXX-XXX-XXX-XXX.compute-1.amazonaws.com

You should replace this with the actual hostname of a running instance. Additionally, whenever you secure shell into your instance, it is possible that you will get a message from the system regarding the installation of updates.  You may want to do that before you begin.

## Package install and graphics environment setup

Some of the packages to be installed were related to building Apache, see the [Apache as a front end](index.html#!/guide/apache_front_end) guide for details.  For information about graphics environment setup, please see the [EC2 Graphics Setup](index.html#!/guide/graphics_on_ec2_g2) guide.

## ParaView

The instructions for setting up ParaView have been simplified by a python script recently added to the ParaView superbuild.  You can download the script directly from [here](http://www.paraview.org/gitweb?p=ParaViewSuperbuild.git;a=blob_plain;f=Scripts/pvw-setup.py;hb=HEAD).  Then you just run:

    $ python pvw-setup.py

and answer the prompts.  This will download the release version of ParaView, as well as documentation and data, and set up a website with links to the ParaViewWeb applications.

At this point, you can check to make sure that at least the static content is available by pointing your browser at the instance.  As an example, you can paste a URL like the following one into your browser's location bar:

    http://ec2-XXX-XXX-XXX-XXX.compute-1.amazonaws.com

Just make sure to use the correct instance DNS name.

## Apache

Instructions for configuring (and also building) Apache can now be found in the [Apache as a front end](index.html#!/guide/apache_front_end) guide.

## Configure and run the launcher

The instructions in the section illustrated how to set up Jetty as the launcher for use with the Apache front end.  Those instructions can now be found in the [Jetty Launcher](index.html#!/guide/jetty_session_manager) guide.  However, we now recommend using the  instead.

Once the launcher is configured and ready, there are a few more steps before you can start it.  The final steps involve starting an X server (in the background), setting the DISPLAY environment variable, and finally running the launcher itself (also in the background).

### Prepare to render

    $ sudo X :0 &
    $ export DISPLAY=:0.0

### Start the launcher

Now you can start the launcher.  See the appropriate guide ([Python Launcher](index.html#!/guide/python_launcher) or [Jetty Launcher](index.html#!/guide/jetty_session_manager)) for how to start the launcher of your choice.

## Final Testing

Now your instance should be set up to run ParaViewWeb.  Go back to your browser and choose any of the applications linked from the ParaViewWeb page you tested above.
