/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPrismView.h"

#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPrismRepresentation.h"
#include "vtkPVCompositeRepresentation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkstd/set>

vtkStandardNewMacro(vtkPrismView);
vtkInformationKeyRestrictedMacro(vtkPrismView, PRISM_WORLD_SCALE, DoubleVector,3);
//----------------------------------------------------------------------------
vtkPrismView::vtkPrismView()
{

}

//----------------------------------------------------------------------------
vtkPrismView::~vtkPrismView()
{
}

//----------------------------------------------------------------------------
void vtkPrismView::GatherRepresentationInformation()
{
  this->Superclass::GatherRepresentationInformation();
  vtkstd::set<void*> current_producers;
  int num_reprs = this->ReplyInformationVector->GetNumberOfInformationObjects();
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkInformation* info =
      this->ReplyInformationVector->GetInformationObject(cc);
    if (info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX()))
      {
      current_producers.insert(info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX()));
      }
    }
}


//----------------------------------------------------------------------------
void vtkPrismView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
