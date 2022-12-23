## Translatable string refactor

Most strings which are only displayed in the UI and whose value are not used for comparison are now framed into the [QObject's tr() method](https://doc.qt.io/qt-5/qobject.html#tr) or [QCoreApplication's translate() method](https://doc.qt.io/qt-6/qcoreapplication.html#translate).

### Proxies translation

Proxies, proxy groups, properties labels, Categories, ShowInMenu and entries can now be translated.

### Good practices

Good practices are described in the [Localization page in the Doxygen documentation](https://kitware.github.io/paraview-docs/latest/cxx/LocalizationHowto.html).
