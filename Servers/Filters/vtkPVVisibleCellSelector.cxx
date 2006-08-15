/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVVisibleCellSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVVisibleCellSelector.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkProcessModule.h"
#include "vtkIdTypeArray.h"
#include "vtkIdentColoredPainter.h"

vtkCxxRevisionMacro(vtkPVVisibleCellSelector, "1.3");
vtkStandardNewMacro(vtkPVVisibleCellSelector);

//----------------------------------------------------------------------------
void vtkPVVisibleCellSelector::SavePixelBuffer(int pass, unsigned char *src)
{
  this->Superclass::SavePixelBuffer(pass, src);
}

//----------------------------------------------------------------------------
void vtkPVVisibleCellSelector::ComputeSelectedIds()
{
  this->Superclass::ComputeSelectedIds();
}

//----------------------------------------------------------------------------
void vtkPVVisibleCellSelector::SetSelectMode(int m)
{
  this->Superclass::SetSelectMode(m);
}

//----------------------------------------------------------------------------
void vtkPVVisibleCellSelector::LookupProcessorId()
{
  int id = vtkProcessModule::GetProcessModule()->GetPartitionId();
  this->Superclass::SetProcessorId(id);
}

//----------------------------------------------------------------------------
void vtkPVVisibleCellSelector::SetRenderer(vtkRenderer *r)
{
  this->Superclass::SetRenderer(r);
  
  //Now create an actor map to render each renderer with its client server id.
  vtkPropCollection *props = this->Renderer->GetViewProps();
  if ( props->GetNumberOfItems() == 0 )
    {
    return;
    }

  vtkIdTypeArray *arr = vtkIdTypeArray::New();
  arr->SetNumberOfComponents(2);

  vtkCollectionSimpleIterator pit;
  vtkProp *aProp;
  vtkIdType aTuple[2];
  for ( props->InitTraversal(pit); 
        (aProp = props->GetNextProp(pit)); )
    {
    vtkClientServerID CSId = 
      vtkProcessModule::GetProcessModule()->GetIDFromObject(aProp);
    
    aTuple[0] = (int)aProp;
    aTuple[1] = CSId.ID;
    arr->InsertNextTupleValue(aTuple);
    }

  vtkIdentColoredPainter *ip = vtkIdentColoredPainter::New();
  ip->SetActorLookupTable(arr);
  this->Superclass::SetIdentPainter(ip);

  //now that we have given these away, we can delete our reference to them
  ip->Delete();
  arr->Delete();
}

//----------------------------------------------------------------------------
void vtkPVVisibleCellSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

