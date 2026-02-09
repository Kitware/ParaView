<!--
This template is for tracking a release of ParaView. Please replace the
following strings with the associated values:

  - `@VERSION@` - replace with base version, e.g., 5.7.0
  - `@RC@` - for release candidates, replace with "-RC?". For final, replace with "".
  - `@MAJOR@` - replace with major version number
  - `@MINOR@` - replace with minor version number
  - `@PATCH@` - replace with patch version number
  - `@BASEBRANCH@`: The branch to create the release on (for `x.y.0-RC1`,
    `master`, otherwise `release`)
  - `@BRANCHPOINT@`: The ParaView commit where the release should be started

Please remove this comment.
-->

# Preparatory steps

  - Getting Started Guide
      - [ ] Rename to ParaViewGettingStarted-@VERSION@.pdf
      - [ ] Upload to www.paraview.org/files/v@MAJOR@.@MINOR@
  - macOS signing machine
    - [ ] Check that the macOS signing machine is reachable. If not, request it to be switched on.

# Update ParaView

  - Update the local copy of `@BASEBRANCH@`.
    - If `@PATCH@@RC@` is `0-RC1`, update `master`
    - Otherwise, update `release`
      ```
      git fetch origin
      git checkout @BASEBRANCH@
      git merge --ff-only origin/@BASEBRANCH@ # if this fails, there are local commits that need to be removed
      git submodule update --recursive --init
      ```
    - If `@BASEBRANCH@` is not `master`, ensure merge requests which should be
      in the release have been merged. The [`backport-mrs.py`][backport-mrs]
      script can be used to find and ensure that merge requests assigned to the
      associated milestone are available on the `release` branch.

  - Integrate changes.
    - Make a commit for each of these `release` changes on a single topic
      (suggested branch name: `update-to-v@VERSION@`):
      - [ ] Move individual notes from `Documentation/release/dev` to
        `Documentation/release/@MAJOR@.@MINOR@/`, keeping sample-topic.md.
<!-- if @RC@ == "" -->
      - [ ] Remove `Documentation/release/@MAJOR@.@MINOR@/`
      - [ ] Assemble release notes into `Documentation/release/ParaView-@VERSION@.md`.
<!-- endif -->
<!-- if RC1 and patch == 0 -->
      - [ ] Update `version.txt` to bump the minor version number.
<!-- endif -->
    - Make a commit for each of these `release`-only changes
<!-- if RC1 and patch == 0 -->
      - [ ] `.gitmodules` to track the `release` branch of VTK
<!-- endif -->
      - [ ] Update `.gitlab/ci/cdash-groups.json` to track the `release` CDash
            groups
      - [ ] Update `version.txt` and tag the commit (tag this commit below)
        ```
        git checkout -b update-to-v@VERSION@@RC@ @BRANCHPOINT@
        echo @VERSION@@RC@ > version.txt
        git commit -m 'Update version number to @VERSION@@RC@' version.txt
        ```
    - Create a merge request targeting `release`
      - [ ] Obtain a GitLab API token for the `kwrobot.release.paraview` user
            (ask `@utils/maintainers/release` if you do not have one)
      - [ ] Add the `kwrobot.release.paraview` user to your fork with at least
            `Developer` privileges (so it can open MRs)
      - [ ] Use [the `release-mr`][release-mr] script to open the create the
            Merge Request (see script for usage)
        - [ ] Pull the script for each release; it may have been updated since it
          was last used
        - [ ] Add your development fork as a remote named `gitlab`
        - [ ] `release-mr.py -t TOKEN_STRING -c .kitware-release.json -m @BRANCHPOINT@`
        - [ ] The script outputs the information it will be using to create the
          merge request. Please verify that it is all correct before creating
          the merge request. See usage at the top of the script to provide
          information that is either missing or incorrect (e.g., if its data
          extraction heuristics fail).
    - [ ] Run the test with `Do: test` and get a green dashboard.
<!-- if not RC1 and patch == 0 -->
    - [ ] Remove fixup commit: `git reset --hard @^`
<!-- endif -->
    - [ ] Get positive review
    - [ ] `Do: merge`
    - [ ] Create tag: `git tag -a -m '@VERSION@@RC@' v@VERSION@@RC@ commit-that-updated-version.txt`
  - Create tarballs
    - [ ] ParaView (`Utilities/Maintenance/create_tarballs.bash --txz --tgz --zip -v v@VERSION@@RC@`)
  - Upload tarballs to `paraview.org`
    - [ ] Setup your `~/.ssh/config` and add the web host (@vbolea).
    - [ ] `rsync -rptv $tarballs web:ParaView_Release/v@MAJOR@.@MINOR@/`
  - Software process updates (these can all be done independently)
    - [ ] Update kwrobot with the new `release` branch rules (`@utils/maintainers/ghostflow`)
    - [ ] Run [this script][cdash-update-groups] to update the CDash groups
      - This must be done after a nightly run to ensure all builds are in the
        `release` group
      - See the script itself for usage documentation
    - [ ] Add (or update if `@BASEBRANCH@` is `release`) version selection entry
          in paraview-superbuild

[backport-mrs]: https://gitlab.kitware.com/utils/release-utils/-/blob/master/backport-mrs.py
[release-mr]: https://gitlab.kitware.com/utils/release-utils/-/blob/master/release-mr.py
[cdash-update-groups]: https://gitlab.kitware.com/utils/cdash-utils/-/blob/master/cdash-update-groups.py

# Update ParaView-Superbuild

  - [ ] Update @BASEBRANCH@ branch for **paraview-superbuild**
```
git fetch origin
git checkout @BASEBRANCH@
git merge --ff-only origin/@BASEBRANCH@
git submodule update --recursive --init
```
  - [ ] Create new branch `git checkout -b update-to-v@VERSION@@RC@ @BASEBRANCH@`

  - Integrate changes.
    - Update versions
<!-- if not RC -->
      - [ ] Update the version in the `Buliding a specific version` section
            example in `README.md`
<!-- endif -->
      - [ ] `paraview_SOURCE_SELECTION` version in `README.md`
      - [ ] `PARAVIEW_VERSION_DEFAULT` in  CMakeLists.txt
      - [ ] Commit changes
        - [ ] `git add README.md versions.cmake CMakeLists.txt`
        - [ ] `git commit -m "Update the default version to @VERSION@@RC@"`
    - Make a commit for each of these `release`-only changes
      - [ ] Update `.gitlab/ci/cdash-groups.json` to track the `release` CDash
            groups (if `@BASEBRANCH@` is `master`)
<!-- if RC1 and patch == 0 -->
      - [ ] Create a commit that changes the paraview _DEFAULT_ source to the git
            url source in the `versions.cmake` file.
<!-- endif -->
      - [ ] Create a commit which will be tagged:
        - [ ] `git commit --allow-empty -m "paraview: add release @VERSION@"`
        - [ ] Create tag: `git tag -a -m 'ParaView superbuild @VERSION@@RC@' v@VERSION@@RC@ HEAD`
      - [ ] Force `@VERSION@@RC@` in CMakeLists.txt
        - [ ] Append to the top of CMakeLists.txt (After project...) The following
            ```
            # Force source selection setting here.
            set(paraview_SOURCE_SELECTION "@VERSION@@RC@" CACHE STRING "Force version to @VERSION@@RC@" FORCE)
            set(paraview_FROM_SOURCE_DIR OFF CACHE BOOL "Force source dir off" FORCE)
            ```
      - [ ] Create fixup commit with the above changes `git commit -a --fixup=@`. The fixup commit will prevent merging of the temporary code above; it will be removed in a future step.
      - [ ] Create a merge request targeting `release`
    - [ ] Obtain a GitLab API token for the `kwrobot.release.paraview` user
          (ask @ben.boeckel if you do not have one)
    - [ ] Add the `kwrobot.release.paraview` user to your fork with at least
          `Developer` privileges (so it can open MRs)
    - [ ] Use [the `release-mr`][release-mr] script to open the create the
          Merge Request (see script for usage)
      - [ ] Pull the script for each release; it may have been updated since it
        was last used
      - [ ] `release-mr.py -t TOKEN_STRING -c .kitware-release.json -m @BASEBRANCH@`
<!-- if not RC and patch == 0-->
      - [ ] Make sure that the backporting directive in the merge-request
            description skips the last commit such as: `Backport: master:HEAD~`
<!-- endif -->
  - Build binaries
    - [ ] Build binaries (start all pipelines)
    - [ ] Download the binaries that have been generated from the Pipeline
          build products. They will be deleted within 24 hours.

  - [ ] Remove fixup commit: `git reset --hard @^`
  - [ ] Get positive review
  - [ ] Force push `git push -f gitlab`
  - [ ] `Do: merge`
  - Software process updates (these can all be done independently)
    - [ ] Update kwrobot with the new `release` branch rules (@ben.boeckel)
    - [ ] Run [this script][cdash-update-groups] to update the CDash groups
      - This must be done after a nightly run to ensure all builds are in the
        `release` group
      - See the script itself for usage documentation
    - [ ] Add (or update if `@BASEBRANCH@` is `release`) version selection entry
          in paraview-superbuild

# Sign Windows binaries
  -  [ ] Request Windows binary signings (only .exe archives) on the Package
         Signing repo. Example request [here][win-sign-example].

# Sign macOS binaries
  - [ ] Upload to signing server, run script, download resulting .pkg and .dmg files
  - [ ] Install on x86\_64 from .pkg and verify that it is signed with `codesign -dvvv /Applications/ParaView-@VERSION@@RC@.app/`
  - [ ] Install on arm64 from .pkg and verify that it is signed with `codesign -dvvv /Applications/ParaView-@VERSION@@RC@.app/`
  - [ ] Install on x86\_64 from .dmg and verify that it is signed with `codesign -dvvv /Applications/ParaView-@VERSION@@RC@.app/`
  - [ ] Install on arm64 from .dmg and verify that it is signed with `codesign -dvvv /Applications/ParaView-@VERSION@@RC@.app/`

# Validating binaries


## Linux

- Run in client-server configuration with 4 server ranks.

```
> mpirun -n 4 pvserver --mpi --hostname=localhost -p 11111 &
> paraview --server localhost:11111
```

- Start trace. Open disk_out_ref. Clip.Create Screenshot. Create Animation. Stop trace. Save macro. Reset Session. Delete screenshot and animation. Run macro. Check generate screenshots and animations are correct.
- Open View -> Memory Inspector.
- Change opacity to 0.3 and ensure rendering looks correct.

## All other binaries

Open ParaView's _Python Shell_ and run the following:

```python
import numpy
s = Show(Sphere())
ColorBy(s, ('POINTS', 'Normals', 'X'))
Show(Text(Text="$A^2$"))
```

Check that
  - Help -> Getting Started with ParaView menu opens PDF document
  - Help -> Reader, Filter, and Writer lists information about selected sources properly
  - Help -> try every other item in the menu. Note that the Release Notes link will bring you to a missing page until the release notes are published, which may not be until the very end of the release cycle. Check that the URL is the expected one, though.
  - Run remote server with 8 ranks. Connect the client to it and check that each visualization in Help -> Example Visualizations load and match thumbnails in dialog:

```
> mpirun -n 8 pvserver --mpi --hostname=localhost -p 11111 &
> paraview --server localhost:11111
```

  - Help -> About shows reasonable and accurate information
  - Check that plugins are present and load properly. Select Tools -> Manage Plugins menu item and load each plugin in the list.
  - OSPRay raycasting and pathtracing runs ("Enable Ray Tracing" property in View panel). With Samples Per Pixel set to 4, leave the Denoise option on.
  - OptiX pathtracing runs (not macOS)
    - ref. !22372 for current expected results
  -
  - IndeX runs (load pvNVIDIAIndeX plugin, add a Wavelet dataset, change representation to NVIDIA IndeX)
  - Open can.ex2 example. Split screen horizontally. Switch to Volume rendering in one view, ray tracing in the other. Save screenshot (.png). Save Animation (.avi).

Binary checklist
  - [ ] macOS arm64
  - [ ] macOS x86\_64
  - [ ] Linux
  - [ ] Linux osmesa
  - [ ] Windows MPI (.exe)
  - [ ] Windows MPI (.zip)
  - [ ] Windows no-MPI (.exe)
  - [ ] Windows no-MPI (.zip)

# Upload binaries

  - [ ] Upload binaries to `paraview.org` (`rsync -rptv $binaries paraview.release:ParaView_Release/v@MAJOR@.@MINOR@/`)
  - [ ] Ask @cory.quammen to regenerate `https://www.paraview.org/files/listing.txt` and `md5sum.txt` on the website from within the directory corresponding to www.paraview.org/files/

```
updateMD5sum.sh v@MAJOR@.@MINOR@
buildListing.sh
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
  - [ ] Submit a Merge Request for release that updates the version to @VERSION@ in https://gitlab.kitware.com/paraview/paraview-docs/-/blob/master/doc/source/conf.py` for `paraview-docs`
  - [ ] Upload versioned documentation to `https://github.com/kitware/paraview-docs` (see `https://github.com/Kitware/paraview-docs/blob/master/README.md`)
  - [ ] Tag the HEAD of release in [ParaView docs](https://gitlab.kitware.com/paraview/paraview-docs/-/tags) with v@VERSION@.
  - [ ] Activate the tag on [readthedocs](https://readthedocs.org/projects/paraview/versions/) and build it [here](https://readthedocs.org/projects/paraview/)
  - [ ] Go to readthedocs.org and activate
  - [ ] Head to [ParaView developer docs](github.com/Kitware/paraview-docs) and generate the new developer documentation, following the directions in the README.
  - [ ] Write and publish blog post with release notes.
-->

# Post-release

  - [ ] Post an announcement in the Announcements category on
        [discourse.paraview.org](https://discourse.paraview.org/).
  - [ ] Request an XRInterface plugin validation using [TESTING.md](https://gitlab.kitware.com/paraview/paraview/-/blob/master/Plugins/XRInterface/TESTING.md) protocol from KEU

<!--
If making a non-RC release:

  - [ ] Request from marketing@kitware.com an update of version number in "Download Latest Release" text on www.paraview.org
  - [ ] Move unclosed issues in GitLab to the next release milestone in GitLab
-->

/cc @ben.boeckel

/cc @cory.quammen

/cc @mwestphal

/cc @wascott

/label ~"priority:required"

[win-sign-example]:  https://kwgitlab.kitware.com/software-process/package-signing/-/issues/12
[classroom-tutorials]:  https://docs.paraview.org/en/latest/Tutorials/ClassroomTutorials/index.html
