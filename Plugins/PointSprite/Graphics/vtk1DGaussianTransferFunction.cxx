/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DGaussianTransferFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DGaussianTransferFunction
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

#include "vtk1DGaussianTransferFunction.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"

#ifndef MAX
#define MAX(a, b) (a>b? a : b)
#endif

vtkStandardNewMacro(vtk1DGaussianTransferFunction)

vtk1DGaussianTransferFunction::vtk1DGaussianTransferFunction()
{
  this->GaussianControlPoints = vtkDoubleArray::New();
  this->GaussianControlPoints->SetNumberOfComponents(5);
}

vtk1DGaussianTransferFunction::~vtk1DGaussianTransferFunction()
{
  this->GaussianControlPoints->Delete();
}

double vtk1DGaussianTransferFunction::MapValue(double value, double* range)
{
  double opacity = 0.0;

  double delta = range[1] - range[0];
  if (delta == 0)
    delta = 1.0;

  double gp[5];
  double ratio = (value - range[0]) / delta;

  for (int p = 0; p < this->GetNumberOfGaussianControlPoints(); p++)
    {
    this->GaussianControlPoints->GetTuple(p, gp);
    double pos = gp[0];
    double height = gp[1];
    double width = gp[2];
    double xbias = gp[3];
    double ybias = gp[4];

    // clamp non-zero values to pos +/- width
    if (ratio > pos + width || ratio < pos - width)
      {
      opacity = MAX(opacity, (double) 0);
      }
    else
      {

      // non-zero width
      if (width == 0)
        width = .00001f;

      // translate the original x to a new x based on the xbias
      double x0;
      if (xbias == 0 || ratio == pos + xbias)
        {
        x0 = ratio;
        }
      else if (ratio > pos + xbias)
        {
        if (width == xbias)
          x0 = pos;
        else
          x0 = pos + (ratio - pos - xbias) * (width / (width - xbias));
        }
      else // (x < pos+xbias)
        {
        if (-width == xbias)
          x0 = pos;
        else
          x0 = pos - (ratio - pos - xbias) * (width / (width + xbias));
        }

      // center around 0 and normalize to -1,1
      float x1 = (x0 - pos) / width;

      // do a linear interpolation between:
      //    a gaussian and a parabola        if 0<ybias<1
      //    a parabola and a step function   if 1<ybias<2
      float h0a = exp(-(4* x1 * x1));
      float h0b = 1. - x1 * x1;
      float h0c = 1.;
      float h1;
      if (ybias < 1)
        h1 = ybias * h0b + (1 - ybias) * h0a;
      else
        h1 = (2 - ybias) * h0b + (ybias - 1) * h0c;
      float h2 = height * h1;
      // perform the MAX over different guassians, not the sum
      opacity = MAX(opacity, h2);
      }
    }

  return opacity;
}

// Description:
// Set/Get the number of Gaussian control points.
void vtk1DGaussianTransferFunction::SetNumberOfGaussianControlPoints(vtkIdType size)
{
  if (this->GaussianControlPoints->GetNumberOfTuples() != size)
    {
    this->GaussianControlPoints->SetNumberOfTuples(size);
    this->Modified();
    }
}

vtkIdType vtk1DGaussianTransferFunction::GetNumberOfGaussianControlPoints()
{
  return this->GaussianControlPoints->GetNumberOfTuples();
}

// Description:
// Set/Get a single Gaussian control point.
void vtk1DGaussianTransferFunction::SetGaussianControlPoint(vtkIdType index,
    double pos,
    double height,
    double width,
    double xbias,
    double ybias)
{
  double values[5] = { pos, height, width, xbias, ybias };
  this->SetGaussianControlPoint(index, values);
}

void vtk1DGaussianTransferFunction::SetGaussianControlPoint(vtkIdType index,
    double* values)
{
  double tuple[5];
  if (index < 0)
    return;
  if (index >= this->GetNumberOfGaussianControlPoints())
    {
    this->SetNumberOfGaussianControlPoints(index - 1);
    }
  this->GetGaussianControlPoint(index, tuple);
  if (tuple[0] != values[0] || tuple[1] != values[1] || tuple[2] != values[2]
      || tuple[3] != values[3] || tuple[4] != values[4])
    {
    this->GaussianControlPoints->SetTuple(index, values);
    this->Modified();
    }
}

void vtk1DGaussianTransferFunction::GetGaussianControlPoint(vtkIdType index,
    double* tuple)
{
  if (index < 0 || index >= this->GetNumberOfGaussianControlPoints())
    return;
  this->GaussianControlPoints->GetTuple(index, tuple);
}

// Description:
// add/remove a Gaussian control point.
void vtk1DGaussianTransferFunction::AddGaussianControlPoint(double pos,
    double width,
    double height,
    double xbiais,
    double ybiais)
{
  this->SetGaussianControlPoint(this->GetNumberOfGaussianControlPoints(), pos,
      width, height, xbiais, ybiais);
}

void vtk1DGaussianTransferFunction::AddGaussianControlPoint(double* values)
{
  this->SetGaussianControlPoint(this->GetNumberOfGaussianControlPoints(),
      values);
}

void vtk1DGaussianTransferFunction::RemoveGaussianControlPoint(vtkIdType index)
{
  if (index < 0 || index >= this->GetNumberOfGaussianControlPoints())
    return;
  if (this->GetNumberOfGaussianControlPoints() == 1)
    {
    this->RemoveAllGaussianControlPoints();
    return;
    }
  vtkDoubleArray* newGCP = vtkDoubleArray::New();
  newGCP->SetNumberOfComponents(5);
  vtkIdType ngcp = this->GaussianControlPoints->GetNumberOfTuples() - 1;
  newGCP->SetNumberOfTuples(ngcp);
  for (vtkIdType ii = 0; ii < index; ii++)
    {
    newGCP->SetTuple(ii, this->GaussianControlPoints->GetTuple(ii));
    }
  for (vtkIdType ii = index; ii < ngcp; ii++)
    {
    newGCP->SetTuple(ii, this->GaussianControlPoints->GetTuple(ii + 1));
    }
  this->GaussianControlPoints->Delete();
  this->GaussianControlPoints = newGCP;
  this->Modified();
}

// Description:
// clean all control points.
void vtk1DGaussianTransferFunction::RemoveAllGaussianControlPoints()
{
  this->GaussianControlPoints->SetNumberOfTuples(0);
}

void vtk1DGaussianTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

