<!--
This template is for tracking a release of ParaView. Please replace the
following strings with the associated values:

  - `VERSION`
  - `MAJOR`
  - `MINOR`

Please remove this comment.
-->

# Preparatory steps

  - Update ParaView guides
    - [ ] User manual
      - [ ] Uploaded
    - [ ] Catalyst guide
      - [ ] Uploaded
    - [ ] Getting Started
      - [ ] Uploaded

<!--
Keep the relevant items for the kind of release this is.

If making a first release candidate from master, i.e., `vMAJOR.MINOR.0-RC1`:

  - Update `master` branch for **paraview**
    - [ ] `git fetch origin`
    - [ ] `git checkout master`
    - [ ] `git merge --ff-only origin/master`
  - Update `version.txt` and tag the commit
    - [ ] `git checkout -b update-to-VERSION`
    - [ ] `echo VERSION > version.txt`
    - [ ] `git commit -m 'Update version number to VERSION'`
    - [ ] `git -m 'Update version number to VERSION'`
    - [ ] `git tag -a -m 'ParaView VERSION' vVERSION HEAD`
  - Integrate changes to `master` branch
    - [ ] Create a merge request targeting `master` (do *not* add `Backport: release`)
    - [ ] Get positive review
    - [ ] `Do: merge`
  - Integrate changes to `release` branch
    - [ ] `git push origin update-to-VERSION:release vVERSION`
    - [ ] Update kwrobot with the new `release` branch rules

If making a release from the `release` branch, e.g., `vMAJOR.MINOR.0-RC2 or above`:

  - Update `release` branch for **paraview**
    - [ ] `git fetch origin`
    - [ ] `git checkout release`
    - [ ] `git merge --ff-only origin/release`
  - Update `version.txt` and tag the commit
    - [ ] `git checkout -b update-to-VERSION`
    - [ ] `echo VERSION > version.txt`
    - [ ] `git commit -m 'Update version number to VERSION'`
    - [ ] `git -m 'Update version number to VERSION'`
    - [ ] `git tag -a -m 'ParaView VERSION' vVERSION HEAD`
  - Integrate changes to `master` branch
    - [ ] Create a merge request targeting `master` (do *not* add `Backport: release`)
    - [ ] Build binaries (`Do: test --superbuild`)
    - [ ] Get positive review
    - [ ] `Do: merge`
  - Integrate changes to `release` branch
    - [ ] `git push origin update-to-VERSION:release vVERSION`
-->

  - Create tarballs
    - [ ] ParaView (`Utilities/Maintenance/create_tarballs.bash --tgz --zip -v vVERSION`)
    - [ ] Catalyst (`Catalyst/generate-tarballs.sh vVERSION`)
  - Upload tarballs to `paraview.org`
    - [ ] `rsync -rptv $tarballs paraview.release:ParaView_Release/vMAJOR.MINOR/`
  - [ ] Update the superbuild

<!--
Keep the relevant items for the kind of release this is.

If making a first release candidate from master, i.e., `vMAJOR.MINOR.0-RC1`:

  - Update `master` branch for **paraview/paraview-superbuild**
    - [ ] `git fetch origin`
    - [ ] `git checkout master`
    - [ ] `git merge --ff-only origin/master`
  - Update `versions.cmake` and `CMakeLists.txt`
    - [ ] ParaView source selections
    - [ ] Guide selections
    - [ ] Assumed version in `CMakeLists.txt`
  - Integrate changes to `master` branch
    - [ ] Create a merge request targeting `master`
    - [ ] Build binaries (`Do: test`)
    - [ ] Download the binaries that have been generated in the dashboard results. They will be deleted within 24 hours.
    - [ ] Get positive review
    - [ ] `Do: merge`
  - Integrate changes to `release` branch
    - [ ] `git push origin update-to-VERSION:release vVERSION`
    - [ ] Update kwrobot with the new `release` branch rules

If making a release from the `release` branch, e.g., `vMAJOR.MINOR.0-RC2 or above`:

  - Update `release` branch for **paraview/paraview-superbuild**
    - [ ] `git fetch origin`
    - [ ] `git checkout release`
    - [ ] `git merge --ff-only origin/release`
  - Update `versions.cmake` and `CMakeLists.txt`
    - [ ] ParaView source selections
    - [ ] Guide selections
    - [ ] Assumed version in `CMakeLists.txt`
  - Integrate changes to `master` branch
    - [ ] Create a merge request targeting `master`
    - [ ] Build binaries (`Do: test`)
    - [ ] Download the binaries that have been generated in the dashboard results. They will be deleted within 24 hours.
    - [ ] Get positive review
    - [ ] `Do: merge`
  - Integrate changes to `release` branch
    - [ ] `git push origin update-to-VERSION:release vVERSION`
-->

# Validating binaries

  - For each binary, check
    - [ ] Python
    - [ ] `import compiler`
    - [ ] `import numpy`
    - [ ] Plugins are present and load properly
    - [ ] Text source LaTeX `$A^2$`
    - [ ] OSPRay
    - [ ] AutoMPI

  - Binary checklist
    - [ ] macOS
    - [ ] Linux
    - [ ] Windows MPI (.exe)
    - [ ] Windows MPI (.zip)
    - [ ] Windows no-MPI (.exe)
    - [ ] Windows no-MPI (.zip)

# Upload binaries

  - [ ] Ask @chuck.atkins to sign macOS binary
  - Upload binaries to `paraview.org`
    - [ ] `rsync -rptv $binaries paraview.release:ParaView_Release/vMAJOR.MINOR/`
  - [ ] Update `md5sum.txt`
  - [ ] Ask @utkarsh.ayachit to regenerate `https://www.paraview.org/files/listing.txt`
  - [ ] Test download links

# Post-release

  - [ ] Notify `paraview@paraview.org` and `paraview-developers@paraview.org`
        that the release is available.

<!--
These items only apply to non-RC releases.

  - [ ] Update release notes
    (https://www.paraview.org/Wiki/ParaView_Release_Notes)
-->

<!--
# Code snippets:

## Updating `version.txt`:

```sh
git checkout -b update-to-VERSION
echo VERSION > version.txt
git commit -m 'Update version number to VERSION'
git tag -a -m 'ParaView VERSION' vVERSION HEAD
```
-->

/cc @ben.boeckel
/cc @cory-quammen
/cc @utkarsh.ayachit
/label ~"priority:required"
