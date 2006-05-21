/*=========================================================================

   Program:   ParaQ
   Module:    pqClipPanel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqClipPanel.h"
#include "pqPropertyLinks.h"
#include "pqPropertyManager.h"

#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>

#include <QLineEdit>

/// constructor
pqClipPanel::pqClipPanel(QWidget* p) :
  pqWidgetObjectPanel(":/pqWidgets/ClipPanel.ui", p)
{
}

/// destructor
pqClipPanel::~pqClipPanel()
{
  this->setProxy(NULL);
}

void pqClipPanel::setProxy(pqSMProxy p)
{
  pqWidgetObjectPanel::setProxy(p);
  
  if(!this->Proxy)
    return;
  
  if(!this->Widget)
    return;

  if(vtkSMDoubleVectorProperty* const widget_origin = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Widget->GetProperty("Origin")))
    {
    if(QLineEdit* const originX = this->findChild<QLineEdit*>("originX"))
      {
      this->getPropertyLinks().addPropertyLink(originX, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_origin, 0);
      }
 
    if(QLineEdit* const originY = this->findChild<QLineEdit*>("originY"))
      {
      this->getPropertyLinks().addPropertyLink(originY, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_origin, 1);
      }
    
    if(QLineEdit* const originZ = this->findChild<QLineEdit*>("originZ"))
      {
      this->getPropertyLinks().addPropertyLink(originZ, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_origin, 2);
      }
    }

  if(vtkSMDoubleVectorProperty* const widget_normal = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Widget->GetProperty("Normal")))
    {
    if(QLineEdit* const normalX = this->findChild<QLineEdit*>("normalX"))
      {
      this->getPropertyLinks().addPropertyLink(normalX, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_normal, 0);
      }
 
    if(QLineEdit* const normalY = this->findChild<QLineEdit*>("normalY"))
      {
      this->getPropertyLinks().addPropertyLink(normalY, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_normal, 1);
      }
    
    if(QLineEdit* const normalZ = this->findChild<QLineEdit*>("normalZ"))
      {
      this->getPropertyLinks().addPropertyLink(normalZ, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_normal, 2);
      }
    }

  this->getPropertyLinks().reset();
}

