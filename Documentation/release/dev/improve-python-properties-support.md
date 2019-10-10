# Improve python properties support

Improving Python Properties support by returning the property instead of the value for domain based properties.
Which allows to do the following:
rep = GetDisplayProperties()
rep.Representation.Available
['3D Glyphs', 'Feature Edges', 'Outline', 'Point Gaussian', 'Points', 'Slice', 'Surface', 'Surface With Edges', 'Volume', 'Wireframe']
