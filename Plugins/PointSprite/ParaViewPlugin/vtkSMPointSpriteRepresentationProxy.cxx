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
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"
#include "vtkPVDataInformation.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"

using vtkstd::string;

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

  // Initialize the Transfer functions if needed
  int nop = vtkSMVectorProperty::SafeDownCast(proxy->GetProperty("OpacityTableValues"))->GetNumberOfElements();
  if (nop == 0)
    {
    InitializeTableValues(proxy->GetProperty("OpacityTableValues"));
    }

  int nrad = vtkSMVectorProperty::SafeDownCast(proxy->GetProperty("RadiusTableValues"))->GetNumberOfElements();
  if (nrad == 0)
    {
    InitializeTableValues(proxy->GetProperty("RadiusTableValues"));
    }

  InitializeSpriteTextures(proxy);

  proxy->UpdateVTKObjects();
}

void vtkSMPointSpriteRepresentationProxy::InitializeTableValues(vtkSMProperty* prop)
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

void vtkSMPointSpriteRepresentationProxy::InitializeSpriteTextures(
  vtkSMProxy* repr)
{
  vtkSMProxyIterator* proxyIter;
  string texName;
  bool created;
  vtkSMProxy* texture;
  int extent[6] = {0, 65, 0, 65, 0, 0};
  vtkSMIntVectorProperty* extentprop;
  vtkSMIntVectorProperty* maxprop;
  vtkSMDoubleVectorProperty* devprop;
  vtkSMIntVectorProperty* alphamethodprop;
  vtkSMIntVectorProperty* alphathresholdprop;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  texName = "Sphere";
  created = false;
  proxyIter = vtkSMProxyIterator::New();
  proxyIter->SetModeToOneGroup();
  for (proxyIter->Begin("textures"); !proxyIter->IsAtEnd(); proxyIter->Next())
    {
    string name = proxyIter->GetKey();
    if (name == texName)
      {
      created = true;
      break;
      }
    }
  proxyIter->Delete();

  if (!created)
    {
    // create the texture proxy
    texture = pxm->NewProxy("textures", "SpriteTexture");
    texture->SetConnectionID(repr->GetConnectionID());
    texture->SetServers(vtkProcessModule::CLIENT
        | vtkProcessModule::RENDER_SERVER);
    pxm->RegisterProxy("textures", texName.c_str(), texture);
    texture->Delete();

    // set the texture parameters
    extentprop = vtkSMIntVectorProperty::SafeDownCast(texture->GetProperty("WholeExtent"));
    extentprop->SetNumberOfElements(6);
    extentprop->SetElements(extent);
    maxprop = vtkSMIntVectorProperty::SafeDownCast(texture->GetProperty("Maximum"));
    maxprop->SetElements1(255);
    devprop = vtkSMDoubleVectorProperty::SafeDownCast(texture->GetProperty("StandardDeviation"));
    devprop->SetElements1(0.3);
    alphamethodprop = vtkSMIntVectorProperty::SafeDownCast(texture->GetProperty("AlphaMethod"));
    alphamethodprop->SetElements1(2);
    alphathresholdprop = vtkSMIntVectorProperty::SafeDownCast(texture->GetProperty("AlphaThreshold"));
    alphathresholdprop->SetElements1(63);
    texture->UpdateVTKObjects();

    vtkSMProxyProperty* textureProperty = vtkSMProxyProperty::SafeDownCast(
      repr->GetProperty("Texture"));
    if(textureProperty->GetNumberOfProxies() == 0)
      {
      // set this texture as default texture
      textureProperty->SetProxy(0, texture);
      repr->UpdateVTKObjects();
      }
    }

  texName = "Blur";
  created = false;
  proxyIter = vtkSMProxyIterator::New();
  proxyIter->SetModeToOneGroup();
  for (proxyIter->Begin("textures"); !proxyIter->IsAtEnd(); proxyIter->Next())
    {
    string name = proxyIter->GetKey();
    if (name == texName)
      {
      created = true;
      break;
      }
    }

  if (!created)
    {
    // create the texture proxy
    texture = pxm->NewProxy("textures", "SpriteTexture");
    texture->SetConnectionID(repr->GetConnectionID());
    texture->SetServers(vtkProcessModule::CLIENT
        | vtkProcessModule::RENDER_SERVER);
    pxm->RegisterProxy("textures", texName.c_str(), texture);

    // set the texture parameters
    extentprop = vtkSMIntVectorProperty::SafeDownCast(texture->GetProperty("WholeExtent"));
    extentprop->SetNumberOfElements(6);
    extentprop->SetElements(extent);
    maxprop = vtkSMIntVectorProperty::SafeDownCast(texture->GetProperty("Maximum"));
    maxprop->SetElements1(255);
    devprop = vtkSMDoubleVectorProperty::SafeDownCast(texture->GetProperty("StandardDeviation"));
    devprop->SetElements1(0.2);
    alphamethodprop = vtkSMIntVectorProperty::SafeDownCast(texture->GetProperty("AlphaMethod"));
    alphamethodprop->SetElements1(1);

    texture->UpdateVTKObjects();

    texture->Delete();
    }
  proxyIter->Delete();

}

