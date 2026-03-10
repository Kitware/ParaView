## Add **Binary Threshold** filter

ParaView now has a new image filter called **Binary Threshold**. This filter gives the ability to replace image values based on 2 thresholds:
- Any values outside of the thresholds will be replaced by the **Out Value** property. Likewise, any values inside the thresholds range will be replaced by the **In Value** property.
- One can specify whether the **Out Value** or **In Value** has an effect of the filter output by checking **Replace Out** or **Replace In** respectively.
