/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGridGeneratorSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkExplicitStructuredGridGeneratorSource
 *
 * Generators of explicit structured reservoir grids.
 */

#ifndef __vtkExplicitStructuredGridGeneratorSource_h
#define __vtkExplicitStructuredGridGeneratorSource_h

#include <vtkExplicitStructuredGridModule.h>

#include "vtkExplicitStructuredGridAlgorithm.h"
#include <string> // For cache stamp

class VTKEXPLICITSTRUCTUREDGRID_EXPORT vtkExplicitStructuredGridGeneratorSource
  : public vtkExplicitStructuredGridAlgorithm
{
public:
  static vtkExplicitStructuredGridGeneratorSource* New();
  vtkTypeMacro(vtkExplicitStructuredGridGeneratorSource, vtkExplicitStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the generator mode using a value contained in the GeneratorType
   * enum.
   */
  vtkGetMacro(GeneratorMode, int);
  vtkSetMacro(GeneratorMode, int);
  //@}

  //@{
  /**
   * Get/Set the data extent.
   */
  vtkGetVector6Macro(DataExtent, int);
  vtkSetVector6Macro(DataExtent, int);
  //@}

  //@{
  /**
   * Get/Set the pyramid step size for the GENERATOR_PYRAMID mode.
   */
  vtkGetMacro(PyramidStepSize, int);
  vtkSetMacro(PyramidStepSize, int);
  //@}

  //@{
  /**
   * Get/Set the number of time steps to generate.
   */
  vtkGetMacro(NumberOfTimeSteps, int);
  vtkSetMacro(NumberOfTimeSteps, int);
  //@}

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
