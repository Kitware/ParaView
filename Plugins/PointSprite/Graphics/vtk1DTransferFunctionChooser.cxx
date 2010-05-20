/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DTransferFunctionChooser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DTransferFunctionChooser
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "vtk1DTransferFunctionChooser.h"

#include "vtkObjectFactory.h"
#include "vtk1DLookupTableTransferFunction.h"
#include "vtk1DGaussianTransferFunction.h"

vtkStandardNewMacro(vtk1DTransferFunctionChooser)

vtkCxxSetObjectMacro(vtk1DTransferFunctionChooser, LookupTableTransferFunction, vtk1DLookupTableTransferFunction)
vtkCxxSetObjectMacro(vtk1DTransferFunctionChooser, GaussianTransferFunction, vtk1DGaussianTransferFunction)

vtk1DTransferFunctionChooser::vtk1DTransferFunctionChooser()
{
  this->TransferFunctionMode = LookupTable;
  this->LookupTableTransferFunction = vtk1DLookupTableTransferFunction::New();
  this->GaussianTransferFunction = vtk1DGaussianTransferFunction::New();
}

vtk1DTransferFunctionChooser::~vtk1DTransferFunctionChooser()
{
  if (this->LookupTableTransferFunction)
    this->LookupTableTransferFunction->Delete();
  if (this->GaussianTransferFunction)
    this->GaussianTransferFunction->Delete();
}

void vtk1DTransferFunctionChooser::MapArray(vtkDataArray* input,
    vtkDataArray* output)
{
  switch (this->TransferFunctionMode)
    {
    case LookupTable:
      if (this->LookupTableTransferFunction)
        {
        this->LookupTableTransferFunction->SetInputRange(this->GetInputRange());
        this->LookupTableTransferFunction->SetUseScalarRange(
            this->GetUseScalarRange());
        this->LookupTableTransferFunction->SetVectorComponent(
            this->GetVectorComponent());
        this->LookupTableTransferFunction->MapArray(input, output);
        }
      break;
    case Gaussian:
      if (this->GaussianTransferFunction)
        {
        this->GaussianTransferFunction->SetInputRange(this->GetInputRange());
        this->GaussianTransferFunction->SetUseScalarRange(
            this->GetUseScalarRange());
        this->GaussianTransferFunction->SetVectorComponent(
            this->GetVectorComponent());
        this->GaussianTransferFunction->MapArray(input, output);
        }
      break;
    default:
      vtkWarningMacro("Unknown Transfert Function Mode, aborting")
      return;
    }
}

double vtk1DTransferFunctionChooser::MapValue(double value, double* range)
{
  switch (this->TransferFunctionMode)
    {
    case LookupTable:
      if (this->LookupTableTransferFunction)
        {
        this->LookupTableTransferFunction->SetInputRange(this->GetInputRange());
        this->LookupTableTransferFunction->SetUseScalarRange(
            this->GetUseScalarRange());
        this->LookupTableTransferFunction->SetVectorComponent(
            this->GetVectorComponent());
        return this->LookupTableTransferFunction->MapValue(value, range);
        }
      break;

    case Gaussian:
      if (this->GaussianTransferFunction)
        {
        this->GaussianTransferFunction->SetInputRange(this->GetInputRange());
        this->GaussianTransferFunction->SetUseScalarRange(
            this->GetUseScalarRange());
        this->GaussianTransferFunction->SetVectorComponent(
            this->GetVectorComponent());
        return this->GaussianTransferFunction->MapValue(value, range);
        }
      break;

    default:
      vtkWarningMacro("Unknown Transfert Function Mode, returning 0")
    }
  return 0.0;
}

void vtk1DTransferFunctionChooser::BuildDefaultTransferFunctions()
{
  if (this->LookupTableTransferFunction)
    this->LookupTableTransferFunction->BuildDefaultTable();
}

unsigned long vtk1DTransferFunctionChooser::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();

  if (this->LookupTableTransferFunction && mtime
      < this->LookupTableTransferFunction->GetMTime())
    mtime = this->LookupTableTransferFunction->GetMTime();

  if (this->GaussianTransferFunction && mtime
      < this->GaussianTransferFunction->GetMTime())
    mtime = this->GaussianTransferFunction->GetMTime();

  return mtime;
}

void vtk1DTransferFunctionChooser::SetInputRange(double* range)
{
  this->SetInputRange(range[0], range[1]);
}

void vtk1DTransferFunctionChooser::SetInputRange(double rmin, double rmax)
{
  if (this->LookupTableTransferFunction)
    this->LookupTableTransferFunction->SetInputRange(rmin, rmax);
  if (this->GaussianTransferFunction)
    this->GaussianTransferFunction->SetInputRange(rmin, rmax);
  this->Superclass::SetInputRange(rmin, rmax);
}

void vtk1DTransferFunctionChooser::SetVectorComponent(int comp)
{
  if (this->LookupTableTransferFunction)
    this->LookupTableTransferFunction->SetVectorComponent(comp);
  if (this->GaussianTransferFunction)
    this->GaussianTransferFunction->SetVectorComponent(comp);
  this->Superclass::SetVectorComponent(comp);
}

void vtk1DTransferFunctionChooser::SetUseScalarRange(int use)
{
  if (this->LookupTableTransferFunction)
    this->LookupTableTransferFunction->SetUseScalarRange(use);
  if (this->GaussianTransferFunction)
    this->GaussianTransferFunction->SetUseScalarRange(use);
  this->Superclass::SetUseScalarRange(use);
}

void vtk1DTransferFunctionChooser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

