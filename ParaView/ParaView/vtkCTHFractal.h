/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFractal.h
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
// .NAME vtkCTHFractal - A source to test my new CTH AMR data object.
// .SECTION Description
// vtkCTHFractal is a collection of image datas.  All have the same dimensions.
// Each block has a different origin and spacing.  It uses mandelbrot
// to create cell data.  I scale the fractal array to look like a volme fraction.
// I may also add block id and level as extra cell arrays.

#ifndef __vtkCTHFractal_h
#define __vtkCTHFractal_h

#include "vtkCTHSource.h"

class vtkCTHData;

class VTK_EXPORT vtkCTHFractal : public vtkCTHSource
{
public:
  static vtkCTHFractal *New();

  vtkTypeRevisionMacro(vtkCTHFractal,vtkCTHSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // XYZ dimensions of cells.
  vtkSetMacro(Dimensions, int);
  vtkGetMacro(Dimensions, int);

  // Description:
  // For testing ghost levels.
  vtkSetMacro(GhostLevels, int);
  vtkGetMacro(GhostLevels, int);
  vtkBooleanMacro(GhostLevels, int);

protected:
  vtkCTHFractal();
  ~vtkCTHFractal();

  virtual void Execute();
  void SetDimensions(int xDim, int yDim, int zDim);
  void SetBlockInfo(int blockId, 
                    float ox, float oy, float oz,
                    float sx, float sy, float sz);

  void AddFractalArray();
  void AddBlockIdArray();
  void AddDepthArray(float sx1);
  void AddGhostLevelArray();

private:
  void InternalImageDataCopy(vtkCTHFractal *src);

  int Dimensions;
  float FractalValue;
  int GhostLevels;

private:
  vtkCTHFractal(const vtkCTHFractal&);  // Not implemented.
  void operator=(const vtkCTHFractal&);  // Not implemented.
};


#endif



