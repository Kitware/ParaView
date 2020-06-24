/*=========================================================================

   Program: ParaView
   Module:  pqSettingsDialog.h

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
  * Set to true if a setting that requires a restart to take effect
  * is modified. Made static to persist across instantiations of
  * this class.
  */
  static bool ShowRestartRequired;
};

#endif
