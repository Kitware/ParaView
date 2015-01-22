
Automating ParaViewWeb Deployment
=================================

Introduction
------------

The goal of this document is to describe the steps needed to set up a machine to routinely download, install, and configure everything necessary to deploy the latest version of ParaViewWeb for automated testing.  We describe a system which uses Apache HTTP server to serve static content and do websocket forwarding, and which uses the python launcher to start ParaViewWeb processes for clients.

In this document, we avoid explicitly typing `sudo` anywhere, so please assume its use wherever it might be necessary.  Additionally, we frequently use angle brackets, <>, to enclose some string that should be replaced with a value appropriate to the target system.  Use cautious judgement however, as we may have missed one or two strings that still need to be replaced.

Create a directory structure
----------------------------

The first step is to plan out the directory structure for autodeployment.  In this case, our goal was to keep most components static, and just update the ParaView binaries, data, and docs.  Here is a recent example of how we organized one system:

    /pvw-test                     # Top level directory for everything
    /pvw-test/autodeploy          # Install dir for all ParaView files
    /pvw-test/bin                 # Place launcher start script here
    /pvw-test/conf                # Launcher config, logrotate config
    /pvw-test/logs                # Apache, python launcher, and pvweb logs
    /pvw-test/logs/autodeploy     # Output of update script
    /pvw-test/www                 # ParaView version hash, and symlinks to autodeploy

Files contained in the top level directory were limited to:

    /pvw-test/proxy.txt           # Mapping file, referenced below
    /pvw-test/pvw-setup.py        # ParaView update script (Python)
    /pvw-test/run-pvw-setup.sh    # Bash script to run above Python script

Though in the future, I'll be downloading the Python script from the code repo where it lives each time I need to run it.

Virtual host setup
------------------

In order to configure Apache to serve ParaViewWeb static content, proxy launcher requests, and forward websocket requests, you can now create a virtual host file something like this one:

    <VirtualHost *:80>
        ServerName <my-domain-dot-com>
        ServerAdmin <me>@<my-domain-dot-com>
        DocumentRoot /pvw-test/www
        ErrorLog /pvw-test/logs/error.log
        CustomLog /pvw-test/logs/access.log combined

        <Directory /pvw-test/www>
            Options Indexes FollowSymLinks
            Order allow,deny
            Allow from all
            AllowOverride None
            Require all granted
        </Directory>

        # Handle launcher forwarding
        ProxyPass /paraview http://localhost:8081/paraview

        # Handle WebSocket forwarding
        RewriteEngine On
        RewriteMap  session-to-port  txt:/pvw-test/proxy.txt
        RewriteCond %{QUERY_STRING}  ^sessionId=(.*)$               [NC]
        RewriteRule ^/proxy.*$       ws://${session-to-port:%1}/ws  [P]
    </VirtualHost>

Then, depending on your system, you need to configure Apache to use this file.  In a typical modern Apache setup, you might put this file in `/etc/apache2/sites-available/`, giving it a numbered name to come after the other virtual hosts files that already exist, then create a symlink to it from `/etc/apache2/sites-enabled/`.  Then just restart Apache, and you should be up and running.  If you now put an index.html in the `/pvw-test/www/` directory now, you should be able to point your browser at `<ServerName>` and see your virtual host working.

Mapping file setup
------------------

Next add the mapping file (needed for communication between Apache and the launcher) and create a group for the system users that need to access it.  The mapping file needs to be readable by both the user under whom Apache is running as well as the user under whom the python launcher is running.  In the following example, the `kitware` user runs the launcher, while the `www-data` user runs the Apache web server.

    touch /pvw-test/proxy.txt
    groupadd mappingfileusers
    usermod -a -G mappingfileusers kitware
    usermod -a -G mappingfileusers www-data
    chgrp mappingfileusers /pvw-test/proxy.txt
    chmod 660 /pvw-test/proxy.txt

ParaViewWeb launcher setup
--------------------------

Now you're ready to set up a launcher config file which can launch the basic paraviewweb applications, and after that you can create a shell script which can run the launcher with this configuration.  Here is a sample launcher configuration, which on a recent target system, we placed in `/pvw-test/conf/launch.json`:

    {
        "configuration": {
            "log_dir": "/pvw-test/logs",
            "host": "localhost",
            "endpoint": "paraview",
            "sessionURL": "ws://<my-domain-dot-com>/proxy?sessionId=${id}",
            "timeout": 25,
            "fields": ["file", "host", "port"],
            "port": 8081,
            "proxy_file": "/pvw-test/proxy.txt"
        },
        "resources": [{"port_range": [9104, 9204], "host": "localhost"}],
        "properties": {
            "python_path": "/pvw-test/autodeploy/paraview/lib/paraview-4.1/site-packages/",
            "data": "/pvw-test/autodeploy/data",
            "python_exec": "/pvw-test/autodeploy/paraview/bin/pvpython"
        },
        "apps": {
            "pipeline": {
                "cmd": ["${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_visualizer.py", "--port", "${port}", "--data-dir", "${data}", "-f", "--authKey", "${secret}"],
                "ready_line" : "Starting factory"
             },
             "visualizer": {
                "cmd": ["${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_visualizer.py", "--port", "${port}", "--data-dir", "${data}", "-f", "--authKey", "${secret}"],
                "ready_line" : "Starting factory"
             },
             "loader": {
                "cmd": ["${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_file_loader.py", "--port", "${port}", "--data-dir", "${data}", "-f", "--authKey", "${secret}"],
                "ready_line" : "Starting factory"
             }
             "data_prober": {
                "cmd": ["${python_exec}", "-dr", "${python_path}/paraview/web/pv_web_data_prober.py", "--port", "${port}", "--data-dir", "${data}", "-f", "--authKey", "${secret}"],
                "ready_line" : "Starting factory"
             }
         }
    }

And here are the contents of a sample start script which refers to the above configuration, which we placed in `/pvw-test/bin/start.sh`:

    #!/bin/bash

    export DISPLAY=:0.0
    /pvw-test/autodeploy/paraview/bin/pvpython /pvw-test/autodeploy/paraview/lib/paraview-4.1/site-packages/vtk/web/launcher.py /pvw-test/conf/launch.json &

Scripts for automated deployment of ParaViewWeb
-----------------------------------------------

Now you will need the python script which can download and install different versions of paraview binaries.  It is linked from the [ParaViewWeb Quick Start Guide](http://www.paraview.org/ParaView3/Doc/Nightly/www/js-doc/#!/guide/quick_start), or can be downloaded directly from [here](http://www.paraview.org/gitweb?p=ParaViewSuperbuild.git;a=blob_plain;f=Scripts/pvw-setup.py;hb=HEAD).  This script can be run either interactively or with command line arguments, and takes care of downloading everything you need to run ParaViewWeb (the correct binary version of ParaView for your system, the documentation, as well as the ParaView data).  Additionally, you will need a bash script or something which can perform the following tasks as a cron job:

1. stop the currently running python launcher
1. remove the previous installed ParaViewWeb folders
1. run the above-mentioned python installation script with the proper arguments, perhaps emailing yourself in case of a failure
1. restart the (newly installed) python launcher

Below is a complete example of such a script which is currently running on an auto-deployed ParaViewWeb instance.  This script actually downloads the python script mentioned above directly from the repo HTTP links, so that it will always get the most recent version.  Remember to replace the values within angle brackets <> to reasonable values for your own system.

    #!/bin/bash

    # Provide a usage message, in case we forget to supply the command
    # line argument we need.
    usage(){
        echo "Usage: $0 clobber"
        echo "   clobber: Should be either 'clobber' or 'noclobber', depending on whether you want to first remove the contents of the install directory or not"
    }

    # Check if we got the arguments we need
    if [[ $# -ne 1 ]]
    then
        usage
    fi

    # Grab 'clobber' or 'noclobber' off the command line
    clobber=$1

    rootDir="/pvw-test"

    # Download the most recent version of the paraviewweb setup script from
    # the repository (raw) link
    curl -o "${rootDir}/pvw-setup.py" "http://www.paraview.org/gitweb?p=ParaViewSuperbuild.git;a=blob_plain;f=Scripts/pvw-setup.py;hb=HEAD"

    # Set some variables: where is the install script, where to install, where
    # to put the hash file, etc...
    scriptFile="${rootDir}/pvw-setup.py"
    installPath="${rootDir}/autodeploy"

    # This next path identifies a location where the install script will
    # write the hash it generates of the downloaded ParaView binary.  This
    # hash file can then be retrieved by automated tests to verify that
    # a certain version of the ParaView software was used in the test.
    hashFilePath="${rootDir}/www/hash.json"
    launcherStartScript="${rootDir}/bin/start.sh"

    # First empty the install directory to make sure we start clean,
    # if that is what we were asked to do
    if [[ $clobber = "clobber" ]]
    then
        rm -rf ${installPath}/*
    fi

    # Gather up output in case we need to log it and email
    output=""

    # Find out if the launcher is currently running, you will have to change this
    # line to make sure it finds the desired launcher process
    pid=`ps -ef | grep pvpython | grep "launcher\\.py" | grep pvw-test | awk '{print $2}'`

    # If it is running, kill it before installing latest version
    if [[ -z ${pid} ]]
    then
        echo "There was no launcher process running"
    else
        echo "Process id of launcher is ${pid}, killing it..."
        out=`kill ${pid}`
        killResult=$?
        output="$output\n$out"
        echo "Output from killing launcher: ${output}"
        echo "Result of killing launcher: ${killResult}"
    fi

    # Capture the hostname and the time/date
    host=`hostname`
    date=`date`

    # Make a copy of the current hash file so we can compare afterwards
    cp ${hashFilePath} "${hashFileDir}/hash.json.previous"

    # Now run the python script which downloads and installs paraview.  You can run
    # "python pvw-setup.py --help" to find out more about these options.  Any exception
    # in this python script will result in a non-zero exit status, which will in turn
    # trigger an email to be sent to someone who cares.
    out=`python ${scriptFile} -p ${installPath} -m arguments -v nightly -t linux64 -s ${hashFilePath}`
    installResult=$?
    output="$output\n$out"
    if [[ $installResult == 1 ]]
    then
        echo "There was an error, emailing administrator"
        msg="There was an error autodeploying paraviewweb on ${host} at ${date}.  Please check logs."
        echo "${msg}" | mailx -v -s "ParaViewWeb Autodeploy Error" -r "caring.user@example.com" "caring.user@example.com"
        echo "Install script output"
        echo "${output}"
        exit 1
    else
        echo "Installation finished without errors"
    fi

    # Compare hashes (before and after).  Send mail if no change.
    hashDiff=`diff "${hashFileDir}/hash.json.previous" $hashFilePath`
    if [[ $hashDiff != "" ]]
    then
        echo "Hashes were different: ${hashDiff}"
    else
        echo "No change in hash, sending mail"
        mailMsg="Automatic update ran, but there was no apparent change in paraview version on ${host} at ${date}."
        echo "${mailMsg}" | mailx -v -s "ParaViewWeb Autodeploy Warning" -r "scott.wittenburg@kitware.com" "scott.wittenburg@kitware.com"
    fi

    # Now just run the launcher start script again
    echo "Starting launcher..."
    $launcherStartScript
    echo "Launcher is running in background"

Place the above script in the root of your autodeploy directory structure, namely, in `/pvw-test/run-pvw-setup.sh`.  Once you have tested out your script to make sure it runs, you can create a cron job for it, so it gets done regularly.  First run the crontab editor:

    crontab -e

Now enter a line like the following at the bottom:

    0 4 * * 1 /pvw-test/run-pvw-setup.sh clobber >> <path-to-logs-dir>/autoDeploy.log 2>&1

This line tells the system to run your script, with the argument "clobber", at the 0th minute of the hour, the 4th hour of the day, every Monday (0 is Sunday, 6 is Saturday).  All the script output (stderr and stdout) is redirected (in append mode) to the log file `<path-to-logs-dir>/autoDeploy.log`.

Optionally, you can also do a little bit of work to manage the log file that this will generate.  The output of the python installation script is fairly verbose, and in the case of errors, the log file we pointed to above can start to get large over time.  To manage this in a reasonable way, you can use the logrotate program which comes with most linux distributions.  The idea will be to create a custom logrotate configuration file, then just run logrotate via another cron job, perhaps just before cron is about to run the ParaViewWeb update script.  Here's what the configuration file could look like, and let's say you put it in `<path-to-conf-directory>/customRotate.conf`:

    <path-to-logs-dir>/autoDeploy.log {       # This is the logfile we are targeting for rotation
        size 5M                               # Rather than periodically, rotate only when size exceeds 5MB
        rotate 10                             # Maintain up to 10 compressed logs before deleting oldest
        compress                              # Please do compress rotated logs, default is gzip
        create 0660 kitware kitware           # Afterwards, create empty log file with given owner, group, and perms
    }

Now we just set up a second cron job to run `logrotate` on this configuration file, and it will take care of everything for us.  Add a line like the following one to your crontab list:

    55 3 * * 1 logrotate -s <path-to-any-directory-you-can-write>/customRotate.state <path-to-conf-directory>/customRotate.conf

This command tells logrotate to run, writing to it's state file in a custom location where you have write permissions, and to use the configuration file we just described above.  The command will run each week 5 minutes before the ParaViewWeb update script runs.

Setting up a mail program
-------------------------

Before the automatic email can be generated by the script example given above, you must set up your machine to be able to send mail from the command line using `mailx`.  On a recent system running Ubuntu 14.04, this program was not installed, but very little work was involved to get it working.  The steps were as follows:

    sudo apt-get install postfix

During the above installation process, the system displayed a UI with which I had to interact to answer 2 questions:

1. Configuration type (multiple choice): I chose "Internet site"
2. Fully qualified host name: I chose the DNS name of the machine

The next step is to install the `mailx` program, which was declared to be a virtual package provided by several other packages.  I ran:

    sudo apt-get install heirloom-mailx

After this, you should be able to test the actual line above which sends mail:

    echo "this is a test email" | mailx -v -s "hello" -r "caring.user@example.com" "caring.user@example.com"

This line should send email to `caring.user@example.com`, and the email will appear to come from this same email address.
