// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMomentVectors.h

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

// .NAME vtkMomentVectors - Convert flux or circulation scalars to vectors.
//
// .SECTION Description
//
// Often times a simulation that computes circulation or flux in a mesh will
// simply write out the scalar that represents the amount of movement and the
// direction is implied by the normal of the surface cell (for flux) or the
// direction of the line cell (for circulation).  This filter converts the
// scalar representation to the vector representation.
//

#ifndef vtkMomentVectors_h
#define vtkMomentVectors_h

#include "vtkDataSetAlgorithm.h"
#include "vtkMomentFiltersModule.h" // for export macro

class VTKMOMENTFILTERS_EXPORT vtkMomentVectors : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkMomentVectors, vtkDataSetAlgorithm);
  static vtkMomentVectors* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // These are basically a convenience method that calls SetInputArrayToProcess
  // to set the array used as the input scalars.  The fieldAttributeType comes
  // from the vtkDataSetAttributes::AttributeTypes enum.
  virtual void SetInputMoment(const char* name);
  virtual void SetInputMoment(int fieldAttributeType);

  // Description:
  // If off (the default), then the input array is taken to be the total flux
  // through or or circulation along each element.  If on, then the input array
  // is taken to be the density of the flux or circulation.
  vtkGetMacro(InputMomentIsDensity, int);
  vtkSetMacro(InputMomentIsDensity, int);
  vtkBooleanMacro(InputMomentIsDensity, int);

  // Description:
  // Set the name assigned to the total or density of flux or circulation output
  // array.  If the string is NULL or empty, then the output names are taken
  // from the input flux array.
  vtkSetStringMacro(OutputMomentTotalName);
  vtkSetStringMacro(OutputMomentDensityName);

  // Description:
  // Given the current (or specified) input and current state of this filter,
  // returns the name actually assigned to the total or density of flux output
  // array.  The returned string is not guaranteed to be equal to array names
  // created from the last call to Update, but will be the same if neither the
  // input or state of this filter changes.  This method is NOT thread safe.
  const char* GetOutputMomentTotalName()
  {
    return this->GetOutputMomentTotalName(this->GetInput());
  }
  const char* GetOutputMomentDensityName()
  {
    return this->GetOutputMomentDensityName(this->GetInput());
  }
  virtual const char* GetOutputMomentTotalName(vtkDataObject* input);
  virtual const char* GetOutputMomentDensityName(vtkDataObject* input);

protected:
  vtkMomentVectors();
  ~vtkMomentVectors() override;

  int InputMomentIsDensity;

  char* OutputMomentTotalName;
  char* OutputMomentDensityName;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkMomentVectors(const vtkMomentVectors&) = delete;
  void operator=(const vtkMomentVectors&) = delete;
};

#endif // vtkMomentVectors_h
