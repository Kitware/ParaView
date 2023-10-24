## Add vtkThreshold component modes

You can now apply Threshold on multi-component arrays. When doing so,
it is possible to change how the components are handled. You can
indeed choose a Component Mode among:

* Selected: the Selected Component needs to satisfy the threshold
* All: all components need to satisfy the threshold
* Any: any component needs to satisfy the threshold

When using the Selected mode, you can either select a specific
component, or even the magnitude of the components.

Note that multi-components arrays options are not available with
Hyper Tree Grid inputs.
