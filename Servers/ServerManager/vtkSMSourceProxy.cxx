/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSourceProxy.h"

#include "vtkClientServerStream.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVRenderModule.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMPart.h"
#include "vtkSMProperty.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMSourceProxy);
vtkCxxRevisionMacro(vtkSMSourceProxy, "1.19");

struct vtkSMSourceProxyInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMPart> > Parts;
};

//---------------------------------------------------------------------------
vtkSMSourceProxy::vtkSMSourceProxy()
{
  this->PInternals = new  vtkSMSourceProxyInternals;
  this->PartsCreated = 0;

  this->DataInformation = vtkPVDataInformation::New();
  this->DataInformationValid = 0;
}

//---------------------------------------------------------------------------
vtkSMSourceProxy::~vtkSMSourceProxy()
{
  delete this->PInternals;

  this->DataInformation->Delete();
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfParts()
{
  return this->PInternals->Parts.size();
}

//---------------------------------------------------------------------------
vtkSMPart* vtkSMSourceProxy::GetPart(unsigned int idx)
{
  return this->PInternals->Parts[idx].GetPointer();
}

//---------------------------------------------------------------------------
// Call UpdateInformation() on all sources
// TODO this should update information properties.
void vtkSMSourceProxy::UpdateInformation()
{
  int numIDs = this->GetNumberOfIDs();
  if (numIDs <= 0)
    {
    return;
    }

  vtkClientServerStream command;
  for(int i=0; i<numIDs; i++)
    {
    command << vtkClientServerStream::Invoke << this->GetID(i)
            << "UpdateInformation" << vtkClientServerStream::End;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, command, 0);
  
  this->Superclass::UpdateInformation();
}

//---------------------------------------------------------------------------
// Call Update() on all sources
// TODO this should update information properties.
void vtkSMSourceProxy::UpdatePipeline()
{
  int numIDs = this->GetNumberOfIDs();
  if (numIDs <= 0)
    {
    return;
    }

  vtkClientServerStream command;
  for(int i=0; i<numIDs; i++)
    {
    command << vtkClientServerStream::Invoke << this->GetID(i)
            << "Update" << vtkClientServerStream::End;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, command, 0);

  this->CreateParts();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateParts()
{
  if (this->PartsCreated)
    {
    return;
    }
  this->PartsCreated = 1;

  // This will only create objects if they are not already created.
  // This happens when connecting a filter to a source which is not
  // initialized. In other situations, SetInput() creates the VTK
  // objects before this gets called.
  this->CreateVTKObjects(1);

  this->PInternals->Parts.clear();

  int numIDs = this->GetNumberOfIDs();

  vtkPVNumberOfOutputsInformation* info = vtkPVNumberOfOutputsInformation::New();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // Create one part each output of each filter
  vtkClientServerStream stream;
  for (int i=0; i<numIDs; i++)
    {
    vtkClientServerID sourceID = this->GetID(i);
    // TODO replace this with UpdateInformation and OutputInformation
    // property.
    pm->GatherInformation(info, sourceID);
    int numOutputs = info->GetNumberOfOutputs();
    for (int j=0; j<numOutputs; j++)
      {
      stream << vtkClientServerStream::Invoke << sourceID
             << "GetOutput" << j <<  vtkClientServerStream::End;
      vtkClientServerID dataID = pm->GetUniqueID();
      stream << vtkClientServerStream::Assign << dataID
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;

      vtkSMPart* part = vtkSMPart::New();
      part->CreateVTKObjects(0);
      part->SetID(0, dataID);
      this->PInternals->Parts.push_back(part);
      part->Delete();
      }
    }
  pm->SendStream(this->Servers, stream, 0);
  stream.Reset();
  info->Delete();

  vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
     this->PInternals->Parts.begin();

  for(; it != this->PInternals->Parts.end(); it++)
    {
    it->GetPointer()->CreateTranslatorIfNecessary();
    it->GetPointer()->InsertExtractPiecesIfNecessary();
    }

}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::CleanInputs(const char* method)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  int numSources = this->GetNumberOfIDs();

  for (int sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
    {
    vtkClientServerID sourceID = this->GetID(sourceIdx);
    stream << vtkClientServerStream::Invoke 
           << sourceID << method 
           << vtkClientServerStream::End;
    }

  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, stream, 0);
    }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::AddInput(
  vtkSMSourceProxy *input, const char* method, int hasMultipleInputs)
{

  if (!input)
    {
    return;
    }

  input->CreateParts();
  int numInputs = input->GetNumberOfParts();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  if (hasMultipleInputs)
    {
    // One filter, multiple inputs
    this->CreateVTKObjects(1);
    vtkClientServerID sourceID = this->GetID(0);
    for (int partIdx = 0; partIdx < numInputs; ++partIdx)
      {
      vtkSMPart* part = input->GetPart(partIdx);
      stream << vtkClientServerStream::Invoke 
             << sourceID << method << part->GetID(0) 
             << vtkClientServerStream::End;
      }
    pm->SendStream(this->Servers, stream, 0);
    }
  else
    {
    // n inputs, n filters
    this->CreateVTKObjects(numInputs);
    int numSources = this->GetNumberOfIDs();
    for (int sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
      {
      vtkClientServerID sourceID = this->GetID(sourceIdx);
      // This is to handle the case when there are multiple
      // inputs and the first one has multiple parts. For
      // example, in the Glyph filter, when the input has multiple
      // parts, the glyph source has to be applied to each.
      // NOTE: Make sure that you set the input which has as
      // many parts as there will be filters first. OR call
      // CreateVTKObjects() with the right number of inputs.
      int partIdx = sourceIdx % numInputs;
      vtkSMPart* part = input->GetPart(partIdx);
      stream << vtkClientServerStream::Invoke 
             << sourceID << method << part->GetID(0) 
             << vtkClientServerStream::End;
      }
    pm->SendStream(this->Servers, stream, 0);
    }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::MarkConsumersAsModified()
{
  this->Superclass::MarkConsumersAsModified();
  this->InvalidateDataInformation();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->GetRenderModule())
    {
    pm->GetRenderModule()->SetTotalVisibleMemorySizeValid(0);
    }
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::UpdateSelfAndAllInputs()
{
  this->Superclass::UpdateSelfAndAllInputs();
  this->UpdateInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetDataInformation()
{
  if (this->DataInformationValid == 0)
    {
    this->GatherDataInformation();
    this->UpdateDataInformation();
    }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::InvalidateDataInformation()
{
  this->DataInformationValid = 0;
  vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
    this->PInternals->Parts.begin();
  for (; it != this->PInternals->Parts.end(); it++)
    {
    it->GetPointer()->InvalidateDataInformation();
    }
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMSourceProxy::GetProperty(const char* name, int selfOnly)
{
  if (!name)
    {
    return 0;
    }

  // Make sure DataInformation is always up to date
  if ( strcmp(name,"DataInformation") == 0 )
    {
    this->GetDataInformation();
    }

  return this->Superclass::GetProperty(name, selfOnly);
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::GatherDataInformation()
{
  this->DataInformation->Initialize();

  vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
    this->PInternals->Parts.begin();
  for (; it != this->PInternals->Parts.end(); it++)
    {
    this->DataInformation->AddInformation(
      it->GetPointer()->GetDataInformation());
    }
  this->DataInformationValid = 1;
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::UpdateDataInformation()
{
  vtkPVDataInformation* info = this->GetDataInformation();
  vtkSMProperty* property = this->GetProperty("DataInformation");
  if (!property)
    {
    vtkSMProperty* prop = vtkSMProperty::New();
    this->AddProperty("DataInformation", prop);
    property = prop;
    // Assignment above is still valid. Adding pointer increments 
    // it's ref. count
    prop->Delete();
    }
  property->SetInformationOnly(0);
  this->ConvertDataInformationToProperty(info, property);
  property->SetInformationOnly(1);
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::ConvertDataInformationToProperty(
  vtkPVDataInformation* info, vtkSMProperty* prop)
{
  vtkSMIdTypeVectorProperty* numId = vtkSMIdTypeVectorProperty::SafeDownCast(
    prop->GetSubProperty("NumberOfPoints"));
  if (!numId)
    {
    numId = vtkSMIdTypeVectorProperty::New();
    prop->AddSubProperty("NumberOfPoints", numId);
    // This is OK. AddSubProperty increments ref count
    numId->Delete();
    }
  numId->SetNumberOfElements(1);
  numId->SetElements1(info->GetNumberOfPoints());

  numId = vtkSMIdTypeVectorProperty::SafeDownCast(
    prop->GetSubProperty("NumberOfCells"));
  if (!numId)
    {
    numId = vtkSMIdTypeVectorProperty::New();
    prop->AddSubProperty("NumberOfCells", numId);
    // This is OK. AddSubProperty increments ref count
    numId->Delete();
    }
  numId->SetNumberOfElements(1);
  numId->SetElements1(info->GetNumberOfCells());

  vtkSMIntVectorProperty* num = vtkSMIntVectorProperty::SafeDownCast(
    prop->GetSubProperty("MemorySize"));
  if (!num)
    {
    num = vtkSMIntVectorProperty::New();
    prop->AddSubProperty("MemorySize", num);
    // This is OK. AddSubProperty increments ref count
    num->Delete();
    }
  num->SetNumberOfElements(1);
  num->SetElements1(info->GetMemorySize());

  num = vtkSMIntVectorProperty::SafeDownCast(
    prop->GetSubProperty("Extent"));
  if (!num)
    {
    num = vtkSMIntVectorProperty::New();
    prop->AddSubProperty("Extent", num);
    // This is OK. AddSubProperty increments ref count
    num->Delete();
    }
  num->SetNumberOfElements(6);
  int i;
  for (i=0; i<6; i++)
    {
    num->SetElement(i, info->GetExtent()[i]);
    }

  vtkSMDoubleVectorProperty* bounds = vtkSMDoubleVectorProperty::SafeDownCast(
    prop->GetSubProperty("Bounds"));
  if (!bounds)
    {
    bounds = vtkSMDoubleVectorProperty::New();
    prop->AddSubProperty("Bounds", bounds);
    // This is OK. AddSubProperty increments ref count
    bounds->Delete();
    }
  bounds->SetNumberOfElements(6);
  for (i=0; i<6; i++)
    {
    bounds->SetElement(i, info->GetBounds()[i]);
    }

  vtkSMProperty* pd = prop->GetSubProperty("PointData");
  if (!pd)
    {
    pd = vtkSMProperty::New();
    prop->AddSubProperty("PointData", pd);
    // This is OK. AddSubProperty increments ref count
    pd->Delete();
    }
  this->ConvertFieldDataInformationToProperty(
    info->GetPointDataInformation(), pd);

  vtkSMProperty* cd = prop->GetSubProperty("CellData");
  if (!cd)
    {
    cd = vtkSMProperty::New();
    prop->AddSubProperty("CellData", cd);
    // This is OK. AddSubProperty increments ref count
    cd->Delete();
    }
  this->ConvertFieldDataInformationToProperty(
    info->GetCellDataInformation(), cd);

  prop->SetInformationOnly(1);
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::ConvertFieldDataInformationToProperty(
  vtkPVDataSetAttributesInformation* info, vtkSMProperty* prop)
{
  int i;

  vtkSMProperty* arrays = prop->GetSubProperty("Arrays");
  if (!arrays)
    {
    arrays = vtkSMProperty::New();
    prop->AddSubProperty("Arrays", arrays);
    // This is OK. AddSubProperty increments ref count
    arrays->Delete();
    }

  int numArrays = info->GetNumberOfArrays();
  for (i=0; i<numArrays; i++)
    {
    vtkPVArrayInformation* ainfo = info->GetArrayInformation(i);
    vtkSMProperty* array = arrays->GetSubProperty(ainfo->GetName());
    if (!array)
      {
      array = vtkSMProperty::New();
      arrays->AddSubProperty(ainfo->GetName(), array);
      // This is OK. AddSubProperty increments ref count
      array->Delete();
      }
    this->ConvertArrayInformationToProperty(ainfo, array);
    }

  vtkSMStringVectorProperty* idx = vtkSMStringVectorProperty::SafeDownCast(
    prop->GetSubProperty("AttributeNames"));
  if (!idx)
    {
    idx = vtkSMStringVectorProperty::New();
    prop->AddSubProperty("AttributeNames", idx);
    // This is OK. AddSubProperty increments ref count
    idx->Delete();
    }
  idx->SetNumberOfElements(vtkDataSetAttributes::NUM_ATTRIBUTES);
  for (i=0; i<vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
    {
    vtkPVArrayInformation* ainfo = info->GetAttributeInformation(i);
    if (ainfo)
      {
      idx->SetElement(i, ainfo->GetName());
      }
    else
      {
      idx->SetElement(i, 0);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::ConvertArrayInformationToProperty(
  vtkPVArrayInformation* info, vtkSMProperty* prop)
{
  vtkSMIntVectorProperty* num = vtkSMIntVectorProperty::SafeDownCast(
    prop->GetSubProperty("DataType"));
  if (!num)
    {
    num = vtkSMIntVectorProperty::New();
    prop->AddSubProperty("DataType", num);
    // This is OK. AddSubProperty increments ref count
    num->Delete();
    }
  num->SetNumberOfElements(1);
  num->SetElements1(info->GetDataType());

  num = vtkSMIntVectorProperty::SafeDownCast(
    prop->GetSubProperty("NumberOfComponents"));
  if (!num)
    {
    num = vtkSMIntVectorProperty::New();
    prop->AddSubProperty("NumberOfComponents", num);
    // This is OK. AddSubProperty increments ref count
    num->Delete();
    }
  num->SetNumberOfElements(1);
  int numComps = info->GetNumberOfComponents();
  num->SetElements1(numComps);

  vtkSMDoubleVectorProperty* dnum = vtkSMDoubleVectorProperty::SafeDownCast(
    prop->GetSubProperty("ComponentRanges"));
  if (!dnum)
    {
    dnum = vtkSMDoubleVectorProperty::New();
    prop->AddSubProperty("ComponentRanges", dnum);
    // This is OK. AddSubProperty increments ref count
    dnum->Delete();
    }
  dnum->SetNumberOfElements(numComps*2);
  for(int i=0; i<numComps; i++)
    {
    double* range = info->GetComponentRange(i);
    dnum->SetElement(2*i  , range[0]);
    dnum->SetElement(2*i+1, range[1]);
    }

  prop->SetInformationOnly(1);
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
