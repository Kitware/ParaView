#!/usr/bin/env bash

# Make sure we are inside the repository.
cd "${BASH_SOURCE%/*}/.." || exit

Utilities/Git/GitInfo # Help CMake find Git

echo "Configuring push urls..."
git config remote.origin.pushurl git@paraview.org:ParaView.git

echo "Initializing and updating git submodules..."
git submodule update --init --recursive
# Rebase master by default
git config rebase.stat true
git config branch.master.rebase true

Utilities/GitSetup/setup-user && echo &&
Utilities/GitSetup/setup-hooks && echo &&
Utilities/GitSetup/setup-stage && echo &&
(Utilities/GitSetup/setup-ssh ||
 echo 'Failed to setup SSH.  Run this again to retry.') && echo &&
(Utilities/GitSetup/setup-gerrit ||
 echo 'Failed to setup Gerrit.  Run this again to retry.') && echo &&
Utilities/Scripts/SetupGitAliases.sh && echo &&
Utilities/Scripts/SetupExternalData.sh && echo &&
Utilities/Scripts/SetupPVVTK.sh && echo &&
Utilities/GitSetup/tips


# Record the version of this setup so Scripts/pre-commit can check it.
SetupForDevelopment_VERSION=1
git config hooks.SetupForDevelopment ${SetupForDevelopment_VERSION}
