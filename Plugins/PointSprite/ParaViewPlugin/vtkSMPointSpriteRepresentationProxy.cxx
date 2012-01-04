/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPointSpriteRepresentationProxy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkSMPointSpriteRepresentationProxy
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "vtkSMPointSpriteRepresentationProxy.h"

#include "vtkAbstractMapper.h"
#include "vtkObjectFactory.h"
#include "vtkGlyphSource2D.h"
#include "vtkProperty.h"

#include "vtkProcessModule.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"
#include "vtkPVDataInformation.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"

using std::string;

vtkStandardNewMacro(vtkSMPointSpriteRepresentationProxy)
//----------------------------------------------------------------------------
vtkSMPointSpriteRepresentationProxy::vtkSMPointSpriteRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMPointSpriteRepresentationProxy::~vtkSMPointSpriteRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMPointSpriteRepresentationProxy::InitializeDefaultValues(
  vtkSMProxy* proxy)
{
  if (vtkSMPropertyHelper(proxy, "PointSpriteDefaultsInitialized").GetAsInt()!=0)
    {
    return;
    }

  vtkSMPropertyHelper(proxy, "PointSpriteDefaultsInitialized").Set(1);
  proxy->GetProperty("ConstantRadius")->ResetToDefault();
  proxy->GetProperty("RadiusRange")->ResetToDefault();
  proxy->UpdateVTKObjects();
}
namespace
{
  void vtkInitializeTableValues(vtkSMProperty* prop)
    {
    vtkSMDoubleVectorProperty* tableprop = vtkSMDoubleVectorProperty::SafeDownCast(prop);
    tableprop->SetNumberOfElements(256);
    double values[256];
    for (int i = 0; i < 256; i++)
      {
      values[i] = ((double) i) / 255.0;
      }
    tableprop->SetElements(values);
    }
}

int vtkSMPointSpriteRepresentationProxy::ReadXMLAttributes(
  vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }
  vtkInitializeTableValues(
   this->GetProperty("OpacityTableValues"));
  vtkInitializeTableValues(
   this->GetProperty("RadiusTableValues"));
  return 1;
}
