# Gradient: Hyper tree grid improvements

Extend the Hyper tree grid version of the gradient filter with a new
**UNLIMITED** mode. In this version, the gradient is computed using unlimited
cursors, refining neighboring nodes to have a local computation similar to
a regular grid.

In this mode, it is possible to handle extensive attributes so the
virtual subdivision reduces their influence.
