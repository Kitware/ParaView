## ParaView's new imaging testing framework

### Overview

The ParaView's imaging testing framework has been enhanced to incorporate the Structural Similarity Index (SSIM) metric
on the Lab color space. This contribution provides an opt-in feature to ensure that as a project that is using VTK,
it does encounter broken tests when updating.

### SSIM Metric

#### Definition

The Structural Similarity Index (SSIM) is a metric introduced in the paper titled "Image Quality Assessment: From Error
Visibility to Structural Similarity" by Wang et al. It measures the "structural correlation" between two images,
providing a comprehensive evaluation of their similarity.

#### Computation

In this implementation, SSIM is computed per pixel around a patch in the Lab color space. The Lab color space is chosen
for its perceptual uniformity, ensuring that differences in color are consistent with human perception.

The SSIM metric produces a value between 0 and 1, where 1 indicates perfect structural similarity between the images,
and 0 indicates no similarity.
Domain of Output

The output of SSIM metric ranges from 0 to 1, where 1 indicates perfect similarity, and 0 indicates no similarity.

#### Integration

Users can enable this feature by defining the CMake variable `DEFAULT_USE_SSIM_IMAGE_COMP` as true. When turned on, SSIM
will be used by default.

#### Configuration

There are three ways to use the SSIM metric when setting a test:

##### `TIGHT_VALID` (Default)

Uses Euclidean metrics (L2 and 2-Wasserstein) on each channel.
Computes the maximum discrepancy (from 0 to 1).
Recommended for images sensitive to outliers.
Default threshold is around 0.05.

##### `LOOSE_VALID`

Uses both Manhattan distance and Earth-mover's distance on each channel.
Less sensitive to outliers.
Suitable for images dependent on graphics drivers or containing text.
Default threshold is recommended.

##### `LEGACY_VALID`

Uses the old metric (not recommended).
For compatibility with previous testing methods.

##### Example:

```cmake
set(Test1_METHOD TIGHT_VALID) # Default
set(Test1_METHOD LOOSE_VALID)
set(Test2_METHOD LEGACY_VALID)
```

#### Manual Configuration

Set `VTK_TESTING_IMAGE_COMPARE_METHOD` to one of the following values: `TIGHT_VALID`, `LOOSE_VALID`, or `LEGACY_VALID`.

#### Migration

It is advised to migrate to the new SSIM metric within a reasonable time frame.
The new metric is more selective and results in fewer false negatives compared to the legacy method.
