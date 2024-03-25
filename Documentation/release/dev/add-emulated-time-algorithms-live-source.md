## Introducing Emulated Time Algorithm using Live Sources

Expend live sources capabilities, with a new emulated time algorithm.

Once you've inherited your temporal algorithm from `vtkEmulatedTimeAlgorithm`. You can utilize the LiveSource concept to synchronize the algorithm's timesteps on a "Real Time" interval (in seconds). It will skip frames if the pipeline processing does not keep up.

An example scenario for this algorithm would be to replay a LiveSource stream from a recorded file while maintaining the original conditions and playback speed.

To implement this behavior, inherit your algorithm from `vtkEmulatedTimeAlgorithm` and define the proxy as a LiveSource with the `emulated_time` attribute.
```xml
  <Hints>
    <LiveSource interval="100" emulated_time="1" />
  </Hints>
```
