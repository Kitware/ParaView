// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqTranslationsTesting
 * @brief   PV UI translatability testing class
 *
 * This class recursively access all children QWidgets of the client
 * main window and check if the widget and its actions only contains
 * '_TranslationTesting's in their UI-visible attributes.
 *
 * Some text are known to be untranslated and may be ignored.
 * Please explain if those texts should not be translated at all and why (e.g. the application name)
 * or if it is a bug (explanation welcome too).
 * `TRANSLATION_IGNORE_STRINGS` contains identifer of widget that should be ignored,
 * while `TRANSLATION_IGNORE_REGEXES` uses a regex matching.
 */

#ifndef pqTranslationsTesting_h
#define pqTranslationsTesting_h

#include <QObject>
#include <QRegularExpression>

/**
 * Ignore list of {<widget>, <attribute>}, where <widget> is the exact path as computed
 * by QtTesting pqObjectNaming, and <attribute> is the Qt attribute to ignore (empty for all)
 * see recursiveFindUntranslatedStrings() for attributes and isIgnored() for usage
 */
static const QPair<QString, QString> TRANSLATION_IGNORE_STRINGS[] = {
  // Text that SHOULD NOT be translated
  // -----

  // default window title (basically application name)
  QPair<QString, QString>("pqClientMainWindow", "windowTitle"),
  // host infos (name and memory footprint)
  QPair<QString, QString>("pqClientMainWindow/statusbar/1QProgressBar0", "text"),

  // TODO Redesign needed
  // -----
  // Text comes from property value (TimeLabel property of TimeKeeper proxy)
  // (see pqAnimationTimeWidget.cxx)
  QPair<QString, QString>(
    "pqClientMainWindow/timeManagerDock/timeManagerPanel/timeProperties/currentTime/timeLabel",
    "text"),
  QPair<QString, QString>(
    "pqClientMainWindow/currentTimeToolbar/AnimationTimeWidget/timeLabel", "text"),
  // Text comes from non-Qt world: view registration name is displayed (see pqViewFrame.cxx:205)
  QPair<QString, QString>(
    "pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/"
    "MultiViewWidget1/Container/Frame.0/TitleBar/TitleLabel",
    "text"),
  // Text comes from non-Qt world (see `vtkSMFieldDataDomain`, called at pqSelectionEditor.cxx:327)
  QPair<QString, QString>(
    "pqClientMainWindow/selectionEditorDock/selectionEditorPanel/ElementTypeInfo", "text"),

  // TODO investigate
  QPair<QString, QString>("pqClientMainWindow/informationDock/informationWidgetFrame/"
                          "informationScrollArea/qt_scrollarea_viewport/informationWidget/extents",
    "text"),
  QPair<QString, QString>("pqClientMainWindow/informationDock/informationWidgetFrame/"
                          "informationScrollArea/qt_scrollarea_viewport/informationWidget/bounds",
    "text")
};

/**
 * Ignore list of {<widget_regex>, <attribute>}, where <widget_regex> is a regular expression
 * tested against the name given by QtTesting pqObjectNaming, and <attribute> is the Qt attribute
 * to ignore (empty for all)
 * see recursiveFindUntranslatedStrings() for attributes and isIgnored() for usage
 *
 * Please prefer using strings when possible, as regexes are way slower than string comparison.
 */
static const QPair<QRegularExpression, QString> TRANSLATION_IGNORE_REGEXES[] = {
  // Text that SHOULD NOT be translated
  // -----
  // application name
  QPair<QRegularExpression, QString>(QRegularExpression("^pqClientMainWindow$"), "windowTitle"),
  // image preview resolutions
  QPair<QRegularExpression, QString>(
    QRegularExpression("^pqClientMainWindow/menubar/menu_View/Preview/QAction[0-9]*$"), ""),
  // memory inspector display system infos (hostname, memory footprint)
  QPair<QRegularExpression, QString>(
    QRegularExpression("^pqClientMainWindow/memoryInspectorDock/dockWidgetContents/splitter/"
                       "configView/qt_scrollarea_viewport/[01]QFrame0/[01]QLabel0$"),
    "toolTip"),
  QPair<QRegularExpression, QString>(
    QRegularExpression("^pqClientMainWindow/memoryInspectorDock/dockWidgetContents/splitter/"
                       "configView/qt_scrollarea_viewport/[01]QFrame0/[01]QLabel1$"),
    "text"),
  QPair<QRegularExpression, QString>(
    QRegularExpression("^pqClientMainWindow/memoryInspectorDock/dockWidgetContents/splitter/"
                       "configView/qt_scrollarea_viewport/[01]QFrame1/[01]QLabel[012]$"),
    "text"),
  QPair<QRegularExpression, QString>(
    QRegularExpression("^pqClientMainWindow/memoryInspectorDock/dockWidgetContents/splitter/"
                       "configView/qt_scrollarea_viewport/[01]QFrame1/[01]QLabel2$"),
    "text"),
  QPair<QRegularExpression, QString>(
    QRegularExpression("^pqClientMainWindow/memoryInspectorDock/dockWidgetContents/splitter/"
                       "configView/qt_scrollarea_viewport/[01]QFrame1/[01]QProgressBar0$"),
    "text"),
  QPair<QRegularExpression, QString>(
    QRegularExpression("^pqClientMainWindow/memoryInspectorDock/dockWidgetContents/splitter/"
                       "configView/qt_scrollarea_viewport/[01]QFrame1/[01]QProgressBar1$"),
    "text"),

  // Text translated by Qt: our test string will not be found
  QPair<QRegularExpression, QString>(
    QRegularExpression("^pqClientMainWindow/PluginManagerDialog/buttonBox/[01]QPushButton[0-9]*$"),
    "text"),
  QPair<QRegularExpression, QString>(
    QRegularExpression(
      "^pqClientMainWindow/pqSetBreakpointDialog/ButtonBox/[01]QPushButton[0-9]*$"),
    "text")
};

class pqTranslationsTesting : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqTranslationsTesting(QObject* parent = nullptr);
  ~pqTranslationsTesting() override;

  /**
   * Print a warning if the given object's property value does not begin
   * with "_TranslationTesting"s.
   */
  void printWarningIfUntranslated(QObject*, const char*) const;

  /**
   * Check if a widget should be ignored by the testing.
   */
  bool shouldBeIgnored(QWidget*) const;

  /**
   * Recursively call `printWarningIfUntranslated` on UI-visible properties
   * of then given widget and its children actions.
   */
  void recursiveFindUntranslatedStrings(QWidget*) const;

  /**
   * Return if the given object's property is ignored either by
   * `TRANSLATION_IGNORE_STRINGS` or `TRANSLATION_IGNORE_REGEXES`.
   * @return true if object's property is ignored, false otherwise.
   */
  bool isIgnored(QObject*, const char*) const;

  /**
   * Call `recursiveFindUntranslatedStrings` on the ParaView main window.
   */
  void onStartup();

  /**
   * Do nothing but still implemented because of the interface
   * pqAutoStartInterface it implements.
   */
  void onShutdown() {}

private:
  Q_DISABLE_COPY(pqTranslationsTesting)
};

#endif
