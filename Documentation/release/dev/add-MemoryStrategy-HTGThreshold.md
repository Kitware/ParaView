## Memory Stretgies for **Threshold** filter with Hyper Tree Grid input

You can now choose how to describe the output of the **Threshold** filter when inputting a hyper tree grid using memory strategies. This drop down list of options determines how the output hyper tree grid output is represented in memory:

* MaskInput (default): shallow copy the input and mask all cells being thresholded
* Regenerate Trees And Index Fields: generate a new tree structure but use indexed arrays on the input for cell data
* New HyperTreeGrid: construct a new HyperTreeGrid from scratch representing the minimal threshold of the input
