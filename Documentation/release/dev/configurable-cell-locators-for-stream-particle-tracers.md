## Stream/Particle Tracing Filters: Replace Interpolator Type with Configurable Cell Locators

ParaView provides the following stream/particle tracing filters to interpolate velocity fields:
**EvenlySpacedStreamlines2D**, **ParticlePath**, **ParticleTracer**, **StreakLine**, **StreamTracer**,
**ArbitrarySourceStreamTracer** and the legacy **LegacyParticlePath**, **LegacyStreakLine**.

Previously, these filters used the **InterpolatorType** property, which was restricted to the following options:

1. _Interpolator with Point Locator_
2. _Interpolator with Cell Locator_

This configuration was not flexible as it did not allow the use of other **cell locators**. To resolve this, the
_Jump And Walk Cell Locator_ was created to provide "Point Locator" style performance within a cell locator framework.
The legacy **InterpolatorType** integer property has been replaced by a configurable **CellLocator** proxy property.

This change allows users to choose and configure any cell locator from the **cell_locators** proxy group. The verbatim
list of
available locators is:

1. _Cell Locator_
2. _Tree Cell Locator_
3. _OBB Tree Cell Locator_
4. _Static Cell Locator_
5. _Jump And Walk Cell Locator_
6. _BSP Tree Cell Locator_

For backward compatibility, the values of the legacy **InterpolatorType** property from older state files and Python
scripts are automatically replaced using the new **CellLocator** property using the following mapping:

1. _Interpolator with Point Locator_ $\rightarrow$ _Jump And Walk Cell Locator_
2. _Interpolator with Cell Locator_ $\rightarrow$ _Static Cell Locator_
