/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataVisitor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockDataVisitor.h"

#include "vtkDataSet.h"
#include "vtkMultiBlockDataIterator.h"
#include "vtkCompositeDataVisitorCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiBlockDataVisitor, "1.1");
vtkStandardNewMacro(vtkMultiBlockDataVisitor);

vtkCxxSetObjectMacro(vtkMultiBlockDataVisitor,
                     DataIterator, 
                     vtkMultiBlockDataIterator);

//----------------------------------------------------------------------------
vtkMultiBlockDataVisitor::vtkMultiBlockDataVisitor()
{
  this->DataIterator = 0;
}

//----------------------------------------------------------------------------
vtkMultiBlockDataVisitor::~vtkMultiBlockDataVisitor()
{
  this->SetDataIterator(0);
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataVisitor::Execute()
{
  if (!this->DataIterator)
    {
    vtkErrorMacro("No iterator has been specified. Aborting.");
    return;
    }

  if (!this->Command)
    {
    vtkErrorMacro("No command has been specified. Aborting.");
    return;
    }

  this->Command->Initialize();
  this->DataIterator->GoToFirstItem();
  while (!this->DataIterator->IsDoneWithTraversal())
    {
    vtkDataSet* curDataSet = vtkDataSet::SafeDownCast(
      this->DataIterator->GetCurrentDataObject());
    if (curDataSet)
      {
      this->Command->Execute(this, 
                             curDataSet,
                             0);
      }
    this->DataIterator->GoToNextItem();
    }
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataVisitor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

