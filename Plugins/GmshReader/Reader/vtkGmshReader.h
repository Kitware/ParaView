/*=========================================================================

  Program:   ParaView
  Module:    vtkGmshReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGmshReader
 *
 * Reader for visualization of high-order polynomial solutions under
 * the Gmsh format.
 * @par Thanks:
 * ParaViewGmshReaderPlugin - Copyright (C) 2015 Cenaero
 * See the Copyright.txt and License.txt files provided
 * with ParaViewGmshReaderPlugin for license information.
 *
 */

#ifndef vtkGmshReader_h
#define vtkGmshReader_h

#include "vtkGmshReaderModule.h"
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

  //@{
  /**
   * Specify file name of Gmsh geometry file to read.
   */
  vtkSetStringMacro(GeometryFileName);
  vtkGetStringMacro(GeometryFileName);
  //@}

  //@{
  /**
   * Specify XML file name
   */
  vtkSetStringMacro(XMLFileName);
  vtkGetStringMacro(XMLFileName);
  //@}

  //@{
  /**
   * Specify current time step
   */
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);
  //@}

  //@{
  /**
   * Specify number of mesh partitions
   */
  vtkSetMacro(NumPieces, int);
  vtkGetMacro(NumPieces, int);
  //@}

  //@{
  /**
   * Specify number of partition files
   */
  vtkSetMacro(NumFiles, int);
  vtkGetMacro(NumFiles, int);
  //@}

  //@{
  /**
   * Specify part id
   */
  vtkSetMacro(PartID, int);
  vtkGetMacro(PartID, int);
  //@}

  //@{
  /**
   * Specify file id
   */
  vtkSetMacro(FileID, int);
  vtkGetMacro(FileID, int);
  //@}

  //@{
  /**
   * Clear/Set info. in FieldInfoMap for object of vtkGmshReaderInternal
   */
  void ClearFieldInfo();
  void SetAdaptInfo(int adaptLevel, double adaptTol);
  void SetFieldInfoPath(const std::string& addPath);
  int GetSizeFieldPathPattern();
  //@}

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
