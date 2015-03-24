Download ParaView with Git
==========================

This page documents how to download ParaView source code through [Git][].
See the [README](README.md) for more information.

[Git]: http://git-scm.com

Clone
-----

Clone ParaView using the commands:

    $ git clone --recursive https://gitlab.kitware.com/paraview/paraview.git ParaView
    $ cd ParaView

Update
------

Users that have made no local changes and simply want to update a
clone with the latest changes may run:

    $ git pull
    $ git submodule update --init

Avoid making local changes unless you have read our [developer instructions][].

[developer instructions]: develop.md

Release
-------

After cloning your local repository will be configured to follow the upstream
`master` branch by default.  One may create a local branch to track the
upstream `release` branch instead, which should guarantee only bug fixes to
the functionality available in the latest release:

    $ git checkout --track -b release origin/release

This local branch will always follow the latest release.
Use the [above instructions](#update) to update it.
Alternatively one may checkout a specific release tag:

    $ git checkout v4.3.1

Release tags never move.  Repeat the command with a different tag to get a
different release.  One may list available tags:

    $ git tag

and then checkout any tag listed.
