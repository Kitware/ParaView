## Fix the pause button not stopping the animation

Clicking **Play** or **Reverse** several times while an animation was playing could leave a hidden restart behind.
After that, pausing stopped the animation and then started it again, so it looked like **Pause** did not work. This
was most likely with slow time steps, where several clicks queue up before the next frame is drawn. Only the latest
request is kept now, and **Pause** cancels a request that is waiting for the playback to end.
