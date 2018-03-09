<!--
This template is for tracking a release of ParaView. Please replace the
following strings with the associated values:

  - `VERSION`
  - `MAJOR`
  - `MINOR`

Please remove this comment.
-->

# Prepatory steps

  - [ ] Announce to `paraview-developers@paraview.org`
  - Update ParaView guides
    - [ ] User manual
      - [ ] Uploaded
    - [ ] Catalyst guide
      - [ ] Uploaded
    - [ ] Getting Started
      - [ ] Uploaded

<!--
Keep the relevant items for the kind of release this is.

If the `release` branch is being updated (`vMAJOR.MINOR.0-RC1`):

  - Create release branch
    - [ ] Pull latest master
    - [ ] Update `version.txt`
    - [ ] Create a merge request targeting `master`
    - [ ] Build binaries (`Do: test --superbuild`)
    - [ ] `Do: merge`
    - [ ] `git push origin update-to-VERSION:release vVERSION`
    - [ ] Update kwrobot with the new `release` branch rules
  - Create `release` branch for paraview/paraview-superbuild
    - [ ] Pull latest master
    - Update `versions.cmake` and `CMakeLists.txt`
      - [ ] ParaView source selections
      - [ ] Guide selections
      - [ ] Assumed version in `CMakeLists.txt`
    - [ ] Create a merge request targeting `master`
    - [ ] Build binaries (`Do: test`)
    - [ ] `Do: merge`
    - [ ] `git push origin update-to-VERSION:release vVERSION`
    - [ ] Update kwrobot with the new `release` branch rules


If making a release from the `release` branch:

  - Update release branch
    - [ ] Update `version.txt`
    - [ ] Create a merge request targeting `master`
    - [ ] Build binaries (`Do: test --superbuild`)
    - [ ] `Do: merge`
    - [ ] `git push origin update-to-VERSION:release vVERSION`
  - Update `release` branch for paraview/paraview-superbuild
    - Update `versions.cmake` and `CMakeLists.txt`
      - [ ] ParaView source selections
      - [ ] Guide selections
      - [ ] Assumed version in `CMakeLists.txt`
    - [ ] Create a merge request targeting `master`
    - [ ] Build binaries (`Do: test`)
    - [ ] `Do: merge`
    - [ ] `git push origin update-to-VERSION:release vVERSION`
-->

  - Create tarballs
    - [ ] ParaView (`Utilities/Maintenance/create_tarballs.bash --tgz --zip -v vVERSION`)
    - [ ] Catalyst (`Catalyst/generate-tarballs.sh vVERSION`)
  - Upload tarballs to `paraview.org`
    - [ ] `rsync -rptv $tarballs paraview.release:ParaView_Release/vMAJOR.MINOR/`
  - [ ] Update the superbuild

# Validating binaries

  - [ ] Python
  - [ ] `import compiler`
  - [ ] `import numpy`
  - [ ] Plugins are present and load properly
  - [ ] Text source LaTeX `$A^2$`
  - [ ] OSPRay
  - [ ] AutoMPI

# Upload binaries

  - [ ] Sign macOS binary
  - Upload binaries to `paraview.org`
    - [ ] `rsync -rptv $binaries paraview.release:ParaView_Release/vMAJOR.MINOR/`
  - [ ] Update `md5sum.txt`
  - [ ] Update JavaScript for `paraview.org/downloads`
    - [ ] `rsync -rptv paraview-downloads.js paraview.release:ParaView_Release/`
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
