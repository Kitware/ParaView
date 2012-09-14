/*=========================================================================

  Program:   ParaView
  Module:    vtkQuadRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadRepresentation.h"

#include "vtkAlgorithm.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineRepresentation.h"
#include "vtkPVMultiSliceView.h"
#include "vtkPVQuadRenderView.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSliceFriendGeometryRepresentation.h"
#include "vtkStringArray.h"
#include "vtkThreeSliceFilter.h"
#include "vtkView.h"

#include <assert.h>

vtkStandardNewMacro(vtkQuadRepresentation);
//----------------------------------------------------------------------------
vtkQuadRepresentation::vtkQuadRepresentation() : vtkCompositeSliceRepresentation()
{
}
//----------------------------------------------------------------------------
vtkQuadRepresentation::~vtkQuadRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkQuadRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkQuadRepresentation::AddToView(vtkView* view)
{
  // Custom management of representation for QuadView
  if(vtkPVQuadRenderView* quadView = vtkPVQuadRenderView::SafeDownCast(view))
    {
    for(int i=0; i < 3; ++i)
      {
      if(this->Slices[i+1] == NULL)
        {
        continue;
        }

      vtkPVRenderView* internalQuadView = quadView->GetOrthoRenderView(i);

      // Make the main view as master for delivery management
      quadView->AddRepresentation(this->Slices[i+1]);

      // Move actor from main view to our internal view
      this->Slices[i+1]->RemoveFromView(quadView);
      this->Slices[i+1]->AddToView(internalQuadView);
      }
    }

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkQuadRepresentation::RemoveFromView(vtkView* view)
{
  // Custom management of representation for QuadView
  if(vtkPVQuadRenderView* quadView = vtkPVQuadRenderView::SafeDownCast(view))
    {
    for(int i=0; i < 3; ++i)
      {
      if(this->Slices[i+1] == NULL)
        {
        continue;
        }

      vtkPVRenderView* internalQuadView = quadView->GetOrthoRenderView(i);
      quadView->RemoveRepresentation(this->Slices[i+1]);
      this->Slices[i+1]->RemoveFromView(internalQuadView);
      }
    }

  return this->Superclass::RemoveFromView(view);
}
