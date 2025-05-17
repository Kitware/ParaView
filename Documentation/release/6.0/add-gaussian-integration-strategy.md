## Add Gaussian Integration Strategy in _IntegrateVariables_

Paraview now provides two strategies for the _IntegrateVariables_ filter. The original strategy is now called Linear Strategy and the new Gaussian Strategy is now available. This enables the proper computation of degenerate linear cells (e.g. with non planar faces) and opens the way to high-order cells computation.
