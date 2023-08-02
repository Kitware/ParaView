// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPrismSESAMEReader
 * @brief   Reads in a SESAME file and creates a vtkPolyData object.
 *
 * vtkPrismSESAMEReader is a source object that reads in a SESAME file and creates a vtkPolyData
 * surface. The surface can be generated from tables: 301, 303-305, 502-505, 601-605. If the
 * selected table is 301, and any of the curve tables with id: 306, 401, 411 or 412 are present,
 * then the reader will also generate a vtkPartitionedDataSetCollection curves also.
 *
 * The supported tables are: 301, 303-306, 401, 411-412, 502-505, 601-605.
 *
 * You can convert the available variables by setting a multiplication factor.
 *
 * This reader can read the file format defined in:
 *
 * Stanford P. Lyon, "Sesame: the Los Alamos National Laboratory equation of state database",
 * Los Alamos National Laboratory report LA-UR-92-3407, 1992
 */

#ifndef vtkPrismSESAMEReader_h
#define vtkPrismSESAMEReader_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkNew.h"                // For vtkNew
#include "vtkPrismReadersModule.h" // For export macro

#include <map>    // for map
#include <vector> // for vector

class vtkDoubleArray;
class vtkIntArray;
class vtkPolyData;
class vtkStringArray;
class vtkPartitionedDataSetCollection;

class VTKPRISMREADERS_EXPORT vtkPrismSESAMEReader : public vtkDataObjectAlgorithm
{
public:
  static vtkPrismSESAMEReader* New();
  vtkTypeMacro(vtkPrismSESAMEReader, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of the file to read.
   */
  virtual void SetFileName(VTK_FILEPATH const char* filename);
  vtkGetFilePathMacro(FileName);
  ///@}

  /**
   * Get the all table ids.
   *
   * Note: These ids don't include the table ids of curves, i.e. 306, 401, 411, 412.
   */
  vtkGetObjectMacro(TableIds, vtkIntArray);

  /**
   * Get the surface table ids (not including curves).
   *
   * Note: These ids don't include the table ids of curves, i.e. 306, 401, 411, 412.
   */
  vtkGetObjectMacro(SurfaceTableIds, vtkIntArray);

  /**
   * Get the curve table ids.
   *
   * Note: These ids will include only curves if available, i.e. 306, 401, 411, 412.
   */
  vtkGetObjectMacro(CurveTableIds, vtkIntArray);

  ///@{
  /**
   * Set/Get the table to read in.
   */
  virtual void SetTableId(int tableId);
  vtkGetMacro(TableId, int);
  ///@}

  /**
   * Get the arrays of a table.
   */
  vtkStringArray* GetArraysOfTable(int tableId);

  /**
   * Get the arrays of TableId.
   */
  vtkStringArray* GetArraysOfSelectedTable();

  /**
   * Get the arrays of the tables in the following format:
   * T1, Array1, Array2, ...,  T2, Array1, Array2, ..., ...
   *
   * Decode the values by detecting the table id which would be the only string
   * that can be converted to an integer.
   */
  vtkStringArray* GetFlatArraysOfTables();

  ///@{
  /**
   * Set/Get the X array name.
   */
  vtkSetStringMacro(XArrayName);
  vtkGetStringMacro(XArrayName);
  ///@}

  ///@{
  /**
   * Set/Get the Y array name.
   */
  vtkSetStringMacro(YArrayName);
  vtkGetStringMacro(YArrayName);
  ///@}

  ///@{
  /**
   * Set/Get the Z array name.
   */
  vtkSetStringMacro(ZArrayName);
  vtkGetStringMacro(ZArrayName);
  ///@}

  /**
   * Get if curves are available.
   */
  bool GetCurvesAvailable();

  ///@{
  /**
   * Set/Get if curves will be read (assuming they are available).
   *
   * Default is true.
   */
  vtkSetMacro(ReadCurves, bool);
  vtkGetMacro(ReadCurves, bool);
  vtkBooleanMacro(ReadCurves, bool);
  ///@}

  ///@{
  /**
   * Set/Get the variable conversion values.
   */
  void SetNumberOfVariableConversionValues(int);
  void SetVariableConversionValue(int i, double value);
  double GetVariableConversionValue(int i);
  ///@}

protected:
  vtkPrismSESAMEReader();
  ~vtkPrismSESAMEReader() override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void RequestCurvesData(std::FILE* file, vtkPartitionedDataSetCollection* curves);
  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;

  char* FileName = nullptr;
  int TableId = -1;
  vtkNew<vtkIntArray> TableIds;
  vtkNew<vtkIntArray> SurfaceTableIds;
  vtkNew<vtkIntArray> CurveTableIds;

  std::map<int, vtkSmartPointer<vtkStringArray>> ArraysOfTables;
  vtkNew<vtkStringArray> FlatArraysOfTables;

  char* XArrayName = nullptr;
  char* YArrayName = nullptr;
  char* ZArrayName = nullptr;

  bool ReadCurves = true;

  vtkNew<vtkDoubleArray> VariableConversionValues;

private:
  vtkPrismSESAMEReader(const vtkPrismSESAMEReader&) = delete;
  void operator=(const vtkPrismSESAMEReader&) = delete;

  enum class SESAMEFormat
  {
    LANL,
    ASC
  };
  SESAMEFormat FileFormat = SESAMEFormat::ASC;
  std::vector<long> TableLocations;

  void Reset();
  bool OpenFile(std::FILE*& file);
  bool ReadTableHeader(char* buffer, int& tableId);
  bool ReadTableHeader(std::FILE* file, int& tableId);
  int ReadTableValueLine(std::FILE* file, float* v1, float* v2, float* v3, float* v4, float* v5);
  int JumpToTable(std::FILE* file, int tableId);

  /**
   * Read supported tables: 301, 303-306, 401, 411-412, 502-505, 601-605.
   */
  void ReadTable(std::FILE* file, vtkPolyData* output, int tableId);

  /**
   * Read table 301, 303-305, 502-505, 601-605.
   */
  void ReadSurfaceTable(std::FILE* file, vtkPolyData* output, int tableId);

  /**
   * Read curves table 306, 411, 412.
   */
  void ReadCurveTable(std::FILE* file, vtkPolyData* output, int tableId);

  /**
   * Read vaporization curve table 401.
   */
  void ReadVaporizationCurveTable(std::FILE* file, vtkPolyData* output, int tableId);
};

#endif // vtkPrismSESAMEReader_h
