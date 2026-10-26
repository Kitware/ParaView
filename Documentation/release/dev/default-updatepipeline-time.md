# Fix default time in Python UpdatePipeline method

ParaView does not trigger an extra pipeline update when calling `Show()` just after `UpdatePipeline()` anymore.
Instead, `UpdatePipeline()` mimics what was done in `Show()` and properly reuse the current time as the requested time.

The extra update used to appear only under certain conditions, see !7984 for more info.
