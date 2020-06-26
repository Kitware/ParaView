/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef pqProxyWidgetDialog_h
#define pqProxyWidgetDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

class vtkSMProxy;

/**
* pqProxyWidgetDialog is used to show properties of any proxy in a dialog. It
* simply wraps the pqProxyWidget for the proxy in a dialog with Apply,
* Cancel, and Ok buttons. Tool buttons (QPushButtons with only icons) are
* also provided to save the currently applied settings as default properties
* as well as to reset the defaults to the application defaults.
*/
class PQCOMPONENTS_EXPORT pqProxyWidgetDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqProxyWidgetDialog(
    vtkSMProxy* proxy, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  pqProxyWidgetDialog(vtkSMProxy* proxy, const QStringList& properties, QWidget* parent = nullptr,
    Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqProxyWidgetDialog() override;

  /**
  * Returns whether that dialog has any visible widgets.
  */
  bool hasVisibleWidgets() const;

  /**
  * Returns true if the proxy has any advanced properties.
  * This may be useful to determine if the search bar should be enabled or
  * not.
  */
  bool hasAdvancedProperties() const;

  /**
  * Returns the proxy.
  */
  vtkSMProxy* proxy() const;

  /**
  * Get/set whether the search-bar should be shown on the dialog.
  * Default is false. If search-bar is not enabled, the dialog cannot support
  * default/advanced viewing modes either and hence it will simply show all
  * property widgets.
  */
  void setEnableSearchBar(bool val);
  bool enableSearchBar() const;

  /**
  * By default, the pqProxyWidgetDialog will show advanced properties on the
  * proxy either by default (if EnableSearchBar is false), or based on the
  * ui-element in search bar (if EnableSearchBar is true). However, there may
  * be cases when an application doesn't want to show the advanced properties.
  * In that case, you can use this method to hide all advanced properties.
  * If EnableSearchBar is true and HideAdvancedProperties is false, then the
  * gear-icon shown next to search bar to allow user to toggle the advanced
  * properties is not shown.
  */
  void setHideAdvancedProperties(bool val);
  bool hideAdvancedProperties() const;

  /**
  * When set to true (default is false), pqProxyWidgetDialog will not show the
  * 'Apply' and 'Reset' buttons. This has one major implication, however.
  * Whenever the user changes a widget in the UI, the corresponding property
  * on the proxy will immediately be changed. Thus, even if the user hits
  * 'Cancel', the proxy will be left in modified state.
  */
  void setApplyChangesImmediately(bool val);
  bool applyChangesImmediately() const;

  /**
  * Set the new setting key that will be used to restore/save the advanced
  * button check state. If the given key is valid (i.e. not empty), the
  * button state will be restored from the key value.
  * The old key is left unchanged in the setting to whatever its last value
  * was. Although returned, removing (or not) the old key is up to the user.
  * \sa settingKey property
  */
  QString setSettingsKey(const QString& key);

protected:
  /**
  * Overridden to resize widget before showing it the first time.
  */
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void done(int r) override;

private Q_SLOTS:
  void filterWidgets();
  void onChangeAvailable();

  /**
  * Callbacks for buttons on the UI.
  */
  void onApply();
  void onReset();
  void onRestoreDefaults();
  void onSaveAsDefaults();

private:
  Q_DISABLE_COPY(pqProxyWidgetDialog)

  friend class pqInternals;
  class pqInternals;
  pqInternals* Internals;
};

#endif
