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
vtkCxxRevisionMacro(vtkSMDisplayProxy, "1.1.2.5");
//-----------------------------------------------------------------------------
vtkSMDisplayProxy::vtkSMDisplayProxy()
{
  this->GeometryInformationIsValid = 0;
  this->GeometryInformation = vtkPVGeometryInformation::New();
}

//-----------------------------------------------------------------------------
vtkSMDisplayProxy::~vtkSMDisplayProxy()
{
  cout << "vtkSMDisplayProxy::~vtkSMDisplayProxy()" << endl;
  this->GeometryInformation->Delete();
}

//-----------------------------------------------------------------------------
vtkPVGeometryInformation* vtkSMDisplayProxy::GetGeometryInformation()
{
  if (!this->GeometryInformationIsValid)
    {
    this->GatherGeometryInformation();
    }
  return this->GeometryInformation;
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::cmSetInterpolation(int flag)
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
int vtkSMDisplayProxy::cmGetInterpolation()
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
void vtkSMDisplayProxy::cmSetPointSize(double size)
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
double vtkSMDisplayProxy::cmGetPointSize()
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
void vtkSMDisplayProxy::cmSetLineWidth(double width)
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
double vtkSMDisplayProxy::cmGetLineWidth()
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
void vtkSMDisplayProxy::cmSetScalarMode(int mode)
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
int vtkSMDisplayProxy::cmGetScalarMode()
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
void vtkSMDisplayProxy::cmSetOpacity(double op)
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
double vtkSMDisplayProxy::cmGetOpacity()
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
void vtkSMDisplayProxy::cmSetColorMode(int mode)
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
int vtkSMDisplayProxy::cmGetColorMode()
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
void vtkSMDisplayProxy::cmSetColor(double rgb[3])
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
void vtkSMDisplayProxy::cmGetColor(double rgb[3])
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
void vtkSMDisplayProxy::cmSetInterpolateScalarsBeforeMapping(int flag)
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
int vtkSMDisplayProxy::cmGetInterpolateScalarsBeforeMapping()
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
void vtkSMDisplayProxy::cmSetScalarVisibility(int v)
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
int vtkSMDisplayProxy::cmGetScalarVisibility()
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
void vtkSMDisplayProxy::cmSetPosition(double pos[3])
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
void vtkSMDisplayProxy::cmGetPosition(double pos[3])
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
void vtkSMDisplayProxy::cmGetScale(double pos[3])
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
void vtkSMDisplayProxy::cmSetScale(double pos[3])
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
void vtkSMDisplayProxy::cmGetOrientation(double pos[3])
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
void vtkSMDisplayProxy::cmSetOrientation(double pos[3])
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
void vtkSMDisplayProxy::cmGetOrigin(double pos[3])
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
void vtkSMDisplayProxy::cmSetOrigin(double pos[3])
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
void vtkSMDisplayProxy::cmSetRepresentation(int r)
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
int vtkSMDisplayProxy::cmGetRepresentation()
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
void vtkSMDisplayProxy::cmSetVisibility(int v)
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
int vtkSMDisplayProxy::cmGetVisibility()
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
void vtkSMDisplayProxy::cmSetScalarArray(const char* arrayname)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetProperty("SelectScalarArray"));
  
  if (!svp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    }
  svp->SetElement(0, arrayname);
  this->UpdateVTKObjects();
    
}

//-----------------------------------------------------------------------------
const char* vtkSMDisplayProxy::cmGetScalarArray()
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetProperty("SelectScalarArray"));

  if (!svp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    }
  return svp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::cmSetImmediateModeRendering(int i)
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
int vtkSMDisplayProxy::cmGetImmediateModeRendering()
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
  *file << "# SaveInBatchScript not defined for " << this->GetClassName()
    << endl;
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
