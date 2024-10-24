## Fix the MultiBlock Inspector in client/server local rendering

Fix a bug where the changes done in the Multiblock Inspector had unexpected behavior in the view.
For example, color or opacity changes wasn't taken into account or wrong blocks were hidden (only happening in c/s local rendering).
