/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtrusionMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtrusionMapper
 * @brief   Mapper for showing datasets with extrusion along the normal.
 *
 * This mapper displays the geometry of a mesh by extruding the points along
 * the direction of the cell normals according a cell-data array. If input mesh
 * is made of squares, the result will be show a "city-map" where every cell
 * is displayed as a building of height depending the cell-data array.
 * When input is a point data, the representation is similar to a height field
 * representation.
 *
 * The direction of extrusion is based on the triangles normals.
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Michael Migliore, Kitware 2017
 * This work was supported by the German Climate Computing Center (DKRZ).
 *
 * @sa
 * vtkCompositePolyDataMapper2
 *
 */

#ifndef vtkExtrusionMapper_h
#define vtkExtrusionMapper_h

#include "vtkCompositePolyDataMapper2.h"
#include "vtkEmbossingRepresentationsModule.h"

class vtkMultiProcessController;

class VTKEMBOSSINGREPRESENTATIONS_EXPORT vtkExtrusionMapper : public vtkCompositePolyDataMapper2
{
public:
  static vtkExtrusionMapper* New();
  vtkTypeMacro(vtkExtrusionMapper, vtkCompositePolyDataMapper2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the factor used to scale the extrusion. Its value is a percentage,
   * defined according to the maximum length of the bounding box of the mesh.
   * The default value is 50.0, i.e. 50% of the maximum length of the bounding box.
   * Negative values are accepted in order to reverse the direction of extrusion.
   * Default is 50.
   */
  void SetExtrusionFactor(float factor);
  vtkGetMacro(ExtrusionFactor, float);
  //@}

  //@{
  /**
   * Set/Get the basis visibility flag. If disabled, the original cell is not drawn.
   * Default is disabeld.
   */
  vtkSetMacro(BasisVisibility, bool);
  vtkGetMacro(BasisVisibility, bool);
  vtkBooleanMacro(BasisVisibility, bool);
  //@}

  //@{
  /**
   * Set/Get the auto scaling flag. If disabled, user range is used.
   * Default is enabled.
   */
  vtkSetMacro(AutoScaling, bool);
  vtkGetMacro(AutoScaling, bool);
  vtkBooleanMacro(AutoScaling, bool);
  //@}

  //@{
  /**
   * Set/Get the user range.
   * Default is [0,1].
   */
  vtkSetVector2Macro(UserRange, float);
  vtkGetVector2Macro(UserRange, float);
  //@}

  //@{
  /**
   * Set/Get the multi process controller object.
   */
  void SetController(vtkMultiProcessController* c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * Set/Get the data normalization flag.
   * Default is enabled.
   */
  vtkSetMacro(NormalizeData, bool);
  vtkGetMacro(NormalizeData, bool);
  vtkBooleanMacro(NormalizeData, bool);
  //@}

  //@{
  /**
   * Override SetInputArrayToProcess to update data range and save field association.
   */
  using vtkAlgorithm::SetInputArrayToProcess;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) override;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, int fieldAttributeType) override;
  void SetInputArrayToProcess(int idx, vtkInformation* info) override;
  //@}

  void ResetDataRange();

protected:
  vtkExtrusionMapper();
  ~vtkExtrusionMapper() override;

  /**
   * Creation of a helper
   */
  vtkCompositeMapperHelper2* CreateHelper() override;

  /**
   * Extends bounds to take into account extrusion
   */
  void ComputeBounds() override;

  /**
   * Override to compute data range
   */
  void InitializeHelpersBeforeRendering(vtkRenderer* ren, vtkActor* actor) override;

  vtkMultiProcessController* Controller = nullptr;
  bool NormalizeData = true;
  int FieldAssociation;
  double LocalDataRange[2];
  double GlobalDataRange[2];

  float ExtrusionFactor = 50.f;
  float MaxBoundsLength = 0.f;
  float UserRange[2];
  bool BasisVisibility = false;
  bool AutoScaling = true;

  friend class vtkExtrusionMapperHelper;

private:
  vtkExtrusionMapper(const vtkExtrusionMapper&) = delete;
  void operator=(const vtkExtrusionMapper&) = delete;
};

#endif
