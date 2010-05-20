/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCSVWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPCSVWriter.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCharArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyLineToRectilinearGridFilter.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPCSVWriter);

vtkCxxSetObjectMacro(vtkPCSVWriter, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPCSVWriter::vtkPCSVWriter()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPCSVWriter::~vtkPCSVWriter()
{
  this->SetController(0);
}

//-----------------------------------------------------------------------------
void vtkPCSVWriter::WriteData()
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() < 2)
    {
    this->Superclass::WriteData();
    return;
    }

  // Collect the 
}

//----------------------------------------------------------------------------
void vtkPCSVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "Controller " << this->Controller << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces<< endl;
  os << indent << "Piece: " << this->Piece<< endl;
}
