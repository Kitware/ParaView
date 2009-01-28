// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFluxGlyphs.h

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

// .NAME vtkFluxGlyphs - Create arrow glyphs representing flux or circulation.
//
// .SECTION Description
//
// Circulation is a vector field on 1D cells that represents flow along the path
// of the cell.  Flux is a vector field on 2D cells that represents flow through
// the area of the cell.  This filter creates arrow glyphs in the direction of
// the flow.
//

#ifndef __vtkFluxGlyphs_h
#define __vtkFluxGlyphs_h

#include "vtkPolyDataAlgorithm.h"

#include "vtkSmartPointer.h"    // For internal methods.

class vtkFluxGlyphs : public vtkPolyDataAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkFluxGlyphs, vtkPolyDataAlgorithm);
  static vtkFluxGlyphs *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // These are basically a convenience method that calls SetInputArrayToProcess
  // to set the array used as the input flux or circulation.  The
  // fieldAttributeType comes from the vtkDataSetAttributes::AttributeTypes
  // enum.
  virtual void SetInputFlux(const char *name);
  virtual void SetInputFlux(int fieldAttributeType);

  // Description:
  // If off (the default), then the input array is taken to be the total flux
  // or circulation through each element.  If on, then the input array is taken
  // to be the density of the flux or circulation.
  vtkGetMacro(InputFluxIsDensity, int);
  vtkSetMacro(InputFluxIsDensity, int);
  vtkBooleanMacro(InputFluxIsDensity, int);

  // Description:
  // If off (the default), then the glyphs are scaled by the total flux or
  // circulation through each element.  If on, then the glyphs are scaled by the
  // flux or circulation density.
  vtkGetMacro(ScaleByDensity, int);
  vtkSetMacro(ScaleByDensity, int);
  vtkBooleanMacro(ScaleByDensity, int);

protected:
  vtkFluxGlyphs();
  ~vtkFluxGlyphs();

  int InputFluxIsDensity;
  int ScaleByDensity;

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

//BTX
  virtual void MakeFluxVectors(vtkSmartPointer<vtkDataSet> &input,
                               vtkSmartPointer<vtkDataArray> &inputArray);
  virtual vtkSmartPointer<vtkDataArray> MakeGlyphScaleFactors(
                                                      vtkDataSet *input,
                                                      vtkDataArray *inputArray);
  virtual vtkSmartPointer<vtkPolyData> MakeGlyphs(vtkDataSet *input,
                                                  vtkDataArray *inputArray);
//ETX

private:
  vtkFluxGlyphs(const vtkFluxGlyphs &);         // Not implemented
  void operator=(const vtkFluxGlyphs &);        // Not implemented
};

#endif //__vtkFluxGlyphs_h
