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
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMPart.h"
#include "vtkSMProperty.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMSourceProxy);
vtkCxxRevisionMacro(vtkSMSourceProxy, "1.12");

struct vtkSMSourceProxyInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMPart> > Parts;
};

//---------------------------------------------------------------------------
vtkSMSourceProxy::vtkSMSourceProxy()
{
  this->PInternals = new  vtkSMSourceProxyInternals;
  this->PartsCreated = 0;

}

//---------------------------------------------------------------------------
vtkSMSourceProxy::~vtkSMSourceProxy()
{
  delete this->PInternals;
}

//---------------------------------------------------------------------------
int vtkSMSourceProxy::GetNumberOfParts()
{
  return this->PInternals->Parts.size();
}

//---------------------------------------------------------------------------
vtkSMPart* vtkSMSourceProxy::GetPart(int idx)
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

// //----------------------------------------------------------------------------
// void vtkSMSourceProxy::AddConsumer(vtkSMSourceProxy *c)
// {
//   // make sure it isn't already there
//   if (this->IsConsumer(c))
//     {
//     return;
//     }
//   // add it to the list, reallocate memory
//   vtkSMSourceProxy **tmp = this->Consumers;
//   this->NumberOfConsumers++;
//   this->Consumers = new vtkSMSourceProxy* [this->NumberOfConsumers];
//   for (int i = 0; i < (this->NumberOfConsumers-1); i++)
//     {
//     this->Consumers[i] = tmp[i];
//     }
//   this->Consumers[this->NumberOfConsumers-1] = c;
//   // free old memory
//   delete [] tmp;
// }

// //----------------------------------------------------------------------------
// void vtkSMSourceProxy::RemoveConsumer(vtkSMSourceProxy *c)
// {
//   // make sure it is already there
//   if (!this->IsConsumer(c))
//     {
//     return;
//     }
//   // remove it from the list, reallocate memory
//   vtkSMSourceProxy **tmp = this->Consumers;
//   this->NumberOfConsumers--;
//   if (this->NumberOfConsumers > 0)
//     {
//     this->Consumers = new vtkSMSourceProxy* [this->NumberOfConsumers];
//     int cnt = 0;
//     int i;
//     for (i = 0; i <= this->NumberOfConsumers; i++)
//       {
//       if (tmp[i] != c)
//         {
//         this->Consumers[cnt] = tmp[i];
//         cnt++;
//         }
//       }
//     }
//   else
//     {
//     this->Consumers = 0;
//     }
//   // free old memory
//   delete [] tmp;
// }

// //----------------------------------------------------------------------------
// int vtkSMSourceProxy::IsConsumer(vtkSMSourceProxy *c)
// {
//   int i;
//   for (i = 0; i < this->NumberOfConsumers; i++)
//     {
//     if (this->Consumers[i] == c)
//       {
//       return 1;
//       }
//     }
//   return 0;
// }

// //----------------------------------------------------------------------------
// vtkSMSourceProxy *vtkSMSourceProxy::GetConsumer(int i)
// {
//   if (i >= this->NumberOfConsumers)
//     {
//     return 0;
//     }
//   return this->Consumers[i];
// }

//---------------------------------------------------------------------------
void vtkSMSourceProxy::UpdateSelfAndAllInputs()
{
  this->Superclass::UpdateSelfAndAllInputs();
  this->UpdateInformation();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::ConvertDataInformationToProperty(
  vtkPVDataInformation* /*info*/, vtkSMProperty* /*prop*/)
{
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
