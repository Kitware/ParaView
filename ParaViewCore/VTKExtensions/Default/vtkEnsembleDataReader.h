/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnsembleDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEnsembleDataReader - reader for ensemble data sets
// .SECTION Description
// vtkEnsembleDataReader reads a collection of data sources from a metadata
// file (of extension .pve).

#ifndef __vtkEnsembleDataReader_h
#define __vtkEnsembleDataReader_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkAlgorithm.h"

struct vtkEnsembleDataReaderInternal;
class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkEnsembleDataReader : public vtkAlgorithm
{
public:
  static vtkEnsembleDataReader *New();
  vtkTypeMacro(vtkEnsembleDataReader, vtkAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set/Get the filename of the ensemble (.pve extension).
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set/Get the current ensemble member to process.
  vtkSetMacro(CurrentMember, int);
  vtkGetMacro(CurrentMember, int);

  // Description:
  // Returns the number of ensemble members
  int GetNumberOfMembers() const;

  // Description:
  // Get the file path associated with the specified row of the meta data
  vtkStdString GetFilePath(const int rowIndex) const;

  // Description:
  // Set the file reader for the specified row of data
  bool SetReader(const int rowIndex, vtkAlgorithm *reader);

protected:
  vtkEnsembleDataReader();
  ~vtkEnsembleDataReader();

  virtual int ProcessRequest(vtkInformation*, vtkInformationVector**,
    vtkInformationVector*);

  virtual int FillOutputPortInformation(int, vtkInformation *info);

  vtkAlgorithm *GetCurrentReader();

  bool ReadMetaData();

private:
  char *FileName;
  int CurrentMember;

  vtkEnsembleDataReaderInternal *Internal;

  vtkEnsembleDataReader(const vtkEnsembleDataReader&); // Not implemented
  void operator=(const vtkEnsembleDataReader&); // Not implemented
};

#endif
