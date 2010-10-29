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
#include "vtkMath.h"

vtkStandardNewMacro(vtkSMLookupTableProxy);

//---------------------------------------------------------------------------
vtkSMLookupTableProxy::vtkSMLookupTableProxy()
{
  this->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->SetVTKClassName("vtkLookupTable");
  this->ArrayName = 0;
  this->LowOutOfRangeColor[0] = this->LowOutOfRangeColor[1] =
    this->LowOutOfRangeColor[2] = 0;
  this->HighOutOfRangeColor[0] = this->HighOutOfRangeColor[1] =
    this->HighOutOfRangeColor[2] = 1;
  this->UseLowOutOfRangeColor = 0;
  this->UseHighOutOfRangeColor = 0;
}

//---------------------------------------------------------------------------
vtkSMLookupTableProxy::~vtkSMLookupTableProxy()
{
  this->SetVTKClassName(0);
  this->SetArrayName(0);
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::UpdateVTKObjects(vtkClientServerStream& stream)
{
  this->Superclass::UpdateVTKObjects(stream);
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
  

  if (hueRange[0]<1.1) // Hack to deal with sandia color map.
    { // not Sandia interpolation.
    stream << vtkClientServerStream::Invoke << this->GetID()
           << "ForceBuild" << vtkClientServerStream::End;
    int numColors = (numberOfTableValues < 1) ? 1 : numberOfTableValues;
    if (this->UseLowOutOfRangeColor)
      {
      stream << vtkClientServerStream::Invoke
             << this->GetID() << "SetTableValue" << 0
             << this->LowOutOfRangeColor[0] << this->LowOutOfRangeColor[1]
             << this->LowOutOfRangeColor[2] << 1
             << vtkClientServerStream::End;
      }
    if (this->UseHighOutOfRangeColor)
      {
      stream << vtkClientServerStream::Invoke << this->GetID()
             << "SetTableValue" << numColors-1
             << this->HighOutOfRangeColor[0] << this->HighOutOfRangeColor[1]
             << this->HighOutOfRangeColor[2]
             << 1 << vtkClientServerStream::End;
      }
    }
  else
    {
    //now we need to loop through the number of colors setting the colors
    //in the table
    stream << vtkClientServerStream::Invoke
           << this->GetID() << "SetNumberOfTableValues"
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
      vtkMath::LabToXYZ(lab,xyz);
      vtkMath::XYZToRGB(xyz,rgba);
      stream << vtkClientServerStream::Invoke
             << this->GetID() << "SetTableValue" << j
             << rgba[0] << rgba[1] << rgba[2] << rgba[3] 
             << vtkClientServerStream::End;
      }
    if (this->UseLowOutOfRangeColor)
      {
      stream << vtkClientServerStream::Invoke << this->GetID()
             << "SetTableValue 0" << this->LowOutOfRangeColor[0]
             << this->LowOutOfRangeColor[1] << this->LowOutOfRangeColor[2]
             << 1 << vtkClientServerStream::End;
      }
    if (this->UseHighOutOfRangeColor)
      {
      stream << vtkClientServerStream::Invoke << this->GetID()
             << "SetTableValue" << numColors-1
             << this->HighOutOfRangeColor[0] << this->HighOutOfRangeColor[1]
             << this->HighOutOfRangeColor[2]
             << 1 << vtkClientServerStream::End;
      }
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayName: "
     << (this->ArrayName?this->ArrayName:"(none)") << endl;
  os << indent << "LowOutOfRangeColor: " << this->LowOutOfRangeColor[0]
     << " " << this->LowOutOfRangeColor[1] << " "
     << this->LowOutOfRangeColor[2] << endl;
  os << indent << "HighOutOfRangeColor: " << this->HighOutOfRangeColor[0]
     << " " << this->HighOutOfRangeColor[1] << " "
     << this->HighOutOfRangeColor[2] << endl;
  os << indent << "UseLowOutOfRangeColor: " << this->UseLowOutOfRangeColor
     << endl;
  os << indent << "UseHighOutOfRangeColor: " << this->UseHighOutOfRangeColor
     << endl;
}







