/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTwoScalarsToColorsPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTwoScalarsToColorsPainter
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
// .SECTION Description
// this painter blends two arrays in the color.
// It sends the two array through two different lookup tables, and blends the result
// two blending modes are possible : Multiply and Add.
// in the Multiply mode, the two colors are scaled to 0-1, multiplied, and rescaled to 0-255.
// in the Add mode, the two components are simply added and clamped to 0-255.

#ifndef __vtkTwoScalarsToColorsPainter_h__
#define __vtkTwoScalarsToColorsPainter_h__

#include "vtkOpenGLScalarsToColorsPainter.h"

class VTK_EXPORT vtkTwoScalarsToColorsPainter : public vtkOpenGLScalarsToColorsPainter
{
public :
  static vtkTwoScalarsToColorsPainter* New();
  vtkTypeMacro(vtkTwoScalarsToColorsPainter, vtkOpenGLScalarsToColorsPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the name of the second array to blend with.
  vtkSetStringMacro(OpacityArrayName);
  vtkGetStringMacro(OpacityArrayName);

  // Description:
  // Enable/disble this painter
  vtkSetMacro(EnableOpacity, int);
  vtkGetMacro(EnableOpacity, int);

  // Description:
  // if ScalarVisibility is disabled, then this filter creates an array
  // of constant colors and blend it with the opacity array.
  // This parameter says if this should be done on point/cell or field data.
  vtkSetMacro(OpacityScalarMode, int);
  vtkGetMacro(OpacityScalarMode, int);

protected :
  char *OpacityArrayName;
  int EnableOpacity;
  int OpacityScalarMode;
  vtkTimeStamp  BlendTime;

  void PrepareForRendering(vtkRenderer* renderer,
      vtkActor* actor);

  void RenderInternals(vtkRenderer* renderer,
      vtkActor* actor,
      unsigned long typeflags,
      bool forceCompileOnly);

  virtual void MapScalars(vtkDataSet* output,
    double alpha, int multiply_with_alpha,
    vtkDataSet* input, vtkActor* actor);
  virtual void MapScalars(vtkDataSet* output, double alpha,
    int multiply_with_alpha, vtkDataSet* input)
    {
    this->Superclass::MapScalars(output, alpha, multiply_with_alpha, input);
    }
  // Description:
  // Create a new shallow-copied clone for data with no scalars.
  virtual vtkDataObject* NewClone(vtkDataObject* data);

  vtkTwoScalarsToColorsPainter();
  ~vtkTwoScalarsToColorsPainter();

private:
  vtkTwoScalarsToColorsPainter(const vtkTwoScalarsToColorsPainter&); // Not implemented.
  void operator=(const vtkTwoScalarsToColorsPainter&); // Not implemented.
};

#endif
