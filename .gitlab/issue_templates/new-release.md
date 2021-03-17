<!--
Use this template when making a second or higher release candidate or final version.
If creating a first release candidate , use the `new-release-first-rc.md` template instead.

This template is for tracking a release of ParaView. Please replace the
following strings with the associated values:

  - `@VERSION@` - replace with base version, e.g., 5.7.0
  - `@RC@` - for release candidates, replace with "-RC?". For final, replace with "".
  - `@MAJOR@` - replace with major version number
  - `@MINOR@` - replace with minor version number

Please remove this comment.
-->

# Preparatory steps

  - Update ParaView guides
    - User manual
      - [ ] Rename to ParaViewGuide-@VERSION@.pdf
      - [ ] Upload to www.paraview.org/files/v@MAJOR@.@MINOR@
    - Catalyst Guide
      - [ ] Rename to ParaViewCatalystGuide-@VERSION@.pdf
      - [ ] Upload to www.paraview.org/files/v@MAJOR@.@MINOR@
    - Getting Started Guide
      - [ ] Rename to ParaViewGettingStarted-@VERSION@.pdf
      - [ ] Upload to www.paraview.org/files/v@MAJOR@.@MINOR@
    - Assemble release notes into `Documentation/release/ParaView-@VERSION@`.
      - [ ] Get positive review and merge.

# Update ParaView

  - [ ] Update `release` branch for **paraview**
```
git fetch origin
git checkout release
git merge --ff-only origin/release
git submodule update --recursive --init
```
  - [ ] Update `version.txt` and tag the commit
```
git checkout -b update-to-v@VERSION@@RC@
echo @VERSION@@RC@ > version.txt
git commit -m 'Update version number to @VERSION@@RC@' version.txt
git tag -a -m 'ParaView @VERSION@@RC@' v@VERSION@@RC@ HEAD
```
  - Integrate changes to `master` branch
    - [ ] Create a merge request targeting `master` (do *not* add `Backport: release`)
    - [ ] `Do: merge`
  - Integrate changes to `release` branch (push the `update-to-v@version@@RC@` branch to be the new `release` branch)
    - [ ] `git push origin update-to-v@VERSION@@RC@:release`
  - Create tarballs
    - [ ] ParaView (`Utilities/Maintenance/create_tarballs.bash --txz --tgz --zip -v v@VERSION@@RC@`)
  - Upload tarballs to `paraview.org`
    - [ ] `rsync -rptv $tarballs user@host:ParaView_Release/v@MAJOR@.@MINOR@/`

# Update ParaView-Superbuild

  - [ ] Update `release` branch for **paraview/paraview-superbuild**
```
git fetch origin
git checkout release
git merge --ff-only origin/release
git submodule update --recursive --init
git checkout -b update-to-v@VERSION@@RC@
```
  - Update `CMakeLists.txt`
    - [ ] Update PARAVIEW_VERSION_DEFAULT to the release version (without RC*)
    - [ ] Set ParaView source selections in `CMakeLists.txt` and force explicit
      version in `CMakeLists.txt`:
```
# Force source selection setting here.
set(paraview_SOURCE_SELECTION "@VERSION@@RC@" CACHE STRING "Force version to @VERSION@@RC@" FORCE)
set(paraview_FROM_SOURCE_DIR OFF CACHE BOOL "Force source dir off" FORCE)
```
  - Update versions
    - [ ] Guide selections in `versions.cmake`
    - [ ] `paraview_SOURCE_SELECTION` version in `README.md`
    - [ ] Docker: update default tag strings (in `Scripts/docker/ubuntu/development/Dockerfile`)
      - [ ] ARG PARAVIEW_TAG=v@VERSION@@RC@
      - [ ] ARG SUPERBUILD_TAG=v@VERSION@@RC@
      - [ ] ARG PARAVIEW_VERSION_STRING=paraview-@MAJOR@.@MINOR@
    - [ ] Commit changes and push to GitLab
```
git add versions.cmake CMakeLists.txt Scripts/docker/ubuntu/development/Dockerfile
git commit -m "Update the default version to @VERSION@@RC@"
git gitlab-push
```
  - Integrate changes to `master` branch
    - [ ] Create a merge request targeting `master`, title beginning with WIP (do *not* add `Backport: release` to description)
    - [ ] Build binaries (start all pipelines)
    - [ ] Download the binaries that have been generated from the Pipeline build products. They will be deleted within 24 hours.
    - [ ] Remove explicit version forcing added in `CMakeLists.txt` and force push
```
git add CMakeLists.txt
git commit --amend --no-edit
git gitlab-push -f
```
  - Finalize merge request
    - [ ] Remove WIP from merge request title
    - [ ] Get positive review
    - [ ] `Do: merge`
    - [ ] `git tag -a -m 'ParaView superbuild @VERSION@@RC@' v@VERSION@@RC@ HEAD`
  - Integrate changes to `release` branch
    - [ ] `git push origin update-to-v@VERSION@@RC@:release`

# Sign macOS binaries

  - [ ] Upload to signing server, run script, download resulting .pkg and .dmg files
  - [ ] Install from .pkg and verify that it is signed with `codesign -dvvv /Applications/ParaView-@VERSION@@RC@.app/`
  - [ ] Install from .dmg and verify that it is signed with `codesign -dvvv /Applications/ParaView-@VERSION@@RC@.app/`

# Validating binaries

For each binary, open the Python shell and run the following:

```python
import numpy
s = Show(Sphere())
ColorBy(s, ('POINTS', 'Normals', 'X'))
Show(Text(Text="$A^2$"))
```

  Check that
  - Getting started guide opens
  - Examples load and match thumbnails in dialog
  - Python. Open the Python shell and run
  - Plugins are present and load properly
  - OSPRay raycasting and pathtracing runs
  - OptiX pathtracing runs
  - IndeX runs
  - AutoMPI


Binary checklist
  - [ ] macOS
  - [ ] Linux
  - [ ] Linux osmesa
  - [ ] Windows MPI (.exe)
  - [ ] Windows MPI (.zip)
  - [ ] Windows no-MPI (.exe)
  - [ ] Windows no-MPI (.zip)

# Upload binaries

  - [ ] Upload binaries to `paraview.org` (`rsync -rptv $binaries paraview.release:ParaView_Release/v@MAJOR@.@MINOR@/`)
  - [ ] Ask @utkarsh.ayachit to regenerate `https://www.paraview.org/files/listing.txt` and `md5sum.txt` on the website

```
buildListing.sh
updateMD5sum.sh v@MAJOR@.@MINOR@
```

  - [ ] Test download links on https://www.paraview.org/download

# Push tags

 - [ ] In the `paraview` repository, run `git push origin v@VERSION@@RC@`.
 - [ ] In the `paraview-superbuild` repository, run `git push origin v@VERSION@@RC@`.

 # Spack

 - [ ] Update Spack package: https://github.com/spack/spack/blob/develop/var/spack/repos/builtin/packages/paraview/package.py

<!--
If making a non-RC release:

# Update documentation

  - [ ] Upload versioned documentation to `https://github.com/kitware/paraview-docs` (see `https://github.com/Kitware/paraview-docs/blob/master/README.md`)
  - [ ] Tag the [ParaView docs](https://gitlab.kitware.com/paraview/paraview-docs/-/tags) with v@VERSION@.
  - [ ] Activate the tag on [readthedocs](https://readthedocs.org/projects/paraview/versions/) and build it [here](https://readthedocs.org/projects/paraview/)
  - [ ] Go to readthedocs.org and activate
  - [ ] Write and publish blog post with release notes.
  - [ ] Update release notes
    (https://www.paraview.org/Wiki/ParaView_Release_Notes)
-->

# Post-release

  - [ ] Post an announcement in the Announcements category on
        [discourse.paraview.org](https://discourse.paraview.org/).
  - [ ] Request DoD vulnerability scan
<!--
If making a non-RC release:

  - [ ] Request update of version number in "Download Latest Release" text on www.paraview.org
  - [ ] Request update of link to ParaView Guide PDF at https://www.paraview.org/paraview-guide/
  - [ ] Move unclosed issues to next release milestone in GitLab
-->

/cc @ben.boeckel
/cc @cory.quammen
/cc @utkarsh.ayachit
/cc @charles.guenuet
/label ~"priority:required"
