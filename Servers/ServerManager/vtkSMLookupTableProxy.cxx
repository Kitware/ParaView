/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLookupTableProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLookupTableProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkKWMath.h"

vtkStandardNewMacro(vtkSMLookupTableProxy);
vtkCxxRevisionMacro(vtkSMLookupTableProxy, "1.8");

//---------------------------------------------------------------------------
vtkSMLookupTableProxy::vtkSMLookupTableProxy()
{
  this->SetVTKClassName("vtkLookupTable");
  this->ArrayName = 0;
}

//---------------------------------------------------------------------------
vtkSMLookupTableProxy::~vtkSMLookupTableProxy()
{
  this->SetVTKClassName(0);
  this->SetArrayName(0);
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->SetServers(vtkProcessModule::CLIENT | 
    vtkProcessModule::RENDER_SERVER);
  this->Superclass::CreateVTKObjects(numObjects);
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  this->Build();
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::Build()
{
  vtkClientServerStream stream;

  vtkSMProperty* p;
  vtkSMIntVectorProperty* intVectProp;
  vtkSMDoubleVectorProperty* doubleVectProp;

  int numberOfTableValues;
  double hueRange[2];
  double saturationRange[2];
  double valueRange[2];
  
  p = this->GetProperty("NumberOfTableValues");
  intVectProp = vtkSMIntVectorProperty::SafeDownCast(p);
  numberOfTableValues = intVectProp->GetElement(0);
  
  p = this->GetProperty("HueRange");
  doubleVectProp = vtkSMDoubleVectorProperty::SafeDownCast(p);
  hueRange[0] = doubleVectProp->GetElement(0);
  hueRange[1] = doubleVectProp->GetElement(1);
  
  p = this->GetProperty("ValueRange");
  doubleVectProp = vtkSMDoubleVectorProperty::SafeDownCast(p);
  valueRange[0] = doubleVectProp->GetElement(0);
  valueRange[1] = doubleVectProp->GetElement(1);
  
  p = this->GetProperty("SaturationRange");
  doubleVectProp = vtkSMDoubleVectorProperty::SafeDownCast(p);
  saturationRange[0] = doubleVectProp->GetElement(0);
  saturationRange[1] = doubleVectProp->GetElement(1);
  

  int numObjects = this->GetNumberOfIDs();

  for (int i=0; i<numObjects; i++)
    {
    if (hueRange[0]<1.1) // Hack to deal with sandia color map.
      { // not Sandia interpolation.
      stream << vtkClientServerStream::Invoke << this->GetID(i)
             << "ForceBuild" << vtkClientServerStream::End;
      }
    else
      {
      //now we need to loop through the number of colors setting the colors
      //in the table
      stream << vtkClientServerStream::Invoke
             << this->GetID(i) << "SetNumberOfTableValues"
             << numberOfTableValues
             << vtkClientServerStream::End;

      int j;
      double rgba[4];
      double xyz[3];
      double lab[3];
      //only use opaque colors
      rgba[3]=1;
      
      int numColors= numberOfTableValues - 1;
      if (numColors<=0) numColors=1;

      for (j=0;j<numberOfTableValues;j++) 
        {
        // Get the color
        lab[0] = hueRange[0]+(hueRange[1]-hueRange[0])/(numColors)*j;
        lab[1] = saturationRange[0]+(saturationRange[1]-saturationRange[0])/
            (numColors)*j;
        lab[2] = valueRange[0]+(valueRange[1]-valueRange[0])/
            (numColors)*j;
        vtkKWMath::LabToXYZ(lab,xyz);
        vtkKWMath::XYZToRGB(xyz,rgba);
        stream << vtkClientServerStream::Invoke
               << this->GetID(i) << "SetTableValue" << j
               << rgba[0] << rgba[1] << rgba[2] << rgba[3] 
               << vtkClientServerStream::End;
        }
      }
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, stream, 0);
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::SaveInBatchScript(ofstream* file)
{
  unsigned int cc;
  unsigned numObjects = this->GetNumberOfIDs();
  vtkSMIntVectorProperty* ivp;
  vtkSMDoubleVectorProperty* dvp;

  *file << endl;
  for (cc=0; cc < numObjects; cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    *file << "set pvTemp" << id.ID
      << " [$proxyManager NewProxy lookup_tables LookupTable]" << endl;
    *file << "  $proxyManager RegisterProxy lookup_tables pvTemp"
      << id.ID << " $pvTemp" << id.ID << endl;
    *file << "  $pvTemp" << id.ID << " UnRegister {}" << endl;

    //Set ArrayName
    *file << "  [$pvTemp" << id.ID << " GetProperty ArrayName]"
      << " SetElement 0 {"
      << this->ArrayName << "}" << endl;

    // Set number of colors
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("NumberOfTableValues"));
    *file << "  [$pvTemp" << id.ID << " GetProperty "
      << "NumberOfTableValues] SetElements1 "
      << ivp->GetElement(0) << endl;

    // Set color ranges
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("HueRange"));
    *file << "  [$pvTemp" << id.ID << " GetProperty "
      << "HueRange] SetElements2 "
      << dvp->GetElement(0) << " " << dvp->GetElement(1) << endl;

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("SaturationRange"));
    *file << "  [$pvTemp" << id.ID << " GetProperty "
      << "SaturationRange] SetElements2 "
      << dvp->GetElement(0) << " " << dvp->GetElement(1) << endl;

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("ValueRange"));
    *file << "  [$pvTemp" << id.ID << " GetProperty "
      << "ValueRange] SetElements2 "
      << dvp->GetElement(0) << " " << dvp->GetElement(1) << endl;

    //Set scalar range
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("ScalarRange"));
    *file << "  [$pvTemp" << id.ID << " GetProperty "
      << "ScalarRange] SetElements2 "
      << dvp->GetElement(0) << " " << dvp->GetElement(1) << endl;

    //Set Vector component and mode
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("VectorComponent"));
    *file << "  [$pvTemp" << id.ID << " GetProperty "
      << "VectorComponent] SetElements1 " << ivp->GetElement(0) << endl;

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("VectorMode"));
    *file << "  [$pvTemp" << id.ID << " GetProperty "
      << "VectorMode] SetElements1 " << ivp->GetElement(0) << endl;
    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects" << endl;
    *file << endl;
    }
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayName: "
     << (this->ArrayName?this->ArrayName:"(none)") << endl;
}







