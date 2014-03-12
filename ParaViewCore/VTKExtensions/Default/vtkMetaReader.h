/*=========================================================================

  Program:   ParaView
  Module:    vtkMetaReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMetaReader - Common functionality for a meta-reader.
//
// .SECTION Description:
//
//

#ifndef __vtkMetaReader_h
#define __vtkMetaReader_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkDataObjectAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkMetaReader :
  public vtkDataObjectAlgorithm
{
public:
  static vtkMetaReader* New();
  vtkTypeMacro(vtkMetaReader, vtkDataObjectAlgorithm);

  vtkMetaReader();
  ~vtkMetaReader();

  // Description:
  // Set/get the internal reader.
  vtkSetObjectMacro(Reader, vtkAlgorithm);
  vtkGetObjectMacro(Reader, vtkAlgorithm);


  // Description:
  // Get/set the filename for the meta-file.
  // Description:
  // Get/Set the meta-file name
  void SetFileName (const char* name)
  {
    Set_FileName (name);
    this->FileNameMTime = this->GetMTime ();
  }
  char* GetFileName ()
  {
    return Get_FileName();
  }

  // Description:
  // Returns the available range of file indexes. It is
  // 0, ..., GetNumberOfFiles () - 1.
  vtkGetVector2Macro(FileIndexRange, vtkIdType);

  // Description:
  // Get/set the index of the file to read.
  vtkIdType GetFileIndex ()
  {
    return this->Get_FileIndex();
  }
  void SetFileIndex (vtkIdType i)
  {
    this->Set_FileIndex (i);
    this->FileIndexMTime = this->GetMTime();
  }

  // Description:
  // Return the MTime also considering the internal reader.
  virtual unsigned long GetMTime();

  // Description:
  // Name of the method used to set the file name of the internal
  // reader. By default, this is SetFileName.
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);

protected:
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  vtkSetStringMacro(_FileName);
  vtkGetStringMacro(_FileName);

  vtkSetMacro(_FileIndex, vtkIdType);
  vtkGetMacro(_FileIndex, vtkIdType);

  void ReaderSetFileName(const char* filename);
  int ReaderCanReadFile(const char *filename);

protected:
  // Reader that handles requests for the meta-reader
  vtkAlgorithm* Reader;
  // Modification time for the file name for the reader
  unsigned long ReaderFileNameMTime;
  // Modification time before the file name for the reader was changed.
  unsigned long ReaderBeforeFileNameMTime;
  // Method name used to set the file name for the Reader
  char* FileNameMethod;

  // File name for the meta-reader
  char *_FileName;
  // File name modification time
  unsigned long FileNameMTime;
  // The index of the file to read.
  vtkIdType _FileIndex;
  unsigned long FileIndexMTime;
  // Range for the file index
  vtkIdType FileIndexRange[2];
};

#endif
