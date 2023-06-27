## pqPresetDialog layering on MacOS

Place the pqPresetDialog in the Qt::Tool layer, same as dock widgets, so this
dialog doesn't always get put behind dock widgets on MacOs.
