/*=========================================================================

  Program:   ParaView
  Module:    vtkPMSILProperty

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMSILProperty.h"

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
#include "vtkPMProxy.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMMessage.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/string>
#include <vtkstd/vector>

class vtkPMSILProperty::vtkIdTypeSet : public vtkstd::set<vtkIdType> {};

vtkStandardNewMacro(vtkPMSILProperty);
//----------------------------------------------------------------------------
vtkPMSILProperty::vtkPMSILProperty()
{
  this->SubTree = 0;
  this->OutputPort = 0;
}

//----------------------------------------------------------------------------
vtkPMSILProperty::~vtkPMSILProperty()
{
  this->SetSubTree(0);
}

//----------------------------------------------------------------------------
void vtkPMSILProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkPMSILProperty::ReadXMLAttributes( vtkPMProxy* proxyhelper,
                                          vtkPVXMLElement* element)
{
  bool retValue = this->Superclass::ReadXMLAttributes(proxyhelper, element);

  // Parse extra attribute
  this->SetSubTree(element->GetAttribute("subtree")); // if none => set NULL

  // If error we reset it to 0
  if(!element->GetScalarAttribute("output_port", &this->OutputPort))
    {
    this->OutputPort = 0;
    }
  return retValue;
}

//----------------------------------------------------------------------------
bool vtkPMSILProperty::Pull(vtkSMMessage* msgToFill)
{
  if (!this->InformationOnly)
    {
    return false;
    }

  // Build SIL vtkGraph object
  vtkAlgorithm *reader = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if(!reader)
    {
    vtkWarningMacro("Could not get the reader.");
    return false;
    }
  vtkInformation* info =
      reader->GetExecutive()->GetOutputInformation(this->OutputPort);
  if (!info || !info->Has(vtkDataObject::SIL()))
    {
    vtkWarningMacro("No SIL infornation on the reader.");
    return false;
    }
  vtkSmartPointer<vtkGraph> graphSIL = vtkGraph::SafeDownCast(info->Get(vtkDataObject::SIL()));

  // Build the meta-data
  vtkIdType numVertices = graphSIL->GetNumberOfVertices();
  vtkstd::map<vtkstd::string, vtkIdType> vertexNameMap;
  vtkStringArray* names =
      vtkStringArray::SafeDownCast(
          graphSIL->GetVertexData()->GetAbstractArray("Names"));
  for (vtkIdType kk=0; kk < numVertices; kk++)
    {
    vertexNameMap[names->GetValue(kk)] = kk;
    }

  // Search for specific subtree if any
  vtkIdType subTreeVertexId = 0;
  if(this->SubTree)
    {
    vtkstd::map<vtkstd::string, vtkIdType>::iterator iter;
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
  vtkPMSILProperty::GetLeaves(graphSIL.GetPointer(), subTreeVertexId, leaves, false);

  // Build property
  ProxyState_Property *prop = msgToFill->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant *var = prop->mutable_value();
  var->set_type(Variant::STRING);

  // Fill property
  vtkIdTypeSet::iterator iter;
  for (iter = leaves.begin(); iter != leaves.end(); ++iter)
    {
    if((*iter) >= 0 && (*iter) < numVertices)
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
void vtkPMSILProperty::GetLeaves( vtkGraph *sil, vtkIdType vertexid,
                                  vtkIdTypeSet& list,
                                  bool traverse_cross_edges)
{
  vtkDataArray* crossEdgesArray =
      vtkDataArray::SafeDownCast(
          sil->GetEdgeData()->GetAbstractArray("CrossEdges"));

  bool has_child_edge = false;
  vtkOutEdgeIterator* iter = vtkOutEdgeIterator::New();
  sil->GetOutEdges(vertexid, iter);
  while (iter->HasNext())
    {
    vtkOutEdgeType edge = iter->Next();
    if (traverse_cross_edges || crossEdgesArray->GetTuple1(edge.Id) == 0)
      {
      has_child_edge = true;
      vtkPMSILProperty::GetLeaves(sil, edge.Target, list, traverse_cross_edges);
      }
    }
  iter->Delete();

  if (!has_child_edge)
    {
    list.insert(vertexid);
    }
}
