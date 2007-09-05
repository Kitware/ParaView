/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionSerializer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectionSerializer.h"

#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationIterator.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationKey.h"
#include "vtkInstantiator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkStringArray.h"

vtkStandardNewMacro(vtkSelectionSerializer);
vtkCxxRevisionMacro(vtkSelectionSerializer, "1.15");

vtkInformationKeyMacro(vtkSelectionSerializer,ORIGINAL_SOURCE_ID,Integer);

//----------------------------------------------------------------------------
vtkSelectionSerializer::vtkSelectionSerializer()
{
}

//----------------------------------------------------------------------------
vtkSelectionSerializer::~vtkSelectionSerializer()
{
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::PrintXML(int printData,
                                      vtkSelection* selection)
{
  vtkSelectionSerializer::PrintXML(cout, vtkIndent(), printData, selection);
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::PrintXML(
  ostream& os, vtkIndent indent, int printData, vtkSelection* selection)
{
  os << indent << "<Selection>" << endl;

  vtkIndent ni = indent.GetNextIndent();

  // Write out all properties.
  // For now, only keys of type vtkInformationIntegerKey are supported.
  vtkInformationIterator* iter = vtkInformationIterator::New();
  vtkInformation* properties = selection->GetProperties();
  iter->SetInformation(properties);
  for(iter->GoToFirstItem(); 
      !iter->IsDoneWithTraversal(); 
      iter->GoToNextItem())
    {
    vtkInformationKey* key = iter->GetCurrentKey();
    os << ni 
       << "<Property key=\"" << key->GetName() 
       << "\" value=\"";
    if (key->IsA("vtkInformationIntegerKey"))
      {
      vtkInformationIntegerKey* iKey = 
        static_cast<vtkInformationIntegerKey*>(key);
      os << properties->Get(iKey);
      }
    else if (key->IsA("vtkInformationDoubleKey"))
      {
      vtkInformationDoubleKey* dKey = 
        static_cast<vtkInformationDoubleKey*>(key);
      os << properties->Get(dKey);
      }
    else if (key->IsA("vtkInformationStringKey"))
      {
      vtkInformationStringKey* sKey = 
        static_cast<vtkInformationStringKey*>(key);
      os << properties->Get(sKey);
      }
      
    os << "\"/>" << endl;
    }
  iter->Delete();

  // Serialize all children
  unsigned int numChildren = selection->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkSelectionSerializer::PrintXML(
      os, ni, printData, selection->GetChild(i));
    }

  // Write the selection list
  if (printData)
    {
    vtkSelectionSerializer::WriteSelectionData(os, indent, selection);
    }

  os << indent << "</Selection>" << endl;
}

//----------------------------------------------------------------------------
template <class T>
void vtkSelectionSerializerWriteSelectionList(ostream& os, vtkIndent indent,
                                              vtkIdType numElems, T* dataPtr)
{
  os << indent;
  for (vtkIdType idx=0; idx<numElems; idx++)
    {
    os << dataPtr[idx] << " ";
    }
  os << endl;
}

//----------------------------------------------------------------------------
// Serializes the selection list data array
void vtkSelectionSerializer::WriteSelectionData(
  ostream& os, vtkIndent indent, vtkSelection* selection)
{
  vtkFieldData* data = selection->GetSelectionData();
  for (int i = 0; i < data->GetNumberOfArrays(); i++)
    {
    if (vtkDataArray::SafeDownCast(data->GetAbstractArray(i)))
      {
      vtkDataArray* list = vtkDataArray::SafeDownCast(data->GetAbstractArray(i));
      vtkIdType numTuples = list->GetNumberOfTuples();
      vtkIdType numComps  = list->GetNumberOfComponents();
      
      os << indent 
         << "<SelectionList"
         << " classname=\""
         << list->GetClassName()
         << "\" name=\""
         << (list->GetName() ? list->GetName() : "")
         << "\" number_of_tuples=\""
         << numTuples
         << "\" number_of_components=\""
         << numComps
         << "\">"
         << endl;
      void* dataPtr = list->GetVoidPointer(0);
      switch (list->GetDataType())
        {
        vtkTemplateMacro(
          vtkSelectionSerializerWriteSelectionList(
            os, indent,
            numTuples*numComps, (VTK_TT*)(dataPtr)
            ));
        }
      os << indent << "</SelectionList>" << endl;
      }
    else if (vtkStringArray::SafeDownCast(selection->GetSelectionList()))
      {
      vtkStringArray* stringList = vtkStringArray::SafeDownCast(
        selection->GetSelectionList());
      vtkIdType numTuples = stringList->GetNumberOfTuples();
      vtkIdType numComps  = stringList->GetNumberOfComponents();
      os << indent 
         << "<SelectionList"
         << " classname=\""
         << stringList->GetClassName()
         << "\" name=\""
         << (stringList->GetName() ? stringList->GetName() : "")
         << "\" number_of_tuples=\""
         << numTuples
         << "\" number_of_components=\""
         << numComps
         << "\">"
         << endl;
      vtkIndent ni = indent.GetNextIndent();
      for (vtkIdType i = 0; i < numTuples*numComps; i++)
        {
        os << ni << "<String>";
        os << stringList->GetValue(i);
        os << "</String>" << endl;
        }
      os << indent << "</SelectionList>" << endl;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::Parse(const char* xml, vtkSelection* root)
{
  root->Clear();

  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->Parse(xml);
  if (parser->GetRootElement())
    {
    vtkSelectionSerializer::ParseNode(parser->GetRootElement(), root);
    }
  parser->Delete();
}

//----------------------------------------------------------------------------
void vtkSelectionSerializer::ParseNode(vtkPVXMLElement* nodeXML, 
                                       vtkSelection* node)
{
  if (!nodeXML || !node)
    {
    return;
    }

  unsigned int numNested = nodeXML->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numNested; i++)
    {
    vtkPVXMLElement* elem = nodeXML->GetNestedElement(i);
    const char* name = elem->GetName();
    if (!name)
      {
      continue;
      }

    if (strcmp("Selection", name) == 0 )
      {
      vtkSelection* newNode = vtkSelection::New();
      node->AddChild(newNode);
      vtkSelectionSerializer::ParseNode(elem, newNode);
      newNode->Delete();
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
            node->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), val);
            }
          }
        else if (strcmp("FIELD_TYPE", key) == 0)
          {
          int val;
          if (elem->GetScalarAttribute("value", &val))
            {
            node->GetProperties()->Set(vtkSelection::FIELD_TYPE(), val);
            }
          }
        else if (strcmp("SOURCE_ID", key) == 0)
          {
          int val;
          if (elem->GetScalarAttribute("value", &val))
            {
            node->GetProperties()->Set(vtkSelection::SOURCE_ID(), val);
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
            node->GetProperties()->Set(vtkSelection::PROP_ID(), val);
            }
          }
        else if (strcmp("PROCESS_ID", key) == 0)
          {
          int val;
          if (elem->GetScalarAttribute("value", &val))
            {
            node->GetProperties()->Set(vtkSelection::PROCESS_ID(), val);
            }
          }
        else if (strcmp("EPSILON", key) == 0)
          {
          double val;
          if (elem->GetScalarAttribute("value", &val))
            {
            node->GetProperties()->Set(vtkSelection::EPSILON(), val);
            }
          }
        else if (strcmp("PRESERVE_TOPOLOGY", key) == 0)
          {
          int val;
          if (elem->GetScalarAttribute("value", &val))
            {
            node->GetProperties()->Set(vtkSelection::PRESERVE_TOPOLOGY(), val);
            }
          }
        else if (strcmp("CONTAINING_CELLS", key) == 0)
          {
          int val;
          if (elem->GetScalarAttribute("value", &val))
            {
            node->GetProperties()->Set(vtkSelection::CONTAINING_CELLS(), val);
            }
          }
        else if (strcmp("INVERSE", key) == 0)
          {
          int val;
          if (elem->GetScalarAttribute("value", &val))
            {
            node->GetProperties()->Set(vtkSelection::INVERSE(), val);
            }
          }
        else if (strcmp("PIXEL_COUNT", key) == 0)
          {
          int val;
          if (elem->GetScalarAttribute("value", &val))
            {
            node->GetProperties()->Set(vtkSelection::PIXEL_COUNT(), val);
            }
          }
        else if (strcmp("INDEXED_VERTICES", key) == 0)
          {
          int val;
          if (elem->GetScalarAttribute("value", &val))
            {
            node->GetProperties()->Set(vtkSelection::INDEXED_VERTICES(), val);
            }
          }
        }
      }
    else if (strcmp("SelectionList", name) == 0)
      {
      if (elem->GetAttribute("classname"))
        {
        vtkAbstractArray* arr = 
          vtkAbstractArray::SafeDownCast(
            vtkInstantiator::CreateInstance(elem->GetAttribute("classname")));
        vtkDataArray* dataArray = 
          vtkDataArray::SafeDownCast(arr);
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
            vtkIdType numValues = numTuples*numComps;
            double* data = new double[numValues];
            if (elem->GetCharacterDataAsVector(numValues, data))
              {
              for (vtkIdType i2=0; i2<numTuples; i2++)
                {
                for (int j=0; j<numComps; j++)
                  {
                  dataArray->SetComponent(i2, j, data[i2*numComps+j]);
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
          vtkStringArray* stringArray = 
            vtkStringArray::SafeDownCast(arr);
          stringArray->SetName(elem->GetAttribute("name"));
          vtkIdType numTuples;
          int numComps;
          if (elem->GetScalarAttribute("number_of_tuples", &numTuples) &&
              elem->GetScalarAttribute("number_of_components", &numComps))
            {
            stringArray->SetNumberOfComponents(numComps);
            stringArray->SetNumberOfTuples(numTuples);
            unsigned int numNestedStr = elem->GetNumberOfNestedElements();
            for (unsigned int ind=0; ind<numNestedStr; ind++)
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

