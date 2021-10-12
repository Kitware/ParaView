# Add Threshold Table Filter to ParaView

Add a table thresholding filter to the ParaView interface. The *Threshold Table* filter can now be found under *Filters* > *Data Analysis* in order to perform four different thresholding operations. Its input parameters are :

- **Column** : Specifies which column to use for thresholding.
- **Min Value** : Lower bound of the thresholding operation.
- **Max Value** : Upper bound of the thresholding operation.
- **Mode** : Specifies one of the four available thresholding methods.

The four different modes are :

- **Below Max Threshold** : Accepts rows with values < MaxValue
- **Above Min Threshold** : Accepts rows with values > MinValue
- **Between** : Accepts rows with values > MinValue and < MaxValue
- **Outside** :  Accepts rows with values < MinValue and > MaxValue
