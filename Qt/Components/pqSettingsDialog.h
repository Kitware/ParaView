// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSettingsDialog_h
#define pqSettingsDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

class pqServer;
class QAbstractButton;
class vtkSMProperty;

/**
 * pqSettingsDialog provides a dialog for controlling application settings
 * for a ParaView application. It's designed to look show all proxies
 * registered under the "settings" group by default, unless specified in the
 * proxyLabelsToShow constructor argument. For each proxy, it creates
 * a pqProxyWidget and adds that to a tab-widget contained in the dialog.
 */
class PQCOMPONENTS_EXPORT pqSettingsDialog : public QDialog
{
  Q_OBJECT;
  typedef QDialog Superclass;

public:
  pqSettingsDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{},
    const QStringList& proxyLabelsToShow = QStringList());
  ~pqSettingsDialog() override;

  /**
   * Make the tab with the given title text current, if possible.
   * Does nothing if title is invalid or not present.
   */
  void showTab(const QString& title);

private Q_SLOTS:
  void clicked(QAbstractButton*);
  void onAccepted();
  void onRejected();

  void onTabIndexChanged(int index);
  void onChangeAvailable();
  void showRestartRequiredMessage();

  void filterPanelWidgets();

  /**
   * Callback for when pqServerManagerModel notifies the application that the
   * server is being removed. We close the dialog.
   */
  void serverRemoved(pqServer*);

Q_SIGNALS:
  void filterWidgets(bool showAdvanced, const QString& text);

private:
  Q_DISABLE_COPY(pqSettingsDialog)
  class pqInternals;
  pqInternals* Internals;

  /**
   * Set to true if a setting that requires a restart to take effect is
   * modified. Made static to persist across instantiations of this class.
   */
  static bool ShowRestartRequired;
};

#endif
