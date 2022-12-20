# Localization Howto      {#LocalizationHowto}

## Introduction

ParaView comes with a localization system that handles the complete language introspection workflow, from translation searching in the source code to language selection in the settings.

The theoretical workflow for a single language runs as follows:

- The ParaView build system generates translation source files (.ts files) in the chosen destination repository.
- The translation from English to the desired language is written into these source files
- These translations files are compiled into a binary file (.qm file) and installed in the `translations` shared folder of ParaView.
- The language can then be selected in the settings `Interface Language` section.

In practice, the process is slightly more complex since we use an official translation source repository that is updated by a translations CMS, for simplifying contributions.

### Technologies used

Most of the steps are done using two of the [Qt5's linguist utilities](https://doc.qt.io/qt-5/linguist-manager.html):
- `lupdate`, a source file parser that can parse C++ and Qt's UI files for translatable strings, and then generates a .ts file from it
- `lrelease`, a .ts file compiler, that generates the .qm files.

These utilities are invoked in the ParaView CMake build system and in the official ts repository respectively.

### What is translated

- Interface (Menus, buttons...)
- Proxy and property names/labels and docs (see below for the exhaustive list).

## Glossary

- locale: In this context, a locale is a string in the format `<language>_<COUNTRY>`. It is a language's national variant.
- lrelease: A Qt utility that transforms ts files into a qm.
- lupdate: A Qt utility that parses .ui and C++ files into ts files.
- .qm: A Qt resource file containing translations.
- Qt linguist: A Qt tool to fill ts files with translations.
- translation context: Qt enforces that all translations are within a context, possibly user-specified. In ParaView, the standard is to use the class name as context. For a translation to be used, the context must be the same at parse-time and at runtime.
- .ts: A Qt's xml containing string to translate and its translation if filled.
- .ui: A Qt resource file containing user interface layouts.

## Writing translatable source code

All the source code in ParaView should ideally be translatable. A string is _translatable_ if it can be found by lupdate.

Strings displayed in the ParaView client can come from three main sources:
- Qt's UI files
- C++ source code that is Qt-related
- XML proxy files

Qt's UI files are automatically found by lupdate, so if a string shall **not** be translated (ie ParaView's name !) it has to have its `translatable` property disabled in Qt Designer.

### In the ParaView Qt classes

An exhaustive guide to translatable source code using Qt5 can be found [here](https://doc.qt.io/qt-5/i18n-source-translation.html).

For setting the translatable property of a string in the most common cases:
- If an explicit string is in a class inheriting QObject that has the `Q_OBJECT` macro, you can use the `tr(<explicitStringToTranslate>)` method of `QObject`, without specifying `this->` before ([example](https://gitlab.kitware.com/paraview/paraview/-/blob/c055918678939470ba25e2334e131870da8f2fde/Qt/ApplicationComponents/pqSaveDataReaction.cxx#L159)).
- If a class cannot be given the `Q_OBJECT` macro, or does not inherit from `QObject`, you can either:
    - add the `Q_DECLARE_TR_FUNCTIONS(<className>)` to the class and then call `tr(<explicitStringToTranslate>)` inside ([example](https://gitlab.kitware.com/paraview/paraview/-/blob/c055918678939470ba25e2334e131870da8f2fde/Qt/Components/pqArrayListWidget.cxx#L46)) or
    - write `QCoreApplication::translate(<className>, <explicitStringToTranslate>)` for every string to translate in the class ([example](https://gitlab.kitware.com/paraview/paraview/-/blob/c055918678939470ba25e2334e131870da8f2fde/Qt/ApplicationComponents/pqParaViewMenuBuilders.cxx#L329)).
- If the string is located outside of a class, method or even function, you can use `QT_TRANSLATE_NOOP(<contextName>, <explicitStringToTranslate>)`.

All these methods requires the include of `QCoreApplication`.

Some examples:
```cpp
// When inside a class method with Q_OBJECT macro
class someClass : QObject
{
    Q_OBJECT
    QString someMethod();
};

QString someClass::someMethod()
{
    return tr("Translated string");
}

// When inside a class without Q_OBJECT macro
class someNonQtClass
{
    Q_DECLARE_TR_FUNCTIONS(someNonQtClass)
    QString someMethod();
};

QString someNonQtClass::someMethod()
{
    return tr("Translated string");
}

class someNonQtClass2
{
    QString someMethod();
};

QString someNonQtClass2::someMethod()
{
    return QCoreApplication::translate("someNonQtClass2", "Translated string");
}

// When outside of any method
QString someFunction()
{
    return QT_TRANSLATE_NOOP("translationContext", "Translated string");
}
```

### Making Proxy XML elements translatable

The XML format is not natively supported by lupdate. Therefore, `CMake/XML_translations_header_generator.py` was developed. It transforms proxy XMLs into C++ headers in order to be found by lupdate.

In a C++ code that displays text from the XML, it is now mandatory to use the context "ServerManagerXML" so that the translation can be found at runtime. E.g.: `QCoreApplication::translate("ServerManagerXML", <translated XML element>));`.

Currently translatable XML fields are:

- `label`, which can be an attribute of any element
- `menu_label`, which can be an attribute of any element
- `Documentation` element's content
    - `long_help` attribute
    - `short_help` attribute
- All `*Property`'s `name`s

This behavior can be extended by adding new elements or attributes to parse in the python utility.


## Testing new translations

The plugin `TranslationsTesting`, bundled with ParaView, is built by default when enabling the `PARAVIEW_BUILD_TRANSLATIONS` option. It will search though the main window for untranslated strings and print a warning if found.

This plugin is meant to be run through ctest, with the `TranslationsTesting` test. It might be slow since it uses Regexes for its widget-ignore list (hopefully this list will be empty, otherwise, you will have the time to admire the improved version of ParaView).


### Ignoring untranslated strings

Some source strings could not, or should not be translatable (ie ParaView's name !).
ParaView translatability is tested on the ParaView's Gitlab CI, so if a string cannot be translated, the widget/action containing it **has** to be be added on the ignore list of the plugin.

The ignore list is located at `Plugins/TranslationsTesting/pqTranslationsTesting.h`. You can use either
- `TRANSLATION_IGNORE_STRINGS` to ignore a precise widget/action, or
- `TRANSLATION_IGNORE_REGEXES` to ignore a range of widgets.

Both lists take pairs of strings: the first element is the name of the widget and the second is the attribute to ignore or an empty string to ignore all attributes.

The tested Qt attributes are text, `toolTip`, `windowTitle`, `placeholderText`, `text` and `label`.

## Translation workflow

### Source files generation

When `PARAVIEW_BUILD_TRANSLATIONS` is enabled, translations source files are built in the given `PARAVIEW_TRANSLATIONS_DIRECTORY`. A specific build target `localization` can be used separatly from the ALL target to build them.

### Source files filling
There is an official [paraview-translations](https://gitlab.kitware.com/paraview/paraview-translations) repository containing up-to-date source files.

The target `files_update` will complete all ts files of all languages with the source strings. A single language can also be updated using the special target `<locale>_update`.

After this step, translators can fill ts files with translations using [Qt Linguist](https://doc.qt.io/qt-5/linguist-translators.html) or any other tool supporting ts files.

### Source file compilation

Source files can be compiled into qm files using the CMake process of the translations repository. Each locale with a folder in the translations repository is a valid target that will create a qm.

It is also possible to compile ts files manually using lrelease. The command is `lrelease <input files> -qm <outputQm>`.

### Binary files loading

Qm files can be loaded from the general settings. If a qm is found by ParaView, the language it provides will be available in the `Interface language` Combobox. The selected language is saved in the settings, and a restart is required.

By default, ParaView searches for qms in the `translations` shared folder. The environment variable `PV_TRANSLATIONS_DIR` can provide other searching paths. When multiple qm files have the same language, the first qm found will be used, using the `PV_TRANSLATIONS_DIR` paths first, and the default path after.

The locale can also be forced using the `PV_TRANSLATIONS_LOCALE` environment variable.
