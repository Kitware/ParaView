Download ParaView with Git
==========================

This page documents how to download ParaView source code through [Git][].
See the [README](README.md) for more information.

[Git]: http://git-scm.com

Clone
-----

Optionally configure Git to [use SSH instead of HTTPS](#use-ssh-instead-of-https).

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

Use SSH instead of HTTPS
------------------------

Git can be configured to access ``gitlab.kitware.com`` repositories through
the ``ssh`` protocol instead of ``https`` without having to manually change
every URL found in instructions, scripts, and submodule configurations.

1.  Register [GitLab Access][] to create an account and select a user name.

2.  Add [SSH Keys][] to your GitLab account to authenticate your user via
    the ``ssh`` protocol.

3.  Configure Git to use ``ssh`` instead of ``https`` for all repositories
    on ``gitlab.kitware.com``:

        $ git config --global url."git@gitlab.kitware.com:".insteadOf https://gitlab.kitware.com/
    The ``--global`` option causes this configuration to be stored in
    ``~/.gitconfig`` instead of in any repository, so it will map URLs
    for all repositories.

4.  Return to the [Clone](#clone) step above and use the instructions as
    written.  There is no need to manually specify the ssh protocol when
    cloning.  The Git ``insteadOf`` configuration will map it for you.

[GitLab Access]: https://gitlab.kitware.com/users/sign_in
[SSH Keys]: https://gitlab.kitware.com/profile/keys
