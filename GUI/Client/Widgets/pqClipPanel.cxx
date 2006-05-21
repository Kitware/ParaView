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
#include <vtkSMProxyProperty.h>

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
  
  QLineEdit* const originX = this->findChild<QLineEdit*>("originX");
  QLineEdit* const originY = this->findChild<QLineEdit*>("originY");
  QLineEdit* const originZ = this->findChild<QLineEdit*>("originZ");
  QLineEdit* const normalX = this->findChild<QLineEdit*>("normalX");
  QLineEdit* const normalY = this->findChild<QLineEdit*>("normalY");
  QLineEdit* const normalZ = this->findChild<QLineEdit*>("normalZ");
  
  // Connect Qt widgets to the 3D widget ...
  if(this->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_origin = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Origin")))
      {
      if(originX)
        {
        this->getPropertyLinks().addPropertyLink(originX, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_origin, 0);
        }
   
      if(originY)
        {
        this->getPropertyLinks().addPropertyLink(originY, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_origin, 1);
        }
      
      if(originZ)
        {
        this->getPropertyLinks().addPropertyLink(originZ, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_origin, 2);
        }
      }

    if(vtkSMDoubleVectorProperty* const widget_normal = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Normal")))
      {
      if(normalX)
        {
        this->getPropertyLinks().addPropertyLink(normalX, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_normal, 0);
        }
   
      if(normalY)
        {
        this->getPropertyLinks().addPropertyLink(normalY, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_normal, 1);
        }
      
      if(normalZ)
        {
        this->getPropertyLinks().addPropertyLink(normalZ, "text", SIGNAL(textChanged(const QString&)), this->Widget, widget_normal, 2);
        }
      }
    }

  // Connect Qt widgets to the implicit plane function ...
/*
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("ClipFunction")))
      {
      if(vtkSMProxy* const clip_function = clip_function_property->GetProxy(0))
        {
        if(vtkSMDoubleVectorProperty* const plane_origin = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Origin")))
          {
          if(originX)
            {
            this->getPropertyLinks().addPropertyLink(originX, "text", SIGNAL(textChanged(const QString&)), this->Widget, plane_origin, 0);
            }
       
          if(originY)
            {
            this->getPropertyLinks().addPropertyLink(originY, "text", SIGNAL(textChanged(const QString&)), this->Widget, plane_origin, 1);
            }
          
          if(originZ)
            {
            this->getPropertyLinks().addPropertyLink(originZ, "text", SIGNAL(textChanged(const QString&)), this->Widget, plane_origin, 2);
            }
          }

        if(vtkSMDoubleVectorProperty* const plane_normal = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Normal")))
          {
          if(normalX)
            {
            this->getPropertyLinks().addPropertyLink(normalX, "text", SIGNAL(textChanged(const QString&)), this->Widget, plane_normal, 0);
            }
       
          if(normalY)
            {
            this->getPropertyLinks().addPropertyLink(normalY, "text", SIGNAL(textChanged(const QString&)), this->Widget, plane_normal, 1);
            }
          
          if(normalZ)
            {
            this->getPropertyLinks().addPropertyLink(normalZ, "text", SIGNAL(textChanged(const QString&)), this->Widget, plane_normal, 2);
            }
          }
        }
      }
    }
*/
}

