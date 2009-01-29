// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFluxVectors.h

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

// .NAME vtkFluxVectors - Convert flux or circulation scalars to vectors.
//
// .SECTION Description
//
// Often times a simulation that computes circulation or flux in a mesh will
// simply write out the scalar that represents the amount of movement and the
// direction is implied by the normal of the surface cell (for flux) or the
// direction of the line cell (for circulation).  This filter converts the
// scalar representation to the vector representation.
//

#ifndef __vtkFluxVectors_h
#define __vtkFluxVectors_h

#include "vtkDataSetAlgorithm.h"

class vtkFluxVectors : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkFluxVectors, vtkDataSetAlgorithm);
  static vtkFluxVectors *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // These are basically a convenience method that calls SetInputArrayToProcess
  // to set the array used as the input scalars.  The fieldAttributeType comes
  // from the vtkDataSetAttributes::AttributeTypes enum.
  virtual void SetInputFlux(const char *name);
  virtual void SetInputFlux(int fieldAttributeType);

  // Description:
  // If off (the default), then the input array is taken to be the total flux
  // through or or circulation along each element.  If on, then the input array
  // is taken to be the density of the flux or circulation.
  vtkGetMacro(InputFluxIsDensity, int);
  vtkSetMacro(InputFluxIsDensity, int);
  vtkBooleanMacro(InputFluxIsDensity, int);

  // Description:
  // Set the name assigned to the total or density of flux output array.  If
  // the string is NULL or empty, then the output names are taken from the
  // input flux array.
  vtkSetStringMacro(OutputFluxTotalName);
  vtkSetStringMacro(OutputFluxDensityName);

  // Description:
  // Given the current (or specified) input and current state of this filter,
  // returns the name actually assigned to the total or density of flux output
  // array.  The returned string is not guaranteed to be equal to array names
  // created from the last call to Update, but will be the same if neither the
  // input or state of this filter changes.  This method is NOT thread safe.
  const char *GetOutputFluxTotalName() {
    return this->GetOutputFluxTotalName(this->GetInput());
  }
  const char *GetOutputFluxDensityName() {
    return this->GetOutputFluxDensityName(this->GetInput());
  }
  virtual const char *GetOutputFluxTotalName(vtkDataObject *input);
  virtual const char *GetOutputFluxDensityName(vtkDataObject *input);

protected:
  vtkFluxVectors();
  ~vtkFluxVectors();

  int InputFluxIsDensity;

  char *OutputFluxTotalName;
  char *OutputFluxDensityName;

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

private:
  vtkFluxVectors(const vtkFluxVectors &);       // Not implemented
  void operator=(const vtkFluxVectors &);       // Not implemented
};

#endif //__vtkFluxVectors_h
