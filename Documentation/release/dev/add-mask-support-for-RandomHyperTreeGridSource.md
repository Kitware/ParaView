## Add Mask generation for the Random Hyper Tree Grid source.

You can now generate a mask for the Random Hyper Tree Grid source, it can be controlled by the property  `MaskedFraction` that allows to control the spatial proportion of the HTG that will be masked. This value is a target and the actual masked fraction of the generated HTG can differ up to an error margin, depending on the number of tree and the branching factor. Which means a masked fraction with a value near 1 can lead to a non completely masked HTG, since the error margin still applies for this value.
