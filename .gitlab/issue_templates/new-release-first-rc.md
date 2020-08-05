<!--
Use this template when making a new first release candidate from master.
If creating a second or final release, use the `new-release.md` template instead.

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

  - [ ] Update `master` branch for **paraview**
```
git fetch origin
git checkout master
git merge --ff-only origin/master
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
  - Update VTK's `paraview/release` branch
    - [ ] Change directory to VTK source
    - [ ] `git push origin <paraview-vtk-submodule-hash>:paraview/release`
    - [ ] Update kwrobot with the new `paraview/release` branch rules
  - Integrate changes to `release` branch
    - [ ] Change directory to ParaView source. Stay on the `update-to-v@VERSION@@RC@` branch.
    - [ ] `git config -f .gitmodules submodule.VTK.branch paraview/release`
    - [ ] `git commit -m "release: follow VTK's paraview/release branch" .gitmodules`
    - [ ] Merge new `release` branch into `master` using `-s ours`
      - `git checkout master`
      - `git merge --no-ff -s ours -m "Merge branch 'release'" update-to-v@VERSION@@RC@`
    - [ ] `git push origin master update-to-v@VERSION@@RC@:release`
    - [ ] Update kwrobot with the new `release` branch rules
  - Create tarballs
    - [ ] ParaView (`Utilities/Maintenance/create_tarballs.bash --txz --tgz --zip -v v@VERSION@@RC@`)
  - Upload tarballs to `paraview.org`
    - [ ] `rsync -rptv $tarballs paraview.release:ParaView_Release/v@MAJOR@.@MINOR@/`

# Update ParaView-Superbuild

  - [ ] Update `release` branch for **paraview/paraview-superbuild**
```
git fetch origin
git checkout release
git merge --ff-only origin/release
git submodule update
git checkout -b update-to-v@VERSION@@RC@
```
  - Update `CMakeLists.txt`
    - [ ] Set ParaView source selections in `CMakeLists.txt` and force explicit
      version in `CMakeLists.txt`:
```
# Force source selection setting here.
set(paraview_SOURCE_SELECTION "@VERSION@@RC@" CACHE STRING "Force version to @VERSION@@RC@" FORCE)
set(paraview_FROM_SOURCE_DIR OFF CACHE BOOL "Force source dir off" FORCE)
```
  - Update versions
    - [ ] Guide selections in `versions.cmake`
    - [ ] Docker: update default tag strings (in `Scripts/docker/ubuntu/development/Dockerfile`)
      - [ ] ARG PARAVIEW_TAG=v@VERSION@@RC@
      - [ ] ARG SUPERBUILD_TAG=v@VERSION@@RC@
    - [ ] Commit changes and push to GitLab
```
git add versions.cmake CMakeLists.txt Scripts/docker/ubuntu/development/Dockerfile
git commit -m "Update the default version to @VERSION@@RC@"
git gitlab-push
```
  - Integrate changes to `master` branch
    - [ ] Create a merge request targeting `master`, title beginning with WIP (do *not* add `Backport: release` to description)
    - [ ] Build binaries (`Do: test`)
    - [ ] Download the binaries that have been generated in the dashboard results. They will be deleted within 24 hours.
    - [ ] Remove explicit version forcing added in CMakeLists.txt, amend the commit, and force push
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

# Post-release

  - [ ] Post an announcement in the Announcements category on
        [discourse.paraview.org](https://discourse.paraview.org/).

/cc @ben.boeckel
/cc @cory.quammen
/cc @utkarsh.ayachit
/label ~"priority:required"
