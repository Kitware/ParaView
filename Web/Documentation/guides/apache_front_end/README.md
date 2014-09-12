# Using Apache as a front end

## Introduction

In order to use Apache as a front end for ParaViewWeb, we recommend that you use version 2.4.7 or later of Apache.  This is because as of version 2.4.7 Apache has, out of the box, everything required for it to serve as a ParaViewWeb front end.  If you must use an older version, you will have to compile it yourself, after patching it so that mod_rewrite can handle web sockets, and so that it can do websocket tunnelling.  At the bottom of this guide, you can find some notes on how to patch and build Apache yourself.  Although those instructions are specific to an Amazon Linux AMI instance, you should hopefully be able to adapt them to your own environment without too much trouble.

In either case, once Apache is ready to run, you will need to configure it together with your chosen launcher (see [Multi-User Setup](index.html#!/guide/multi_user_setup) for information) to manage visualization sessions.

If you build Apache yourself, it's usually a good idea to put it somewhere out of the way, where it won't conflict with any system Apache you may have in place.  We usually choose somewhere in `/opt`.  Otherwise, if you use a package-installed version of Apache, it's typically already located in `/etc/apache2`.

## Getting Apache

The easiest thing to do is just to install Apache from a package.  On recent Ubuntu distributions, this can be done as follows:

    $ sudo apt-get install apache2-dev apache2 libapr1-dev apache2-utils

If the packaged version of Apache is not recent enough on your system, or in case you want to compile it yourself for some other reason, there are instructions on how to do that at the bottom of this guide.

## Configure Apache httpd for use with ParaViewWeb

Next we address the configuration of Apache for use with ParaViewWeb.

### Create a proxy/mapping file

Choose a directory for the mapping file that the launcher and Apache use to communicate about sessions and port locations.  Then create a group so that both components have access to the file.  For the purpose of these instructions, assume the full path to the directory you have chosen is `<MAPPING-FILE-DIR>`, assume that `daemon` is the user who will run Apache, and assume that `pvw-user` is the user who will run the launcher and you are logged in as this user.  Then you would do the following:

    $ sudo mkdir -p <MAPPING-FILE-DIR>
    $ sudo touch <MAPPING-FILE-DIR>/proxy.txt
    $ sudo groupadd mappingfileusers
    $ sudo usermod -a -G mappingfileusers pvw-user
    $ newgrp mappingfileusers
    $ sudo usermod -a -G mappingfileusers daemon
    $ sudo chgrp mappingfileusers <MAPPING-FILE-DIR>/proxy.txt
    $ sudo chmod 660 <MAPPING-FILE-DIR>/proxy.txt

### Add a virtual host

Now add a virtual host to the Apache configuration.

#### If you compiled Apache yourself

In this case, you will probably find a file named `httpd-vhosts.conf` located in `/opt/apache-2.4.7/conf/extra/` or similar.  You should remove the contents of that file and replace them with the virtual host configuration shown below.

#### If you installed Apache 2.4.7 from a package

In this case you should create a file in `/etc/apache2/sites-available/` and make a symbolic link to it from `/etc/apache2/sites-enabled/`.  We'll assume you named this file `001-pvw.conf`.

In either case, make sure to replace the `ServerName` value (shown below as `<MY-SERVER-NAME>`) with the correct host name.  Also make sure the `DocumentRoot` value (shown below as `<MY-DOCUMENT-ROOT>`) makes sense for your particular deployment, we typically point it at the `www` directory of the ParaView build or install tree.

    <VirtualHost *:80>
        ServerName <MY-SERVER-NAME>
        ServerAdmin webmaster@example-host.example.com
        DocumentRoot "<MY-DOCUMENT-ROOT>"
        ErrorLog "logs/pv-error_log"
        CustomLog "logs/pv-access_log" common

        # Have Apache pass these requests to the launcher
        ProxyPass /paraview http://localhost:9000/paraview

        # Turn on the rewrite engine
        RewriteEngine On

        # This is the path the mapping file Jetty creates
        RewriteMap session-to-port txt:<MAPPING-FILE-DIR/proxy.txt

        # This is the rewrite condition. Look for anything with a sessionId= in the query part of the URL and capture the value to use below.
        RewriteCond %{QUERY_STRING}     ^sessionId=(.*)$ [NC]

        # This does the rewrite using the mapping file and the sessionId
        RewriteRule    ^/proxy.*$  ws://${session-to-port:%1}/ws  [P]

        <Directory "<MY-DOCUMENT-ROOT">
            Options Indexes FollowSymLinks
            Order allow,deny
            Allow from all
            AllowOverride None
            Require all granted
        </Directory>

    </VirtualHost>

#### If you compiled Apache yourself

The following notes are only relevant if you compiled Apache yourself.  If you installed Apache >= 2.4.7 from a package, skip to the next subsection.

Include the virtual host file you configured above in the main httpd configuration file.  Find the following line in `httpd.conf` and uncomment it:

    Include conf/extra/httpd-vhosts.conf

Find the following lines in the httpd.conf file and uncomment them in order to load some necessary modules:

    LoadModule slotmem_shm_module modules/mod_slotmem_shm.so
    LoadModule rewrite_module modules/mod_rewrite.so

Start the httpd daemon.

    $ sudo /opt/apache-2.4.7/bin/apachectl -k start

#### If you installed Apache 2.4.7 from a package

First of all, you will need to enable the modules that will be used by our ParaViewWeb virtual host.

    $ sudo a2enmod vhost_alias
    $ sudo a2enmod proxy
    $ sudo a2enmod proxy_http
    $ sudo a2enmod proxy_wstunnel
    $ sudo a2enmod rewrite

Then enable the virtual host you created above and restart Apache

    $ sudo a2ensite 001-pvw.conf
    $ sudo service apache2 restart

If you run into problems with your new virtual host listening properly, you may need to disable the default virtual hosts file as follows:

    $ sudo a2dissite 000-default.conf

## Building Apache: EC2 Amazon Linux AMI

This section describes steps for compiling Apache on an EC2 instance running the Amazon Linux AMI.  It is recommended to obtain Apache 2.4.7 or later to ease compilation, but instructions for patching earlier versions of Apache are included below, in case Apache 2.4.7 can not be used for some reason.  The steps for compiling Apache on other operating systems will be very similar to the ones below, the major differences are in how you install packages and the names of those packages.

The first step is to install some required packages on the instance.  The `patch` package is required for patching the Apache httpd source, and `pcre-devel` is required by the Apache compilation phase.

    $ sudo yum install patch.x86_64
    $ sudo yum install pcre-devel.x86_64

### Download, build, and install Apache httpd

General instructions for building and installing Apache 2.4.7 on the EC2 machine can be found at the following URL:

    http://httpd.apache.org/docs/2.4/install.html

However, the instructions below give detailed steps that can be followed on the Amazon EC2 AMI instance.

Obtain the necessary source tarballs. You will need httpd source, as well as apr and apr-util sources.

    $ mkdir /home/ec2-user/downloads
    $ cd /home/ec2-user/downloads
    $ wget http://mirrors.sonic.net/apache/httpd/httpd-2.4.7.tar.gz -O httpd.tgz
    $ wget http://apache.petsads.us/apr/apr-1.5.0.tar.gz -O apr.tgz
    $ wget http://apache.petsads.us/apr/apr-util-1.5.3.tar.gz -O apr-util.tgz

Now unpack everything in the right places.

    $ mkdir /home/ec2-user/apache-2.4.7-src
    $ cd /home/ec2-user/apache-2.4.7-src
    $ tar zxvf /home/ec2-user/downloads/httpd.tgz
    $ cd httpd-2.4.7/srclib
    $ tar zxvf /home/ec2-user/downloads/apr.tgz
    $ mv apr-1.5.0 apr
    $ tar zxvf /home/ec2-user/downloads/apr-util.tgz
    $ mv apr-util-1.5.3 apr-util

Note that for Apache version less than 2.4.7, you will need to patch
the sources before building:

    $ wget -O httpd-patch.txt https://issues.apache.org/bugzilla/attachment.cgi?id=30886&action=diff&context=patch&collapsed=&headers=1&format=raw
    $ cd httpd-[version]
    $ patch -u -p1 < /home/ec2-user/downloads/httpd-patch.txt

Now configure the Apache build.

    $ cd /home/ec2-user/apache-2.4.7-src/httpd-2.4.7
    $ ./configure --prefix /opt/apache-2.4.7 --with-included-apr --enable-proxy

Check how many processors are available and use them all to build httpd.  The nproc command returns the number of processors that are available. Use this number in the make command.  When the build is finished, install httpd.

    $ nproc
    $ make -j<number>
    $ sudo make install

### Run your new version of Apache

Now that you have compiled and installed the new version of Apache, you run it as follows:

    $ sudo /opt/apache-2.4.7/bin/apachectl -k start
