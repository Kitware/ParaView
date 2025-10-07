## pqAnimationTimeWidget: Repopulate timesteps only when needed

`pqAnimationTimeWidget` now repopulates the timesteps combo box only when needed i.e. they have changed.
This change improves performance when working with datasets with thousands of timesteps.
