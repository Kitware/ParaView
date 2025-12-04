# Enable CDIReader usage with big datasets, thanks to CDI lazy loading feature

CDIReader was unusable with FileSeries, because it was loading all files all at once.

By enabling the lazy load feature, it is now possible to work with large datasets.

For example, consider the following code snippet:

```python
LoadPlugin("CDIReader", ns=globals())
reader = CDIReader(FileNames=FILE_LIST)
print("Updating pipeline information...", end="")
reader.UpdatePipelineInformation()
print("done!")

timesteps = reader.TimestepValues
print(f"Number of timesteps: {len(timesteps)}")
```

With a `FILE_LIST` of 50 files (8 timesteps each):

BEFORE (~15')
```
Updating pipeline information...done!
Number of timesteps: 435
1052.26 seconds.
```

NOW (~12")
```
Updating pipeline information...done!
Number of timesteps: 435
12.50 seconds.
```
