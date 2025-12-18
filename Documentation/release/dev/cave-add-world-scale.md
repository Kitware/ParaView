## Add world scale to CAVE config panel

You can now set a world scaling factor in the CAVE configuration panel, and it is immediately applied to the `ModelTransformMatrix` (aka `NavigationMatrix`). This new UI field also reflects any scaling done by other interactor styles, thanks to a new navigation event observer that extracts the scale from the `ModelTransformMatrix`, so that the UI always reflects the correct current scale.

In your interactor styles you can now make use of new static convenience methods that have been added to `vtkSMVRInteractorStyleProxy` to support extracting/updating the scale factor from/to the `ModelTransformMatrix`.

You can also experiement with this new scaling functionality using a new custom Python interactor style that performs scaling, `SimpleScalingInteractor.py`. This new interactor style allows you to observe and verify the connection between scaling with an interactor style and scaling with the panel UI element.

Of course, whenever you use the world scaling feature, it is also properly saved to and loaded from state files.
