# Choosing DISPLAY or EGL device using command line arguments

When multiple GPUs are available on a system, users have to set either the
DISPLAY environment variable or the EGL device index for each ParaView rank
correctly to ensure that a rank uses a specific GPU. To make this easier, we
now have two new command line arguments added to pvserver, pvrendering and
pvbatch executables.

`--displays=` can be used to sepecify a comma separated list of available
display names, for example:

    > .. pvserver --displays=:0,:1,:2
    > .. pvserver --displays=host1:0,host2:0

For EGL, these can be EGL device index, e.g.

    > .. pvserver --displays=0,1

These displays (or devices) are then assigned to each of the ranks in a
round-robin fashion. Thus, if there are 5 ranks and 2 displays, the displays are
assigned as `0, 1, 0, 1, 0` sequentially for the 5 ranks.

`--displays-assignment-mode=` argument can be used to customize this
default assignment mode to use a contiguous assigment instead. Accepted values
are 'contiguous' and 'round-robin'. If contiguous mode is used
for the 5 ranks and 2 displays example, the displays are assigned as
`0, 0, 0, 1, 1`.
