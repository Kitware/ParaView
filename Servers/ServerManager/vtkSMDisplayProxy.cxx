/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDisplayProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVGeometryInformation.h"

#include "vtkSMPropertyIterator.h"
#include "vtkSMInputProperty.h"
vtkCxxRevisionMacro(vtkSMDisplayProxy, "1.2");
//-----------------------------------------------------------------------------
vtkSMDisplayProxy::vtkSMDisplayProxy()
{
  this->GeometryInformationIsValid = 0;
  this->GeometryInformation = vtkPVGeometryInformation::New();
}

//-----------------------------------------------------------------------------
vtkSMDisplayProxy::~vtkSMDisplayProxy()
{
  this->GeometryInformation->Delete();
}

//-----------------------------------------------------------------------------
vtkPVGeometryInformation* vtkSMDisplayProxy::GetGeometryInformation()
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return 0;
    }
  if (!this->GeometryInformationIsValid)
    {
    this->GatherGeometryInformation();
    }
  return this->GeometryInformation;
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetInterpolationCM(int flag)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Interpolation"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Interpolation on Display Proxy.");
    return ;
    }
  ivp->SetElement(0, flag);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDisplayProxy::GetInterpolationCM()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Interpolation"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Interpolation on Display Proxy.");
    return -1;
    } 
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetPointSizeCM(double size)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("PointSize"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property PointSize on DisplayProxy.");
    return ;
    }
  dvp->SetElement(0, size);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
double vtkSMDisplayProxy::GetPointSizeCM()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("PointSize"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property PointSize on DisplayProxy.");
    return 0.0;
    }
  return dvp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetLineWidthCM(double width)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("LineWidth"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property LineWidth on DisplayProxy.");
    return ;
    }
  dvp->SetElement(0, width);
  this->UpdateVTKObjects(); 
}

//-----------------------------------------------------------------------------
double vtkSMDisplayProxy::GetLineWidthCM()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("LineWidth"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property LineWidth on DisplayProxy.");
    return 0.0;
    }
  return dvp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetScalarModeCM(int mode)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ScalarMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, mode);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDisplayProxy::GetScalarModeCM()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ScalarMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return -1;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetOpacityCM(double op)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Opacity"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Opacity on DisplayProxy.");
    return ;
    }
  dvp->SetElement(0, op);
  this->UpdateVTKObjects(); 

}

//-----------------------------------------------------------------------------
double vtkSMDisplayProxy::GetOpacityCM()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Opacity"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Opacity on DisplayProxy.");
    return 0;
    }
  return dvp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetColorModeCM(int mode)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ColorMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, mode);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDisplayProxy::GetColorModeCM()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ColorMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetColorCM(double rgb[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Color"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Color on DisplayProxy.");
    return;
    }
  dvp->SetElements(rgb);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::GetColorCM(double rgb[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Color"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Color on DisplayProxy.");
    return;
    }
  rgb[0] = dvp->GetElement(0);
  rgb[1] = dvp->GetElement(1);
  rgb[2] = dvp->GetElement(2);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetInterpolateScalarsBeforeMappingCM(int flag)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("InterpolateScalarsBeforeMapping"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property InterpolateScalarsBeforeMapping on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, flag);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDisplayProxy::GetInterpolateScalarsBeforeMappingCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("InterpolateScalarsBeforeMapping"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property InterpolateScalarsBeforeMapping on DisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetScalarVisibilityCM(int v)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ScalarVisibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarVisibility on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, v);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDisplayProxy::GetScalarVisibilityCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ScalarVisibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarVisibility on DisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetPositionCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Position on DisplayProxy.");
    return;
    }
  dvp->SetElements(pos);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::GetPositionCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Position on DisplayProxy.");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
  pos[2] = dvp->GetElement(2);
}
//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::GetScaleCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Scale"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Scale on DisplayProxy.");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
  pos[2] = dvp->GetElement(2);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetScaleCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Scale"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Scale on DisplayProxy.");
    return;
    }
  dvp->SetElements(pos);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::GetOrientationCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Orientation"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Orientation on DisplayProxy.");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
  pos[2] = dvp->GetElement(2);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetOrientationCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Orientation"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Orientation on DisplayProxy.");
    return;
    }
  dvp->SetElements(pos);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::GetOriginCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Origin"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Origin on DisplayProxy.");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
  pos[2] = dvp->GetElement(2);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetOriginCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Origin"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Origin on DisplayProxy.");
    return;
    }
  dvp->SetElements(pos);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetRepresentationCM(int r)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Representation"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Representation on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, r);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDisplayProxy::GetRepresentationCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Representation"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Representation on DisplayProxy.");
    return 0;
    } 
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetVisibilityCM(int v)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, v);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDisplayProxy::GetVisibilityCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on DisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetScalarArrayCM(const char* arrayname)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetProperty("SelectScalarArray"));
  
  if (!svp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return;
    }
  svp->SetElement(0, arrayname);
  this->UpdateVTKObjects();
    
}

//-----------------------------------------------------------------------------
const char* vtkSMDisplayProxy::GetScalarArrayCM()
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetProperty("SelectScalarArray"));

  if (!svp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return 0;
    }
  return svp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetImmediateModeRenderingCM(int i)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ImmediateModeRendering"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on ImmediateModeRendering.");
    return;
    }
  ivp->SetElement(0, i);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDisplayProxy::GetImmediateModeRenderingCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ImmediateModeRendering"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on ImmediateModeRendering.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SaveInBatchScript(ofstream* file)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Display Proxy not created!");
    return;
    }

  // Some displays do not have VTKClassName set and hence only create Subproxies.
  // For such displays we use their self ids. 
  
  unsigned int count = this->GetNumberOfIDs();
  vtkClientServerID id = (count)? this->GetID(0) : this->SelfID;
  count = (count)? count : 1;
   
  for (unsigned int kk = 0; kk < count ; kk++)
    {
    if (kk > 0)
      {
      id = this->GetID(kk);
      }
    
    *file << endl;
    *file << "set pvTemp" << id
      << " [$proxyManager NewProxy " << this->GetXMLGroup() << " "
      << this->GetXMLName() << "]" << endl;
    *file << "  $proxyManager RegisterProxy " << this->GetXMLGroup()
      << " pvTemp" << id <<" $pvTemp" << id << endl;
    *file << "  $pvTemp" << id << " UnRegister {}" << endl;

    //First set the input to the display.
    vtkSMInputProperty* ipp;
    ipp = vtkSMInputProperty::SafeDownCast(
      this->GetProperty("Input"));
    if (ipp && ipp->GetNumberOfProxies() > 0)
      {
      *file << "  [$pvTemp" << id << " GetProperty Input] "
        " AddProxy $pvTemp" << ipp->GetProxy(0)->GetID(0)
        << endl;
      }
    else
      {
      *file << "# Input to Display Proxy not set properly or takes no Input." 
        << endl;
      }

    // Now, we save all the properties that are not Input.
    // Also note that only exposed properties are getting saved.

    vtkSMPropertyIterator* iter = this->NewPropertyIterator();
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      vtkSMProperty* p = iter->GetProperty();
      if (vtkSMInputProperty::SafeDownCast(p))
        {
        // Input property has already been saved...so skip it.
        continue;
        }

      if (!p->GetSaveable())
        {
        *file << "  # skipping not-saveable property " << p->GetXMLName() << endl;
        continue;
        }

      vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(p);
      vtkSMDoubleVectorProperty* dvp = 
        vtkSMDoubleVectorProperty::SafeDownCast(p);
      vtkSMStringVectorProperty* svp = 
        vtkSMStringVectorProperty::SafeDownCast(p);
      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(p);
      if (ivp)
        {
        for (unsigned int i=0; i < ivp->GetNumberOfElements(); i++)
          {
          *file << "  [$pvTemp" << id << " GetProperty "
            << ivp->GetXMLName() << "] SetElement "
            << i << " " << ivp->GetElement(i) 
            << endl;
          }
        }
      else if (dvp)
        {
        for (unsigned int i=0; i < dvp->GetNumberOfElements(); i++)
          {
          *file << "  [$pvTemp" << id << " GetProperty "
            << dvp->GetXMLName() << "] SetElement "
            << i << " " << dvp->GetElement(i) 
            << endl;
          }
        }
      else if (svp)
        {
        for (unsigned int i=0; i < svp->GetNumberOfElements(); i++)
          {
          *file << "  [$pvTemp" << id << " GetProperty "
            << svp->GetXMLName() << "] SetElement "
            << i << " {" << svp->GetElement(i) << "}"
            << endl;
          }
        }
      else if (pp)
        {
        for (unsigned int i=0; i < pp->GetNumberOfProxies(); i++)
          {
          *file << "  [$pvTemp" << id << " GetProperty "
            << pp->GetXMLName() << "] AddProxy $pvTemp"
            << pp->GetProxy(i)->GetID(0) << endl;
          }
        }
      else
        {
        *file << "  # skipping property " << p->GetXMLName() << endl;
        }
      }

    iter->Delete();
    *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::GatherGeometryInformation()
{
  this->GeometryInformation->Initialize();
  this->GeometryInformationIsValid =1;
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
