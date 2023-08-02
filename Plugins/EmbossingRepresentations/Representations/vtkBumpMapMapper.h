// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBumpMapMapper
 * @brief   Mapper for showing data sets with bump mapping effect.
 *
 * This mapper display its input mesh using a bump mapping effect.
 * A bump mapping effect is a modification of the normals in order to add
 * embossing on a flat mesh thanks to the lights of the scene.
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Michael Migliore, Kitware 2017
 * This work was supported by the German Climate Computing Center (DKRZ).
 *
 * @sa
 * vtkCompositePolyDataMapper
 */

#ifndef vtkBumpMapMapper_h
#define vtkBumpMapMapper_h

#include "vtkCompositePolyDataMapper.h"
#include "vtkEmbossingRepresentationsModule.h" // for export macro

class vtkMultiProcessController;

class VTKEMBOSSINGREPRESENTATIONS_EXPORT vtkBumpMapMapper : public vtkCompositePolyDataMapper
{
public:
  static vtkBumpMapMapper* New();
  vtkTypeMacro(vtkBumpMapMapper, vtkCompositePolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the factor used to scale the bump mapping effect.
   * This factor scales the gradient vector magnitude computed, based on input array.
   * The default value is 50.0
   */
  vtkSetMacro(BumpMappingFactor, float);
  vtkGetMacro(BumpMappingFactor, float);
  ///@}

protected:
  vtkBumpMapMapper() = default;
  ~vtkBumpMapMapper() override = default;

  /**
   * Creation of a delegator
   */
  vtkCompositePolyDataMapperDelegator* CreateADelegator() override;

  float BumpMappingFactor = 50.f;

private:
  vtkBumpMapMapper(const vtkBumpMapMapper&) = delete;
  void operator=(const vtkBumpMapMapper&) = delete;
};

#endif
