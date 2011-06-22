#!/usr/bin/env bash

# Centralize project variables for each script
project="PVVTK"
projectUrl="paraview.org"

GIT=git
GITCONFIG="${GIT} config"

# General aliases that could be global
${GITCONFIG} alias.prepush 'log --graph --stat origin/master..'

# Staging aliases
stage_cmd="ssh git@${projectUrl} stage ${project}"
git_branch="\$(git symbolic-ref HEAD | sed -e 's|^refs/heads/||')"
${GITCONFIG} alias.pvvtk-cmd "!${stage_cmd}"
${GITCONFIG} alias.pvvtk-push "!sh -c \"git fetch pvvtk --prune && git push pvvtk HEAD\""
${GITCONFIG} alias.pvvtk-branch "!sh -c \"${stage_cmd} print\""
${GITCONFIG} alias.pvvtk-merge-next "!sh -c \"${stage_cmd} merge -b next ${git_branch}\""
${GITCONFIG} alias.pvvtk-merge-master "!sh -c \"${stage_cmd} merge -b master ${git_branch}\""
${GITCONFIG} alias.pvvtk-merge "!sh -c \"${stage_cmd} merge ${git_branch}\""
