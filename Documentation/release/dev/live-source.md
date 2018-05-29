# Live Programmable Source

To support use-cases where one wants to develop a programmable source that is *refreshed*
periodically, we have added **Live Programmable Source**. With this source, one can provide a
Python script for **CheckNeedsUpdateScript** that can indicate that the source may have new data
and hence should be updated. The ParaView Qt client can periodically check such source and update
them, if needed.
