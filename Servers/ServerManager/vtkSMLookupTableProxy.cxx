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

vtkStandardNewMacro(vtkSMLookupTableProxy);
vtkCxxRevisionMacro(vtkSMLookupTableProxy, "1.5");

//---------------------------------------------------------------------------
vtkSMLookupTableProxy::vtkSMLookupTableProxy()
{
  this->SetVTKClassName("vtkLookupTable");
  this->ArrayName = 0;
}

//---------------------------------------------------------------------------
vtkSMLookupTableProxy::~vtkSMLookupTableProxy()
{
  if (this->ArrayName)
    {
    delete [] this->ArrayName;
    this->ArrayName = 0;
    }
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
        this->LabToXYZ(lab,xyz);
        this->XYZToRGB(xyz,rgba);
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

//----------------------------------------------------------------------------
void vtkSMLookupTableProxy::SetArrayName(const char* str)
{
  if (this->ArrayName == NULL && str == NULL)
    {
    return;
    }
  if (this->ArrayName && str && (!strcmp(this->ArrayName,str)))
    {
    return;
    }
  if (this->ArrayName)
    {
    delete [] this->ArrayName;
    this->ArrayName = NULL;
    }
  if (str)
    {
    this->ArrayName = new char[strlen(str) + 1];
    strcpy(this->ArrayName,str);
    }
}

//----------------------------------------------------------------------------
void vtkSMLookupTableProxy::LabToXYZ(double Lab[3], double xyz[3])
{
  
  //LAB to XYZ
  double var_Y = ( Lab[0] + 16 ) / 116;
  double var_X = Lab[1] / 500 + var_Y;
  double var_Z = var_Y - Lab[2] / 200;
    
  if ( pow(var_Y,3) > 0.008856 ) var_Y = pow(var_Y,3);
  else var_Y = ( var_Y - 16 / 116 ) / 7.787;
                                                            
  if ( pow(var_X,3) > 0.008856 ) var_X = pow(var_X,3);
  else var_X = ( var_X - 16 / 116 ) / 7.787;

  if ( pow(var_Z,3) > 0.008856 ) var_Z = pow(var_Z,3);
  else var_Z = ( var_Z - 16 / 116 ) / 7.787;
  double ref_X =  95.047;
  double ref_Y = 100.000;
  double ref_Z = 108.883;
  xyz[0] = ref_X * var_X;     //ref_X =  95.047  Observer= 2 Illuminant= D65
  xyz[1] = ref_Y * var_Y;     //ref_Y = 100.000
  xyz[2] = ref_Z * var_Z;     //ref_Z = 108.883
}

//----------------------------------------------------------------------------
void vtkSMLookupTableProxy::XYZToRGB(double xyz[3], double rgb[3])
{
  
  //double ref_X =  95.047;        //Observer = 2 Illuminant = D65
  //double ref_Y = 100.000;
  //double ref_Z = 108.883;
 
  double var_X = xyz[0] / 100;        //X = From 0 to ref_X
  double var_Y = xyz[1] / 100;        //Y = From 0 to ref_Y
  double var_Z = xyz[2] / 100;        //Z = From 0 to ref_Y
 
  double var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
  double var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
  double var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;
 
  if ( var_R > 0.0031308 ) var_R = 1.055 * ( pow(var_R, ( 1 / 2.4 )) ) - 0.055;
  else var_R = 12.92 * var_R;
  if ( var_G > 0.0031308 ) var_G = 1.055 * ( pow(var_G ,( 1 / 2.4 )) ) - 0.055;
  else  var_G = 12.92 * var_G;
  if ( var_B > 0.0031308 ) var_B = 1.055 * ( pow(var_B, ( 1 / 2.4 )) ) - 0.055;
  else var_B = 12.92 * var_B;
                                                                                                 
  rgb[0] = var_R;
  rgb[1] = var_G;
  rgb[2] = var_B;
  
  //clip colors. ideally we would do something different for colors
  //out of gamut, but not really sure what to do atm.
  if (rgb[0]<0) rgb[0]=0;
  if (rgb[1]<0) rgb[1]=0;
  if (rgb[2]<0) rgb[2]=0;
  if (rgb[0]>1) rgb[0]=1;
  if (rgb[1]>1) rgb[1]=1;
  if (rgb[2]>1) rgb[2]=1;

}


//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::SaveInBatchScript(ofstream* file)
{
  *file << endl;
  unsigned int cc;
  unsigned numObjects = this->GetNumberOfIDs();
  vtkSMIntVectorProperty* ivp;
  vtkSMDoubleVectorProperty* dvp;

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
  os << indent << "ArrayName: " <<
    ((this->ArrayName)? this->ArrayName : "NULL") <<endl;
}







