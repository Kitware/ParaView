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
  virtual void SetInputArray(const char *name);
  virtual void SetInputArray(int fieldAttributeType);

protected:
  vtkFluxGlyphs();
  ~vtkFluxGlyphs();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

//BTX
  virtual vtkSmartPointer<vtkDataSet> MakeFluxVectors(vtkDataSet *input);
  virtual vtkSmartPointer<vtkDataArray> MakeGlyphScaleFactors(vtkDataSet*input);
  virtual vtkSmartPointer<vtkPolyData> MakeGlyphs(vtkDataSet *input);
//ETX

private:
  vtkFluxGlyphs(const vtkFluxGlyphs &);         // Not implemented
  void operator=(const vtkFluxGlyphs &);        // Not implemented
};

#endif //__vtkFluxGlyphs_h
