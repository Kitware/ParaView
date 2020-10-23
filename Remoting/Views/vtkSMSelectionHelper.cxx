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
#include "vtkCompositeRepresentation.h"
#include "vtkDataRepresentation.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationStringKey.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVSelectionInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedIntArray.h"
#include "vtkView.h"

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkSMSelectionHelper);

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::NewSelectionSourceFromSelectionInternal(vtkSMSession* session,
  vtkSelectionNode* selection, vtkSMProxy* selSource, bool ignore_composite_keys)
{
  assert("Session need to be provided and need to be valid" && session);

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

    case vtkSelectionNode::VALUES:
      proxyname = "ValueSelectionSource";
      break;

    case vtkSelectionNode::INDICES:
      // we need to choose between IDSelectionSource,
      // CompositeDataIDSelectionSource and HierarchicalDataIDSelectionSource.
      proxyname = "IDSelectionSource";
      if (!ignore_composite_keys && selProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
      {
        proxyname = "CompositeDataIDSelectionSource";
        use_composite = true;
      }
      else if (!ignore_composite_keys &&
        selProperties->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
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

    case vtkSelectionNode::THRESHOLDS:
      proxyname = "ThresholdSelectionSource";
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
    vtkSMSessionProxyManager* pxm =
      vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
    selSource = pxm->NewProxy("sources", proxyname);
  }

  // Set some common property values using the state of the vtkSelection.
  if (selProperties->Has(vtkSelectionNode::FIELD_TYPE()))
  {
    vtkSMIntVectorProperty* ivp =
      vtkSMIntVectorProperty::SafeDownCast(selSource->GetProperty("FieldType"));
    ivp->SetElement(0, selProperties->Get(vtkSelectionNode::FIELD_TYPE()));
  }

  if (selProperties->Has(vtkSelectionNode::CONTAINING_CELLS()))
  {
    vtkSMIntVectorProperty* ivp =
      vtkSMIntVectorProperty::SafeDownCast(selSource->GetProperty("ContainingCells"));
    ivp->SetElement(0, selProperties->Get(vtkSelectionNode::CONTAINING_CELLS()));
  }

  if (selProperties->Has(vtkSelectionNode::INVERSE()))
  {
    vtkSMIntVectorProperty* ivp =
      vtkSMIntVectorProperty::SafeDownCast(selSource->GetProperty("InsideOut"));
    ivp->SetElement(0, selProperties->Get(vtkSelectionNode::INVERSE()));
  }

  if (contentType == vtkSelectionNode::FRUSTUM)
  {
    // Set the selection ids, which is the frustum vertex.
    vtkSMDoubleVectorProperty* dvp =
      vtkSMDoubleVectorProperty::SafeDownCast(selSource->GetProperty("Frustum"));

    vtkDoubleArray* verts = vtkDoubleArray::SafeDownCast(selection->GetSelectionList());
    dvp->SetElements(verts->GetPointer(0));
  }
  else if (contentType == vtkSelectionNode::GLOBALIDS)
  {
    vtkSMIdTypeVectorProperty* ids =
      vtkSMIdTypeVectorProperty::SafeDownCast(selSource->GetProperty("IDs"));
    if (!originalSelSource)
    {
      ids->SetNumberOfElements(0);
    }
    unsigned int curValues = ids->GetNumberOfElements();
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(selection->GetSelectionList());
    if (idList)
    {
      vtkIdType numIDs = idList->GetNumberOfTuples();
      ids->SetNumberOfElements(curValues + numIDs);
      for (vtkIdType cc = 0; cc < numIDs; cc++)
      {
        ids->SetElement(curValues + cc, idList->GetValue(cc));
      }
    }
  }
  else if (contentType == vtkSelectionNode::BLOCKS)
  {
    std::set<vtkIdType> block_ids;
    vtkSMIdTypeVectorProperty* blocks =
      vtkSMIdTypeVectorProperty::SafeDownCast(selSource->GetProperty("Blocks"));
    vtkUnsignedIntArray* idList = vtkUnsignedIntArray::SafeDownCast(selection->GetSelectionList());
    if (idList)
    {
      for (unsigned int cc = 0, max = blocks->GetNumberOfElements();
           (cc < max && originalSelSource != NULL); ++cc)
      {
        block_ids.insert(blocks->GetElement(cc));
      }
      assert(idList->GetNumberOfComponents() == 1 || idList->GetNumberOfTuples() == 0);
      for (vtkIdType cc = 0, max = idList->GetNumberOfTuples(); cc < max; ++cc)
      {
        block_ids.insert(idList->GetValue(cc));
      }
    }
    if (block_ids.size() > 0)
    {
      std::vector<vtkIdType> block_ids_vec(block_ids.size());
      std::copy(block_ids.begin(), block_ids.end(), block_ids_vec.begin());
      blocks->SetElements(block_ids_vec.data(), static_cast<unsigned int>(block_ids_vec.size()));
    }
    else
    {
      blocks->SetNumberOfElements(0);
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
    vtkSMIdTypeVectorProperty* ids =
      vtkSMIdTypeVectorProperty::SafeDownCast(selSource->GetProperty("IDs"));
    if (!originalSelSource)
    {
      // remove default values set by the XML if we created a brand new proxy.
      ids->SetNumberOfElements(0);
    }
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(selection->GetSelectionList());
    if (idList)
    {
      vtkIdType numIDs = idList->GetNumberOfTuples();
      if (!use_composite && !use_hierarchical)
      {
        std::vector<vtkIdType> newVals(2 * numIDs);
        for (vtkIdType cc = 0; cc < numIDs; cc++)
        {
          newVals[2 * cc] = procID;
          newVals[2 * cc + 1] = idList->GetValue(cc);
        }
        ids->AppendElements(newVals.data(), static_cast<unsigned int>(newVals.size()));
      }
      else if (use_composite)
      {
        vtkIdType composite_index = 0;
        if (selProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
        {
          composite_index = selProperties->Get(vtkSelectionNode::COMPOSITE_INDEX());
        }

        std::vector<vtkIdType> newVals(3 * numIDs);
        for (vtkIdType cc = 0; cc < numIDs; cc++)
        {
          newVals[3 * cc] = composite_index;
          newVals[3 * cc + 1] = procID;
          newVals[3 * cc + 2] = idList->GetValue(cc);
        }
        ids->AppendElements(newVals.data(), static_cast<unsigned int>(newVals.size()));
      }
      else if (use_hierarchical)
      {
        vtkIdType level = selProperties->Get(vtkSelectionNode::HIERARCHICAL_LEVEL());
        vtkIdType dsIndex = selProperties->Get(vtkSelectionNode::HIERARCHICAL_INDEX());

        std::vector<vtkIdType> newVals(3 * numIDs);
        for (vtkIdType cc = 0; cc < numIDs; cc++)
        {
          newVals[3 * cc] = level;
          newVals[3 * cc + 1] = dsIndex;
          newVals[3 * cc + 2] = idList->GetValue(cc);
        }
        ids->AppendElements(newVals.data(), static_cast<unsigned int>(newVals.size()));
      }
    }
  }
  else if (contentType == vtkSelectionNode::VALUES)
  {
    vtkIdType procID = -1;
    if (selProperties->Has(vtkSelectionNode::PROCESS_ID()))
    {
      procID = selProperties->Get(vtkSelectionNode::PROCESS_ID());
    }

    vtkSMIdTypeVectorProperty* ids =
      vtkSMIdTypeVectorProperty::SafeDownCast(selSource->GetProperty("Values"));
    unsigned int curValues = ids->GetNumberOfElements();
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(selection->GetSelectionList());
    vtkSMPropertyHelper(selSource, "ArrayName").Set(idList->GetName());
    if (idList)
    {
      vtkIdType numIDs = idList->GetNumberOfTuples();
      ids->SetNumberOfElements(curValues + numIDs * 2);
      for (vtkIdType cc = 0; cc < numIDs; cc++)
      {
        ids->SetElement(curValues + 2 * cc, procID);
        ids->SetElement(curValues + 2 * cc + 1, idList->GetValue(cc));
      }
    }
  }
  else if (contentType == vtkSelectionNode::THRESHOLDS)
  {
    vtkDoubleArray* selectionList = vtkDoubleArray::SafeDownCast(selection->GetSelectionList());
    assert(selectionList);

    vtkSMPropertyHelper(selSource, "ArrayName").Set(selectionList->GetName());
    vtkSMPropertyHelper tHelper(selSource, "Thresholds");
    for (vtkIdType cc = 0, max = selectionList->GetNumberOfTuples(); (cc + 1) < max; cc += 2)
    {
      tHelper.Set(cc, selectionList->GetValue(cc));
      tHelper.Set(cc + 1, selectionList->GetValue(cc + 1));
    }
  }

  return selSource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::NewSelectionSourceFromSelection(
  vtkSMSession* session, vtkSelection* selection, bool ignore_composite_keys)
{
  vtkSMProxy* selSource = 0;
  unsigned int numNodes = selection->GetNumberOfNodes();
  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    vtkSelectionNode* node = selection->GetNode(cc);
    selSource = vtkSMSelectionHelper::NewSelectionSourceFromSelectionInternal(
      session, node, selSource, ignore_composite_keys);
  }
  if (selSource)
  {
    selSource->UpdateVTKObjects();
  }
  return selSource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::ConvertSelection(
  int outputType, vtkSMProxy* selectionSourceProxy, vtkSMSourceProxy* dataSource, int dataPort)
{
  const char* inproxyname = selectionSourceProxy ? selectionSourceProxy->GetXMLName() : 0;
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
      const char* dataName = dataSource->GetOutputPort(dataPort)->GetDataClassName();
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
    ids = vtkSMVectorProperty::SafeDownCast(selectionSourceProxy->GetProperty("IDs"));
    // this "if" condition does not do any conversion in input is GLOBALIDS
    // selection with no ids.

    if (!ids || ids->GetNumberOfElements() > 0)
    {
      // convert from *anything* to indices.
      return vtkSMSelectionHelper::ConvertInternal(
        vtkSMSourceProxy::SafeDownCast(selectionSourceProxy), dataSource, dataPort,
        vtkSelectionNode::INDICES);
    }
  }
  else if (outputType == vtkSelectionNode::GLOBALIDS && selectionSourceProxy)
  {
    vtkSMVectorProperty* ids =
      vtkSMVectorProperty::SafeDownCast(selectionSourceProxy->GetProperty("IDs"));
    // This "if" condition avoid doing any conversion if input is a ID based
    // selection and has no ids.
    if (!ids || ids->GetNumberOfElements() > 0)
    {
      // convert from *anything* to global IDs.
      return vtkSMSelectionHelper::ConvertInternal(
        vtkSMSourceProxy::SafeDownCast(selectionSourceProxy), dataSource, dataPort,
        vtkSelectionNode::GLOBALIDS);
    }
  }
  else if (outputType == vtkSelectionNode::BLOCKS && selectionSourceProxy &&
    (strcmp(inproxyname, "GlobalIDSelectionSource") == 0 ||
             strcmp(inproxyname, "HierarchicalDataIDSelectionSource") == 0 ||
             strcmp(inproxyname, "CompositeDataIDSelectionSource") == 0))
  {
    return vtkSMSelectionHelper::ConvertInternal(
      vtkSMSourceProxy::SafeDownCast(selectionSourceProxy), dataSource, dataPort,
      vtkSelectionNode::BLOCKS);
  }

  // Conversion not possible, so simply create a new proxy of the requested
  // output type with some empty defaults.
  vtkSMSessionProxyManager* pxm = dataSource->GetSessionProxyManager();
  vtkSMProxy* outSource = pxm->NewProxy("sources", outproxyname);
  if (!outSource)
  {
    return outSource;
  }

  // Note that outSource->ConnectionID and outSource->Servers are not yet set.
  if (vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(outSource->GetProperty("IDs")))
  {
    // remove default ID values.
    vp->SetNumberOfElements(0);
  }

  if (selectionSourceProxy)
  {
    // try to copy as many properties from the old-source to the new one.
    outSource->GetProperty("ContainingCells")
      ->Copy(selectionSourceProxy->GetProperty("ContainingCells"));
    outSource->GetProperty("FieldType")->Copy(selectionSourceProxy->GetProperty("FieldType"));
    outSource->GetProperty("InsideOut")->Copy(selectionSourceProxy->GetProperty("InsideOut"));
    outSource->UpdateVTKObjects();
  }
  return outSource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::ConvertInternal(
  vtkSMSourceProxy* inSource, vtkSMSourceProxy* dataSource, int dataPort, int outputType)
{
  vtkSMSessionProxyManager* pxm = dataSource->GetSessionProxyManager();

  // * Update all inputs.
  inSource->UpdatePipeline();
  dataSource->UpdatePipeline();

  // * Filter that converts selections.
  vtkSMSourceProxy* convertor =
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ConvertSelection"));
  // convertor->SetServers(inSource->GetServers());

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(convertor->GetProperty("Input"));
  ip->AddInputConnection(inSource, 0);

  ip = vtkSMInputProperty::SafeDownCast(convertor->GetProperty("DataInput"));
  ip->AddInputConnection(dataSource, dataPort);

  vtkSMIntVectorProperty* ivp =
    vtkSMIntVectorProperty::SafeDownCast(convertor->GetProperty("OutputType"));
  ivp->SetElement(0, outputType);

  vtkSMIntVectorProperty* ivp2 =
    vtkSMIntVectorProperty::SafeDownCast(convertor->GetProperty("AllowMissingArray"));
  ivp2->SetElement(0, true);

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
  vtkSMSourceProxy* output, vtkSMSourceProxy* input, vtkSMSourceProxy* dataSource, int dataPort)
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

  if (vtkSMPropertyHelper(output, "ContainingCells", true).GetAsInt() !=
    vtkSMPropertyHelper(input, "ContainingCells", true).GetAsInt())
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
    std::string inputType = input->GetXMLName();
    std::string outputType = output->GetXMLName();

    if ((inputType == "GlobalIDSelectionSource" && outputType == "IDSelectionSource") ||
      (inputType == "GlobalIDSelectionSource" && outputType == "CompositeDataIDSelectionSource") ||
      (inputType == "IDSelectionSource" && outputType == "GlobalIDSelectionSource") ||
      (inputType == "CompositeDataIDSelectionSource" && outputType == "GlobalIDSelectionSource"))
    {
      int type = vtkSelectionNode::INDICES;
      if (outputType == "GlobalIDSelectionSource")
      {
        type = vtkSelectionNode::GLOBALIDS;
      }

      // Conversion is possible!.
      tempInput.TakeReference(vtkSMSourceProxy::SafeDownCast(
        vtkSMSelectionHelper::ConvertSelection(type, input, dataSource, dataPort)));
      input = tempInput;
    }
    else
    {
      return false;
    }
  }

  // merges IDs, Values or Blocks properties.
  if (output->GetProperty("IDs") && input->GetProperty("IDs"))
  {
    vtkSMPropertyHelper outputIDs(output, "IDs");
    vtkSMPropertyHelper inputIDs(input, "IDs");

    unsigned int cc;
    unsigned int iCount = inputIDs.GetNumberOfElements();
    unsigned int oCount = outputIDs.GetNumberOfElements();
    std::vector<vtkIdType> ids;
    ids.reserve(iCount + oCount);
    for (cc = 0; cc < iCount; cc++)
    {
      ids.emplace_back(inputIDs.GetAsIdType(cc));
    }
    for (cc = 0; cc < oCount; cc++)
    {
      ids.emplace_back(outputIDs.GetAsIdType(cc));
    }
    outputIDs.Set(ids.data(), static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
  }

  if (output->GetProperty("Values") && input->GetProperty("Values"))
  {
    vtkSMPropertyHelper outputIDs(output, "Values");
    vtkSMPropertyHelper inputIDs(input, "Values");

    unsigned int cc;
    unsigned int iCount = inputIDs.GetNumberOfElements();
    unsigned int oCount = outputIDs.GetNumberOfElements();
    std::vector<vtkIdType> ids;
    ids.reserve(iCount + oCount);
    for (cc = 0; cc < iCount; cc++)
    {
      ids.emplace_back(inputIDs.GetAsIdType(cc));
    }
    for (cc = 0; cc < oCount; cc++)
    {
      ids.emplace_back(outputIDs.GetAsIdType(cc));
    }
    outputIDs.Set(ids.data(), static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
  }

  if (output->GetProperty("Blocks") && input->GetProperty("Blocks"))
  {
    vtkSMPropertyHelper outputIDs(output, "Blocks");
    vtkSMPropertyHelper inputIDs(input, "Blocks");

    unsigned int cc;
    unsigned int iCount = inputIDs.GetNumberOfElements();
    unsigned int oCount = outputIDs.GetNumberOfElements();
    std::vector<vtkIdType> ids;
    ids.reserve(iCount + oCount);
    for (cc = 0; cc < iCount; cc++)
    {
      ids.emplace_back(inputIDs.GetAsIdType(cc));
    }
    for (cc = 0; cc < oCount; cc++)
    {
      ids.emplace_back(outputIDs.GetAsIdType(cc));
    }
    outputIDs.Set(ids.data(), static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkSMSelectionHelper::SubtractSelection(
  vtkSMSourceProxy* output, vtkSMSourceProxy* input, vtkSMSourceProxy* dataSource, int dataPort)
{
  if (!output || !input)
  {
    return false;
  }

  // Currently only index based selections i.e. ids, global ids based selections
  // are subtractable and that too only is input and output are identical in all
  // respects (except the indices ofcourse).
  if (vtkSMPropertyHelper(output, "FieldType").GetAsInt() !=
    vtkSMPropertyHelper(input, "FieldType").GetAsInt())
  {
    return false;
  }

  if (vtkSMPropertyHelper(output, "ContainingCells", true).GetAsInt() !=
    vtkSMPropertyHelper(input, "ContainingCells", true).GetAsInt())
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
    std::string inputType = input->GetXMLName();
    std::string outputType = output->GetXMLName();

    if ((inputType == "GlobalIDSelectionSource" && outputType == "IDSelectionSource") ||
      (inputType == "GlobalIDSelectionSource" && outputType == "CompositeDataIDSelectionSource") ||
      (inputType == "IDSelectionSource" && outputType == "GlobalIDSelectionSource") ||
      (inputType == "CompositeDataIDSelectionSource" && outputType == "GlobalIDSelectionSource"))
    {
      int type = vtkSelectionNode::INDICES;
      if (outputType == "GlobalIDSelectionSource")
      {
        type = vtkSelectionNode::GLOBALIDS;
      }

      // Conversion is possible!.
      tempInput.TakeReference(vtkSMSourceProxy::SafeDownCast(
        vtkSMSelectionHelper::ConvertSelection(type, input, dataSource, dataPort)));
      input = tempInput;
    }
    else
    {
      return false;
    }
  }

  // Recover type of selection, so we know what is contained in selection source
  std::string inputType = input->GetXMLName();
  int selectionTupleSize = 1;
  if (inputType == "ThresholdSelectionSource" || inputType == "IDSelectionSource" ||
    inputType == "ValueSelectionSource")
  {
    selectionTupleSize = 2;
  }
  else if (inputType == "CompositeDataIDSelectionSource" ||
    inputType == "HierarchicalDataIDSelectionSource")
  {
    selectionTupleSize = 3;
  }

  // subtract IDs, Values or Blocks properties.
  if (output->GetProperty("IDs") && input->GetProperty("IDs"))
  {
    vtkSMPropertyHelper outputIDs(output, "IDs");
    vtkSMPropertyHelper inputIDs(input, "IDs");

    // Store ids to remove as vector , so set is ordered correctly.
    std::vector<vtkIdType> ids;
    std::set<std::vector<vtkIdType> > idsToRemove;
    unsigned int cc;
    unsigned int count = outputIDs.GetNumberOfElements() / selectionTupleSize;
    for (cc = 0; cc < count; cc++)
    {
      std::vector<vtkIdType> id;
      id.reserve(selectionTupleSize);
      for (int i = 0; i < selectionTupleSize; i++)
      {
        id.push_back(outputIDs.GetAsIdType(cc * selectionTupleSize + i));
      }
      idsToRemove.insert(id);
    }

    // Insert ids only if non present in set.
    count = inputIDs.GetNumberOfElements() / selectionTupleSize;
    for (cc = 0; cc < count; cc++)
    {
      std::vector<vtkIdType> id;
      id.reserve(selectionTupleSize);
      for (int i = 0; i < selectionTupleSize; i++)
      {
        id.push_back(inputIDs.GetAsIdType(cc * selectionTupleSize + i));
      }
      if (idsToRemove.find(id) == idsToRemove.end())
      {
        for (int i = 0; i < selectionTupleSize; i++)
        {
          ids.push_back(id[i]);
        }
      }
    }
    outputIDs.Set(ids.data(), static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
  }

  if (output->GetProperty("Values") && input->GetProperty("Values"))
  {
    vtkSMPropertyHelper outputIDs(output, "Values");
    vtkSMPropertyHelper inputIDs(input, "Values");

    // Store ids to remove as vector, so set is ordered correctly.
    std::vector<vtkIdType> ids;
    std::set<std::vector<vtkIdType> > idsToRemove;
    unsigned int cc;
    unsigned int count = outputIDs.GetNumberOfElements() / selectionTupleSize;
    for (cc = 0; cc < count; cc++)
    {
      std::vector<vtkIdType> id;
      id.reserve(selectionTupleSize);
      for (int i = 0; i < selectionTupleSize; i++)
      {
        id.push_back(outputIDs.GetAsIdType(cc * selectionTupleSize + i));
      }
      idsToRemove.insert(id);
    }

    // Insert ids only if non present in set.
    count = inputIDs.GetNumberOfElements() / selectionTupleSize;
    for (cc = 0; cc < count; cc++)
    {
      std::vector<vtkIdType> id;
      id.reserve(selectionTupleSize);
      for (int i = 0; i < selectionTupleSize; i++)
      {
        id.push_back(inputIDs.GetAsIdType(cc * selectionTupleSize + i));
      }
      if (idsToRemove.find(id) == idsToRemove.end())
      {
        for (int i = 0; i < selectionTupleSize; i++)
        {
          ids.push_back(id[i]);
        }
      }
    }
    outputIDs.Set(ids.data(), static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
  }

  if (output->GetProperty("Blocks") && input->GetProperty("Blocks"))
  {
    vtkSMPropertyHelper outputIDs(output, "Blocks");
    vtkSMPropertyHelper inputIDs(input, "Blocks");

    std::vector<vtkIdType> ids;
    std::set<vtkIdType> idsToRemove;
    unsigned int cc;
    unsigned int count = outputIDs.GetNumberOfElements();
    for (cc = 0; cc < count; cc++)
    {
      idsToRemove.insert(outputIDs.GetAsIdType(cc));
    }

    count = inputIDs.GetNumberOfElements();
    for (cc = 0; cc < count; cc++)
    {
      vtkIdType id = inputIDs.GetAsIdType(cc);
      if (idsToRemove.find(id) == idsToRemove.end())
      {
        ids.push_back(inputIDs.GetAsIdType(cc));
      }
    }
    outputIDs.Set(ids.data(), static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkSMSelectionHelper::ToggleSelection(
  vtkSMSourceProxy* output, vtkSMSourceProxy* input, vtkSMSourceProxy* dataSource, int dataPort)
{
  if (!output || !input)
  {
    return false;
  }

  // Currently only index based selections i.e. ids, global ids based selections
  // are subtractable and that too only is input and output are identical in all
  // respects (except the indices ofcourse).
  if (vtkSMPropertyHelper(output, "FieldType").GetAsInt() !=
    vtkSMPropertyHelper(input, "FieldType").GetAsInt())
  {
    return false;
  }

  if (vtkSMPropertyHelper(output, "ContainingCells", true).GetAsInt() !=
    vtkSMPropertyHelper(input, "ContainingCells", true).GetAsInt())
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
    std::string inputType = input->GetXMLName();
    std::string outputType = output->GetXMLName();

    if ((inputType == "GlobalIDSelectionSource" && outputType == "IDSelectionSource") ||
      (inputType == "GlobalIDSelectionSource" && outputType == "CompositeDataIDSelectionSource") ||
      (inputType == "IDSelectionSource" && outputType == "GlobalIDSelectionSource") ||
      (inputType == "CompositeDataIDSelectionSource" && outputType == "GlobalIDSelectionSource"))
    {
      int type = vtkSelectionNode::INDICES;
      if (outputType == "GlobalIDSelectionSource")
      {
        type = vtkSelectionNode::GLOBALIDS;
      }

      // Conversion is possible!.
      tempInput.TakeReference(vtkSMSourceProxy::SafeDownCast(
        vtkSMSelectionHelper::ConvertSelection(type, input, dataSource, dataPort)));
      input = tempInput;
    }
    else
    {
      return false;
    }
  }

  // Recover type of selection, so we know what is contained in selection source
  std::string inputType = input->GetXMLName();
  int selectionTupleSize = 1;
  if (inputType == "ThresholdSelectionSource" || inputType == "IDSelectionSource")
  {
    selectionTupleSize = 2;
  }
  else if (inputType == "CompositeDataIDSelectionSource" ||
    inputType == "HierarchicalDataIDSelectionSource")
  {
    selectionTupleSize = 3;
  }

  // Toggle IDs or Blocks properties.
  if (output->GetProperty("IDs") && input->GetProperty("IDs"))
  {
    vtkSMPropertyHelper outputIDs(output, "IDs");
    vtkSMPropertyHelper inputIDs(input, "IDs");

    // Store ids to toggle as vector , so set is ordered correctly.
    std::vector<vtkIdType> ids;
    std::set<std::vector<vtkIdType> > idsToToggle;
    unsigned int cc;
    unsigned int count = outputIDs.GetNumberOfElements() / selectionTupleSize;
    for (cc = 0; cc < count; cc++)
    {
      std::vector<vtkIdType> id;
      for (int i = 0; i < selectionTupleSize; i++)
      {
        id.push_back(outputIDs.GetAsIdType(cc * selectionTupleSize + i));
      }
      idsToToggle.insert(id);
    }

    // Insert ids only if non present in set. remove for idToToggle if present
    count = inputIDs.GetNumberOfElements() / selectionTupleSize;
    for (cc = 0; cc < count; cc++)
    {
      std::vector<vtkIdType> id;
      for (int i = 0; i < selectionTupleSize; i++)
      {
        id.push_back(inputIDs.GetAsIdType(cc * selectionTupleSize + i));
      }
      if (idsToToggle.find(id) == idsToToggle.end())
      {
        for (int i = 0; i < selectionTupleSize; i++)
        {
          ids.push_back(id[i]);
        }
      }
      else
      {
        idsToToggle.erase(id);
      }
    }

    // Insert new element, remaining in idToToggle
    for (std::set<std::vector<vtkIdType> >::const_iterator it = idsToToggle.begin();
         it != idsToToggle.end(); it++)
    {
      for (int i = 0; i < selectionTupleSize; i++)
      {
        ids.push_back((*it)[i]);
      }
    }

    outputIDs.Set(ids.data(), static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
  }

  if (output->GetProperty("Blocks") && input->GetProperty("Blocks"))
  {
    vtkSMPropertyHelper outputIDs(output, "Blocks");
    vtkSMPropertyHelper inputIDs(input, "Blocks");

    // Store ids to toggle as vector , so set is ordered correctly.
    std::vector<vtkIdType> ids;
    std::set<std::vector<vtkIdType> > idsToToggle;
    unsigned int cc;
    unsigned int count = outputIDs.GetNumberOfElements() / selectionTupleSize;
    for (cc = 0; cc < count; cc++)
    {
      std::vector<vtkIdType> id;
      for (int i = 0; i < selectionTupleSize; i++)
      {
        id.push_back(outputIDs.GetAsIdType(cc * selectionTupleSize + i));
      }
      idsToToggle.insert(id);
    }

    // Insert ids only if non present in set. remove for idToToggle if present
    count = inputIDs.GetNumberOfElements() / selectionTupleSize;
    for (cc = 0; cc < count; cc++)
    {
      std::vector<vtkIdType> id;
      for (int i = 0; i < selectionTupleSize; i++)
      {
        id.push_back(inputIDs.GetAsIdType(cc * selectionTupleSize + i));
      }
      if (idsToToggle.find(id) == idsToToggle.end())
      {
        for (int i = 0; i < selectionTupleSize; i++)
        {
          ids.push_back(id[i]);
        }
      }
      else
      {
        idsToToggle.erase(id);
      }
    }

    // Insert new element, remaining in idToToggle
    for (std::set<std::vector<vtkIdType> >::const_iterator it = idsToToggle.begin();
         it != idsToToggle.end(); it++)
    {
      for (int i = 0; i < selectionTupleSize; i++)
      {
        ids.push_back((*it)[i]);
      }
    }

    outputIDs.Set(ids.data(), static_cast<unsigned int>(ids.size()));
    output->UpdateVTKObjects();
    return true;
  }
  return false;
}

namespace
{
// Splits \c selection into a collection of selections based on the
// SOURCE().
void vtkSplitSelection(vtkSelection* selection,
  std::map<vtkPVDataRepresentation*, vtkSmartPointer<vtkSelection> >& map_of_selections)
{
  for (unsigned int cc = 0; cc < selection->GetNumberOfNodes(); cc++)
  {
    vtkSelectionNode* node = selection->GetNode(cc);
    if (node && node->GetProperties()->Has(vtkSelectionNode::SOURCE()))
    {
      vtkPVDataRepresentation* repr = vtkPVDataRepresentation::SafeDownCast(
        node->GetProperties()->Get(vtkSelectionNode::SOURCE()));
      vtkSelection* sel = map_of_selections[repr];
      if (!sel)
      {
        sel = vtkSelection::New();
        map_of_selections[repr] = sel;
        sel->FastDelete();
      }
      sel->AddNode(node);
    }
  }
}

vtkSMProxy* vtkLocateRepresentation(vtkSMProxy* viewProxy, vtkPVDataRepresentation* repr)
{
  vtkView* view = vtkView::SafeDownCast(viewProxy->GetClientSideObject());
  if (!view)
  {
    vtkGenericWarningMacro("View proxy must be a proxy for vtkView.");
    return NULL;
  }

  // now locate the proxy for this repr.
  vtkSMPropertyHelper helper(viewProxy, "Representations");
  for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); cc++)
  {
    vtkSMProxy* reprProxy = helper.GetAsProxy(cc);
    vtkPVDataRepresentation* cur_repr =
      vtkPVDataRepresentation::SafeDownCast(reprProxy ? reprProxy->GetClientSideObject() : NULL);
    if (cur_repr == repr)
    {
      return reprProxy;
    }
    vtkCompositeRepresentation* compRepr = vtkCompositeRepresentation::SafeDownCast(cur_repr);
    if (compRepr && compRepr->GetActiveRepresentation() == repr)
    {
      return reprProxy;
    }
  }
  return NULL;
}

bool vtkInputIsComposite(vtkSMProxy* proxy)
{
  vtkSMPropertyHelper helper(proxy, "Input", true);
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0));
  if (input)
  {
    vtkPVDataInformation* info = input->GetDataInformation(helper.GetOutputPort(0));
    return (info->GetCompositeDataSetType() != -1);
  }
  return false;
}
};

//----------------------------------------------------------------------------
void vtkSMSelectionHelper::NewSelectionSourcesFromSelection(vtkSelection* selection,
  vtkSMProxy* view, vtkCollection* selSources, vtkCollection* selRepresentations)
{
  // Now selection can comprise of selection nodes for more than on
  // representation that was selected. We now need to create selection source
  // proxies for each representation that was selected separately.

  // This relies on SOURCE() defined on the selection nodes to locate the
  // representation proxy for the representation that was selected.

  std::map<vtkPVDataRepresentation*, vtkSmartPointer<vtkSelection> > selections;
  vtkSplitSelection(selection, selections);

  std::map<vtkPVDataRepresentation*, vtkSmartPointer<vtkSelection> >::iterator iter;
  for (iter = selections.begin(); iter != selections.end(); ++iter)
  {
    vtkSMProxy* reprProxy = vtkLocateRepresentation(view, iter->first);
    if (!reprProxy)
    {
      continue;
    }

    // determine if input dataset to this representation is a composite dataset,
    // if not, we ignore the composite-ids that may be present in the selection.
    bool input_is_composite_dataset = vtkInputIsComposite(reprProxy);

    vtkSMProxy* selSource = vtkSMSelectionHelper::NewSelectionSourceFromSelection(
      view->GetSession(), iter->second, input_is_composite_dataset == false);
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
