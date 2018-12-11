// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: pqBlotDialog.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkPVRecoverGeometryWireframe
 * @brief   Get corrected wireframe from tessellated facets
 *
 *
 *
 * When vtkPVGeometryFilter tessellates nonlinear faces into linear
 * approximations, it introduces edges in the middle of the facets of the
 * original mesh (as any valid tessellation would).  To correct for this,
 * vtkPVGeometryFilter also writes out some fields that allows use to identify
 * the edges that are actually part of the original mesh.  This filter works in
 * conjunction with vtkPVGeometryFilter by taking its output, adding an edge
 * flag and making the appropriate adjustments so that rendering with line
 * fill mode will make the correct wireframe.
 *
 * @sa
 * vtkPVGeometryFilter
 *
*/

#ifndef vtkPVRecoverGeometryWireframe_h
#define vtkPVRecoverGeometryWireframe_h

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkPolyDataAlgorithm.h"

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVRecoverGeometryWireframe : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkPVRecoverGeometryWireframe, vtkPolyDataAlgorithm);
  static vtkPVRecoverGeometryWireframe* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * In order to determine which edges existed in the original data, we need an
   * identifier on each cell determining which face (not cell) it originally
   * came from.  The ids should be put in a cell data array with this name.  The
   * existence of this field is also a signal that this wireframe extraction is
   * necessary.
   */
  static const char* ORIGINAL_FACE_IDS() { return "vtkPVRecoverWireframeOriginalFaceIds"; }

protected:
  vtkPVRecoverGeometryWireframe();
  ~vtkPVRecoverGeometryWireframe() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkPVRecoverGeometryWireframe(const vtkPVRecoverGeometryWireframe&) = delete;
  void operator=(const vtkPVRecoverGeometryWireframe&) = delete;
};

#endif // vtkPVRecoverGeometryWireframe_h
