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
#include "vtkInformation.h"
#include "vtkInformationIterator.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationKey.h"
#include "vtkInstantiator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"

vtkStandardNewMacro(vtkSelectionSerializer);
vtkCxxRevisionMacro(vtkSelectionSerializer, "1.5");

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
    vtkSelectionSerializer::WriteSelectionList(os, indent, selection);
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
void vtkSelectionSerializer::WriteSelectionList(
  ostream& os, vtkIndent indent, vtkSelection* selection)
{
  vtkDataArray* selectionList = vtkDataArray::SafeDownCast(
    selection->GetSelectionList());
  if (selectionList)
    {
    vtkIdType numTuples = selectionList->GetNumberOfTuples();
    vtkIdType numComps  = selectionList->GetNumberOfComponents();
    os << indent 
       << "<SelectionList classname=\""
       << selectionList->GetClassName()
       << "\" number_of_tuples=\""
       << numTuples
       << "\" number_of_components=\""
       << numComps
       << "\">"
       << endl;
    void* dataPtr = selectionList->GetVoidPointer(0);
    switch (selectionList->GetDataType())
      {
      vtkTemplateMacro(
        vtkSelectionSerializerWriteSelectionList(
          os, indent,
          numTuples*numComps, (VTK_TT*)(dataPtr)
          ));
      }
    os << indent << "</SelectionList>" << endl;
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
        }
      }
    else if (strcmp("SelectionList", name) == 0)
      {
      if (elem->GetAttribute("classname"))
        {
        vtkDataArray* dataArray = 
          vtkDataArray::SafeDownCast(
            vtkInstantiator::CreateInstance(elem->GetAttribute("classname")));
        if (dataArray)
          {
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
          node->SetSelectionList(dataArray);
          dataArray->Delete();
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

