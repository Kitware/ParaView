## Translatable string refactor

Most strings which are only displayed in the UI and whose value are not used for comparison are now framed into the [QObject's tr() method](https://doc.qt.io/qt-5/qobject.html#tr) or [QCoreApplication's translate() method](https://doc.qt.io/qt-6/qcoreapplication.html#translate).
