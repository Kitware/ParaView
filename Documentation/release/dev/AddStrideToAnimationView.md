## Stride property for the animation

It is now possible to change the granularity of the animation using the `Stride` property in the animation view.
This property only makes sense in the `Snap to Timesteps` and `Sequence` mode i.e. when there's a fixed number of frames.
The `Stride` property allows to skip a fixed number of frames to control how much frame should be displayed.
For example with a dataset having a time domain of `[0,1,2,3,4,5,6]` and when setting the stride to 2, only the times
`[0,2,4,6]` will be used.
The stride is taken into account when using the `Play`, `Go To Next Frame` or `Go To Previous Frame` buttons.
A stride of 1 will act as the default behavior i.e. no frames are skipped.
