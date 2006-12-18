/*=========================================================================

   Program: ParaView
   Module:    pqTextDisplayPropertiesWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqTextDisplayPropertiesWidget.h"
#include "ui_pqTextDisplayPropertiesWidget.h"

#include "vtkSMProxy.h"

#include <QPointer>

#include "pqPropertyLinks.h"
#include "pqDisplay.h"
#include "pqTextWidgetDisplay.h"

class pqTextDisplayPropertiesWidget::pqInternal : 
  public Ui::pqTextDisplayPropertiesWidget
{
public:
  QPointer<pqDisplay> Display;
  pqPropertyLinks Links;
};


//-----------------------------------------------------------------------------
pqTextDisplayPropertiesWidget::pqTextDisplayPropertiesWidget(QWidget* p)
  : QWidget(p)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(renderAllViews()));
}

//-----------------------------------------------------------------------------
pqTextDisplayPropertiesWidget::~pqTextDisplayPropertiesWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqDisplay* pqTextDisplayPropertiesWidget::getDisplay() const
{
  return this->Internal->Display;
}

//-----------------------------------------------------------------------------
void pqTextDisplayPropertiesWidget::setDisplay(pqDisplay* display)
{
  if (this->Internal->Display == display)
    {
    return;
    }

  this->setEnabled(false);
  this->Internal->Links.removeAllPropertyLinks();
  if (this->Internal->Display)
    {
    QObject::disconnect(this->Internal->Display, 0, this, 0);
    }

  this->Internal->Display = qobject_cast<pqTextWidgetDisplay*>(display);
  if (!this->Internal->Display)
    {
    return;
    }

  this->setEnabled(true);
  vtkSMProxy* proxy = display->getProxy();
  this->Internal->Links.addPropertyLink(
    this->Internal->Visibility, "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Visibility"));

  QObject::connect(this->Internal->Visibility, SIGNAL(stateChanged(int)),
    this, SLOT(onVisibilityChanged(int)));

  this->Internal->Links.addPropertyLink(
    this->Internal->Interactivity, "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Enabled"));
  this->Internal->Links.addPropertyLink(
    this->Internal->Position1X, "value", SIGNAL(valueChanged(double)),
    proxy, proxy->GetProperty("Position"), 0);
  this->Internal->Links.addPropertyLink(
    this->Internal->Position1Y, "value", SIGNAL(valueChanged(double)),
    proxy, proxy->GetProperty("Position"), 1);
  this->Internal->Links.addPropertyLink(
    this->Internal->Position2X, "value", SIGNAL(valueChanged(double)),
    proxy, proxy->GetProperty("Position2"), 0);
  this->Internal->Links.addPropertyLink(
    this->Internal->Position2Y, "value", SIGNAL(valueChanged(double)),
    proxy, proxy->GetProperty("Position2"), 1);
}

//-----------------------------------------------------------------------------
void pqTextDisplayPropertiesWidget::onVisibilityChanged(int state)
{
  if (state == Qt::Unchecked)
    {
    this->Internal->Interactivity->setCheckState(Qt::Unchecked);
    }
}

//-----------------------------------------------------------------------------
void pqTextDisplayPropertiesWidget::renderAllViews()
{
  if (this->Internal->Display)
    {
    this->Internal->Display->renderAllViews();
    }
}


