## Rethinking Catalyst

Since Catalyst was first developed several years ago, it has become widely used
by several simulation codes for in situ analysis and visualization. Since its
introduction. the in situ data processing landscape has also changed quite a
bit. To benefit from some of the r&d in the scientific computing work and make
it easier to incorporate ParaView for in situ data processing and visualization
in simulation code, we have redesigned the Catalyst API from ground up.
This release includes what can be considered a preview-release of this new work.

ParaView's in situ components now comprise of the following:

* A new C-API, called [Catalyst API](https://gitlab.kitware.com/paraview/catalyst)
  that simulations use to invoke Catalyst and describe simulation data structures.
  The API uses [Conduit](https://llnl-conduit.readthedocs.io/en/latest/index.html)
  for describing data and meta-data.

* A lightweight stub implementation of this API that simulations can use to
  build against. This simplifies the development process since one does not need
  a SDK deployment for ParaView or build ParaView from source when adding
  Catalyst to simulations.

* A ParaView-specific implementation of the Catalyst API, now referred to as
  ParaView-Catalyst, that can be used instead of the stub implementation to use
  ParaView for in situ processing. This implementation can be easily swapped
  in place of the the stub implementation at load-time.

It must be noted that Legacy Catalyst APIs are still supported in this version and
will continue to be supported until further notice.
