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
#include "vtkVRVectorPropertyStyle.h"

#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

//-----------------------------------------------------------------------------
vtkVRVectorPropertyStyle::vtkVRVectorPropertyStyle(QObject* parentObject)
  : Superclass(parentObject)
  , Mode(DISPLACEMENT)
{
}

//-----------------------------------------------------------------------------
vtkVRVectorPropertyStyle::~vtkVRVectorPropertyStyle()
{
}

//-----------------------------------------------------------------------------
bool vtkVRVectorPropertyStyle::configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  if (!this->Superclass::configure(child, locator))
  {
    return false;
  }
  const char* mode = child->GetAttributeOrEmpty("mode");
  if (strcmp(mode, "direction") == 0)
  {
    this->Mode = DIRECTION_VECTOR;
  }
  else
  {
    this->Mode = DISPLACEMENT;
  }
  return true;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRVectorPropertyStyle::saveConfiguration() const
{
  vtkPVXMLElement* element = this->Superclass::saveConfiguration();
  if (element && this->getSMProperty())
  {
    element->AddAttribute("mode", this->Mode == DIRECTION_VECTOR ? "direction" : "displacement");
  }
  return element;
}

//-----------------------------------------------------------------------------
bool vtkVRVectorPropertyStyle::handleEvent(const vtkVREventData& data)
{
  std::cout << "this is something new" << std::endl;
  //(void)data;
  switch (this->Mode)
  {
    // TODO:handle the data and compute the 3-tuple based on the mode.
    // then call this->setValue(...);
    case DIRECTION_VECTOR:
      break;

    case DISPLACEMENT:
      break;

    default:
      break;
  }

  // FIXME: return true or false as the case may be
  return false;
}

//-----------------------------------------------------------------------------
void vtkVRVectorPropertyStyle::setValue(double x, double y, double z)
{
  vtkSMProperty* smprop = this->getSMProperty();
  vtkSMProxy* smproxy = this->getSMProxy();
  if (smproxy && smprop)
  {
    double values[3] = { x, y, z };
    vtkSMPropertyHelper(smproxy, this->getSMPropertyName().toLocal8Bit().data()).Set(values, 3);
    smproxy->UpdateVTKObjects();
  }
}

bool vtkVRVectorPropertyStyle::update()
{
  vtkSMProperty* smprop = this->getSMProperty();
  vtkSMProxy* smproxy = this->getSMProxy();
  if (smproxy && smprop)
  {
    smproxy->UpdateVTKObjects();
  }
  return false;
}
