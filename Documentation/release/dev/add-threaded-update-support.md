# Add Threaded Update support

It is now possible to update the pipeline in a separate thread, which let ParaView UI
stay responsive while we wait. Please note not much can be done apart aborting the filter
currently being updated.

This is an experimental setting which can be found in Edit -> Settings -> General.
