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

vtkCxxRevisionMacro(vtkPVVisibleCellSelector, "1.2");
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
vtkIdType vtkPVVisibleCellSelector::MapActorIdToActorId(vtkIdType id)
{
  vtkIdType ret = 0;
  vtkProp *prop = this->GetActorFromId(id);
  if (prop != NULL)
    {
    vtkClientServerID CSId = 
      vtkProcessModule::GetProcessModule()->GetIDFromObject(prop);
    ret = CSId.ID;
    }
  return ret;
}

//----------------------------------------------------------------------------
void vtkPVVisibleCellSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

