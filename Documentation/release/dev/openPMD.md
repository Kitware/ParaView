# openPMD

The openPMD python module was updated to fix bugs.

Particles and fields ran out-of-sync in animations.
The changes ensure that if one of the ports is updated, field (0) or particles (1), the other port is updated to the same time even if no changes in it were requested.
