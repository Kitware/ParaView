# Improvements to animations

We have refactored caching of rendered data when playing animations with caching
enabled. The new changes avoid pipeline updates and data movement when cached
data is being used. The cache is now stored on the rendering nodes, thus
avoiding the need to delivery data when playing animations. This also includes
several cleanups to minimize unnecessary pipeline updates for pipelines that are
not impacted by the animation. While such updates are largely no-ops, they still
caused lots of bookkeeping code to execute which adversely impacted the
performance.

This introduces backwords incompatible changes to representations that will
affect developers who have custom `vtkPVDataRepresentation` or `vtkPVView`
subclasses. Please refer to [Major API Changes](@ref MajorAPIChanges) for
details.
