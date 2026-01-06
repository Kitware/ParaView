## Change geometry representation LOD computation behavior

The LOD computation is now performed once when the data from the pipeline is updated. Moreover, resizing the window does not trigger the LOD to be computed again, which fixes potential crashes when big dataset are loaded.
