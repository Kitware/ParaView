# Change Color Map Editor behavior

* Add a combo box to quick apply a preset from the `Default` group directly from the Color Map Editor.

* Add group selection for custom presets. Add a `Groups` field in the imported `json` presets files to add the preset to the specified groups. For example this preset will be added to the `Default`, `CustomGroup1` and `Linear`.
  ```json
  [
    {
      "ColorSpace" : "RGB",
      "Groups" : ["Default", "CustomGroup1", "Linear"],
      "Name" : "X_Ray_Copy_1",
      "NanColor" :
      [
        1,
        0,
        0
      ],
      "RGBPoints" :
      [
        0,
        1,
        1,
        1,
        1,
        0,
        0,
        0
      ]
    }
  ]
  ```
  In this case, `CustomGroup1` does not exist beforehand so it will be created on import. If the `Groups` field does not exist, the preset is added to the `Default` and `User` groups.

* The `DefaultMap` field is no longer used as it is redundant with the new `Groups` field.

* Any preset can now be added or removed from the `Default` group.

* Make the presets dialog non-modal

* Imported presets are displayed in italics to be able to differentiate them from base presets.
