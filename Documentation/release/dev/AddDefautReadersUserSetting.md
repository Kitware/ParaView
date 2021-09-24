# Add a default readers user setting for the file dialog

* Add this setting to let the user specify the reader they want to use by default when selecting a file that matches the reader patterns when the filter of the file dialog is on "Supported Types".

* Add a "Set reader as default" button to the dialog listing compatible readers, that adds lets the user add the setting more easily than manually from the Settings dialog.

* When the the user clicked Ok in the file dialog with the "All Files" filter, all existing readers are listed, and clicking "Set reader as default" will let the user set a custom pattern before adding it to the settings.

* When a specific reader is selected in the file dialog filter, this reader is automatically used even if other readers are compatible with this file.

* Add a `ShowLabel` Hint for `StringVectorProperty` to display its `Label` and `Documentation` in the associated widget.
