# Testing Steps for XRInterface Plugin

Created with the Valve Index controller, some adaptations may be needed for other HMDs.

## After opening ParaView:

 - Create a “Wavelet”, Apply
 - Set “Surface With Edges” representation
 - Set “RTData” as coloring array
 - Create a “Slice” on the Wavelet, Apply
   - This will hide the Wavelet, set it back to visible using the Pipeline Browser
 - Open “Time Manager” (View -> Time Manager)
 - In “Time Manager”, change the source from "Wavelet1" to "Slice1" in the first combo box
 - In “Time Manager”, click the “+” button to add a default animation for the Slice
 - Create a “Poly Line Source”, Apply
 - Load the “XRInterface” plugin

## After loading the plugin, in the dock panel:

 - Check “Base Station Visibility” (only if HMD uses base stations)
 - Choose “OpenVR” or “OpenXR” as runtime depending on what you are testing
 - Click “Send to XR”
 - Press “Show XR View” and check VR scene is visible in a desktop window

## After entering XR:

 - Check that the base stations are visible (only if HMD uses base stations, only with OpenVR)
 - Uncheck “Base Station Visibility” in ParaView and check they are not visible
 - Check actor translation (press “A” on both controllers and move them in the same direction simultaneously)
 - Check actor scaling (press “A” on both controllers and move them away from each other or close to each other simultaneously)
 - Check actor rotation (press “A” on both controllers then move one of them around the other, like a driving wheel)
 - Open menu (press right controller “B”)

## In the XR menu, in “Interaction” tab

 - “Right Trigger” combo box:
   - Check “Pick” action (pick with the ray)
   - Check “Grab” action (controller must be in actor bounding box)
   - Check “Probe” (pick a cell)
   - Check “Interactive Crop” (keep trigger pressed)
   - Check “Add Point to Source” (will automatically add to the poly line source, make sure it is selected in the pipeline view)
   - Check “Teleportation” (teleport with the ray, make sure that the ray intersect with the prop you want to teleport to)
   - Back to “Pick” action
 - Outside of the menu, use picking or grabbing to move the wavelet
 - Press “Reset Actors Position“ (actors should come back to their position before picking)
 - Toggle and check “Interactive Ray” (ray should change color upon intersecting an actor)

## In the XR menu, in “Movement” tab

 - “Movement Style” combo box:
   - Check “Flying” movement style (only left joystick, follows controller orientation/direction)
   - Check “Grounded” movement style (left joystick for horizontal movement, right joystick for vertical movement)
 - Move “Movement speed” slider and check movement speed change (with joystick)
 - Press “Reset Camera” (actors must come back to center of the room)
 - Save camera poses:
   - Click “Save Pose” (a new button should appear in the toolbar)
   - Move away from your current position (with joystick or by moving physically)
   - Click on button “1” (camera should be moved at the saved position)
   - Move away from your current position (with joystick or by moving physically)
   - Press the left trigger to load the saved position (camera should be moved at the saved position again)
   - Press “Clear” (all save poses should be removed)
   - Click “Save Pose” (to test state file later)

## In the XR menu, in “Environment” tab

 - Change “View Up” and check orientation change
 - Press one of “Scene Scale” buttons and check scaling change (recommended to test the smaller ones)
 - Toggle “Show Floor” (floor should disappear)

## In the XR menu, in “Widgets” tab

 - Press “Ruler”
 - Check that white cross marker is displayed at the tip of the right controller
 - Place two points with right trigger (should place distance info)
 - Press “Ruler” again
 - Toggle “Navigation Panel” (should appear above the left controller)
 - Press “Add Crop Plane” (should place a clip widget in front)
 - Grab it and check that it moves with the controller
 - Toggle “Snap Crop Planes”
 - Grab the plane and move it slowly in a horizontal position (should snap)
 - Toggle “Hide Crop Planes” to ensure it hides the crop plane
 - Press “Remove All Crop Planes”
 - Press “Add Thick Crop” (should place a box widget in front)
 - Grab it and check that it moves with the controller
 - Toggle “Hide Crop Planes” to ensure it hides the thick crop
 - Press “Remove All Crop Planes”

## In the XR menu:

 - Hide the wavelet from the XR menu pipeline browser, so you can see the slice
 - Play slice animation from XR menu, test all buttons (goto begin/end, single step, play, loop)
 - Let the animation loop, and check that the animation is also played in Paraview Desktop view
 - Check that the loop button is synced between Paraview Desktop and the XR menu
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
