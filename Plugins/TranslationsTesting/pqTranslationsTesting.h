/*=========================================================================

   Program: ParaView
   Module:    pqTranslationsTesting.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
/**
 * @class   pqTranslationsTesting
 * @brief   PV UI translatability testing class
 *
 * This class recursively access all children QWidgets of the client
 * main window and check if the widget and its actions only contains
 * '_TranslationTesting's in their UI-visible attributes.
 * Some widgets can be ignored by providing their names or a regex
 * matching their names in `TRANSLATION_IGNORE_STRINGS` or
 * `TRANSLATION_IGNORE_REGEXES` below.
 */

#ifndef pqTranslationsTesting_h
#define pqTranslationsTesting_h

#include <QObject>
#include <QRegularExpression>

/*
 * These lists should ideally be empty.
 * For now, these widgets could be debugged.
 */

static const QPair<QString, QString> TRANSLATION_IGNORE_STRINGS[] = {
  QPair<QString, QString>("pqClientMainWindow", "windowTitle"),

  // xml-to-header should handle "category" attribute of "ShowInMenu" tag
  QPair<QString, QString>("pqClientMainWindow/menubar/menuExtractors/Data/QAction0", ""),
  QPair<QString, QString>("pqClientMainWindow/menubar/menuExtractors/Image/QAction0", ""),
  QPair<QString, QString>("pqClientMainWindow/menubar/menuExtractors/Experimental/QAction0", ""),

  // contains host infos, should not be translated
  QPair<QString, QString>("pqClientMainWindow/statusbar/1QProgressBar0", "text"),

  // comes from property link with TimeLabel property of TimeKeeper proxy
  // (pqAnimationTimeWidget.cxx)
  QPair<QString, QString>(
    "pqClientMainWindow/animationViewDock/animationView/AnimationTimeWidget/timeLabel", "text"),
  QPair<QString, QString>(
    "pqClientMainWindow/timeInspectorDock/timeInspectorPanel/AnimationTimeWidget/timeLabel",
    "text"),
  QPair<QString, QString>(
    "pqClientMainWindow/currentTimeToolbar/AnimationTimeWidget/timeLabel", "text"),

  QPair<QString, QString>("pqClientMainWindow/informationDock/informationWidgetFrame/"
                          "informationScrollArea/qt_scrollarea_viewport/informationWidget/extents",
    "text"),
  QPair<QString, QString>("pqClientMainWindow/informationDock/informationWidgetFrame/"
                          "informationScrollArea/qt_scrollarea_viewport/informationWidget/bounds",
    "text"),
  // To investigate
  QPair<QString, QString>(
    "pqClientMainWindow/selectionEditorDock/selectionEditorPanel/ElementTypeInfo", "text"),
  QPair<QString, QString>(
    "pqClientMainWindow/propertiesDock/propertiesPanel/scrollArea/qt_scrollarea_viewport/"
    "scrollAreaWidgetContents/ViewFrame/ProxyPanel/ServerStereoType",
    "toolTip"),
  QPair<QString, QString>(
    "pqClientMainWindow/propertiesDock/propertiesPanel/scrollArea/qt_scrollarea_viewport/"
    "scrollAreaWidgetContents/ViewFrame/ProxyPanel/Background2",
    "toolTip"),
  QPair<QString, QString>("pqClientMainWindow/propertiesDock/propertiesPanel/scrollArea/"
                          "qt_scrollarea_viewport/scrollAreaWidgetContents/ViewFrame/ProxyPanel/"
                          "EnvironmentalBGEditor/stackedWidget/page_1/Color",
    "text"),
  QPair<QString, QString>(
    "pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/"
    "MultiViewWidget1/Container/Frame.0/TitleBar/TitleLabel",
    "text")
};

// Please prefer using strings when possible, as regexes are way slower than string comparison.
static const QPair<QRegularExpression, QString> TRANSLATION_IGNORE_REGEXES[] = {
  QPair<QRegularExpression, QString>(QRegularExpression("^pqClientMainWindow$"), "windowTitle"),
  QPair<QRegularExpression, QString>(
    QRegularExpression("^pqClientMainWindow/menubar/menu_View/Preview/QAction[0-9]*$"), ""),
  // memory inspector display system infos
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

  // translation done by qt, our own string will not be found
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
  ~pqTranslationsTesting();

  /**
   * Print a warning if the given object's property value does not begin
   * with "_TranslationTesting"s.
   */
  void printWarningIfUntranslated(QObject*, const char*) const;

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
