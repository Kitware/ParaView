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
#include "vtkSMProxy.h"

#include <QPointer>
#include <QStyle>

class pqProxyWidgetDialog::pqInternals
{
  QPointer<QWidget> Container;
  QPointer<pqProxyWidget> ProxyWidget;
  QString SettingsKey;
  bool SearchEnabled;
  bool Resized;
  bool GeometryLoaded;

  QString KEY_VALID()
    {
    Q_ASSERT(!this->SettingsKey.isEmpty());
    return QString("%1.Valid").arg(this->SettingsKey);
    }
  QString KEY_GEOMETRY()
    {
    Q_ASSERT(!this->SettingsKey.isEmpty());
    return QString("%1.Geometry").arg(this->SettingsKey);
    }
  QString KEY_SEARCHBOX()
    {
    Q_ASSERT(!this->SettingsKey.isEmpty());
    return QString("%1.SearchBox").arg(this->SettingsKey);
    }
public:
  Ui::ProxyWidgetDialog Ui;
  vtkSMProxy* Proxy;
  bool HasVisibleWidgets;

  pqInternals(vtkSMProxy* proxy, pqProxyWidgetDialog* self,
    const QStringList& properties = QStringList()) :
    SearchEnabled(false),
    Resized(false),
    GeometryLoaded(false),
    Proxy(proxy),
    HasVisibleWidgets(false)
    {
    Q_ASSERT(proxy != NULL);

    Ui::ProxyWidgetDialog& ui = this->Ui;
    ui.setupUi(self);
    ui.SearchBox->setVisible(this->SearchEnabled);
    ui.SearchBox->setAdvancedSearchEnabled(true);
    ui.SearchBox->setAdvancedSearchActive(false);
    self->connect(ui.SearchBox, SIGNAL(advancedSearchActivated(bool)), SLOT(filterWidgets()));
    self->connect(ui.SearchBox, SIGNAL(textChanged(const QString&)), SLOT(filterWidgets()));

    // There should be no changes initially, so disable the Apply button
    ui.ApplyButton->setEnabled(false);

    QWidget *container = new QWidget(self);
    container->setObjectName("Container");
    QVBoxLayout* vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
    this->Container = container;

    // Set up the widget for the proxy
    pqProxyWidget *widget = properties.size() > 0?
      new pqProxyWidget(proxy, properties, container):
      new pqProxyWidget(proxy, container);
    widget->setObjectName("ProxyWidget");
    this->HasVisibleWidgets = widget->filterWidgets(true);
    vbox->addWidget(widget);
    this->ProxyWidget = widget;

    // Set some icons for the buttons
    QStyle* applicationStyle = QApplication::style();
    ui.RestoreDefaultsButton->
      setIcon(applicationStyle->standardIcon(QStyle::SP_BrowserReload));
    ui.SaveButton->
      setIcon(applicationStyle->standardIcon(QStyle::SP_DialogSaveButton));
    ui.ApplyButton->
      setIcon(applicationStyle->standardIcon(QStyle::SP_DialogApplyButton));
    ui.CancelButton->
      setIcon(applicationStyle->standardIcon(QStyle::SP_DialogCancelButton));
    ui.OKButton->
      setIcon(applicationStyle->standardIcon(QStyle::SP_DialogOkButton));

    QObject::connect(self, SIGNAL(accepted()), widget, SLOT(apply()));
    QObject::connect(self, SIGNAL(accepted()), self, SLOT(onAccepted()));
    QObject::connect(widget, SIGNAL(changeAvailable()),
       self, SLOT(onChangeAvailable()));

    // When restoring defaults, first restore the defaults in the
    // server manager, then reset the values from the server manager
    // after the defaults have been restored.
    QObject::connect(ui.RestoreDefaultsButton, SIGNAL(clicked()),
      widget, SLOT(onRestoreDefaults()));
    QObject::connect(ui.RestoreDefaultsButton, SIGNAL(clicked()),
      widget, SLOT(reset()));

    QObject::connect(ui.SaveButton, SIGNAL(clicked()),
      widget, SLOT(onSaveAsDefaults()));

    QObject::connect(ui.ApplyButton, SIGNAL(clicked()), self, SIGNAL(accepted()));
    QObject::connect(ui.ApplyButton, SIGNAL(clicked()), widget, SLOT(apply()));

    QObject::connect(ui.CancelButton, SIGNAL(clicked()), widget, SLOT(reset()));
    QObject::connect(ui.CancelButton, SIGNAL(clicked()), self, SLOT(reject()));

    QObject::connect(ui.OKButton, SIGNAL(clicked()), self, SLOT(accept()));

    QSpacerItem* spacer = new QSpacerItem(0, 6, QSizePolicy::Fixed,
      QSizePolicy::MinimumExpanding);
    vbox->addItem(spacer);
    }

  void resize(QWidget* self)
    {
    if (this->Resized) { return; }
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

    // Finaly set the maximum and current dialog size
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
  bool enableSearchBar() const
    {
    return this->SearchEnabled;
    }

  void filterWidgets()
    {
    Ui::ProxyWidgetDialog& ui = this->Ui;
    if (this->SearchEnabled)
      {
      this->ProxyWidget->filterWidgets(
        ui.SearchBox->isAdvancedSearchActive(),
        ui.SearchBox->text());
      }
    else
      {
      this->ProxyWidget->filterWidgets(true);
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
      this->GeometryLoaded = self->restoreGeometry(
        settings->value(this->KEY_GEOMETRY()).toByteArray());
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
};

//-----------------------------------------------------------------------------
pqProxyWidgetDialog::pqProxyWidgetDialog(vtkSMProxy* proxy, QWidget* parentObject)
  : Superclass(parentObject),
  Internals(new pqProxyWidgetDialog::pqInternals(proxy, this))
{
}

//-----------------------------------------------------------------------------
pqProxyWidgetDialog::pqProxyWidgetDialog(vtkSMProxy* proxy,
  const QStringList& properties, QWidget* parentObject)
  : Superclass(parentObject),
  Internals(new pqProxyWidgetDialog::pqInternals(proxy, this, properties))
{
}

//-----------------------------------------------------------------------------
pqProxyWidgetDialog::~pqProxyWidgetDialog()
{
  delete this->Internals;
  this->Internals = NULL;
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
void pqProxyWidgetDialog::showEvent(QShowEvent *evt)
{
  this->Internals->resize(this);
  this->Superclass::showEvent(evt);
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::hideEvent(QHideEvent *evt)
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
  Ui::ProxyWidgetDialog &ui = this->Internals->Ui;
  ui.ApplyButton->setEnabled(true);
}

//-----------------------------------------------------------------------------
void pqProxyWidgetDialog::onAccepted()
{
  Ui::ProxyWidgetDialog &ui = this->Internals->Ui;
  ui.ApplyButton->setEnabled(false);
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
