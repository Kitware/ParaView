## Introduce extractors for Catalyst steering

Catalyst 2.0's "steering" capabilities can send data back to the simulation in order to "steer" it. In order to use the steering, the simulation used to send an XML file specifying a `vtkSteeringDataGenerator` proxy and properties to be filled by Paraview Catalyst.

While this still works, we introduce Catalyst **steering extractors**, a new kind of extractor acting as "sinks" in the visualization pipeline. Calling `catalyst_results` from the simulation will trigger these sinks, and their input dataset will be serialized to Conduit's mesh blueprint and sent back to the simulation in the results Conduit node. This special kind of extractor needs to be created in the Catalyst Python pipeline script. They are **not** associated to a trigger and a writer like other extractors do. They are created by the extracts controller but managed externally by the in situ helper.

Example Catalyst pipeline script:
```python
# Data is provided by the simulation.
producer = TrivialProducer(registrationName="grid")

# Perform a reduction on the input data.
sliced = Slice(Input=producer, SliceType="Plane")

# Create a steering extractor from the slice.
steering = CreateExtractor('steering', sliced, registrationName='steered_slice')

```

This script is executed as usual when `catalyst_execute` is called, but the extraction happens only when the simulation calls `catalyst_results`. This feature can be used to perform a reduction on input data, and get the result in the simulation directly.
