## The Threshold filter can now threshold below or above a value

New thresholding methods have been added to the `Threshold` filter:

- `Between`: Keep values between the lower and upper thresholds.
- `Below Lower Threshold`: Keep values smaller than the lower threshold.
- `Above Upper Threshold`: Keep values larger than the upper threshold.

Previously, it was only possible to threshold between two values.

Accordingly, the property `ThresholdBetween` has been removed. Instead, the following three properties have been added: `LowerThreshold`, `UpperThreshold` and `ThresholdMethod`.
