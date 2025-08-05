# CAVE Interactor Styles

The CAVE interactor styles are now much more performant, usable, and
functional.

## Performance improvements

You can now run multiple interactor styles at the same time with far better
performance, as all interactor styles included with ParaView (both Python
and C++) have been updated to defer as much computation as possible to the
`Update()` method, rather than doing computation in the event handling
methods.

To learn about the key details of the plugin event loop that make this a
sensible practice, as well as read about other best practices for writing
interactor styles, see the new `Incubator/CAVEInteraction/README.md`.

## Usability improvements

You can now more readily experiment with interactor styles, as each style has
been given a "Reset" functionality. This allows you to reset the scene and any
objects properties to their original values, in case you fly too far away or get
turned upside down or faced away from your data.

Interactor styles have been catalogued and renamed to match their functionality.
See the new `Incubator/CAVEInteraction/README.md` for a brief discussion of the
features and corresponding nomenclature of the built-in interactor styles.

Additionally, you can now hover over each style in the combo box and see a
tooltip describing the behavior and requirements of each interactor style.

## Functionality improvements

You can now be more confident that interactor styles work properly, especially
when used in conjunction with each other, as all interactor styles have
been thoroughly tested and had bugs fixed.  Many interactor styles did not
properly account for the fact that other styles might already have updated
the `ModelTransformMatrix`, and these have all been fixed. To read more
about best practices for writing interoperable interactor styles, see the
new `Incubator/CAVEInteraction/README.md`.
