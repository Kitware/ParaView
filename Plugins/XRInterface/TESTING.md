# Testing Steps for XRInterface Plugin

Created with the Valve Index controller, some adaptations may be needed for other HMDs.
Some features visible in the UI have not been added (eg: Animation) as they are not working properly.
See https://gitlab.kitware.com/vtk/vtk/-/issues/18302 for reference.

## After opening ParaView:

 - Create a “Wavelet”, Apply
 - Create a “Point Data to Cell Data” filter and Apply
 - Set “Surface With Edges” representation
 - Set “RTData” as coloring array (on cells)
 - Create a “Poly Line Source”, Apply
 - Load the “XRInterface” plugin

## After loading the plugin, in the dock panel:

 - Check “Base Station Visibility” (only if HMD uses base stations)
 - Choose “OpenVR” or “OpenXR” as runtime depending on what you are testing
 - Enter “RTData” in “Editable Field”
 - Enter values separated by commas, e.g., “1, 2, 3” in “Field Values”
 - Click “Send to XR”
 - Output message may appear, close it.
 - Press “Show XR View” and check VR scene is visible in a desktop window

## After entering XR:

 - Check that the base stations are visible (only if HMD uses base stations, only with OpenVR)
 - Uncheck “Base Station Visibility” in ParaView and check they are not visible
 - Check actor translation (press “A” on both controllers and move them in the same direction simultaneously)
 - Check actor scaling (press “A” on both controllers and move them away from each other or close to each other simultaneously)
 - Check actor rotation (press “A” on both controllers then move one of them around the other, like a driving wheel)
 - Open menu (press right controller “B”)

## In the XR menu - “Movement Style” combo box:

 - Check “Flying” movement style (only left joystick, follows controller orientation/direction)
 - Check “Grounded” movement style (left joystick for horizontal movement, right joystick for vertical movement, only with OpenVR)

## In the XR menu - “Right Trigger” combo box:

 - Check “Pick” action (pick with the ray)
 - Check “Grab” action (controller must be in actor bounding box)
 - Check “Probe” (pick a cell)
 - Check “Interactive Crop” (keep trigger pressed)
 - Check “Add Point to Source” (will automatically add to the poly line source, make sure it is selected in the pipeline view)
 - Back to “Pick” action

## In the XR menu - Central section:

 - Press "Add Crop Plane" (should place a clip widget in front)
 - Grab it and check that it moves with the controller
 - Check "Snap Crop Planes"
 - Grab the plane and move it slowly in a horizontal position (should snap)
 - Press "Remove All Crop Planes"
 - Press "Add Thick Crop" (should place a box widget in front)
 - Grab it and check that it moves with the controller
 - Press "Remove All Crop Planes"

 - Press “Distance Widget” (ruler)
 - Place two points with right trigger (should place distance info)
 - Press “Remove Distance Widget”

 - Change “View Up” and check orientation change
 - Change “Scale Factor” and check scaling change (recommended to test the smaller ones)
 - Change “Motion Factor” and check movement speed change (with joystick)

 - Choose a save index in the combo box “Save Camera Pose”
 - Check that the index appears in the combo box “Load Camera Pose”
 - Move away from your current position (with joystick or by moving physically)
 - Choose the previous index in the combo box “Load Camera Pose” (camera should be moved at the saved position)
 - Move away from your current position (with joystick or by moving physically)
 - Press the left trigger to load the saved position (camera should be moved at the saved position again)

## After selecting a cell with the “Probe” action:

 - Check that the combo box “Field Value” has the values entered previously (1, 2, 3)
 - Choose a value, e.g. “2”
 - Press “Assign Value” (selected cell should now have that value)

## In the XR menu - Bottom left section:

 - Press “Reset Camera” (actors must come back to center of the room)
 - Outside of the menu, use picking or grabbing to move the wavelet
 - Press “Reset Actors Position" (actors should come back to their position before picking)
 - Uncheck “Show Floor” (floor should disappear)
 - Check “Interactive Ray” (ray should change color upon intersecting an actor)
 - Check “Navigation Panel” (should appear above the left controller)
 - Press “Exit XR” button and confirm XR is closed

## After exiting XR:

 - Save state file (.pvsm)
 - Exit ParaView
 - Relaunch ParaView
 - Load the “XRInterface” plugin
 - Load previous state file
 - Click “Send to XR”
 - Open menu (press right controller “B”)
 - Check that saved position is available in the combo box “Load Camera Pose”
