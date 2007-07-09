/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionHelper.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVSelectionInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <vtkstd/set>

vtkStandardNewMacro(vtkSMSelectionHelper);
vtkCxxRevisionMacro(vtkSMSelectionHelper, "1.5");

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::SendSelection(vtkSelection* sel, vtkSMProxy* proxy)
{
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  ostrstream res;
  vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, sel);
  res << ends;
  vtkClientServerStream stream;
  vtkClientServerID parserID =
    processModule->NewStreamObject("vtkSelectionSerializer", stream);
  stream << vtkClientServerStream::Invoke
         << parserID << "Parse" << res.str() << proxy->GetID()
         << vtkClientServerStream::End;
  processModule->DeleteStreamObject(parserID, stream);

  processModule->SendStream(proxy->GetConnectionID(), 
    proxy->GetServers(), 
    stream);
  delete[] res.str();
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelection(
  vtkIdType connectionID,
  vtkSelection* input,
  vtkSelection* output)
{
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelectionInternal(
    connectionID, input, output, 0);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::ConvertSurfaceSelectionToGlobalIDVolumeSelection(
  vtkIdType connectionID,
  vtkSelection* input, vtkSelection* output)
{
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelectionInternal(
    connectionID, input, output, 1);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelectionInternal(
  vtkIdType connectionID,
  vtkSelection* input,
  vtkSelection* output,
  int global_ids)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  // Convert the selection of geometry cells to surface cells of the actual
  // data by:

  // * First sending the selection to the server
  vtkSMProxy* selectionP = pxm->NewProxy("selection_helpers", "Selection");
  selectionP->SetServers(vtkProcessModule::DATA_SERVER);
  selectionP->SetConnectionID(connectionID);
  selectionP->UpdateVTKObjects();
  vtkSMSelectionHelper::SendSelection(input, selectionP);

  // * Then converting the selection using a helper
  vtkSMProxy* volumeSelectionP = 
    pxm->NewProxy("selection_helpers", "Selection");
  volumeSelectionP->SetServers(vtkProcessModule::DATA_SERVER);
  volumeSelectionP->SetConnectionID(connectionID);
  volumeSelectionP->UpdateVTKObjects();
  vtkClientServerStream stream;
  vtkClientServerID converterID =
    processModule->NewStreamObject("vtkSelectionConverter", stream);
  stream << vtkClientServerStream::Invoke
         << converterID 
         << "Convert" 
         << selectionP->GetID() 
         << volumeSelectionP->GetID()
         << global_ids
         << vtkClientServerStream::End;
  processModule->DeleteStreamObject(converterID, stream);
  processModule->SendStream(connectionID, 
                            vtkProcessModule::DATA_SERVER, 
                            stream);

  // * And finally gathering the information back
  vtkPVSelectionInformation* selInfo = vtkPVSelectionInformation::New();
  processModule->GatherInformation(connectionID,
                                   vtkProcessModule::DATA_SERVER, 
                                   selInfo, 
                                   volumeSelectionP->GetID());

  output->ShallowCopy(selInfo->GetSelection());

  selInfo->Delete();
  volumeSelectionP->Delete();
  selectionP->Delete();
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::NewSelectionSourceFromSelection(
  vtkIdType connectionID,
  vtkSelection* selection)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  // Now pass the selection ids to a selection source.
  vtkSMProxy* selectionSourceP = pxm->NewProxy("sources", "SelectionSource");
  selectionSourceP->SetConnectionID(connectionID);
  selectionSourceP->SetServers(vtkProcessModule::DATA_SERVER);

  unsigned int numChildren = selection->GetNumberOfChildren();
  unsigned int childId;
  vtkIdType numIDs=0;

  // Count the total number of ids over. all children
  for(childId=0; childId< numChildren; childId++)
    {
    vtkSelection* child = selection->GetChild(childId);
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(
      child->GetSelectionList());
    if (idList)
      {
      numIDs += idList->GetNumberOfTuples();
      }
    }

  // Add the selection proc ids and cell ids to the IDs property.
  vtkSMIdTypeVectorProperty* ids = vtkSMIdTypeVectorProperty::SafeDownCast(
    selectionSourceP->GetProperty("IDs"));
  ids->SetNumberOfElements(numIDs*2);

  vtkIdType counter = 0;
  for(childId=0; childId< numChildren; childId++)
    {
    vtkSelection* child = selection->GetChild(childId);
    int procID = 0;
    if (child->GetProperties()->Has(vtkSelection::PROCESS_ID()))
      {
      procID = child->GetProperties()->Get(vtkSelection::PROCESS_ID());
      }
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(
      child->GetSelectionList());
    if (idList)
      {
      vtkIdType numValues = idList->GetNumberOfTuples();
      for (vtkIdType idx=0; idx<numValues; idx++)
        {
        ids->SetElement(counter++, procID);
        ids->SetElement(counter++, idList->GetValue(idx));
        }
      }
    }

  /*
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSourceP->GetProperty("ContentType"));
  ivp->SetElement(0, selection->GetProperties()->Get(vtkSelection::CONTENT_TYPE()));

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSourceP->GetProperty("FieldType"));
  ivp->SetElement(0, selection->GetProperties()->Get(vtkSelection::FIELD_TYPE()));
  */

  selectionSourceP->UpdateVTKObjects();
  return selectionSourceP;
}
