// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxyWidget_h
#define pqProxyWidget_h

#include "pqComponentsModule.h"
#include <QSet>
#include <QWidget>

class pqPropertyWidget;
class pqView;
class vtkSMProperty;
class vtkSMProxy;
class QPoint;

/**
 * pqProxyWidget represents a panel for a vtkSMProxy. pqProxyWidget creates
 * widgets for each of the properties (or proxy groups) of the proxy respecting
 * any registered pqPropertyWidgetInterface instances to create custom widgets.
 * pqProxyWidget is used by pqPropertiesPanel to create panels for the
 * source/filter and the display/representation sections of the panel.
 *
 * pqProxyWidget doesn't show any widgets in the panel by default (after
 * constructor). Use filterWidgets() or updatePanel() to show widgets matching
 * criteria.
 *
 * Note: This class replaces pqProxyPanel (and subclasses). pqProxyPanel is
 * still available (and supported) for backwards compatibility.
 */
class PQCOMPONENTS_EXPORT pqProxyWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqProxyWidget(vtkSMProxy* proxy, const QStringList& properties,
    std::initializer_list<QString> defaultLabels, std::initializer_list<QString> advancedLabels,
    bool showHeadersFooters = true, QWidget* parent = nullptr,
    Qt::WindowFlags flags = Qt::WindowFlags{});

  pqProxyWidget(vtkSMProxy* proxy, std::initializer_list<QString> defaultLabels,
    std::initializer_list<QString> advancedLabels, QWidget* parent = nullptr,
    Qt::WindowFlags flags = Qt::WindowFlags{});

  pqProxyWidget(vtkSMProxy* proxy, const QStringList& properties, bool showHeadersFooters = true,
    QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});

  pqProxyWidget(vtkSMProxy* proxy, QWidget* parent, Qt::WindowFlags flags = Qt::WindowFlags{});

  pqProxyWidget(vtkSMProxy* proxy);

  ~pqProxyWidget() override;

  /**
   * Returns the proxy this panel shows.
   */
  vtkSMProxy* proxy() const;

  /**
   * When set to true, whenever the widget changes, the values are immediately
   * pushed to the ServerManager property without having to wait for apply().
   * This is used for panels such as the display panel. Default is false.
   */
  void setApplyChangesImmediately(bool value);
  bool applyChangesImmediately() const { return this->ApplyChangesImmediately; }

  /**
   * When this is true, the panel uses a descriptive layout where the
   * documentation for properties is used instead of their labels. pqProxyWidget
   * automatically adopts this style of layout if `<UseDocumentationForLabels />`
   * hint is present in the proxy.
   */
  bool useDocumentationForLabels() const { return this->UseDocumentationForLabels; }

  /**
   * Returns a new widget that has the label and a h-line separator. This is
   * used on the pqProxyWidget to separate groups. Other widgets can use it for
   * the same purpose, as needed.
   */
  static QWidget* newGroupLabelWidget(const QString& label, QWidget* parentWidget,
    const QList<QWidget*>& buttons = QList<QWidget*>());

  /**
   * Returns true of the proxy provided has XML hints indicating that labels
   * should use documentation instead of the XML label for the widgets in the
   * UI.
   */
  static bool useDocumentationForLabels(vtkSMProxy* proxy);

  enum DocumentationType
  {
    NONE,
    USE_DESCRIPTION,
    USE_SHORT_HELP,
    USE_LONG_HELP
  };

  /**
   * Returns formatted (HTML or plainText) documentation for the property.
   * \c type cannot be NONE.
   */
  static QString documentationText(
    vtkSMProperty* property, DocumentationType type = USE_DESCRIPTION);

  /**
   * Returns formatted (HTML or plainText) documentation for the proxy.
   * \c type cannot be NONE.
   */
  static QString documentationText(vtkSMProxy* property, DocumentationType type = USE_DESCRIPTION);

  /**
   * Returns true if the proxy has XML hints indicating that the panel should
   * show a header label for the documentation. pqProxyWidget uses the
   * `<ShowProxyDocumentationInPanel />` hint for this purpose.
   */
  static DocumentationType showProxyDocumentationInPanel(vtkSMProxy* proxy);

  ///@{
  /**
   * pqProxyWidget shows widgets for properties in two configurations: basic and
   * advanced. Properties on Proxies can have `PanelVisibility` set to an
   * arbitrary string. This API allows the application to classify the
   * panel-visibility strings into the two configuration categories.
   *
   * By default, defaultVisibilityLabels is set to `{ "default" }` and
   * advancedVisibilityLabels is set to `{ "advanced" }`.
   *
   * Note that "never" is reserved and always interpreted and never show the
   * property (unless explicitly requested in constructor arguments).
   */
  const QSet<QString>& defaultVisibilityLabels() const { return this->DefaultVisibilityLabels; }
  const QSet<QString>& advancedVisibilityLabels() const { return this->AdvancedVisibilityLabels; }
  ///@}

  void showContextMenu(const QPoint& pt, pqPropertyWidget* propWidget);

Q_SIGNALS:
  /**
   * This signal is fired as soon as the user starts editing in the widget. The
   * editing may not be complete.
   */
  void changeAvailable();

  /**
   * This signal is fired as soon as the user is done with making an atomic
   * change. changeAvailable() is always fired before changeFinished().
   */
  void changeFinished();

  /**
   * Indicates that a restart of the program is required for the setting
   * to take effect.
   */
  void restartRequired();

public Q_SLOTS:
  /**
   * Updates the property widgets shown based on the filterText or
   * show_advanced flag. Calling filterWidgets() without any arguments will
   * result in the panel showing all the non-advanced properties.
   * Returns true, if any widgets were shown.
   */
  bool filterWidgets(bool show_advanced = false, const QString& filterText = QString());

  /**
   * Show any interactive widget linked to a specific output port
   * this proxyWdiget has. See XML hint \verbatim<WidgetVisibilityLink port="X" />\endverbatim.
   */
  void showLinkedInteractiveWidget(int portIndex, bool show, bool changeFocus);

  /**
   * Accepts the property widget changes changes.
   */
  virtual void apply() const;

  /**
   * Cleans the property widget changes and resets the widgets.
   */
  void reset() const;

  /**
   * Set the current view to use to show 3D widgets, if any for the panel.
   */
  void setView(pqView*);

  /**
   * Same as calling filterWidgets() with the arguments specified to the most
   * recent call to filterWidgets().
   */
  void updatePanel();

  /**
   * Restores application default proxy settings.
   * Returns true if any properties were modified.
   */
  virtual bool restoreDefaults();

  /**
   * Saves settings as defaults for proxy
   */
  void saveAsDefaults();

  /**
   * create a widget for a property.
   */
  static pqPropertyWidget* createWidgetForProperty(
    vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parentObj);

protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

  void applyInternal() const;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Called when a pqPropertyWidget fires changeFinished() signal.
   * This callback fires changeFinished() signal and handles AutoUpdateVTKObjects.
   */
  virtual void onChangeFinished();

private:
  /**
   * create all widgets
   */
  void createWidgets(const QStringList& properties = QStringList());

  /**
   * create individual property widgets.
   */
  void createPropertyWidgets(const QStringList& properties = QStringList());

  /**
   * create 3D widgets, if any.
   */
  void create3DWidgets();

  Q_DISABLE_COPY(pqProxyWidget);

  QSet<QString> DefaultVisibilityLabels;
  QSet<QString> AdvancedVisibilityLabels;
  bool ApplyChangesImmediately;
  bool UseDocumentationForLabels;
  bool ShowHeadersFooters = false;
  class pqInternals;
  pqInternals* Internals;
};

#endif
