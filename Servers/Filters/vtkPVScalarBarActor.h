/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarBarActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkPVScalarBarActor - A scalar bar with labels of fixed font.
//
// .SECTION Description
//
// vtkPVScalarBarActor has basically the same functionality as vtkScalarBarActor
// except that the fonts are set to a fixed size.  Adjustments to the scalar bar
// happen solely on the bar itself.  There are other slight differences in how
// the size of the scalar bar is determined to make it easier to control in
// ParaView.
//

#ifndef __vtkPVScalarBarActor_h
#define __vtkPVScalarBarActor_h

#include "vtkScalarBarActor.h"

class VTK_EXPORT vtkPVScalarBarActor : public vtkScalarBarActor
{
public:
  vtkTypeRevisionMacro(vtkPVScalarBarActor, vtkScalarBarActor);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkPVScalarBarActor *New();

  // Description:
  // The bar aspect ratio (length/width).  Defaults to 20.  Note that this
  // the aspect ration of the color bar only, not including labels.
  vtkGetMacro(AspectRatio, double);
  vtkSetMacro(AspectRatio, double);

  // Description:
  // If true (the default), the printf format used for the labels will be
  // automatically generated to make the numbers best fit within the widget.  If
  // false, the LabelFormat ivar will be used.
  vtkGetMacro(AutomaticLabelFormat, int);
  vtkSetMacro(AutomaticLabelFormat, int);
  vtkBooleanMacro(AutomaticLabelFormat, int);

protected:
  vtkPVScalarBarActor();
  ~vtkPVScalarBarActor();

  double AspectRatio;

  int AutomaticLabelFormat;

  virtual void AllocateAndSizeLabels(int *labelSize, int *propSize,
                                     vtkViewport *viewport, double *range);

  virtual void SizeTitle(int *titleSize, int *propSize, vtkViewport *viewport);

private:
  vtkPVScalarBarActor(const vtkPVScalarBarActor &);     // Not implemented.
  void operator=(const vtkPVScalarBarActor &);          // Not implemented.
};

#endif //__vtkPVScalarBarActor_h
