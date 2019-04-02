/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGridPythonExtractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExplicitStructuredGridPythonExtractor
 *
 * Filter which extracts a piece of explicit structured grid changing its
 * extents based on a user specified Python expression.
 * The Python expression filters the grid's cells and determines whether
 * a cell is part of the extracted grid or not.
 */

#ifndef vtkExplicitStructuredGridPythonExtractor_h
#define vtkExplicitStructuredGridPythonExtractor_h

#include <vtkExplicitStructuredGridModule.h>

#include "vtkExplicitStructuredGridAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

#include <vector>

class vtkIdList;

class VTKEXPLICITSTRUCTUREDGRID_EXPORT vtkExplicitStructuredGridPythonExtractor
  : public vtkExplicitStructuredGridAlgorithm
{
public:
  static vtkExplicitStructuredGridPythonExtractor* New();
  vtkTypeMacro(vtkExplicitStructuredGridPythonExtractor, vtkExplicitStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set a Python expression to evaluate for grid extraction.
   */
  vtkGetStringMacro(PythonExpression);
  vtkSetStringMacro(PythonExpression);
  //@}

  //@{
  /**
   * Get/Set whether to pass data to the Python script or not.
   * Passing the data to the script performs a copy operation of the data.
   * This operation can become very expensive for larger data both in memory and CPU.
   */
  vtkGetMacro(PassDataToScript, bool);
  vtkSetMacro(PassDataToScript, bool);
  vtkBooleanMacro(PassDataToScript, bool);
  //@}

protected:
  vtkExplicitStructuredGridPythonExtractor();
  ~vtkExplicitStructuredGridPythonExtractor() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool EvaluatePythonExpression(vtkIdType cellId, vtkIdList* ptIds, int i, int j, int k,
    std::vector<vtkDataArray*>& cellArrays, std::vector<vtkDataArray*>& pointArrays);

  char* PythonExpression = nullptr;
  bool PassDataToScript = false;

private:
  vtkExplicitStructuredGridPythonExtractor(
    const vtkExplicitStructuredGridPythonExtractor&) = delete;
  void operator=(const vtkExplicitStructuredGridPythonExtractor&) = delete;
};

#endif
