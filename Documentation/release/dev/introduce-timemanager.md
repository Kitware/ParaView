# Introduce TimeManager panel

In `ParaView`, you can load temporal dataset. You can also set up an animated scene where
analysis and visualization parameters can change at each timestep.

The appropriate controls used to be split into  the `Animation View` and the `Time Inspector`.
As both have a similar interface and some redundancy, we removed them
in favor of the brand new `Time Manager` panel.

From this unique panel, you can now introspect data times, configure the animation
scene timestep list, define a start and end time, use a stride to quickly navigate through
the scene. This is also the place where to configure animation tracks.

This rework was a good opportunity to improve the user experience and to fix several problems,
including the settings usage for time notation.

For more informations, see the [official documentation](https://docs.paraview.org/en/latest/UsersGuide/animation.html)
