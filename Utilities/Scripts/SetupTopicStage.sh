#!/usr/bin/env bash

# Run this script to set up the topic stage for pushing changes.

die() {
        echo 'Failure during topic stage setup' 1>&2
        echo '--------------------------------' 1>&2
        echo '' 1>&2
        echo "$@" 1>&2
        exit 1
}

# Make sure we are inside the repository.
cd "$(echo "$0"|sed 's/[^/]*$//')"

if git config remote.stage.url >/dev/null; then
  echo "Topic stage remote was already configured."
else
  echo "Configuring the topic stage remote..."
  git remote add stage git://paraview.org/stage/ParaView.git || \
    die "Could not add the topic stage remote."
  git config remote.stage.pushurl git@paraview.org:stage/ParaView.git
fi

read -ep "Do you have git push access to paraview.org? [y/N]: " access
if test "$access" = "y"; then
  echo "Testing ssh capabilities..."
  git ls-remote git@paraview.org:stage/ParaView.git refs/heads/master || die "ssh test to git@paraview.org failed."
  echo "Test successful!"
fi

echo "Done."
