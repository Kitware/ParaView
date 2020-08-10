## OpenVR Plugin improvements

Many updates to improve the OpenVR plugin support in Paraview

- Added a "Come to Me" button to bring other collaborators to your current location/scale/pose

- Fixed crop plane sync issues and a hang when in collaboration

- Support desktop users in collaboration with an "Attach to View" option that makes the current view behave more like a VR view (shows avatars/crop planes etc)

- Added a "Show VR View" option to show the VR view of the user when they are in VR. This is like the steamVR option but is camera stabilized making it a better option for recording and sharing via video conferences.

- Broke parts of vtkPVOpenVRHelper into new classes named vtkPVOpenVRExporter and vtkPVOpenVRWidgets. The goal being break what was a large class into smaller classes and files to make the code a bit cleaner and more compartmentalized.

- Add Imago Image Support - added support for displaying images strings are found in the dataset's cell data (optional, off by default)

- Fix thick crop stepping with in collaboration

- Update to new SteamVR Input API, now users can customize their controller mapperings for PV. Provide default bindings for vive controller, hp motion controller, and oculus touch controller

- Add an option controlling base station visibility

- Reenabled the option and GUI settings to set cell data value in VR

- Adding points to a polypoint, spline, or polyline source when in collaboration now works even if the other collaborators do not have the same representation set as active.
