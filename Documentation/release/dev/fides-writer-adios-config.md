# Configure ADIOS2 engine when using FidesWriter

You can now configure how ADIOS2 files are written at runtime, using the `AdiosConfigFile`
property on the `FidesWriter` proxy.

When using the `FidesWriter` proxy (via `SaveData`, for example), you can now supply any
ADIOS2 configuration option your ADIOS2 build supports by providing the path to an ADIOS2
xml config file. Usage example (taken from new test):

```python
SaveData(output_bp_path, proxy=wavelet, AdiosConfigFile=config_path)
```
