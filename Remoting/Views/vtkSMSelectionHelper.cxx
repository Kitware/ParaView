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
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkView.h"

#include <cassert>
#include <list>
#include <map>
#include <regex>
#include <set>
#include <vector>

namespace
{
//------------------------------------------------------------------------------
void ReplaceString(std::string& source, const std::string& replace, const std::string& with)
{
  const std::regex reg("[a-zA-Z0-9]+");
  for (auto match = std::sregex_iterator(source.begin(), source.end(), reg);
       match != std::sregex_iterator(); ++match)
  {
    if (match->str() == replace)
    {
      source.replace(match->position(), match->length(), with);
    }
  }
}
}

//------------------------------------------------------------------------------
const std::string vtkSMSelectionHelper::SubSelectionBaseName = "s";

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSelectionHelper);

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::NewAppendSelectionsFromSelectionSource(
  vtkSMSourceProxy* selectionSource)
{
  if (!selectionSource)
  {
    return nullptr;
  }
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(selectionSource->GetSession());
  vtkSMProxy* appendSelections = pxm->NewProxy("filters", "AppendSelections");
  const std::string selectionName = vtkSMSelectionHelper::SubSelectionBaseName + "0";
  vtkSMPropertyHelper(appendSelections, "Input").Add(selectionSource);
  vtkSMPropertyHelper(appendSelections, "SelectionNames").Set(selectionName.c_str());
  vtkSMPropertyHelper(appendSelections, "Expression").Set(selectionName.c_str());
  appendSelections->UpdateVTKObjects();
  return appendSelections;
}

bool vtkSMSelectionHelper::CombineSelection(vtkSMSourceProxy* appendSelections1,
  vtkSMSourceProxy* appendSelections2, CombineOperation combineOperation, bool deepCopy)
{
  if (!appendSelections1 || !appendSelections2)
  {
    return false;
  }
  if (vtkSMPropertyHelper(appendSelections2, "Input").GetNumberOfElements() == 0)
  {
    return false;
  }
  if (deepCopy)
  {
    if (vtkSMPropertyHelper(appendSelections1, "Input").GetNumberOfElements() == 0)
    {
      // create a combined appendSelections which is deep copy of appendSelections2
      vtkSMSessionProxyManager* pxm = vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(
        appendSelections2->GetSession());
      vtkSmartPointer<vtkSMSourceProxy> combinedAppendSelections;
      combinedAppendSelections.TakeReference(
        vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "AppendSelections")));

      vtkSMPropertyHelper(combinedAppendSelections, "Expression")
        .Set(vtkSMPropertyHelper(appendSelections2, "Expression").GetAsString());
      vtkSMPropertyHelper(combinedAppendSelections, "InsideOut")
        .Set(vtkSMPropertyHelper(appendSelections2, "InsideOut").GetAsInt());

      // add selection input and names of appendSelections2
      unsigned int numInputs =
        vtkSMPropertyHelper(appendSelections2, "Input").GetNumberOfElements();
      for (unsigned int i = 0; i < numInputs; ++i)
      {
        auto selectionSource = vtkSMPropertyHelper(appendSelections2, "Input").GetAsProxy(i);
        vtkSmartPointer<vtkSMSourceProxy> selectionSourceCopy;
        selectionSourceCopy.TakeReference(
          vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", selectionSource->GetXMLName())));
        selectionSourceCopy->Copy(selectionSource);
        selectionSourceCopy->UpdateVTKObjects();

        vtkSMPropertyHelper(combinedAppendSelections, "Input").Add(selectionSourceCopy);
        vtkSMPropertyHelper(combinedAppendSelections, "SelectionNames")
          .Set(i, vtkSMPropertyHelper(appendSelections2, "SelectionNames").GetAsString(i));
      }
      appendSelections2->Copy(combinedAppendSelections);
      appendSelections2->UpdateVTKObjects();
      return true;
    }
  }
  else
  {
    if (vtkSMPropertyHelper(appendSelections1, "Input").GetNumberOfElements() == 0)
    {
      return true;
    }
  }
  if (combineOperation == CombineOperation::DEFAULT)
  {
    return true;
  }
  // appendSelections1 serves as input1 and appendSelections2 serves as input2.
  // The result of this function will be appended into appendSelections2.

  // The appendSelections1 is combine-able with the appendSelections2 if they have the same
  // FieldType/ElementType and if they have the same ContainingCells
  // Checking only one of the inputs is sufficient.
  vtkSMProxy* firstSelectionSourceAP1 =
    vtkSMPropertyHelper(appendSelections1, "Input").GetAsProxy(0);
  vtkSMProxy* firstSelectionSourceAP2 =
    vtkSMPropertyHelper(appendSelections2, "Input").GetAsProxy(0);

  // SelectionQuerySource has element type and not field type
  int fieldTypeOfFirstSelectionSourceAP1 = firstSelectionSourceAP1->GetProperty("FieldType")
    ? vtkSMPropertyHelper(firstSelectionSourceAP1, "FieldType").GetAsInt()
    : vtkSelectionNode::ConvertAttributeTypeToSelectionField(
        vtkSMPropertyHelper(firstSelectionSourceAP1, "ElementType").GetAsInt());
  int fieldTypeOfFirstSelectionSourceAP2 = firstSelectionSourceAP2->GetProperty("FieldType")
    ? vtkSMPropertyHelper(firstSelectionSourceAP2, "FieldType").GetAsInt()
    : vtkSelectionNode::ConvertAttributeTypeToSelectionField(
        vtkSMPropertyHelper(firstSelectionSourceAP2, "ElementType").GetAsInt());
  if (fieldTypeOfFirstSelectionSourceAP1 != fieldTypeOfFirstSelectionSourceAP2)
  {
    return false;
  }

  if (vtkSMPropertyHelper(firstSelectionSourceAP1, "ContainingCells", true).GetAsInt() !=
    vtkSMPropertyHelper(firstSelectionSourceAP2, "ContainingCells", true).GetAsInt())
  {
    return false;
  }

  unsigned int numInputsAP1 = vtkSMPropertyHelper(appendSelections1, "Input").GetNumberOfElements();
  // find the largest selection name id of the appendSelections2
  int maxId = -1;
  for (unsigned int i = 0; i < numInputsAP1; ++i)
  {
    // get the selection name
    std::string selectionName =
      vtkSMPropertyHelper(appendSelections1, "SelectionNames").GetAsString(i);
    // remove the S prefix
    selectionName.erase(0, vtkSMSelectionHelper::SubSelectionBaseName.size());
    // get the id
    maxId = std::max(maxId, std::stoi(selectionName));
  }

  // create new expression and selection names from appendSelections2's selections sources
  std::string newExpressionAP2 = vtkSMPropertyHelper(appendSelections2, "Expression").GetAsString();
  unsigned int numInputsAP2 = vtkSMPropertyHelper(appendSelections2, "Input").GetNumberOfElements();
  std::list<std::string> newSelectionNamesAP2;
  for (int i = static_cast<int>(numInputsAP2) - 1; i >= 0; --i)
  {
    const std::string oldSelectionName = vtkSMPropertyHelper(appendSelections2, "SelectionNames")
                                           .GetAsString(static_cast<unsigned int>(i));
    std::string newSelectionName = oldSelectionName;
    // remove the S prefix
    newSelectionName.erase(0, vtkSMSelectionHelper::SubSelectionBaseName.size());
    // compute new selection name id
    int selectionNameId = std::atoi(newSelectionName.c_str()) + maxId + 1;
    newSelectionName = vtkSMSelectionHelper::SubSelectionBaseName + std::to_string(selectionNameId);
    // save new selection name
    newSelectionNamesAP2.push_front(newSelectionName);
    // update the expression
    ReplaceString(newExpressionAP2, oldSelectionName, newSelectionName);
  }

  // create a combined appendSelections
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(appendSelections2->GetSession());
  vtkSmartPointer<vtkSMSourceProxy> combinedAppendSelections;
  combinedAppendSelections.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "AppendSelections")));
  // add selection input and names of appendSelections1 to appendSelections2
  for (unsigned int i = 0; i < numInputsAP1; ++i)
  {
    auto selectionSource = vtkSMPropertyHelper(appendSelections1, "Input").GetAsProxy(i);
    if (deepCopy)
    {
      vtkSmartPointer<vtkSMSourceProxy> selectionSourceCopy;
      selectionSourceCopy.TakeReference(
        vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", selectionSource->GetXMLName())));
      selectionSourceCopy->Copy(selectionSource);
      selectionSourceCopy->UpdateVTKObjects();
      vtkSMPropertyHelper(combinedAppendSelections, "Input").Add(selectionSourceCopy);
    }
    else
    {
      vtkSMPropertyHelper(combinedAppendSelections, "Input").Add(selectionSource);
    }
    vtkSMPropertyHelper(combinedAppendSelections, "SelectionNames")
      .Set(i, vtkSMPropertyHelper(appendSelections1, "SelectionNames").GetAsString(i));
  }
  auto iter = newSelectionNamesAP2.begin();
  for (unsigned int i = 0; i < numInputsAP2; ++i, ++iter)
  {
    auto selectionSource = vtkSMPropertyHelper(appendSelections2, "Input").GetAsProxy(i);
    if (deepCopy)
    {
      vtkSmartPointer<vtkSMSourceProxy> selectionSourceCopy;
      selectionSourceCopy.TakeReference(
        vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", selectionSource->GetXMLName())));
      selectionSourceCopy->Copy(selectionSource);
      selectionSourceCopy->UpdateVTKObjects();
      vtkSMPropertyHelper(combinedAppendSelections, "Input").Add(selectionSourceCopy);
    }
    else
    {
      vtkSMPropertyHelper(combinedAppendSelections, "Input").Add(selectionSource);
    }
    vtkSMPropertyHelper(combinedAppendSelections, "SelectionNames")
      .Set(numInputsAP1 + i, iter->c_str());
  }

  // add inside out qualifier to the expressions
  const int insideOutAP1 = vtkSMPropertyHelper(appendSelections1, "InsideOut").GetAsInt();
  std::string newExpressionAP1 = vtkSMPropertyHelper(appendSelections1, "Expression").GetAsString();
  if (numInputsAP1 > 1)
  {
    newExpressionAP1 = '(' + newExpressionAP1 + ')';
  }
  newExpressionAP1 = (insideOutAP1 ? "!" : "") + newExpressionAP1;

  const int insideOutAP2 = vtkSMPropertyHelper(appendSelections2, "InsideOut").GetAsInt();
  if (numInputsAP2 > 1)
  {
    newExpressionAP2 = '(' + newExpressionAP2 + ')';
  }
  newExpressionAP2 = (insideOutAP2 ? "!" : "") + newExpressionAP2;

  // combine appendSelections1 and appendSelections2 expressions
  std::string newCombinedExpression;
  switch (combineOperation)
  {
    case CombineOperation::ADDITION:
    {
      newCombinedExpression = newExpressionAP1 + '|' + newExpressionAP2;
      break;
    }
    case CombineOperation::SUBTRACTION:
    {
      newCombinedExpression = newExpressionAP1 + "&!" + newExpressionAP2;
      break;
    }
    case CombineOperation::TOGGLE:
    default:
    {
      newCombinedExpression = newExpressionAP1 + '^' + newExpressionAP2;
      break;
    }
  }
  vtkSMPropertyHelper(combinedAppendSelections, "Expression").Set(newCombinedExpression.c_str());
  appendSelections2->Copy(combinedAppendSelections);
  appendSelections2->UpdateVTKObjects();
  return true;
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
  const char* proxyname = nullptr;
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

    case vtkSelectionNode::BLOCK_SELECTORS:
      proxyname = "BlockSelectorsSelectionSource";
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
    vtkSMPropertyHelper(selSource, "FieldType")
      .Set(selProperties->Get(vtkSelectionNode::FIELD_TYPE()));
  }

  if (selProperties->Has(vtkSelectionNode::CONTAINING_CELLS()))
  {
    vtkSMPropertyHelper(selSource, "ContainingCells")
      .Set(selProperties->Get(vtkSelectionNode::CONTAINING_CELLS()));
  }

  if (selProperties->Has(vtkSelectionNode::INVERSE()))
  {
    vtkSMPropertyHelper(selSource, "InsideOut")
      .Set(selProperties->Get(vtkSelectionNode::INVERSE()));
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
           (cc < max && originalSelSource != nullptr); ++cc)
      {
        block_ids.insert(blocks->GetElement(cc));
      }
      assert(idList->GetNumberOfComponents() == 1 || idList->GetNumberOfTuples() == 0);
      for (vtkIdType cc = 0, max = idList->GetNumberOfTuples(); cc < max; ++cc)
      {
        block_ids.insert(idList->GetValue(cc));
      }
    }
    if (!block_ids.empty())
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
  else if (contentType == vtkSelectionNode::BLOCK_SELECTORS)
  {
    auto sarray = vtkStringArray::SafeDownCast(selection->GetSelectionList());
    const unsigned int count = sarray ? static_cast<unsigned int>(sarray->GetNumberOfValues()) : 0;

    vtkSMPropertyHelper helper(selSource, "BlockSelectors");
    const auto offset = helper.GetNumberOfElements();
    helper.SetNumberOfElements(count + offset);
    for (unsigned int cc = 0; cc < count; ++cc)
    {
      helper.Set(cc + offset, sarray->GetValue(cc).c_str());
    }
    if (const char* aname = sarray ? sarray->GetName() : nullptr)
    {
      vtkSMPropertyHelper(selSource, "BlockSelectorsAssemblyName").Set(aname);
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
      // remove default values set by the XML if we created a brand-new proxy.
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
        vtkIdType compositeIndex = selProperties->Get(vtkSelectionNode::COMPOSITE_INDEX());

        std::vector<vtkIdType> newVals(3 * numIDs);
        for (vtkIdType cc = 0; cc < numIDs; cc++)
        {
          newVals[3 * cc] = compositeIndex;
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
  vtkSMProxy* selSource = nullptr;
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
vtkSMProxy* vtkSMSelectionHelper::ConvertAppendSelections(int outputType,
  vtkSMSourceProxy* appendSelections, vtkSMSourceProxy* dataSource, int dataPort,
  bool& selectionChanged)
{
  selectionChanged = false;
  if (!appendSelections || !dataSource)
  {
    return nullptr;
  }
  unsigned int numInputs = vtkSMPropertyHelper(appendSelections, "Input").GetNumberOfElements();
  // create a new append selections proxy
  vtkSMSourceProxy* newAppendSelections = vtkSMSourceProxy::SafeDownCast(
    appendSelections->GetSessionProxyManager()->NewProxy("filters", "AppendSelections"));
  // copy needed values
  vtkSMPropertyHelper(newAppendSelections, "Expression")
    .Set(vtkSMPropertyHelper(appendSelections, "Expression").GetAsString());
  vtkSMPropertyHelper(newAppendSelections, "InsideOut")
    .Set(vtkSMPropertyHelper(appendSelections, "InsideOut").GetAsInt());

  // convert all sub selections source append selections to the new type
  for (unsigned int i = 0; i < numInputs; ++i)
  {
    auto selectionSource =
      vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy(i));
    // note, if the selection source is already the correct type, it will be returned as is
    // otherwise, a new one will be created
    auto outputTypeSelectionSource =
      vtkSMSourceProxy::SafeDownCast(vtkSMSelectionHelper::ConvertSelectionSource(
        outputType, selectionSource, dataSource, dataPort));
    if (outputTypeSelectionSource)
    {
      // if the selection source is not outputType, update the selection source
      if (outputTypeSelectionSource != selectionSource)
      {
        outputTypeSelectionSource->UpdateVTKObjects();
        selectionChanged = true;
      }
      vtkSMPropertyHelper(newAppendSelections, "Input").Add(outputTypeSelectionSource);
      outputTypeSelectionSource->Delete();
    }
    else
    {
      vtkSMPropertyHelper(newAppendSelections, "Input").Add(selectionSource);
    }
    vtkSMPropertyHelper(newAppendSelections, "SelectionNames")
      .Set(i, vtkSMPropertyHelper(appendSelections, "SelectionNames").GetAsString(i));
  }
  newAppendSelections->UpdateVTKObjects();
  return newAppendSelections;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::ConvertSelectionSource(int outputType,
  vtkSMSourceProxy* selectionSourceProxy, vtkSMSourceProxy* dataSource, int dataPort)
{
  const char* inproxyname = selectionSourceProxy ? selectionSourceProxy->GetXMLName() : nullptr;
  const char* outproxyname = nullptr;
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

    case vtkSelectionNode::BLOCK_SELECTORS:
      outproxyname = "BlockSelectorsSelectionSource";
      break;

    case vtkSelectionNode::INDICES:
    {
      auto dataInformation = dataSource->GetOutputPort(dataPort)->GetDataInformation();
      outproxyname = "IDSelectionSource";
      // check if subclass of vtkUniformGridAMR
      if (dataInformation->GetNumberOfAMRLevels() > 0)
      {
        outproxyname = "HierarchicalDataIDSelectionSource";
      }
      // since it's not subclass of vtkUniformGridAMR, check if it's a composite dataSet
      else if (dataInformation->IsCompositeDataSet())
      {
        outproxyname = "CompositeDataIDSelectionSource";
      }
    }
    break;

    default:
      vtkGenericWarningMacro(
        "Cannot convert to type : " << vtkSelectionNode::GetContentTypeAsString(outputType));
      return nullptr;
  }

  if (selectionSourceProxy && strcmp(inproxyname, outproxyname) == 0)
  {
    // No conversion needed.
    selectionSourceProxy->Register(nullptr);
    return selectionSourceProxy;
  }

  if (outputType == vtkSelectionNode::INDICES && selectionSourceProxy)
  {
    vtkSMVectorProperty* ids = nullptr;
    ids = vtkSMVectorProperty::SafeDownCast(selectionSourceProxy->GetProperty("IDs"));
    // This "if" condition avoid doing any conversion if input is an ID based
    // selection and has no ids.
    if (!ids || ids->GetNumberOfElements() > 0)
    {
      // convert from *anything* to indices.
      return vtkSMSelectionHelper::ConvertInternal(
        selectionSourceProxy, dataSource, dataPort, vtkSelectionNode::INDICES);
    }
  }
  else if (outputType == vtkSelectionNode::GLOBALIDS && selectionSourceProxy)
  {
    vtkSMVectorProperty* ids =
      vtkSMVectorProperty::SafeDownCast(selectionSourceProxy->GetProperty("IDs"));
    // This "if" condition avoid doing any conversion if input is a GLOBALIDS based
    // selection and has no ids.
    if (!ids || ids->GetNumberOfElements() > 0)
    {
      // convert from *anything* to global IDs.
      return vtkSMSelectionHelper::ConvertInternal(
        selectionSourceProxy, dataSource, dataPort, vtkSelectionNode::GLOBALIDS);
    }
  }
  else if ((outputType == vtkSelectionNode::BLOCKS ||
             outputType == vtkSelectionNode::BLOCK_SELECTORS) &&
    selectionSourceProxy &&
    (strcmp(inproxyname, "GlobalIDSelectionSource") == 0 ||
      strcmp(inproxyname, "HierarchicalDataIDSelectionSource") == 0 ||
      strcmp(inproxyname, "CompositeDataIDSelectionSource") == 0))
  {
    return vtkSMSelectionHelper::ConvertInternal(
      selectionSourceProxy, dataSource, dataPort, outputType);
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
  vtkSmartPointer<vtkSMSourceProxy> convertor;
  convertor.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ConvertSelection")));

  vtkSMPropertyHelper(convertor, "Input").Set(inSource, dataPort);
  vtkSMPropertyHelper(convertor, "DataInput").Set(dataSource, dataPort);
  vtkSMPropertyHelper(convertor, "OutputType").Set(outputType);
  vtkSMPropertyHelper(convertor, "AllowMissingArray").Set(true);
  convertor->UpdateVTKObjects();

  // * Request conversion.
  convertor->UpdatePipeline();

  // * And finally gathering the information back
  vtkNew<vtkPVSelectionInformation> selInfo;
  convertor->GatherInformation(selInfo);

  vtkSMProxy* outSource = vtkSMSelectionHelper::NewSelectionSourceFromSelection(
    inSource->GetSession(), selInfo->GetSelection());

  return outSource;
}

namespace
{
// Splits \c selection into a collection of selections based on the
// SOURCE().
void vtkSplitSelection(vtkSelection* selection,
  std::map<vtkPVDataRepresentation*, vtkSmartPointer<vtkSelection>>& map_of_selections)
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
    return nullptr;
  }

  // now locate the proxy for this repr.
  vtkSMPropertyHelper helper(viewProxy, "Representations");
  for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); cc++)
  {
    vtkSMProxy* reprProxy = helper.GetAsProxy(cc);
    vtkPVDataRepresentation* cur_repr =
      vtkPVDataRepresentation::SafeDownCast(reprProxy ? reprProxy->GetClientSideObject() : nullptr);
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
  return nullptr;
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

  std::map<vtkPVDataRepresentation*, vtkSmartPointer<vtkSelection>> selections;
  vtkSplitSelection(selection, selections);

  std::map<vtkPVDataRepresentation*, vtkSmartPointer<vtkSelection>>::iterator iter;
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
