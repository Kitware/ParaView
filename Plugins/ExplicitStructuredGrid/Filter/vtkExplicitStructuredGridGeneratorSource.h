// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkExplicitStructuredGridGeneratorSource
 *
 * Generators of explicit structured reservoir grids.
 */

#ifndef vtkExplicitStructuredGridGeneratorSource_h
#define vtkExplicitStructuredGridGeneratorSource_h

#include "vtkExplicitStructuredGridModule.h" // for export macro

#include "vtkExplicitStructuredGridAlgorithm.h"
#include <string> // For cache stamp

class VTKEXPLICITSTRUCTUREDGRID_EXPORT vtkExplicitStructuredGridGeneratorSource
  : public vtkExplicitStructuredGridAlgorithm
{
public:
  static vtkExplicitStructuredGridGeneratorSource* New();
  vtkTypeMacro(vtkExplicitStructuredGridGeneratorSource, vtkExplicitStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the generator mode using a value contained in the GeneratorType
   * enum.
   */
  vtkGetMacro(GeneratorMode, int);
  vtkSetMacro(GeneratorMode, int);
  ///@}

  ///@{
  /**
   * Get/Set the data extent.
   */
  vtkGetVector6Macro(DataExtent, int);
  vtkSetVector6Macro(DataExtent, int);
  ///@}

  ///@{
  /**
   * Get/Set the pyramid step size for the GENERATOR_PYRAMID mode.
   */
  vtkGetMacro(PyramidStepSize, int);
  vtkSetMacro(PyramidStepSize, int);
  ///@}

  ///@{
  /**
   * Get/Set the number of time steps to generate.
   */
  vtkGetMacro(NumberOfTimeSteps, int);
  vtkSetMacro(NumberOfTimeSteps, int);
  ///@}

  enum GeneratorType
  {
    GENERATOR_PILLAR = 0,
    GENERATOR_DISCONTINOUS = 1,
    GENERATOR_CONTINUOUS = 2,
    GENERATOR_STEPS = 3,
    GENERATOR_PYRAMID = 4
  };

protected:
  vtkExplicitStructuredGridGeneratorSource();
  ~vtkExplicitStructuredGridGeneratorSource() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation* request, vtkInformationVector**, vtkInformationVector*) override;

  int GeneratorMode = GENERATOR_CONTINUOUS;
  int DataExtent[6] = { 0, 50, 0, 50, 0, 50 };
  int PyramidStepSize = 20;
  int NumberOfTimeSteps = 20;

  void AddTemporalData(double time, vtkExplicitStructuredGrid* grid);

  vtkExplicitStructuredGrid* Cache = nullptr;
  std::string CacheStamp;

private:
  vtkExplicitStructuredGridGeneratorSource(
    const vtkExplicitStructuredGridGeneratorSource&) = delete;              // Not implemented.
  void operator=(const vtkExplicitStructuredGridGeneratorSource&) = delete; // Not implemented.
};

#endif
