// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkNastranBDFReader
 * @brief   Reader for Bulk Data Format from Nastran
 *
 */
#ifndef vtkNastranBDFReader_h
#define vtkNastranBDFReader_h

#include "vtkUnstructuredGridAlgorithm.h"

#include "vtkNew.h"
#include "vtkPVVTKExtensionsIOGeneralModule.h" //needed for exports
#include "vtkSmartPointer.h"

#include <map>
#include <string>
#include <vector>

class vtkCellArray;
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkPoints;

class VTKPVVTKEXTENSIONSIOGENERAL_EXPORT vtkNastranBDFReader : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkNastranBDFReader, vtkUnstructuredGridAlgorithm);
  static vtkNastranBDFReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the file path.
   */
  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);
  ///@}

protected:
  vtkNastranBDFReader();
  ~vtkNastranBDFReader() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  ///@{
  /**
   * Inner methods to parse Bulk Data Entries and fill output vtkDataObject.
   */
  // Add title as field data
  bool AddTitle(const std::vector<std::string>& args);
  // Add time as field data
  bool AddTimeInfo(const std::vector<std::string>& args);
  // Add output point
  bool AddPoint(const std::vector<std::string>& args);
  // Add triangle cell. Points should be parsed first.
  bool AddTriangle(const std::vector<std::string>& args);
  // Add Pload2 as cell data. Cells should be parsed first.
  bool AddPload2Data(const std::vector<std::string>& args);
  ///@}

  // Return the id in `Points` of the element id `arg`
  vtkIdType GetVTKPointId(const std::string& arg);

  std::string FileName;

  vtkNew<vtkPoints> Points;
  vtkNew<vtkCellArray> Cells;
  vtkNew<vtkIdTypeArray> OriginalPointIds;
  vtkSmartPointer<vtkDoubleArray> Pload2;

  // Utilities map to store <inputId, VTKId>
  std::map<vtkIdType, vtkIdType> CellsIds;
  std::map<vtkIdType, vtkIdType> PointsIds;

  // Store parsing errors as <Keyword, numberOfOccurence>
  std::map<std::string, vtkIdType> UnsupportedElements;

private:
  vtkNastranBDFReader(const vtkNastranBDFReader&) = delete;
  void operator=(const vtkNastranBDFReader&) = delete;
};

#endif
