/*=========================================================================

  Program:   ParaView
  Module:    vtkSISILProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSISILProperty.h"

#include "vtkAdjacentVertexIterator.h"
#include "vtkAlgorithm.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkExecutive.h"
#include "vtkGraph.h"
#include "vtkInEdgeIterator.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkSIProxy.h"
#include "vtkSMMessage.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

class vtkSISILProperty::vtkIdTypeSet : public std::set<vtkIdType>
{
};

vtkStandardNewMacro(vtkSISILProperty);
//----------------------------------------------------------------------------
vtkSISILProperty::vtkSISILProperty()
{
  this->SubTree = 0;
  this->OutputPort = 0;
}

//----------------------------------------------------------------------------
vtkSISILProperty::~vtkSISILProperty()
{
  this->SetSubTree(0);
}

//----------------------------------------------------------------------------
void vtkSISILProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkSISILProperty::ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element)
{
  bool retValue = this->Superclass::ReadXMLAttributes(proxyhelper, element);

  // Parse extra attribute
  this->SetSubTree(element->GetAttribute("subtree")); // if none => set NULL
  if (!this->SubTree)
  {
    std::ostringstream proxyDefinition;
    element->PrintXML(proxyDefinition, vtkIndent(3));
    vtkWarningMacro(
      "No subtree attribute has been set in the following XML: " << proxyDefinition.str().c_str());
  }

  // If error we reset it to 0
  if (!element->GetScalarAttribute("output_port", &this->OutputPort))
  {
    this->OutputPort = 0;
  }
  return retValue;
}

//----------------------------------------------------------------------------
bool vtkSISILProperty::Pull(vtkSMMessage* msgToFill)
{
  if (!this->InformationOnly)
  {
    return false;
  }

  // Build SIL vtkGraph object
  vtkAlgorithm* reader = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (!reader)
  {
    vtkWarningMacro("Could not get the reader.");
    return false;
  }

  // Check if we should use the executive or a method to retrieve the SIL
  vtkSmartPointer<vtkGraph> graphSIL;
  if (this->GetCommand())
  {
    // Use method call
    vtkClientServerStream css;
    css << vtkClientServerStream::Invoke << reader << this->GetCommand()
        << vtkClientServerStream::End;
    if (this->ProcessMessage(css))
    {
      vtkObjectBase* graphResult;
      this->GetLastResult().GetArgumentObject(0, 0, &graphResult, "vtkGraph");
      graphSIL = vtkGraph::SafeDownCast(graphResult);
    }
    else
    {
      vtkWarningMacro("No SIL infornation on the reader.");
      return false;
    }
  }
  else
  {
    // Use executive
    vtkInformation* info = reader->GetExecutive()->GetOutputInformation(this->OutputPort);
    if (!info || !info->Has(vtkDataObject::SIL()))
    {
      vtkWarningMacro("No SIL infornation on the reader.");
      return false;
    }
    graphSIL = vtkGraph::SafeDownCast(info->Get(vtkDataObject::SIL()));
  }

  // Build the meta-data
  vtkIdType numVertices = graphSIL->GetNumberOfVertices();
  std::map<std::string, vtkIdType> vertexNameMap;
  vtkStringArray* names =
    vtkStringArray::SafeDownCast(graphSIL->GetVertexData()->GetAbstractArray("Names"));
  for (vtkIdType kk = 0; kk < numVertices; kk++)
  {
    vertexNameMap[names->GetValue(kk)] = kk;
  }

  // Search for specific subtree if any
  vtkIdType subTreeVertexId = 0;
  if (this->SubTree)
  {
    std::map<std::string, vtkIdType>::iterator iter;
    iter = vertexNameMap.find(this->SubTree);
    if (iter != vertexNameMap.end())
    {
      subTreeVertexId = iter->second;
    }
    else
    {
      vtkWarningMacro("Failed to locate requested subtree");
      return false;
    }
  }

  // Fill the leaves
  vtkIdTypeSet leaves;
  vtkSISILProperty::GetLeaves(graphSIL.GetPointer(), subTreeVertexId, leaves, false);

  // Build property
  ProxyState_Property* prop = msgToFill->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant* var = prop->mutable_value();
  var->set_type(Variant::STRING);

  // Fill property
  vtkIdTypeSet::iterator iter;
  for (iter = leaves.begin(); iter != leaves.end(); ++iter)
  {
    if ((*iter) >= 0 && (*iter) < numVertices)
    {
      var->add_txt(names->GetValue(*iter));
    }
    else
    {
      vtkErrorMacro("Invalid index: " << *iter);
    }
  }

  return true;
}
//----------------------------------------------------------------------------
void vtkSISILProperty::GetLeaves(
  vtkGraph* sil, vtkIdType vertexid, vtkIdTypeSet& list, bool traverse_cross_edges)
{
  vtkDataArray* crossEdgesArray =
    vtkDataArray::SafeDownCast(sil->GetEdgeData()->GetAbstractArray("CrossEdges"));

  bool has_child_edge = false;
  vtkOutEdgeIterator* iter = vtkOutEdgeIterator::New();
  sil->GetOutEdges(vertexid, iter);
  while (iter->HasNext())
  {
    vtkOutEdgeType edge = iter->Next();
    if (traverse_cross_edges || crossEdgesArray->GetTuple1(edge.Id) == 0)
    {
      has_child_edge = true;
      vtkSISILProperty::GetLeaves(sil, edge.Target, list, traverse_cross_edges);
    }
  }
  iter->Delete();

  if (!has_child_edge)
  {
    list.insert(vertexid);
  }
}
