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
#include "pqProxyWidgetDialog.h"
#include "ui_pqProxyWidgetDialog.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqProxyWidget.h"
#include "pqSettings.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <QPointer>
#include <QStyle>

#include <cassert>

class pqProxyWidgetDialog::pqInternals
{
  QPointer<QWidget> Container;
  QPointer<pqProxyWidget> ProxyWidget;
  QString SettingsKey;
  bool SearchEnabled;
  bool Resized;
  bool GeometryLoaded;
  bool HideAdvancedProperties;

  QString KEY_VALID()
  {
    assert(!this->SettingsKey.isEmpty());
    return QString("%1.Valid").arg(this->SettingsKey);
  }
  QString KEY_GEOMETRY()
  {
    assert(!this->SettingsKey.isEmpty());
    return QString("%1.Geometry").arg(this->SettingsKey);
  }
  QString KEY_SEARCHBOX()
  {
    assert(!this->SettingsKey.isEmpty());
    return QString("%1.SearchBox").arg(this->SettingsKey);
  }

  /// Returns true if the proxy has any advanced properties.
  static bool hasAdvancedProperties(vtkSMProxy* proxy)
  {
    if (!proxy)
    {
      return false;
    }
    vtkSmartPointer<vtkSMPropertyIterator> iter;
    iter.TakeReference(proxy->NewPropertyIterator());
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      if (vtkSMProperty* prop = iter->GetProperty())
      {
        if (prop->GetPanelVisibility() && strcmp(prop->GetPanelVisibility(), "advanced") == 0)
        {
          return true;
        }
      }
    }
    return false;
  }

public:
  Ui::ProxyWidgetDialog Ui;
  vtkWeakPointer<vtkSMProxy> Proxy;
  bool HasVisibleWidgets;
  bool HasAdvancedProperties;

  pqInternals(
    vtkSMProxy* proxy, pqProxyWidgetDialog* self, const QStringList& properties = QStringList())
    : SearchEnabled(false)
    , Resized(false)
    , GeometryLoaded(false)
    , HideAdvancedProperties(false)
    , Proxy(proxy)
    , HasVisibleWidgets(false)
    , HasAdvancedProperties(hasAdvancedProperties(proxy))
  {
    assert(proxy != nullptr);

    Ui::ProxyWidgetDialog& ui = this->Ui;
    ui.setupUi(self);
    ui.SearchBox->setVisible(this->SearchEnabled);
    ui.SearchBox->setAdvancedSearchEnabled(!this->HideAdvancedProperties);
    ui.SearchBox->setAdvancedSearchActive(false);
    self->connect(ui.SearchBox, SIGNAL(advancedSearchActivated(bool)), SLOT(filterWidgets()));
    self->connect(ui.SearchBox, SIGNAL(textChanged(const QString&)), SLOT(filterWidgets()));

    QWidget* container = new QWidget(self);
    container->setObjectName("Container");
    QVBoxLayout* vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
    this->Container = container;

    // Set up the widget for the proxy
    pqProxyWidget* widget = properties.size() > 0 ? new pqProxyWidget(proxy, properties, container)
                                                  : new pqProxyWidget(proxy, container);
    widget->setObjectName("ProxyWidget");
    this->HasVisibleWidgets = widget->filterWidgets(true);
    vbox->addWidget(widget);
    this->ProxyWidget = widget;

    // Set some icons for the buttons
    QStyle* applicationStyle = QApplication::style();
    ui.RestoreDefaultsButton->setIcon(applicationStyle->standardIcon(QStyle::SP_BrowserReload));
    ui.SaveButton->setIcon(applicationStyle->standardIcon(QStyle::SP_DialogSaveButton));
    ui.ApplyButton->setIcon(applicationStyle->standardIcon(QStyle::SP_DialogApplyButton));
    ui.ResetButton->setIcon(applicationStyle->standardIcon(QStyle::SP_DialogResetButton));
    ui.CancelButton->setIcon(applicationStyle->standardIcon(QStyle::SP_DialogCancelButton));
    ui.OKButton->setIcon(applicationStyle->standardIcon(QStyle::SP_DialogOkButton));

    self->connect(ui.RestoreDefaultsButton, SIGNAL(clicked()), SLOT(onRestoreDefaults()));
    self->connect(ui.SaveButton, SIGNAL(clicked()), SLOT(onSaveAsDefaults()));
    self->connect(ui.ApplyButton, SIGNAL(clicked()), SLOT(onApply()));
    self->connect(ui.ResetButton, SIGNAL(clicked()), SLOT(onReset()));
    self->connect(ui.OKButton, SIGNAL(clicked()), SLOT(accept()));
    self->connect(ui.CancelButton, SIGNAL(clicked()), SLOT(reject()));
    self->connect(widget, SIGNAL(changeAvailable()), SLOT(onChangeAvailable()));

    QSpacerItem* spacer = new QSpacerItem(0, 6, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    vbox->addItem(spacer);

    // There should be no changes initially, so disable the Apply button
    this->updateButtons(/*dirty*/ false);
  }

  void resize(QWidget* self)
  {
    if (this->Resized)
    {
      return;
    }
    this->Resized = true;

    Ui::ProxyWidgetDialog& ui = this->Ui;
    QWidget* container = this->Container;

    /// Setup the scroll area. Its minimum size is set to fully contain the
    /// proxy widget so we're sure it'll be completely displayed
    ui.scrollArea->setWidget(container);
    QSize oldMinSize = ui.scrollArea->minimumSize();
    ui.scrollArea->setMinimumSize(container->size());

    /// Get the dialog size. It's the layout minSize since the widget
    /// can't be any smaller right now.
    QSize dialogSize = self->layout()->minimumSize();

    /// Reset the scroll area so it can actually be used.
    ui.scrollArea->setMinimumSize(oldMinSize);

    // Finally set the maximum and current dialog size
    self->setMaximumSize(pqCoreUtilities::mainWidget()->size());
    if (this->GeometryLoaded == false)
    {
      self->resize(dialogSize);
    }
  }

  void setEnableSearchBar(bool val)
  {
    if (val != this->SearchEnabled)
    {
      this->SearchEnabled = val;
      Ui::ProxyWidgetDialog& ui = this->Ui;
      ui.SearchBox->setVisible(val);
      this->filterWidgets();
    }
  }
  bool enableSearchBar() const { return this->SearchEnabled; }

  void setHideAdvancedProperties(bool val)
  {
    if (val != this->HideAdvancedProperties)
    {
      this->HideAdvancedProperties = val;
      Ui::ProxyWidgetDialog& ui = this->Ui;
      ui.SearchBox->setAdvancedSearchEnabled(!val);
      this->filterWidgets();
    }
  }
  bool hideAdvancedProperties() const { return this->HideAdvancedProperties; }

  void filterWidgets()
  {
    Ui::ProxyWidgetDialog& ui = this->Ui;
    if (this->SearchEnabled)
    {
      this->ProxyWidget->filterWidgets(
        ui.SearchBox->isAdvancedSearchActive() && ui.SearchBox->isAdvancedSearchEnabled(),
        ui.SearchBox->text());
    }
    else
    {
      this->ProxyWidget->filterWidgets(!this->HideAdvancedProperties);
    }
  }

  QString setSettingsKey(QWidget* self, const QString& key)
  {
    QString old = this->SettingsKey;
    this->SettingsKey = key;
    pqSettings* settings = pqApplicationCore::instance()->settings();
    if (!key.isEmpty() && settings->value(this->KEY_VALID(), false).toBool())
    {
      this->Ui.SearchBox->setSettingKey(this->KEY_SEARCHBOX());
      this->filterWidgets();
      this->GeometryLoaded =
        self->restoreGeometry(settings->value(this->KEY_GEOMETRY()).toByteArray());
    }
    else
    {
      this->Ui.SearchBox->setSettingKey(key);
    }
    return old;
  }

  void saveSettings(QWidget* self)
  {
    if (!this->SettingsKey.isEmpty())
    {
      pqSettings* settings = pqApplicationCore::instance()->settings();
      settings->setValue(this->KEY_VALID(), true);
      settings->setValue(this->KEY_GEOMETRY(), self->saveGeometry());
    }
  }

  /// update the buttons based on whether any property/widget on the panel is
  /// modified.
  void updateButtons(bool dirty)
  {
    Ui::ProxyWidgetDialog& ui = this->Ui;

    // if applying changes immediately, then we're really not dirty.
    dirty = this->applyChangesImmediately() ? false : dirty;

    ui.ApplyButton->setEnabled(dirty);
    ui.ResetButton->setEnabled(dirty);

    ui.RestoreDefaultsButton->setEnabled(!dirty);
    ui.SaveButton->setEnabled(!dirty);
    ui.OKButton->setEnabled(true);
    ui.CancelButton->setEnabled(true);
  }

  /// accept all changes and update buttons.
  void acceptChanges()
  {
    this->ProxyWidget->apply();
    this->updateButtons(false);
  }

  /// reject all changes and update buttons.
  void rejectChanges()
  {
    this->ProxyWidget->reset();
    this->updateButtons(false);
  }

  /// load values from settings.
  bool restoreDefaults()
  {
    if (this->ProxyWidget->restoreDefaults())
    {
      // If values changed, we pretend the user accepted those changes.
      this->acceptChanges();
      return true;
    }
    return false;
  }

  void saveAsDefaults() { this->ProxyWidget->saveAsDefaults(); }

  void setApplyChangesImmediately(bool val)
  {
    this->ProxyWidget->setApplyChangesImmediately(val);
    Ui::ProxyWidgetDialog& ui = this->Ui;
    ui.ApplyButton->setVisible(!val);
    ui.ResetButton->setVisible(!val);
  }

  bool applyChangesImmediately() const { return this->ProxyWidget->applyChangesImmediately(); }
};

//-----------------------------------------------------------------------------
pqProxyWidgetDialog::pqProxyWidgetDialog(
  vtkSMProxy* smproxy, QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqProxyWidgetDialog::pqInternals(smproxy, this))
{
}

//-----------------------------------------------------------------------------
pqProxyWidgetDialog::pqProxyWidgetDialog(
  vtkSMProxy* smproxy, const QStringList& properties, QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqProxyWidgetDialog::pqInternals(smproxy, this, properties))
{
}

//-----------------------------------------------------------------------------
pqProxyWidgetDialog::~pqProxyWidgetDialog()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::filterWidgets()
{
  this->Internals->filterWidgets();
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::setEnableSearchBar(bool val)
{
  this->Internals->setEnableSearchBar(val);
}

//-----------------------------------------------------------------------------
bool pqProxyWidgetDialog::enableSearchBar() const
{
  return this->Internals->enableSearchBar();
}

//-----------------------------------------------------------------------------
bool pqProxyWidgetDialog::hasAdvancedProperties() const
{
  return this->Internals->HasAdvancedProperties;
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::showEvent(QShowEvent* evt)
{
  this->Internals->resize(this);
  this->Superclass::showEvent(evt);
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::hideEvent(QHideEvent* evt)
{
  this->Internals->saveSettings(this);
  this->Superclass::hideEvent(evt);
}

//-----------------------------------------------------------------------------
bool pqProxyWidgetDialog::hasVisibleWidgets() const
{
  return this->Internals->HasVisibleWidgets;
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::onChangeAvailable()
{
  this->Internals->updateButtons(/*dirty=*/true);
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::onApply()
{
  this->Internals->acceptChanges();
  Q_EMIT this->accepted();
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::onReset()
{
  this->Internals->rejectChanges();
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::done(int status)
{
  // this gets called when user presses the Esc key.
  switch (status)
  {
    case QDialog::Accepted:
      this->Internals->acceptChanges();
      break;

    case QDialog::Rejected:
      this->Internals->rejectChanges();
      break;
  }
  this->Superclass::done(status);
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::onRestoreDefaults()
{
  if (this->Internals->restoreDefaults())
  {
    Q_EMIT this->accepted();
  }
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::onSaveAsDefaults()
{
  this->Internals->saveAsDefaults();
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::setHideAdvancedProperties(bool val)
{
  this->Internals->setHideAdvancedProperties(val);
}

//-----------------------------------------------------------------------------
bool pqProxyWidgetDialog::hideAdvancedProperties() const
{
  return this->Internals->hideAdvancedProperties();
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::setApplyChangesImmediately(bool val)
{
  this->Internals->setApplyChangesImmediately(val);
}

//-----------------------------------------------------------------------------
bool pqProxyWidgetDialog::applyChangesImmediately() const
{
  return this->Internals->applyChangesImmediately();
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqProxyWidgetDialog::proxy() const
{
  return this->Internals->Proxy;
}

//-----------------------------------------------------------------------------
QString pqProxyWidgetDialog::setSettingsKey(const QString& key)
{
  return this->Internals->setSettingsKey(this, key);
}
