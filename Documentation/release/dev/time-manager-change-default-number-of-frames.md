## Time manager: Change default number of frames to 1

In the past, `ParaView` used to always have a default number of frames set to 10.
This created an issue when trying to save animation/extracts with an object that doesn't have time information.
This change sets the default number of frames to 1, which is more appropriate for such cases. If one wants to create
an animation with more frames, they can always change the number of frames in the `Time Manager` panel.
