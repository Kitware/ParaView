/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBumpMapRepresentation
 * @brief   Representation for showing data sets with bump mapping effect.
 *
 * This representation display its input mesh using a bump mapping effect.
 * A bump mapping effect is a modification of the normals in order to add
 * embossing on a flat mesh thanks to the lights of the scene.
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Michael Migliore, Kitware 2017
 * This work was supported by the German Climate Computing Center (DKRZ).
 */

#ifndef vtkBumpMapRepresentation_h
#define vtkBumpMapRepresentation_h

#include "vtkEmbossingRepresentationsModule.h"
#include "vtkGeometryRepresentationWithFaces.h"

class VTKEMBOSSINGREPRESENTATIONS_EXPORT vtkBumpMapRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkBumpMapRepresentation* New();
  vtkTypeMacro(vtkBumpMapRepresentation, vtkGeometryRepresentationWithFaces);

  /**
   * Bump mapping factor information.
   */
  void SetBumpMappingFactor(double val);

  /**
   * Sets the array used by the representation.
   */
  void SetInputDataArray(int idx, int port, int connection, int fieldAssociation, const char* name);

protected:
  vtkBumpMapRepresentation();
  ~vtkBumpMapRepresentation() override = default;

private:
  vtkBumpMapRepresentation(const vtkBumpMapRepresentation&) = delete;
  void operator=(const vtkBumpMapRepresentation&) = delete;
};

#endif
