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
#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMPart.h"
#include "vtkSMCommunicationModule.h"
#include "vtkSMProcessModule.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMSourceProxy);
vtkCxxRevisionMacro(vtkSMSourceProxy, "1.1");

struct vtkSMSourceProxyInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMPart> > Parts;
};

//---------------------------------------------------------------------------
vtkSMSourceProxy::vtkSMSourceProxy()
{
  this->PInternals = new  vtkSMSourceProxyInternals;
  this->PartsCreated = 0;

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  if (!cm)
    {
    vtkErrorMacro("Can not fully initialize without a global "
                  "CommunicationModule. This object will not be fully "
                  "functional.");
    return;
    }
  this->ClientServerID = cm->GetUniqueID();
  vtkClientServerStream initStream;
  initStream << vtkClientServerStream::Assign 
             << this->ClientServerID << this
             << vtkClientServerStream::End;
  cm->SendStreamToServer(&initStream, 0);

  // TODO revise this
  // This is to make the LastResult stream release it's reference
  // on this object.
  //vtkSMProcessModule* pm = this->GetProcessModule();
  //pm->ClearLastResult();

  this->NumberOfConsumers = 0;
  this->Consumers = 0;

  this->NumberOfInputs = 0;
  this->Inputs = 0;

  this->HasMultipleInputs = 0;
}

//---------------------------------------------------------------------------
vtkSMSourceProxy::~vtkSMSourceProxy()
{
  delete this->PInternals;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::UnRegister(vtkObjectBase* obj)
{
  this->Superclass::UnRegister(obj);

  // TODO revise this
//   if (obj)
//     {
//     vtkDebugMacro(
//       << "UnRegistered by "
//       << obj->GetClassName() << " (" << obj << "), ReferenceCount = "
//       << (this->ReferenceCount-1));
//     }
//   else
//     {
//     vtkDebugMacro(
//       << "UnRegistered " << this->GetClassName() 
//       << " by NULL, ReferenceCount = "
//       << (this->ReferenceCount-1));
//     }

//   this->ReferenceCount--;
//   if (this->ReferenceCount == 1)
//     {
//     vtkSMCommunicationModule* cm = this->GetCommunicationModule();
//     if (cm)
//       {
//       vtkClientServerStream deleteStream;
//       deleteStream << vtkClientServerStream::Delete 
//                    << this->ClientServerID 
//                    << vtkClientServerStream::End;
//       cm->SendStreamToServer(&deleteStream, 0);
//       }
//     else
//       {
//       vtkErrorMacro("There is not valid communication module assigned. "
//                     "This object can not be cleanly destroyed.");
//       }
//     }
//   else if (this->ReferenceCount <= 0)
//     {
// #ifdef VTK_DEBUG_LEAKS
//     vtkDebugLeaks::DestructClass(this->GetClassName());
// #endif
//     delete this;
//     }
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

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&command, 
                          this->GetNumberOfServerIDs(), 
                          this->GetServerIDs());
  
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
  vtkSMCommunicationModule* cm = this->GetCommunicationModule();

  vtkClientServerStream stream;
  for (int i=0; i<numIDs; i++)
    {
    vtkClientServerID sourceID = this->GetID(i);
    // TODO replace this with UpdateInformation and OutputInformation
    // property.
    cm->GatherInformation(info, sourceID, 1);
    int numOutputs = info->GetNumberOfOutputs();
    for (int j=0; j<numOutputs; j++)
      {
      stream << vtkClientServerStream::Invoke << sourceID
             << "GetOutput" << j <<  vtkClientServerStream::End;
      vtkClientServerID dataID = cm->GetUniqueID();
      stream << vtkClientServerStream::Assign << dataID
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;

      vtkSMPart* part = vtkSMPart::New();
      part->SetVTKDataID(dataID);
      this->PInternals->Parts.push_back(part);
      part->Delete();
      }
    }
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
  stream.Reset();
  info->Delete();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::SetNumberOfInputs(int num)
{
  // in case nothing has changed.
  if (num == this->NumberOfInputs)
    {
    return;
    }
  
  int idx;
  vtkSMSourceProxy** inputs;

  // Allocate new arrays.
  inputs = new vtkSMSourceProxy* [num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    inputs[idx] = NULL;
    }

  // Copy old inputs
  for (idx = 0; idx < num && idx < this->NumberOfInputs; ++idx)
    {
    inputs[idx] = this->Inputs[idx];
    }
  
  // delete the previous arrays
  if (this->Inputs)
    {
    delete [] this->Inputs;
    this->Inputs = NULL;
    this->NumberOfInputs = 0;
    }
  
  // Set the new array
  this->Inputs = inputs;
  
  this->NumberOfInputs = num;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::SetInput(int idx, vtkSMSourceProxy *input)
{
  this->SetInput(idx, input, "SetInput");
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::SetInput(
  int idx, vtkSMSourceProxy *input, const char* method)
{

  // Set the paraview reference to the new input.
  this->SetNthInput(idx, input);
  if (!input)
    {
    return;
    }

  input->CreateParts();
  int numInputs = input->GetNumberOfParts();

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();

  vtkClientServerStream stream;
  if (this->HasMultipleInputs)
    {
    this->CreateVTKObjects(1);
    vtkClientServerID sourceID = this->GetID(0);
    for (int partIdx = 0; partIdx < numInputs; ++partIdx)
      {
      vtkSMPart* part = input->GetPart(partIdx);
      stream << vtkClientServerStream::Invoke 
             << sourceID << method << part->GetVTKDataID() 
             << vtkClientServerStream::End;
      }
    cm->SendStreamToServers(&stream, 
                            this->GetNumberOfServerIDs(),
                            this->GetServerIDs());
    }
  else
    {
    this->CreateVTKObjects(numInputs);
    int numSources = this->GetNumberOfIDs();
    for (int sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
      {
      vtkClientServerID sourceID = this->GetID(sourceIdx);
      // This is to handle the case when there are multiple
      // inputs and the first one has multiple parts. For
      // example, in the Glyph filter, when the input has multiple
      // parts, the glyph source has to be applied to each.
      // In that case, sourceTclName == glyph input, 
      // inputName == glyph source.
      int partIdx = sourceIdx % numInputs;
      vtkSMPart* part = input->GetPart(partIdx);
      stream << vtkClientServerStream::Invoke 
             << sourceID << method << part->GetVTKDataID() 
             << vtkClientServerStream::End;
        }
    cm->SendStreamToServers(&stream, 
                            this->GetNumberOfServerIDs(),
                            this->GetServerIDs());
    }
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::SetNthInput(int idx, vtkSMSourceProxy *sp)
{
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthInput: " << idx << ", cannot set input. ");
    return;
    }
  
  // Expand array if necessary.
  if (idx >= this->NumberOfInputs)
    {
    this->SetNumberOfInputs(idx + 1);
    }
  
  // Does this change anything?  Yes, it keeps the object from being modified.
  if (sp == this->Inputs[idx])
    {
    return;
    }
  
  if (this->Inputs[idx])
    {
    this->Inputs[idx]->RemoveConsumer(this);
    this->Inputs[idx]->UnRegister(this);
    this->Inputs[idx] = NULL;
    }
  
  if (sp)
    {
    sp->Register(this);
    sp->AddConsumer(this);
    this->Inputs[idx] = sp;
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::AddConsumer(vtkSMSourceProxy *c)
{
  // make sure it isn't already there
  if (this->IsConsumer(c))
    {
    return;
    }
  // add it to the list, reallocate memory
  vtkSMSourceProxy **tmp = this->Consumers;
  this->NumberOfConsumers++;
  this->Consumers = new vtkSMSourceProxy* [this->NumberOfConsumers];
  for (int i = 0; i < (this->NumberOfConsumers-1); i++)
    {
    this->Consumers[i] = tmp[i];
    }
  this->Consumers[this->NumberOfConsumers-1] = c;
  // free old memory
  delete [] tmp;
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::RemoveConsumer(vtkSMSourceProxy *c)
{
  // make sure it is already there
  if (!this->IsConsumer(c))
    {
    return;
    }
  // remove it from the list, reallocate memory
  vtkSMSourceProxy **tmp = this->Consumers;
  this->NumberOfConsumers--;
  this->Consumers = new vtkSMSourceProxy* [this->NumberOfConsumers];
  int cnt = 0;
  int i;
  for (i = 0; i <= this->NumberOfConsumers; i++)
    {
    if (tmp[i] != c)
      {
      this->Consumers[cnt] = tmp[i];
      cnt++;
      }
    }
  // free old memory
  delete [] tmp;
}

//----------------------------------------------------------------------------
int vtkSMSourceProxy::IsConsumer(vtkSMSourceProxy *c)
{
  int i;
  for (i = 0; i < this->NumberOfConsumers; i++)
    {
    if (this->Consumers[i] == c)
      {
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkSMSourceProxy *vtkSMSourceProxy::GetConsumer(int i)
{
  if (i >= this->NumberOfConsumers)
    {
    return 0;
    }
  return this->Consumers[i];
}


//---------------------------------------------------------------------------
void vtkSMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
