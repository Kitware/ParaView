// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2015 Cenaero
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkGmshReader
 *
 * Reader for visualization of high-order polynomial solutions under
 * the Gmsh format.
 */

#ifndef vtkGmshReader_h
#define vtkGmshReader_h

#include "vtkGmshReaderModule.h" // for export macro
#include "vtkUnstructuredGridAlgorithm.h"

struct vtkGmshReaderInternal;

class VTKGMSHREADER_EXPORT vtkGmshReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkGmshReader* New();
  vtkTypeMacro(vtkGmshReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  template <class T>
  void ReplaceAllStringPattern(std::string& input, const std::string& pIdentifier, const T& target);

  ///@{
  /**
   * Specify file name of Gmsh geometry file to read.
   */
  vtkSetStringMacro(GeometryFileName);
  vtkGetStringMacro(GeometryFileName);
  ///@}

  ///@{
  /**
   * Specify XML file name
   */
  vtkSetStringMacro(XMLFileName);
  vtkGetStringMacro(XMLFileName);
  ///@}

  ///@{
  /**
   * Specify current time step
   */
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);
  ///@}

  ///@{
  /**
   * Specify number of mesh partitions
   */
  vtkSetMacro(NumPieces, int);
  vtkGetMacro(NumPieces, int);
  ///@}

  ///@{
  /**
   * Specify number of partition files
   */
  vtkSetMacro(NumFiles, int);
  vtkGetMacro(NumFiles, int);
  ///@}

  ///@{
  /**
   * Specify part id
   */
  vtkSetMacro(PartID, int);
  vtkGetMacro(PartID, int);
  ///@}

  ///@{
  /**
   * Specify file id
   */
  vtkSetMacro(FileID, int);
  vtkGetMacro(FileID, int);
  ///@}

  ///@{
  /**
   * Clear/Set info. in FieldInfoMap for object of vtkGmshReaderInternal
   */
  void ClearFieldInfo();
  void SetAdaptInfo(int adaptLevel, double adaptTol);
  void SetFieldInfoPath(const std::string& addPath);
  int GetSizeFieldPathPattern();
  ///@}

protected:
  vtkGmshReader();
  ~vtkGmshReader() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int ReadGeomAndFieldFile(int& firstVertexNo, vtkUnstructuredGrid* output);

private:
  char* FileName;
  char* GeometryFileName;
  char* XMLFileName;
  int FileID;
  int NumPieces;
  int NumFiles;
  int PartID;
  int TimeStep;

  vtkGmshReaderInternal* Internal;

  vtkGmshReader(const vtkGmshReader&) = delete;
  void operator=(const vtkGmshReader&) = delete;
};

#endif
