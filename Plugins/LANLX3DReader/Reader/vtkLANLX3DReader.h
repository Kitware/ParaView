/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLANLX3DReader.h

  Copyright (c) 2021, Los Alamos National Laboratory
  All rights reserved.
  See Copyright.md for details.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. See the above copyright notice for more information.

=========================================================================*/
/**
 *
 * @class vtkLANLX3DReader
 * @brief class for reading LANL X3D format files
 *
 * @section caveats Caveats
 * The LANL X3D file format is not to be confused with the X3D file format that
 * is the successor to VRML. The LANL X3D format is designed to store geometry
 * for LANL physics codes.
 *
 * @par Thanks:
 * Developed by Jonathan Woodering at Los Alamos National Laboratory
 */

#ifndef vtkLANLX3DReader_h
#define vtkLANLX3DReader_h

#include "vtkLANLX3DReaderModule.h" // for export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkMultiBlockDataSet;

class VTKLANLX3DREADER_EXPORT vtkLANLX3DReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkLANLX3DReader* New();
  vtkTypeMacro(vtkLANLX3DReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetMacro(ReadAllPieces, bool);
  vtkGetMacro(ReadAllPieces, bool);

protected:
  vtkLANLX3DReader();
  ~vtkLANLX3DReader() override;

  char* FileName = 0;
  bool ReadAllPieces = true;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkLANLX3DReader(const vtkLANLX3DReader&) = delete;
  void operator=(const vtkLANLX3DReader&) = delete;
};

#endif
