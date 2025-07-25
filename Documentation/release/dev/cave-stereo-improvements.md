## CAVE Stereo Configuration Improvements

You can now specify simply "Left" or "Right" as the desired stereo type
for paraview or pvserver processes, allowing ParaView to be run in CAVES
where displays are dedicated to each eye. While you can still specify
stereo type on the command line, you can now also provide them on a
per-machine basis in your .pvx file. Like other .pvx file configuration,
this information is also available to you from the client side (e.g. in
Python scripts).

Additionally, you no longer need to specify any stereo type on the client
just because you want stereo on the servers. If you don't specify anything
on the client, a stereo type will be chosen that is compatible with what
you have selected on the server side.

Looking at the stereo options in the view properties panel, you will now
notice a new server stereo type option "Original Configuration", which
sets each process back to initially configured (via CLI or .pvx) values.
