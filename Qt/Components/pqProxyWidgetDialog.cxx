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

#include "pqCoreUtilities.h"
#include "pqProxyWidget.h"

#include <QScrollArea>

class pqProxyWidgetDialog::pqInternals
{
public:
  Ui::ProxyWidgetDialog Ui;

  pqInternals(vtkSMProxy* proxy, pqProxyWidgetDialog* self, 
    const QStringList& properties = QStringList())
    {
    Q_ASSERT(proxy != NULL);

    Ui::ProxyWidgetDialog& ui = this->Ui;
    ui.setupUi(self);

    QObject::connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)),
      self, SLOT(buttonClicked(QAbstractButton*)));

    QWidget *container = new QWidget(self);
    container->setObjectName("Container");
    QVBoxLayout* vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    pqProxyWidget *widget = properties.size() > 0?
      new pqProxyWidget(proxy, properties, container):
      new pqProxyWidget(proxy, container);
    widget->setObjectName("ProxyWidget");
    widget->filterWidgets(true);
    QObject::connect(self, SIGNAL(accepted()), widget, SLOT(apply()));
    vbox->addWidget(widget);

    QSpacerItem* spacer = new QSpacerItem(0, 6, QSizePolicy::Fixed,
      QSizePolicy::MinimumExpanding);
    vbox->addItem(spacer);

    /// Setup the scroll area. Its minimum size is set to fully contain the
    /// proxy widget so we're sure it'll be completly displayed
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
    self->resize(dialogSize);
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
void pqProxyWidgetDialog::buttonClicked(QAbstractButton* button)
{
  Ui::ProxyWidgetDialog& ui = this->Internals->Ui;
  if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
    {
    emit this->accepted();
    }
}
