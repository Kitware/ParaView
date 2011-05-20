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
#include "vtkVRPropertyStyle.h"

#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"

//-----------------------------------------------------------------------------
vtkVRPropertyStyle::vtkVRPropertyStyle(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
vtkVRPropertyStyle::~vtkVRPropertyStyle()
{
}

//-----------------------------------------------------------------------------
bool vtkVRPropertyStyle::configure(vtkPVXMLElement* child,
  vtkSMProxyLocator* locator)
{
  if (!this->Superclass::configure(child, locator))
    {
    return false;
    }

  const char* proxy = child->GetAttributeOrEmpty("proxy");
  const char* property_name = child->GetAttributeOrEmpty("property");
  if (proxy && property_name)
    {
    this->setSMProperty(
      locator->LocateProxy(atoi(proxy)),
      property_name);
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRPropertyStyle::saveConfiguration() const
{
  vtkPVXMLElement* element = this->Superclass::saveConfiguration();
  if (element)
    {
    element->AddAttribute(
      "proxy", this->Proxy? this->Proxy->GetGlobalIDAsString() : "0");
    element->AddAttribute(
      "property", this->PropertyName.toAscii().data());
    }

  return element;
}

//-----------------------------------------------------------------------------
void vtkVRPropertyStyle::setSMProperty(
  vtkSMProxy* proxy, const QString& property_name)
{
  this->Proxy = proxy;
  this->PropertyName = property_name;
}

//-----------------------------------------------------------------------------
vtkSMProperty* vtkVRPropertyStyle::getSMProperty() const
{
  return this->Proxy? this->Proxy->GetProperty(
    this->PropertyName.toAscii().data()) : NULL;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkVRPropertyStyle::getSMProxy() const
{
  return this->Proxy;
}
