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
 * @class   vtkExtrusionRepresentation
 * @brief   Representation for showing data sets with extrusion along the normal.
 *
 * This mapper displays the geometry of a mesh by extruding the points along
 * the direction of the cell normals according a cell-data array. If input mesh
 * is made of squares, the result will be show a "city-map" where every cell
 * is displayed as a building of height depending the cell-data array.
 * When input is a point data, the representation is similar to a height field
 * representation.
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Michael Migliore, Kitware 2017
 * This work was supported by the German Climate Computing Center (DKRZ).
 */

#ifndef vtkExtrusionRepresentation_h
#define vtkExtrusionRepresentation_h

#include "vtkEmbossingRepresentationsModule.h"
#include "vtkGeometryRepresentationWithFaces.h"

class VTKEMBOSSINGREPRESENTATIONS_EXPORT vtkExtrusionRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkExtrusionRepresentation* New();
  vtkTypeMacro(vtkExtrusionRepresentation, vtkGeometryRepresentationWithFaces);

  /**
   * Extrusion factor information.
   */
  void SetExtrusionFactor(double val);

  /**
   * Basis visibility (can be disabled for performance)
   */
  void SetBasisVisibility(bool val);

  /**
   * Enable data normalization.
   */
  void SetNormalizeData(bool val);

  /**
   * Enable autoscaling based on minimum and maximum values.
   */
  void SetAutoScaling(bool val);

  /**
   * Specify a range in case of manual scaling.
   */
  void SetScalingRange(double minimum, double maximum);

  /**
   * Set the data used for the extrusion.
   */
  void SetInputDataArray(int idx, int port, int connection, int fieldAssociation, const char* name);

protected:
  vtkExtrusionRepresentation();
  ~vtkExtrusionRepresentation() override = default;

private:
  vtkExtrusionRepresentation(const vtkExtrusionRepresentation&) = delete;
  void operator=(const vtkExtrusionRepresentation&) = delete;
};

#endif
