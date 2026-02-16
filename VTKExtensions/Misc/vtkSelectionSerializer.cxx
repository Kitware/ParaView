// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSelectionSerializer.h"

#include "vtkArrayDispatch.h"
#include "vtkClientServerStreamInstantiator.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStringArray.h"

#include <iostream>

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSelectionSerializer);

vtkInformationKeyMacro(vtkSelectionSerializer, ORIGINAL_SOURCE_ID, Integer);

//----------------------------------------------------------------------------
vtkSelectionSerializer::vtkSelectionSerializer() = default;

//----------------------------------------------------------------------------
vtkSelectionSerializer::~vtkSelectionSerializer() = default;

//----------------------------------------------------------------------------
void vtkSelectionSerializer::PrintXML(int printData, vtkSelection* selection)
{
  vtkSelectionSerializer::PrintXML(std::cout, vtkIndent(), printData, selection);
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::PrintXML(
  ostream& os, vtkIndent indent, int printData, vtkSelection* selection)
{
  os << indent << "<Selection>" << endl;
  vtkIndent nodeIndent = indent.GetNextIndent();
  unsigned int numNodes = selection->GetNumberOfNodes();
  for (unsigned int i = 0; i < numNodes; i++)
  {
    os << nodeIndent << "<Selection>" << endl;
    vtkSelectionNode* node = selection->GetNode(i);

    vtkIndent ni = nodeIndent.GetNextIndent();

    // Write out all properties.
    // For now, only keys of type vtkInformationIntegerKey are supported.
    vtkInformationIterator* iter = vtkInformationIterator::New();
    vtkInformation* properties = node->GetProperties();
    iter->SetInformation(properties);
    for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkInformationKey* key = iter->GetCurrentKey();
      os << ni << "<Property key=\"" << key->GetName() << "\" value=\"";
      if (key->IsA("vtkInformationIntegerKey"))
      {
        vtkInformationIntegerKey* iKey = static_cast<vtkInformationIntegerKey*>(key);
        os << properties->Get(iKey);
      }
      else if (key->IsA("vtkInformationDoubleKey"))
      {
        vtkInformationDoubleKey* dKey = static_cast<vtkInformationDoubleKey*>(key);
        os << properties->Get(dKey);
      }
      else if (key->IsA("vtkInformationStringKey"))
      {
        vtkInformationStringKey* sKey = static_cast<vtkInformationStringKey*>(key);
        os << properties->Get(sKey);
      }

      os << "\"/>" << endl;
    }
    iter->Delete();

    // Write the selection list
    if (printData)
    {
      vtkSelectionSerializer::WriteSelectionData(os, ni, node);
    }

    os << nodeIndent << "</Selection>" << endl;
  }

  os << indent << "</Selection>" << endl;
}

//----------------------------------------------------------------------------
namespace
{
struct vtkSelectionSerializerWriteSelectionList
{
  template <class TArray>
  void operator()(TArray* array, ostream& os, vtkIndent indent, vtkIdType numElems)
  {
    auto data = vtk::DataArrayValueRange(array);
    os << indent;
    for (vtkIdType idx = 0; idx < numElems; idx++)
    {
      os << data[idx] << " ";
    }
    os << endl;
  }
};
}

//----------------------------------------------------------------------------
// Serializes the selection list data array
void vtkSelectionSerializer::WriteSelectionData(
  ostream& os, vtkIndent indent, vtkSelectionNode* selection)
{
  vtkDataSetAttributes* data = selection->GetSelectionData();
  for (int i = 0; i < data->GetNumberOfArrays(); i++)
  {
    auto aa = data->GetAbstractArray(i);
    auto da = vtkDataArray::SafeDownCast(aa);
    auto sa = vtkStringArray::SafeDownCast(aa);
    if (da || sa)
    {
      vtkIdType numTuples = aa->GetNumberOfTuples();
      vtkIdType numComps = aa->GetNumberOfComponents();
      vtkIdType numValues = numTuples * numComps;

      os << indent << "<SelectionList"
         << " classname=\"" << aa->GetClassName() << "\" name=\""
         << (aa->GetName() ? aa->GetName() : "") << "\" number_of_tuples=\"" << numTuples
         << "\" number_of_components=\"" << numComps << "\">" << endl;
      if (da)
      {
        vtkSelectionSerializerWriteSelectionList worker;
        if (!vtkArrayDispatch::Dispatch::Execute(da, worker, os, indent, numValues))
        {
          worker(da, os, indent, numValues);
        }
      }
      else // if (sa)
      {
        vtkIndent ni = indent.GetNextIndent();
        for (vtkIdType j = 0; j < numValues; j++)
        {
          os << ni << "<String>";
          os << sa->GetValue(j);
          os << "</String>" << endl;
        }
      }
      os << indent << "</SelectionList>" << endl;
    }
  }
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::Parse(const char* xml, vtkSelection* root)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->Parse(xml);
  vtkPVXMLElement* rootElem = parser->GetRootElement();
  vtkSelectionSerializer::Parse(rootElem, root);
  parser->Delete();
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::Parse(const char* xml, unsigned int length, vtkSelection* root)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->Parse(xml, length);
  vtkPVXMLElement* rootElem = parser->GetRootElement();
  vtkSelectionSerializer::Parse(rootElem, root);
  parser->Delete();
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::Parse(vtkPVXMLElement* rootElem, vtkSelection* root)
{
  root->Initialize();
  if (rootElem)
  {
    unsigned int numNested = rootElem->GetNumberOfNestedElements();
    for (unsigned int i = 0; i < numNested; i++)
    {
      vtkPVXMLElement* elem = rootElem->GetNestedElement(i);
      const char* name = elem->GetName();
      if (!name)
      {
        continue;
      }
      if (strcmp("Selection", name) == 0)
      {
        vtkSelectionNode* newNode = vtkSelectionNode::New();
        root->AddNode(newNode);
        vtkSelectionSerializer::ParseNode(elem, newNode);
        newNode->Delete();
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::ParseNode(vtkPVXMLElement* nodeXML, vtkSelectionNode* node)
{
  if (!nodeXML || !node)
  {
    return;
  }

  unsigned int numNested = nodeXML->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numNested; i++)
  {
    vtkPVXMLElement* elem = nodeXML->GetNestedElement(i);
    const char* name = elem->GetName();
    if (!name)
    {
      continue;
    }

    // Only a selected list of keys are supported
    else if (strcmp("Property", name) == 0)
    {
      const char* key = elem->GetAttribute("key");
      if (key)
      {
        if (strcmp("CONTENT_TYPE", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::CONTENT_TYPE(), val);
          }
        }
        else if (strcmp("FIELD_TYPE", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::FIELD_TYPE(), val);
          }
        }
        else if (strcmp("SOURCE_ID", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::SOURCE_ID(), val);
          }
        }
        else if (strcmp("ORIGINAL_SOURCE_ID", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(ORIGINAL_SOURCE_ID(), val);
          }
        }
        else if (strcmp("PROP_ID", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::PROP_ID(), val);
          }
        }
        else if (strcmp("PROCESS_ID", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), val);
          }
        }
        else if (strcmp("EPSILON", key) == 0)
        {
          double val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::EPSILON(), val);
          }
        }
        else if (strcmp("CONTAINING_CELLS", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::CONTAINING_CELLS(), val);
          }
        }
        else if (strcmp("INVERSE", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::INVERSE(), val);
          }
        }
        else if (strcmp("PIXEL_COUNT", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::PIXEL_COUNT(), val);
          }
        }
        else if (strcmp("COMPOSITE_INDEX", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::COMPOSITE_INDEX(), val);
          }
        }
        else if (strcmp("HIERARCHICAL_LEVEL", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(), val);
          }
        }
        else if (strcmp("HIERARCHICAL_INDEX", key) == 0)
        {
          int val;
          if (elem->GetScalarAttribute("value", &val))
          {
            node->GetProperties()->Set(vtkSelectionNode::HIERARCHICAL_INDEX(), val);
          }
        }
      }
    }
    else if (strcmp("SelectionList", name) == 0)
    {
      if (elem->GetAttribute("classname"))
      {
        vtkAbstractArray* arr = vtkAbstractArray::SafeDownCast(
          vtkClientServerStreamInstantiator::CreateInstance(elem->GetAttribute("classname")));
        vtkDataArray* dataArray = vtkDataArray::SafeDownCast(arr);
        if (dataArray)
        {
          dataArray->SetName(elem->GetAttribute("name"));
          vtkIdType numTuples;
          int numComps;
          if (elem->GetScalarAttribute("number_of_tuples", &numTuples) &&
            elem->GetScalarAttribute("number_of_components", &numComps))
          {
            dataArray->SetNumberOfComponents(numComps);
            dataArray->SetNumberOfTuples(numTuples);
            vtkIdType numValues = numTuples * numComps;
            double* data = new double[numValues];
            if (elem->GetCharacterDataAsVector(numValues, data))
            {
              for (vtkIdType i2 = 0; i2 < numTuples; i2++)
              {
                for (int j = 0; j < numComps; j++)
                {
                  dataArray->SetComponent(i2, j, data[i2 * numComps + j]);
                }
              }
            }
            delete[] data;
          }
          node->GetSelectionData()->AddArray(dataArray);
          dataArray->Delete();
        }
        else if (vtkStringArray::SafeDownCast(arr))
        {
          vtkStringArray* stringArray = vtkStringArray::SafeDownCast(arr);
          stringArray->SetName(elem->GetAttribute("name"));
          vtkIdType numTuples;
          int numComps;
          if (elem->GetScalarAttribute("number_of_tuples", &numTuples) &&
            elem->GetScalarAttribute("number_of_components", &numComps))
          {
            stringArray->SetNumberOfComponents(numComps);
            stringArray->SetNumberOfTuples(numTuples);
            unsigned int numNestedStr = elem->GetNumberOfNestedElements();
            for (unsigned int ind = 0; ind < numNestedStr; ind++)
            {
              vtkPVXMLElement* strElem = elem->GetNestedElement(ind);
              stringArray->SetValue(ind, strElem->GetCharacterData());
            }
          }
          node->GetSelectionData()->AddArray(stringArray);
          stringArray->Delete();
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
