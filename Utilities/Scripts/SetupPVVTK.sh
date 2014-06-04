#!/usr/bin/env bash

# Run this script to set up the topic stage for pushing changes to PVVTK
die() {
  echo 'Failure during pvtk setup.' 1>&2
  echo '---------------------------------' 1>&2
  echo '' 1>&2
  echo "$@" 1>&2
  exit 1
}

project="PVVTK"
projectUrl="paraview.org"

# save current directory
pushd .

# enter VTK sub-directory
cd VTK

# configure PVVTK remote.
if git config remote.pvvtk.url >/dev/null; then
  echo "PVTK remote was already configured."
else
  echo "Configuring the remote for PVVTK..."
  git remote add pvvtk git://${projectUrl}/${project}.git || \
    die "Could not add the remote for PVVTK."
  git config remote.pvvtk.pushurl git@${projectUrl}:${project}.git
fi

echo "Setting up git aliases for PVVTK..."
../Utilities/Scripts/SetupPVVTKGitAliases.sh || exit 1

# restore current directory
popd
