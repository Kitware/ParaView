/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEncodeSelectionForServer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVEncodeSelectionForServer.h"

#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <map>
#include <vector>

namespace
{
//-----------------------------------------------------------------------------
static void vtkShrinkSelection(vtkSelection* sel)
{
  std::map<void*, int> pixelCounts;
  unsigned int numNodes = sel->GetNumberOfNodes();
  void* chosen = nullptr;
  int maxPixels = -1;
  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    vtkSelectionNode* node = sel->GetNode(cc);
    vtkInformation* properties = node->GetProperties();
    if (properties->Has(vtkSelectionNode::PIXEL_COUNT()) &&
      properties->Has(vtkSelectionNode::SOURCE()))
    {
      int numPixels = properties->Get(vtkSelectionNode::PIXEL_COUNT());
      void* source = properties->Get(vtkSelectionNode::SOURCE());
      pixelCounts[source] += numPixels;
      if (pixelCounts[source] > maxPixels)
      {
        maxPixels = numPixels;
        chosen = source;
      }
    }
  }

  std::vector<vtkSmartPointer<vtkSelectionNode> > chosenNodes;
  if (chosen != nullptr)
  {
    for (unsigned int cc = 0; cc < numNodes; cc++)
    {
      vtkSelectionNode* node = sel->GetNode(cc);
      vtkInformation* properties = node->GetProperties();
      if (properties->Has(vtkSelectionNode::SOURCE()) &&
        properties->Get(vtkSelectionNode::SOURCE()) == chosen)
      {
        chosenNodes.push_back(node);
      }
    }
  }
  sel->RemoveAllNodes();
  for (unsigned int cc = 0; cc < chosenNodes.size(); cc++)
  {
    sel->AddNode(chosenNodes[cc]);
  }
}
}

vtkObjectFactoryNewMacro(vtkPVEncodeSelectionForServer);

vtkPVEncodeSelectionForServer::vtkPVEncodeSelectionForServer() = default;

vtkPVEncodeSelectionForServer::~vtkPVEncodeSelectionForServer() = default;

void vtkPVEncodeSelectionForServer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkPVEncodeSelectionForServer::ProcessSelection(vtkSelection* rawSelection,
  vtkSMRenderViewProxy* viewProxy, bool multipleSelectionsAllowed,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources, int vtkNotUsed(modifier),
  bool vtkNotUsed(selectBlocks))
{
  if (!multipleSelectionsAllowed)
  {
    // only pass through selection over a single representation.
    vtkShrinkSelection(rawSelection);
  }
  vtkSMSelectionHelper::NewSelectionSourcesFromSelection(
    rawSelection, viewProxy, selectionSources, selectedRepresentations);
  return (selectionSources->GetNumberOfItems() > 0);
}
