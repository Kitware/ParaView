# ParaViewWeb on Ubuntu 14.04 LTS

## Introduction

This document describes how to set up a ParaViewWeb server instance on a freshly installed Ubuntu Desktop 14.04 LTS.
The server version can also be used, but to properly leverage the GPU, you will need to install and run X or build ParaView with OSMesa.

This document is split in 4 sections:

1. Configuring the Linux distribution so all the needed component are available
2. Installing ParaViewWeb
3. Configuring the ParaViewWeb launcher
4. Configuring Apache to serve the standard ParaViewWeb example application and documentation
5. Structuring the Web Site content with the ParaViewWeb sample application

## Installation of Ubuntu 14.04

### Package installation

Once you've properly installed your new Unbuntu Desktop you may want to add all the missing libraries and dependencies.

    $ sudo apt-get update
    $ sudo apt-get upgrade
    $ sudo apt-get install \
        apache2-dev apache2 libapr1-dev apache2-utils        # Apache Web Server
        nvidia-current mesa-common-dev libxt-dev             # OpenGL drivers
        build-essential python2.7-dev cmake-curses-gui       # (Optional) Needed if you want to build ParaView
        ssh wget emacs                                       # (Optional) Useful tools

If you are using Ubuntu Server, you should also install kde to make sure you have a graphics environment:

    $ sudo apt-get install kde-full

In order to properly install the NVidia drivers, you will need to execute the following command line and restart:

    $ sudo nvidia-xconfig

    # => Reboot

    $ sudo nvidia-settings

    # => Make sure your screen and GPU has been properly detected.

### ParaViewWeb user creation

Unless you have your own launcher application that will take care of who is running
the visualization process based on your SSO system, you may want to simply use
the process launcher that comes with ParaView. And if you do so, you might not want that
process to run under your 'powerful' user.

    # Create a new user
    $ adduser pvw-user

    # Add that user to the apache user group
    $ usermod -a -G www-data pvw-user

Then you should make sure that this user will auto login in the graphical environment.
This will allow ParaViewWeb to properly use your GPU.

### Setting up the directory structure for later

The path provided below are just an example on how to structure the different pieces that will
be needed for your web server. But by defining them here it is easier to reference them later on.
In the proposed structure, everything will be nested underneath the /data directory.
So let's create those directories.

    $ sudo mkdir -p /data/pvw /data/pv /data/logs /data/www

Directory structure:

* /data/www will contains the html/js/css files that compose your web site that will leverage ParaViewWeb.
* /data/pv will contains the ParaView installation binaries.
* /data/pvw will contains the ParaView process launcher that will dynamically trigger a new process for each visualization session.
* /data/logs will contains the Apache logs that will serve your ParaViewWeb virtual host.

## Installation of ParaViewWeb

ParaView which contains ParaViewWeb is available for download [here](http://www.paraview.org/paraview/resources/software.php).
And you can even take the latest version that was build on the master branch of ParaView which is supposed to be stable.
If you want to build ParaView yourself, you can refer to the Wiki [here](http://paraview.org/Wiki/ParaView).

In any case, you should have a directory structure that looks like that:

    -> paraview-xxx
        + bin
          - pvpython
          - pvserver
          - ...
        + lib
          + paraview-4.1
            - libvtkXXXX.so
            + site-packages
              + paraview
              + vtk
              + ...
        + share
          + paraview-4.1
            + www
              + apps
              + lib
              + ext

Let's copy it in our global directory structure and assign it to our ParaViewWeb user:

    $ sudo cp -r /.../paraview-xxx /data/pv/pv-20140410
    $ sudo ln -s /data/pv/pv-20140410 /data/pv/pv-current
    $ sudo chown -R pvw-user /data/pv
    $ sudo chgrp -R pvw-user /data/pv

## Configuration of ParaViewWeb launcher

We've already created the /data/pvw directory to embed the ParaView Process Launcher with its configuration.
Let's structure the content of that directory.

    $ sudo mkdir -p /data/pvw/bin /data/pvw/conf /data/pvw/data /data/pvw/logs

When you edit the configuration file, shown below, be sure to replace `YOUR_HOST_NAME_TO_REPLACE` with your actual hostname.

    $ sudo vi /data/pvw/conf/launcher.json

      {
        "resources": [ {"port_range": [9001, 9103], "host": "localhost"} ],
        "sessionData": {
          "updir": "/Home"
        },
        "configuration": {
          "log_dir": "/data/pvw/logs",
          "host": "localhost",
          "endpoint": "paraview",
          "sessionURL": "ws://YOUR_HOST_NAME_TO_REPLACE/proxy?sessionId=${id}",
          "timeout": 25,
          "upload_dir": "/data/pvw/upload",
          "fields": ["file", "host", "port", "updir"],
          "port": 8080,
          "proxy_file": "/data/proxy.txt"
        },
        "properties": {
          "python_path": "/data/pv/pv-current/lib/paraview-4.1/site-packages",
          "dataDir": "/data/pvw/data",
          "python_exec": "/data/pv/pv-current/bin/pvpython"
        },
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

    $ sudo vi /data/pvw/bin/start.sh

      #!/bin/bash

      export DISPLAY=:0.0
      /data/pv/pv-current/bin/pvpython /data/pv/pv-current/lib/paraview-4.1/site-packages/vtk/web/launcher.py /data/pvw/conf/launcher.json &

    $ sudo touch /data/proxy.txt
    $ sudo chown pvw-user /data/proxy.txt
    $ sudo chgrp www-data /data/proxy.txt
    $ sudo chmod 660 /data/proxy.txt

Add ParaView Data as sample data to visualize

    $ wget http://paraview.org/files/v4.0/ParaViewData-v4.0.1.tar.gz
    $ tar zxvf ParaViewData-v4.0.1.tar.gz
    $ sudo cp -r ParaViewData-v4.0.1/Data/* /data/pvw/data

Change the security rights to the directory content

    $ sudo chown -R pvw-user /data/pvw
    $ sudo chgrp -R pvw-user /data/pvw
    $ sudo chmod u+x /data/pvw/bin/start.sh

Then, you need the user pvw-user to execute /data/pvw/bin/start.sh when it is properly logged in with its display setup...

## Configuring Apache

### Introduction

Apache will act as our front-end webserver. This means that both the HTML content and the WebSocket forwarding will be handled by Apache.

### Configuration

First of all, you will need to enable the modules that will be used by our ParaViewWeb virtual host.

    $ sudo a2enmod vhost_alias
    $ sudo a2enmod proxy
    $ sudo a2enmod proxy_http
    $ sudo a2enmod proxy_wstunnel
    $ sudo a2enmod rewrite

Then lets create our virtual host.  Be sure to replace `YOUR_HOST_NAME_TO_REPLACE`, shown in the example below, with your actual host name.  Also include a real email address in place of `YOUR_EMAIL@COMPANY.COM`.

    $ vi  /etc/apache2/sites-available/001-pvw.conf

        <VirtualHost *:80>
          ServerName   YOUR_HOST_NAME_TO_REPLACE
          ServerAdmin  YOUR_EMAIL@COMPANY.COM
          DocumentRoot /data/www

          ErrorLog /data/logs/error.log
          CustomLog /data/logs/access.log combined

          <Directory /data/www>
              Options Indexes FollowSymLinks
              Order allow,deny
              Allow from all
              AllowOverride None
              Require all granted
          </Directory>

          # Handle launcher forwarding
          ProxyPass /paraview http://localhost:8080/paraview

          # Handle WebSocket forwarding
          RewriteEngine On
          RewriteMap  session-to-port  txt:/data/proxy.txt
          RewriteCond %{QUERY_STRING}  ^sessionId=(.*)$               [NC]
          RewriteRule ^/proxy.*$       ws://${session-to-port:%1}/ws  [P]
        </VirtualHost>

Then enable that virtual host and restart Apache

    $ sudo a2ensite 001-pvw.conf
    $ sudo service apache2 restart

If you run into problems with your new virtual host listening properly, you may need to disable the default virtual hosts file as follows:

    $ sudo a2dissite 000-default.conf

## Setting up the ParaViewWeb Web Site

You can download the documentation of ParaView [here](http://paraview.org/paraview/resources/software.php)
And download the ParaView-API-docs-v4.1.zip file or any newer version.

By uncompressing that file, you should get something like that:

     -> ParaView-API-docs-v4.1
        + js-doc
           + ...
        + ...

Then copy the ParaViewWeb documentation over to /data/www

    $ sudo cp -r /.../ParaView-API-docs-v4.1/js-doc/* /data/www

Fix the home page to allow the access to the sample applications

    $ sudo mv /data/www/index.html /data/www/index.origin
    $ sudo sh -c 'cat /data/www/index.origin | grep -v DEMO-APPS > /data/www/index.html'

Add the ParaViewWeb code

    $ sudo cp -r /data/pv/pv-current/share/paraview-4.1/www/* /data/www/


## Hardware graphics notes for Ubuntu 14.04 LTS on EC2

For notes on getting the graphics environment set up properly on instances running Ubuntu 14.04 LTS, the [EC2 Graphics Setup](index.html#!/guide/graphics_on_ec2_g2) guide.
