## Override property visibility

ParaView property visibility is either "default" (i.e. always visible), "advanced" or "never".
You can now override those with a simple `PropertyPanelVisibilities.json` JSON-formatted configuration file, placed under a standard ParaView configuration directory.

It should should be a collection of object per proxy group, containing one object per proxy, itself having a list of properties.

Example of PropertyPanelVisibilities.json
```json
{
  "annotations" :
  {
    "GridAxes3DActor" :
    {
      "XTitle" : "advanced"
    }
  },
  "sources" :
  {
    "SphereSource" :
    {
      "StartTheta" : "default",
      "StartPhi" : "never"
    }
  }
}
```

### Notes for developpers

ParaView has a new behavior, `pqPropertyPanelVisibilitiesBehavior` that can be instanciated to enable the feature.
Upon creation, it will read the configuration json file, and it will hook every proxy registration.
To properly work, this behavior needs to be created before any `pqProxyWidget`, as only further widget creation will be hooked.
