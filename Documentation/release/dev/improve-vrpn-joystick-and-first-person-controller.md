# First Person Camera controller

The parameters of the **First Person Camera Controller** (such as **Mouse Sensitivity X/Y**, **Invert X/Y**, **Move Speed Forward/Right** and **Up Axis**) are now saved in the `.pvvr` file when saving and restoring a VR State.

# Joystick Camera controller

## Add the possibility to invert movements with Joystick Camera controller in CAVEInteraction plugin

It is now possible to invert the forward/backward and left/right movements for the **Joystick Camera** controller in the **CAVEInteraction** plugin.

## Add joystick sensitivity parameter when moving the camera with Joystick Camera controller in CAVEInteraction plugin

It is now possible to control how joystick sensitivity is mapped to speed. Higher values increase the precision near the center, so the speed difference between pushing the joystick slightly and pushing it all the way will be big. At lower values, this difference will be smaller. At 0, the speed will always be constant as soon as we move the joystick in one direction.
Please note that changing this parameter doesn't increase the max speed when we push the joystick all the way : this is handled by the **Move Camera Sensitivity**. Instead, the larger the parameter is, the lower the speed will be when we push the joystick slightly.

## Add fast movement Joystick Camera controller in CAVEInteraction plugin

It is now possible to accelerate the movement speed when pressing a button. The multiplier of the movement speed can be set with a parameter.

## Save configuration parameters

The parameters of the **Joystick Camera controller** (such as **Move Camera Sensitivity**, **Invert X/Y**, **Fast Movement multiplier**, **Up Axis**, etc.) are now saved in the `.pvvr` file when saving and restoring a VR State.
