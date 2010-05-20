/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSILInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSILInformationHelper.h"

#include "vtkAdjacentVertexIterator.h"
#include "vtkClientServerStream.h"
#include "vtkGraph.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVSILInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMSILModel.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMSILInformationHelper);
vtkCxxSetObjectMacro(vtkSMSILInformationHelper, SIL, vtkGraph);
//----------------------------------------------------------------------------
vtkSMSILInformationHelper::vtkSMSILInformationHelper()
{
  this->SIL = 0;
  this->TimestampCommand = 0;
  this->Subtree = 0;
  this->LastUpdateTime = VTK_INT_MIN;
}

//----------------------------------------------------------------------------
vtkSMSILInformationHelper::~vtkSMSILInformationHelper()
{
  this->SetTimestampCommand(0);
  this->SetSubtree(0);
  this->SetSIL(0);
}

//----------------------------------------------------------------------------
void vtkSMSILInformationHelper::UpdateProperty(
  vtkIdType connectionId,
  int serverIds, vtkClientServerID objectId, vtkSMProperty* prop)
{
  bool refetch = true;
  if (this->TimestampCommand)
    {
    // Get the Mtime from the server. We will re-fetch the SIL only if MTime has
    // changed. This avoids unnecessary SIL fetches which can expensive.
    refetch = this->CheckMTime(connectionId, serverIds, objectId);
    }

  if (!refetch)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVSILInformation* info = vtkPVSILInformation::New();
  pm->GatherInformation(connectionId, serverIds, info, objectId);
  this->SetSIL(info->GetSIL());
  info->Delete();

  // Now update the information property with the array list obtained from the
  // SIL.
  this->UpdateArrayList(vtkSMStringVectorProperty::SafeDownCast(prop));
}

//----------------------------------------------------------------------------
bool vtkSMSILInformationHelper::CheckMTime(
  vtkIdType connectionId, int serverIds, vtkClientServerID objectId)
{
  // Invoke property's method on the root node of the server
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke 
      << objectId 
      << this->TimestampCommand
      << vtkClientServerStream::End;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str);

  // Get the result
  const vtkClientServerStream& res = 
    pm->GetLastResult(connectionId, vtkProcessModule::GetRootId(serverIds));

  int numMsgs = res.GetNumberOfMessages();
  if (numMsgs < 1)
    {
    return true;
    }

  int numArgs = res.GetNumberOfArguments(0);
  if (numArgs < 1)
    {
    return true;
    }

  int argType = res.GetArgumentType(0, 0);

  // If single value, all int types
  if (argType == vtkClientServerStream::int32_value ||
      argType == vtkClientServerStream::int16_value ||
      argType == vtkClientServerStream::int8_value ||
      argType == vtkClientServerStream::bool_value)
    {
    int ires;
    int retVal = res.GetArgument(0, 0, &ires);
    if (!retVal)
      {
      vtkErrorMacro("Error getting argument.");
      return true;
      }
    int cur_lut = this->LastUpdateTime;
    this->LastUpdateTime = ires;
    return (ires > cur_lut);
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMSILInformationHelper::UpdateArrayList(vtkSMStringVectorProperty* svp)
{
  svp->SetNumberOfElementsPerCommand(1);
  svp->SetNumberOfElements(0);

  if (!this->SIL)
    {
    // no SIL. Most probably reader is no ready yet.
    return;
    }

  vtkSmartPointer<vtkSMSILModel> model = vtkSmartPointer<vtkSMSILModel>::New();
  model->Initialize(this->SIL);

  vtkIdType subTreeVertexId = this->Subtree?
    model->FindVertex(this->Subtree) : 0;

  if (subTreeVertexId == -1)
    {
    // failed to locate requested subtree.
    return;
    }

  vtkstd::set<vtkIdType> leaves;
  model->GetLeaves(leaves, subTreeVertexId, false);

  vtkstd::set<vtkIdType>::iterator iter;
  for (iter = leaves.begin(); iter != leaves.end(); ++iter)
    {
    svp->SetElement(svp->GetNumberOfElements(), model->GetName(*iter));
    }
}

//----------------------------------------------------------------------------
int vtkSMSILInformationHelper::ReadXMLAttributes(vtkSMProperty* prop,
  vtkPVXMLElement* elem)
{
  const char* timestamp_command = elem->GetAttribute("timestamp_command");
  if (timestamp_command)
    {
    this->SetTimestampCommand(timestamp_command);
    }

  const char* subtree = elem->GetAttribute("subtree");
  if (subtree)
    {
    this->SetSubtree(subtree);
    }

  return this->Superclass::ReadXMLAttributes(prop, elem);
}

//----------------------------------------------------------------------------
void vtkSMSILInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


