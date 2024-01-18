// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkExtractCellsAlongLine
 * @brief Filter to simplify vtkExtractCellsAlongPolyLine usage when probing over a simple line.
 *
 * Internal Paraview filters for API backward compatibilty and ease of use.
 * Internally build a line source as well as a vtkExtractCellsAlongLine and exposes
 * their properties.
 *
 * @sa vtkExtractCellsAlongPolyLine
 */

#ifndef vtkExtractCellsAlongLine_h
#define vtkExtractCellsAlongLine_h

#include "vtkNew.h"                             // needed for internal filters
#include "vtkPVVTKExtensionsExtractionModule.h" //needed for exports
#include "vtkUnstructuredGridAlgorithm.h"

class vtkExtractCellsAlongPolyLine;
class vtkLineSource;

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkExtractCellsAlongLine
  : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkExtractCellsAlongLine* New();
  vtkTypeMacro(vtkExtractCellsAlongLine, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the begin and end points for the line to probe against.
   */
  vtkGetVector3Macro(Point1, double);
  vtkSetVector3Macro(Point1, double);
  vtkGetVector3Macro(Point2, double);
  vtkSetVector3Macro(Point2, double);
  ///@}

protected:
  vtkExtractCellsAlongLine();
  ~vtkExtractCellsAlongLine() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  double Point1[3] = { 0.0, 0.0, 0.0 };
  double Point2[3] = { 1.0, 1.0, 1.0 };

  vtkNew<vtkLineSource> LineSource;
  vtkNew<vtkExtractCellsAlongPolyLine> Extractor;

private:
  vtkExtractCellsAlongLine(const vtkExtractCellsAlongLine&) = delete;
  void operator=(const vtkExtractCellsAlongLine&) = delete;
};

#endif // vtkExtractCellsAlongLine_h
