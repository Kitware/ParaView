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
    vbox->setMargin(0);
    vbox->setSpacing(0);

    pqProxyWidget *widget = properties.size() > 0?
      new pqProxyWidget(proxy, properties, container):
      new pqProxyWidget(proxy, container);
    widget->setObjectName("ProxyWidget");
    widget->filterWidgets(true);
    QObject::connect(self, SIGNAL(accepted()), widget, SLOT(apply()));
    vbox->addWidget(widget);

    QSpacerItem* spacer = new QSpacerItem(0, 6,QSizePolicy::Fixed,
      QSizePolicy::MinimumExpanding);
    vbox->addItem(spacer);

    // We need to know the size of the dialog so (ideally) when first displayed
    // the scroll area isn't visible (i.e. the proxy widget isn't squished in
    // the scroll area).
    // To do so, we force the layout to compute by showing it offscreen with
    // the attribute WA_DontShowOnScreen and get the dialog size this way
    ui.Layout->addWidget(container);
    self->setAttribute(Qt::WA_DontShowOnScreen);
    self->show();
    QSize dialogSize = self->size();
    self->hide();
    self->setAttribute(Qt::WA_DontShowOnScreen, false);

    // Now that we have the dialog ideal size, we can add the container widget
    // to the scroll area.
    ui.Layout->removeWidget(container);
    QScrollArea* scrollArea = new QScrollArea(self);
    scrollArea->setObjectName("scrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    ui.Layout->insertWidget(0, scrollArea);
    scrollArea->setWidget(container);

    // Limit the dialog max size to be as big as the application
    self->setMaximumSize(pqCoreUtilities::mainWidget()->size());
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
