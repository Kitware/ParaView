// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMomentGlyphs.h

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

// .NAME vtkMomentGlyphs - Create arrow glyphs representing flux or circulation.
//
// .SECTION Description
//
// Circulation is a vector field on 1D cells that represents flow along the path
// of the cell.  Flux is a vector field on 2D cells that represents flow through
// the area of the cell.  This filter creates arrow glyphs in the direction of
// the flow.
//

#ifndef vtkMomentGlyphs_h
#define vtkMomentGlyphs_h

#include "vtkMomentFiltersModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkSmartPointer.h" // For internal methods.

class VTKMOMENTFILTERS_EXPORT vtkMomentGlyphs : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMomentGlyphs, vtkPolyDataAlgorithm);
  static vtkMomentGlyphs* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // These are basically a convenience method that calls SetInputArrayToProcess
  // to set the array used as the input flux or circulation.  The
  // fieldAttributeType comes from the vtkDataSetAttributes::AttributeTypes
  // enum.
  virtual void SetInputMoment(const char* name);
  virtual void SetInputMoment(int fieldAttributeType);

  // Description:
  // If off (the default), then the input array is taken to be the total flux
  // through or circulation along each element.  If on, then the input array is
  // taken to be the density of the flux or circulation.
  vtkGetMacro(InputMomentIsDensity, int);
  vtkSetMacro(InputMomentIsDensity, int);
  vtkBooleanMacro(InputMomentIsDensity, int);

  // Description:
  // If off (the default), then the glyphs are scaled by the total flux through
  // or circulation along each element.  If on, then the glyphs are scaled by
  // the flux or circulation density.
  vtkGetMacro(ScaleByDensity, int);
  vtkSetMacro(ScaleByDensity, int);
  vtkBooleanMacro(ScaleByDensity, int);

protected:
  vtkMomentGlyphs();
  ~vtkMomentGlyphs() override;

  int InputMomentIsDensity;
  int ScaleByDensity;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  virtual void MakeMomentVectors(
    vtkSmartPointer<vtkDataSet>& input, vtkSmartPointer<vtkDataArray>& inputArray);
  virtual vtkSmartPointer<vtkDataArray> MakeGlyphScaleFactors(
    vtkDataSet* input, vtkDataArray* inputArray);
  virtual vtkSmartPointer<vtkPolyData> MakeGlyphs(vtkDataSet* input, vtkDataArray* inputArray);

private:
  vtkMomentGlyphs(const vtkMomentGlyphs&) = delete;
  void operator=(const vtkMomentGlyphs&) = delete;
};

#endif // vtkMomentGlyphs_h
