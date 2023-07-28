// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxiesWidget_h
#define pqProxiesWidget_h

#include "pqComponentsModule.h"
#include <QMap>
#include <QScopedPointer>
#include <QWidget>

class vtkSMProxy;
class pqView;

/**
 * pqProxiesWidget similar to pqProxyWidget with the difference being that
 * unlike pqProxyWidget, pqProxiesWidget supports showing of multiple proxies in
 * the same widget. Internally, it indeed creates a pqProxyWidget for each of
 * the added proxies.
 *
 * To use this class, add all proxies to show in this panel using addProxy()
 * and then call updateLayout() to update the layout.
 *
 * pqProxiesWidget supports grouping of proxies in components (separated by
 * using a pqExpanderButton). To use pqExpanderButton simply use a non-empty
 * componentName when calling addProxy().
 *
 * pqProxiesWidget provides API such as filterWidgets(), apply(), etc. the
 * calls to which are simply forwarded to the internal pqProxyWidget instances.
 * Similarly, pqProxiesWidget also forwards signals fired by pqProxyWidget such
 * as changeFinished(), changeAvailable(), and restartRequired().
 */
class PQCOMPONENTS_EXPORT pqProxiesWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqProxiesWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
  ~pqProxiesWidget() override;

  /**
   * Returns a map indicating which expand buttons are expanded and
   * which ones aren't. Useful to save and then restore the state when resetting
   * the panel. The key is the text used for **componentName** argument to `addProxy`
   * call. The value is true for expanded, false otherwise.
   */
  QMap<QString, bool> expanderState() const;

  /**
   * Set the expander state. If any key is unrecognized, it will be
   * silently ignored. State for expanders not referred to in the \c state is
   * left unchanged.
   */
  void setExpanderState(const QMap<QString, bool>& state);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Remove all proxy widgets added to the panel.
   */
  void clear();

  /**
   * Add the widgets for a proxy.
   */
  void addProxy(vtkSMProxy*, const QString& componentName = QString(),
    const QStringList& properties = QStringList(), bool applyChangesImmediately = false,
    bool showHeadersFooters = true);

  /**
   * Call this method once after all proxies have been added (or after clear)
   * to update the layout for the panel.
   */
  void updateLayout();

  /**
   * Updates the property widgets shown based on the filterText or
   * show_advanced flag. Calling filterWidgets() without any arguments will
   * result in the panel showing all the non-advanced properties.
   * Returns true, if any widgets were shown.
   */
  bool filterWidgets(bool show_advanced = false, const QString& filterText = QString());

  /**
   * Accepts the property widget changes changes.
   */
  void apply() const;

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

Q_SIGNALS:
  /**
   * This signal is fired as soon as the user starts editing in the widget. The
   * editing may not be complete.
   */
  void changeAvailable(vtkSMProxy* proxy);

  /**
   * This signal is fired as soon as the user is done with making an atomic
   * change. changeAvailable() is always fired before changeFinished().
   */
  void changeFinished(vtkSMProxy* proxy);

  /**
   * Indicates that a restart of the program is required for the setting
   * to take effect.
   */
  void restartRequired(vtkSMProxy* proxy);

private Q_SLOTS:
  void triggerChangeFinished();
  void triggerChangeAvailable();
  void triggerRestartRequired();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqProxiesWidget)

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif
