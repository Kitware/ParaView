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
#include "vtkDataRepresentation.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationStringKey.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVSelectionInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMSession.h"
#include "vtkUnsignedIntArray.h"
#include "vtkView.h"

#include <vtkstd/vector>
#include <vtkstd/map>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMSelectionHelper);

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::NewSelectionSourceFromSelectionInternal(
  vtkSMSession* vtkNotUsed(session),
  vtkSelectionNode* selection,
  vtkSMProxy* selSource /*=NULL*/)
{
  if (!selection || !selection->GetSelectionList())
    {
    return selSource;
    }

  vtkSMProxy* originalSelSource = selSource;

  vtkInformation* selProperties = selection->GetProperties();
  int contentType = selection->GetContentType();

  // Determine the type of selection source proxy to create that will
  // generate the a vtkSelection same the "selection" instance passed as an
  // argument.
  const char* proxyname = 0;
  bool use_composite = false;
  bool use_hierarchical = false;
  switch (contentType)
    {
  case -1:
    // ContentType is not defined. Empty selection.
    return selSource;

  case vtkSelectionNode::FRUSTUM:
    proxyname = "FrustumSelectionSource";
    break;

  case vtkSelectionNode::INDICES:
    // we need to choose between IDSelectionSource,
    // CompositeDataIDSelectionSource and HierarchicalDataIDSelectionSource.
    proxyname = "IDSelectionSource";
    if (selProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
      {
      proxyname = "CompositeDataIDSelectionSource";
      use_composite = true;
      }
    else if (selProperties->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) && 
       selProperties->Has(vtkSelectionNode::HIERARCHICAL_INDEX()))
      {
      proxyname = "HierarchicalDataIDSelectionSource";
      use_hierarchical = true;
      use_composite = false;
      }
    break;

  case vtkSelectionNode::GLOBALIDS:
    proxyname = "GlobalIDSelectionSource";
    break;

  case vtkSelectionNode::BLOCKS:
    proxyname = "BlockSelectionSource";
    break;

  default:
    vtkGenericWarningMacro("Unhandled ContentType: " << contentType);
    return selSource;
    }

  if (selSource && strcmp(selSource->GetXMLName(), proxyname) != 0)
    {
    vtkGenericWarningMacro("A composite selection has different types of selections."
      "This is not supported.");
    return selSource;
    }
  
  if (!selSource)
    {
    // If selSource is not present we need to create a new one. The type of
    // proxy we instantiate depends on the type of the vtkSelection.
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    selSource = pxm->NewProxy("sources", proxyname);
    }


  // Set some common property values using the state of the vtkSelection.
  if (selProperties->Has(vtkSelectionNode::FIELD_TYPE()))
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      selSource->GetProperty("FieldType"));
    ivp->SetElement(0, selProperties->Get(vtkSelectionNode::FIELD_TYPE()));
    }

  if (selProperties->Has(vtkSelectionNode::CONTAINING_CELLS()))
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      selSource->GetProperty("ContainingCells"));
    ivp->SetElement(0, selProperties->Get(vtkSelectionNode::CONTAINING_CELLS()));
    }

  if (selProperties->Has(vtkSelectionNode::INVERSE()))
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      selSource->GetProperty("InsideOut"));
    ivp->SetElement(0, selProperties->Get(vtkSelectionNode::INVERSE()));
    }

  if (contentType == vtkSelectionNode::FRUSTUM)
    {
    // Set the selection ids, which is the frustum vertex.
    vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      selSource->GetProperty("Frustum"));

    vtkDoubleArray* verts = vtkDoubleArray::SafeDownCast(
      selection->GetSelectionList());
    dvp->SetElements(verts->GetPointer(0));
    }
  else if (contentType == vtkSelectionNode::GLOBALIDS)
    {
    vtkSMIdTypeVectorProperty* ids = vtkSMIdTypeVectorProperty::SafeDownCast(
      selSource->GetProperty("IDs"));
    if (!originalSelSource)
      {
      ids->SetNumberOfElements(0);
      }
    unsigned int curValues = ids->GetNumberOfElements();
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(
      selection->GetSelectionList());
    if (idList)
      {
      vtkIdType numIDs = idList->GetNumberOfTuples();
      ids->SetNumberOfElements(curValues+numIDs);
      for (vtkIdType cc=0; cc < numIDs; cc++)
        {
        ids->SetElement(curValues+cc, idList->GetValue(cc));
        }
      }
    }
  else if (contentType == vtkSelectionNode::BLOCKS)
    {
    vtkSMIdTypeVectorProperty* blocks = vtkSMIdTypeVectorProperty::SafeDownCast(
      selSource->GetProperty("Blocks"));
    if (!originalSelSource)
      {
      blocks->SetNumberOfElements(0);
      }
    unsigned int curValues = blocks->GetNumberOfElements();
    vtkUnsignedIntArray* idList = vtkUnsignedIntArray::SafeDownCast(
      selection->GetSelectionList());
    if (idList)
      {
      vtkIdType numIDs = idList->GetNumberOfTuples();
      blocks->SetNumberOfElements(curValues+numIDs);
      for (vtkIdType cc=0; cc < numIDs; cc++)
        {
        blocks->SetElement(curValues+cc, idList->GetValue(cc));
        }
      }
    }
  else if (contentType == vtkSelectionNode::INDICES)
    {
    vtkIdType procID = -1;
    if (selProperties->Has(vtkSelectionNode::PROCESS_ID()))
      {
      procID = selProperties->Get(vtkSelectionNode::PROCESS_ID());
      }

    // Add the selection proc ids and cell ids to the IDs property.
    vtkSMIdTypeVectorProperty* ids = vtkSMIdTypeVectorProperty::SafeDownCast(
      selSource->GetProperty("IDs"));
    if (!originalSelSource)
      {
      // remove default values set by the XML if we created a brand new proxy.
      ids->SetNumberOfElements(0);
      }
    unsigned int curValues = ids->GetNumberOfElements();
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(
      selection->GetSelectionList());
    if (idList)
      {
      vtkIdType numIDs = idList->GetNumberOfTuples();
      if (!use_composite && !use_hierarchical)
        {
        ids->SetNumberOfElements(curValues+numIDs*2);
        for (vtkIdType cc=0; cc < numIDs; cc++)
          {
          ids->SetElement(curValues+2*cc, procID);
          ids->SetElement(curValues+2*cc+1, idList->GetValue(cc));
          }
        }
      else if (use_composite)
        {
        vtkIdType composite_index = 0;
        if (selProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
          {
          composite_index = selProperties->Get(vtkSelectionNode::COMPOSITE_INDEX());
          }

        ids->SetNumberOfElements(curValues+numIDs*3);
        for (vtkIdType cc=0; cc < numIDs; cc++)
          {
          ids->SetElement(curValues+3*cc, composite_index);
          ids->SetElement(curValues+3*cc+1, procID);
          ids->SetElement(curValues+3*cc+2, idList->GetValue(cc));
          }
        }
      else if (use_hierarchical)
        {
        vtkIdType level = selProperties->Get(vtkSelectionNode::HIERARCHICAL_LEVEL());
        vtkIdType dsIndex = selProperties->Get(vtkSelectionNode::HIERARCHICAL_INDEX());
        ids->SetNumberOfElements(curValues+numIDs*3);
        for (vtkIdType cc=0; cc < numIDs; cc++)
          {
          ids->SetElement(curValues+3*cc, level);
          ids->SetElement(curValues+3*cc+1, dsIndex);
          ids->SetElement(curValues+3*cc+2, idList->GetValue(cc));
          }
        }
      }
    }

  return selSource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::NewSelectionSourceFromSelection(
  vtkSMSession* session, vtkSelection* selection)
{
  vtkSMProxy* selSource= 0;
  unsigned int numNodes = selection->GetNumberOfNodes(); 
  for (unsigned int cc=0; cc < numNodes; cc++)
    {
    vtkSelectionNode* node = selection->GetNode(cc);
    selSource = vtkSMSelectionHelper::NewSelectionSourceFromSelectionInternal(
      session, node, selSource);
    }
  if (selSource)
    {
    selSource->UpdateVTKObjects();
    }
  return selSource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::ConvertSelection(int outputType,
  vtkSMProxy* selectionSourceProxy,
  vtkSMSourceProxy* dataSource, int dataPort)
{
  const char* inproxyname = selectionSourceProxy? 
    selectionSourceProxy->GetXMLName() : 0;
  const char* outproxyname = 0;
  switch (outputType)
    {
  case vtkSelectionNode::GLOBALIDS:
    outproxyname = "GlobalIDSelectionSource";
    break;

  case vtkSelectionNode::FRUSTUM:
    outproxyname = "FrustumSelectionSource";
    break;

  case vtkSelectionNode::LOCATIONS:
    outproxyname = "LocationSelectionSource";
    break;

  case vtkSelectionNode::THRESHOLDS:
    outproxyname = "ThresholdSelectionSource";
    break;

  case vtkSelectionNode::BLOCKS:
    outproxyname = "BlockSelectionSource";
    break;

  case vtkSelectionNode::INDICES:
      {
      const char* dataName = 
        dataSource->GetOutputPort(dataPort)->GetDataClassName();
      outproxyname = "IDSelectionSource";
      if (dataName)
        {
        if (strcmp(dataName, "vtkHierarchicalBoxDataSet") == 0)
          {
          outproxyname = "HierarchicalDataIDSelectionSource";
          }
        else if (strcmp(dataName, "vtkMultiBlockDataSet") == 0)
          {
          outproxyname = "CompositeDataIDSelectionSource";
          }
        }
      }
    break;

  default:
    vtkGenericWarningMacro("Cannot convert to type : " << outputType);
    return 0;
    }

  if (selectionSourceProxy && strcmp(inproxyname, outproxyname) == 0)
    {
    // No conversion needed.
    selectionSourceProxy->Register(0);
    return selectionSourceProxy;
    }

  if (outputType == vtkSelectionNode::INDICES && selectionSourceProxy)
    {
    vtkSMVectorProperty* ids = 0;
    ids = vtkSMVectorProperty::SafeDownCast(
      selectionSourceProxy->GetProperty("IDs"));
    // this "if" condition does not do any conversion in input is GLOBALIDS
    // selection with no ids.
    
    if (!ids || ids->GetNumberOfElements() > 0)
      {
      // convert from *anything* to indices.
      return vtkSMSelectionHelper::ConvertInternal(
        vtkSMSourceProxy::SafeDownCast(selectionSourceProxy),
        dataSource, dataPort, vtkSelectionNode::INDICES);
      }
    }
  else if (outputType == vtkSelectionNode::GLOBALIDS && selectionSourceProxy)
    {
    vtkSMVectorProperty* ids = vtkSMVectorProperty::SafeDownCast(
      selectionSourceProxy->GetProperty("IDs"));
    // This "if" condition avoid doing any conversion if input is a ID based
    // selection and has no ids.
    if (!ids || ids->GetNumberOfElements() > 0)
      {
      // convert from *anything* to global IDs.
      return vtkSMSelectionHelper::ConvertInternal(
        vtkSMSourceProxy::SafeDownCast(selectionSourceProxy),
        dataSource, dataPort, vtkSelectionNode::GLOBALIDS);
      }
    }
  else if (outputType == vtkSelectionNode::BLOCKS && selectionSourceProxy &&
    (strcmp(inproxyname, "GlobalIDSelectionSource") == 0 ||
     strcmp(inproxyname, "HierarchicalDataIDSelectionSource") == 0||
     strcmp(inproxyname, "CompositeDataIDSelectionSource")==0))
    {
    return vtkSMSelectionHelper::ConvertInternal(
      vtkSMSourceProxy::SafeDownCast(selectionSourceProxy),
      dataSource, dataPort, vtkSelectionNode::BLOCKS);
    }

  // Conversion not possible, so simply create a new proxy of the requested
  // output type with some empty defaults.
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMProxy* outSource = pxm->NewProxy("sources", outproxyname);
  if (!outSource)
    {
    return outSource;
    }

  // Note that outSource->ConnectionID and outSource->Servers are not yet set.
  if (vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(
      outSource->GetProperty("IDs")))
    {
    // remove default ID values.
    vp->SetNumberOfElements(0);
    }

  if (selectionSourceProxy)
    {
    // try to copy as many properties from the old-source to the new one.
    outSource->GetProperty("ContainingCells")->Copy(
      selectionSourceProxy->GetProperty("ContainingCells"));
    outSource->GetProperty("FieldType")->Copy(
      selectionSourceProxy->GetProperty("FieldType"));
    outSource->GetProperty("InsideOut")->Copy(
      selectionSourceProxy->GetProperty("InsideOut"));
    outSource->UpdateVTKObjects();
    }
  return outSource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::ConvertInternal(
  vtkSMSourceProxy* inSource, vtkSMSourceProxy* dataSource,
  int dataPort, int outputType)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  // * Update all inputs.
  inSource->UpdatePipeline();
  dataSource->UpdatePipeline();

  // * Filter that converts selections.
  vtkSMSourceProxy* convertor = vtkSMSourceProxy::SafeDownCast(
    pxm->NewProxy("filters", "ConvertSelection"));
  //convertor->SetServers(inSource->GetServers());

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    convertor->GetProperty("Input"));
  ip->AddInputConnection(inSource, 0);

  ip = vtkSMInputProperty::SafeDownCast(convertor->GetProperty("DataInput"));
  ip->AddInputConnection(dataSource, dataPort);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    convertor->GetProperty("OutputType"));
  ivp->SetElement(0, outputType);
  convertor->UpdateVTKObjects();

  // * Request conversion.
  convertor->UpdatePipeline();

  // * And finally gathering the information back
  vtkPVSelectionInformation* selInfo = vtkPVSelectionInformation::New();
  convertor->GatherInformation(selInfo);

  vtkSMProxy* outSource = vtkSMSelectionHelper::NewSelectionSourceFromSelection(
    inSource->GetSession(), selInfo->GetSelection());

  // cleanup.
  convertor->Delete();
  selInfo->Delete();
  return outSource;
}

//-----------------------------------------------------------------------------
bool vtkSMSelectionHelper::MergeSelection(
  vtkSMSourceProxy* output, vtkSMSourceProxy* input,
  vtkSMSourceProxy* dataSource, int dataPort)
{
  if (!output || !input)
    {
    return false;
    }

  // Currently only index based selections i.e. ids, global ids based selections
  // are mergeable and that too only is input and output are identical in all
  // respects (except the indices ofcourse).
  if (vtkSMPropertyHelper(output, "FieldType").GetAsInt() != 
    vtkSMPropertyHelper(input, "FieldType").GetAsInt())
    {
    return false;
    }

  if (vtkSMPropertyHelper(output, "ContainingCells").GetAsInt() != 
    vtkSMPropertyHelper(input, "ContainingCells").GetAsInt())
    {
    return false;
    }

  if (vtkSMPropertyHelper(output, "InsideOut").GetAsInt() != 
    vtkSMPropertyHelper(input, "InsideOut").GetAsInt())
    {
    return false;
    }

  vtkSmartPointer<vtkSMSourceProxy> tempInput;
  if (strcmp(output->GetXMLName(), input->GetXMLName()) != 0)
    {
    // before totally giving up, check to see if the input selection can be
    // converted to the same type as the output.
    vtkstd::string inputType = input->GetXMLName();
    vtkstd::string outputType = output->GetXMLName();

    if (
      (inputType == "GlobalIDSelectionSource" &&
       outputType == "IDSelectionSource") || 
      (inputType == "GlobalIDSelectionSource" &&
       outputType == "CompositeDataIDSelectionSource") ||
      (inputType == "IDSelectionSource" &&
       outputType == "GlobalIDSelectionSource") ||
      (inputType == "CompositeDataIDSelectionSource" &&
       outputType == "GlobalIDSelectionSource"))
      {
      int type = vtkSelectionNode::INDICES;
      if (outputType == "GlobalIDSelectionSource")
        {
        type = vtkSelectionNode::GLOBALIDS;
        }

      // Conversion is possible!.
      tempInput.TakeReference(vtkSMSourceProxy::SafeDownCast(
          vtkSMSelectionHelper::ConvertSelection(type,
            input,
            dataSource,
            dataPort)));
      input = tempInput;
      }
    else
      {
      return false;
      }
    }


  // mergs IDs or Blocks properties.
  if (output->GetProperty("IDs") && input->GetProperty("IDs"))
    {
    vtkSMPropertyHelper outputIDs(output, "IDs");
    vtkSMPropertyHelper inputIDs(input, "IDs");

    vtkstd::vector<vtkIdType> ids;
    unsigned int cc;
    unsigned int count = inputIDs.GetNumberOfElements();
    for (cc=0; cc < count; cc++)
      {
      ids.push_back(inputIDs.GetAsIdType(cc));
      }
    count = outputIDs.GetNumberOfElements();
    for (cc=0; cc < count; cc++)
      {
      ids.push_back(outputIDs.GetAsIdType(cc));
      }
    outputIDs.Set(&ids[0], static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
    }

  if (output->GetProperty("Blocks") && input->GetProperty("Blocks"))
    {
    vtkSMPropertyHelper outputIDs(output, "Blocks");
    vtkSMPropertyHelper inputIDs(input, "Blocks");

    vtkstd::vector<vtkIdType> ids;
    unsigned int cc;
    unsigned int count = inputIDs.GetNumberOfElements();
    for (cc=0; cc < count; cc++)
      {
      ids.push_back(inputIDs.GetAsIdType(cc));
      }
    count = outputIDs.GetNumberOfElements();
    for (cc=0; cc < count; cc++)
      {
      ids.push_back(outputIDs.GetAsIdType(cc));
      }
    outputIDs.Set(&ids[0], static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
    }

  return false;
}


namespace
{
  // Splits \c selection into a collection of selections based on the
  // SOURCE_ID().
  void vtkSplitSelection(vtkSelection* selection,
    vtkstd::map<int, vtkSmartPointer<vtkSelection> >& map_of_selections)
    {
    for (unsigned int cc=0; cc < selection->GetNumberOfNodes(); cc++)
      {
      vtkSelectionNode* node = selection->GetNode(cc);
      if (node && node->GetProperties()->Has(vtkSelectionNode::SOURCE_ID()))
        {
        int source_id =
          node->GetProperties()->Get(vtkSelectionNode::SOURCE_ID());
        vtkSelection* sel = map_of_selections[source_id];
        if (!sel)
          {
          sel = vtkSelection::New();
          map_of_selections[source_id] = sel;
          sel->FastDelete();
          }
        sel->AddNode(node);
        }
      }
    }

  vtkSMProxy* vtkLocateRepresentation(vtkSMProxy* viewProxy, int id)
    {
    vtkView* view = vtkView::SafeDownCast(viewProxy->GetClientSideObject());
    if (!view)
      {
      vtkGenericWarningMacro("View proxy must be a proxy for vtkView.");
      return NULL;
      }

    vtkDataRepresentation* repr = view->GetRepresentation(id);
    // now locate the proxy for this repr.
    vtkSMPropertyHelper helper(viewProxy, "Representations");
    for (unsigned int cc=0; cc < helper.GetNumberOfElements(); cc++)
      {
      vtkSMProxy* reprProxy = helper.GetAsProxy(cc);
      if (reprProxy && reprProxy->GetClientSideObject() == repr)
        {
        return reprProxy;
        }
      }
    return NULL;
    }
};

//----------------------------------------------------------------------------
void vtkSMSelectionHelper::NewSelectionSourcesFromSelection(
  vtkSelection* selection, vtkSMProxy* view,
  vtkCollection* selSources, vtkCollection* selRepresentations)
{
  // Now selection can comprise of selection nodes for more than on
  // representation that was selected. We now need to create selection source
  // proxies for each representation that was selected separately.

  // This relies on SOURCE_ID() defined on the selection nodes to locate the
  // representation proxy for the representation that was selected.

  vtkstd::map<int, vtkSmartPointer<vtkSelection> > selections;
  vtkSplitSelection(selection, selections);

  vtkstd::map<int, vtkSmartPointer<vtkSelection> >::iterator iter;
  for (iter = selections.begin(); iter != selections.end(); ++iter)
    {
    vtkSMProxy* reprProxy = vtkLocateRepresentation(view, iter->first);
    if (!reprProxy)
      {
      continue;
      }

    vtkSMProxy* selSource =
      vtkSMSelectionHelper::NewSelectionSourceFromSelection(
        view->GetSession(), iter->second.GetPointer());
    if (!selSource)
      {
      continue;
      }
    // locate representation proxy for this
    selSources->AddItem(selSource);
    selRepresentations->AddItem(reprProxy);
    selSource->FastDelete();
    }
}
