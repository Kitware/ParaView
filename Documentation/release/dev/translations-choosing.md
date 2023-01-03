## Translations selector

ParaView now provides the ability to select the interface language from its settings
(in the `General` tab, `Interface Language` section).

A combo box gives all the possible languages to load. A restart is then required to
take effect.

Languages are searched from directories specified by the environment variable
`PV_TRANSLATIONS_DIR` or in the translation's resource directory.
If a language is not available anymore, its name will be followed by `(?)` in the menu.

Qt-provided translations for its standard widgets are also loaded, either from the
translations directory of the Qt installation, or from ParaView resource directory.
