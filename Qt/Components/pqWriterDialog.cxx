/*=========================================================================

   Program: ParaView
   Module:    pqWriterDialog.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqWriterDialog.h"
#include "ui_pqWriterDialog.h"

// Qt includes
#include <QGridLayout>
#include <QPointer>

// ParaView Server Manager includes
#include "pqProxyWidget.h"
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h"

class pqWriterDialog::pqImplementation
{
public:
  vtkWeakPointer<vtkSMProxy> Proxy;
  Ui::pqWriterDialog UI;
  QPointer<pqProxyWidget> ProxyWidget;
  bool HasConfigurableProperties;

  pqImplementation() : HasConfigurableProperties(false) {}
};

//-----------------------------------------------------------------------------
pqWriterDialog::pqWriterDialog(vtkSMProxy *proxy, QWidget *p) : Superclass(p), 
  Implementation(new pqImplementation())
{
  this->Implementation->Proxy = proxy;
  this->Implementation->UI.setupUi(this);
  this->Implementation->ProxyWidget = new pqProxyWidget(proxy, this);
  QVBoxLayout* vbox = new QVBoxLayout(this->Implementation->UI.Container);
  vbox->addWidget(this->Implementation->ProxyWidget);
  vbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum,
      QSizePolicy::Expanding));

  this->Implementation->HasConfigurableProperties =
    this->Implementation->ProxyWidget->filterWidgets(true);
}

//-----------------------------------------------------------------------------
pqWriterDialog::~pqWriterDialog()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
bool pqWriterDialog::hasConfigurableProperties()
{
  return this->Implementation->HasConfigurableProperties;
}

//-----------------------------------------------------------------------------
void pqWriterDialog::accept()
{
  this->Implementation->ProxyWidget->apply();
  this->done(QDialog::Accepted);
}

//-----------------------------------------------------------------------------
void pqWriterDialog::reject()
{
  this->Implementation->ProxyWidget->reset();
  this->done(QDialog::Rejected);
}
