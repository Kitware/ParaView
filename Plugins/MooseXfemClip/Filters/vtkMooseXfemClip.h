/*=========================================================================

  Module:    vtkMooseXfemClip.h
  Copyright (c) 2017 Battelle Energy Alliance, LLC

  Based on Visualization Toolkit
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkMooseXfemClip
 * @brief Clip partial elements produced by MOOSE xfem module
 *
 * vtkMooseXfemClip is a filter for use visualizing results produced by
 * the xfem module in the MOOSE code (www.mooseframework.org). The MOOSE
 * xfem implementation uses the phantom node technique, in which elements
 * traversed by a discontinuity are split into two partial elements, each
 * containing physical and non-physical material.
 * @par
 * The xfem code generates two sets of elemental variables:
 * 'xfem_cut_origin_[xyz]' and * 'xfem_cut_normal_[xyz], which define the
 * origin and normal of a cutting plane to be applied to each partial
 * element. If these both contain nonzero values, this filter will cut off
 * the non-physical portions of those elements.
 * @par
 * It is necessary to define the cut planes in this way rather than using
 * a global signed distance function because a global signed distance
 * function has ambiguities at crack tips, where two partial elements share
 * a common edge or face.
 * @par Thanks:
 * This filter is based on the vtkClipDataSet filter by Ken Martin,
 * Will Schroeder, and Bill Lorensen
 */

#ifndef vtkMooseXFEMClip_h
#define vtkMooseXFEMClip_h

#include "vtkMooseXfemClipModule.h" // for export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIncrementalPointLocator;

class VTKMOOSEXFEMCLIP_EXPORT vtkMooseXfemClip : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkMooseXfemClip* New();
  vtkTypeMacro(vtkMooseXfemClip, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points. In this case, a non-merging
   * locator is used because merging is not desired.
   */
  void CreateDefaultLocator();

protected:
  vtkMooseXfemClip();
  ~vtkMooseXfemClip();

  int OutputPointsPrecision;
  vtkIncrementalPointLocator* Locator;

private:
  vtkMooseXfemClip(const vtkMooseXfemClip&) = delete; // Not implemented.
  void operator=(const vtkMooseXfemClip&) = delete;   // Not implemented.
};

#endif
