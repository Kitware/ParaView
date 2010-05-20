/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSpriteDefaultPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPointSpriteDefaultPainter
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
// The vtkPointSpriteDefaultPainter replaces the vtkScalarsToColorsPainter by a vtkTwoScalarsToColorsPainter
// and add a vtkDepthSortPainter in the painter chain.

#ifndef __vtkPointSpriteDefaultPainter_h__
#define __vtkPointSpriteDefaultPainter_h__

#include "vtkDefaultPainter.h"

class vtkDepthSortPainter;

class VTK_EXPORT vtkPointSpriteDefaultPainter : public vtkDefaultPainter
{
public :
  static vtkPointSpriteDefaultPainter* New();
  vtkTypeMacro(vtkPointSpriteDefaultPainter, vtkDefaultPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the PolyDataPainter. This painter is slow but
  // it can send any kind of primitives to the device.
  void SetDepthSortPainter(vtkDepthSortPainter*);
  vtkGetObjectMacro(DepthSortPainter, vtkDepthSortPainter);

protected:
  // Description:
  // Setups the the painter chain.
  virtual void BuildPainterChain();

  // Description:
  // Take part in garbage collection.
  virtual void ReportReferences(vtkGarbageCollector *collector);

  vtkDepthSortPainter* DepthSortPainter;

  vtkPointSpriteDefaultPainter();
  ~vtkPointSpriteDefaultPainter();

private:
  vtkPointSpriteDefaultPainter(const vtkPointSpriteDefaultPainter&); // Not implemented.
  void operator=(const vtkPointSpriteDefaultPainter&); // Not implemented.
};

#endif
