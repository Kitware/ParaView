// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCirculationFeatures.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkCirculationFeatures - Convert flux or circulation scalars to vectors.
//
// .SECTION Description
//
// Given a set of line elements with a circulation scalar or vector field on the
// cells, compute the "feature" points.  Features are sources, sinks, or saddles
// in the circulation.  Any one of these is identified by vectors facing the
// opposite direction.  Since the input can be unstructured and deformed, the
// features are estimated by finding the average circulation vector on each
// point and seeing if any attached circulation vector points more than 90
// degrees in a different direction.
//
// .SECTION Todo
//
// Make this filter work in parallel.  (Basically, it needs ghost cells.)
//

#ifndef __vtkCirculationFeatures_h
#define __vtkCirculationFeatures_h

#include "vtkDataSetAlgorithm.h"

class vtkCirculationFeatures : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkCirculationFeatures, vtkDataSetAlgorithm);
  static vtkCirculationFeatures *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // These are basically a convenience method that calls SetInputArrayToProcess
  // to set the array used as the input scalars.  The fieldAttributeType comes
  // from the vtkDataSetAttributes::AttributeTypes enum.
  virtual void SetInputCirculation(const char *name);
  virtual void SetInputCirculation(int fieldAttributeType);

protected:
  vtkCirculationFeatures();
  ~vtkCirculationFeatures();

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

private:
  vtkCirculationFeatures(const vtkCirculationFeatures &);       // Not implemented
  void operator=(const vtkCirculationFeatures &);       // Not implemented
};

#endif //__vtkCirculationFeatures_h
