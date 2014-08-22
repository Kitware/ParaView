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

// .NAME vtkMetaReader - Common functionality for meta-readers.
//
// .SECTION Description: A meta-reader redirect most pipeline requests
// to another Reader.  The Reader reads from a file selected from a
// list of files using a FileIndex.

#ifndef __vtkMetaReader_h
#define __vtkMetaReader_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkDataObjectAlgorithm.h"

#include <string> // for std::string

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
  void SetMetaFileName (const char* name)
  {
    Set_MetaFileName (name);
    this->MetaFileNameMTime = this->vtkDataObjectAlgorithm::GetMTime ();
  }
  char* GetMetaFileName ()
  {
    return Get_MetaFileName();
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
    this->FileIndexMTime = this->vtkDataObjectAlgorithm::GetMTime();
  }

  // Description:
  // Return the MTime when also considering the internal reader.
  virtual unsigned long GetMTime();

  // Description:
  // Name of the method used to set the file name of the internal
  // reader. By default, this is SetFileName.
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);

protected:
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  vtkSetStringMacro(_MetaFileName);
  vtkGetStringMacro(_MetaFileName);

  vtkSetMacro(_FileIndex, vtkIdType);
  vtkGetMacro(_FileIndex, vtkIdType);

  void ReaderSetFileName(const char* filename);
  int ReaderCanReadFile(const char *filename);

  // Description: Convert 'fileName' that is relative to the
  // 'metaFileName' to either a file path that is relative to the
  // current working directory (CWD) or to an absolute file path.
  // The choice is made based on if 'metaFileName' is relative or absolute.
  // Return the original if 'fileName' is already absolute.
  std::string FromRelativeToMetaFile(
    const char* metaFileName, const char* fileName);


protected:
  // Reader that handles requests for the meta-reader
  vtkAlgorithm* Reader;
  // Reader modification time after changing the Reader's FileName
  // Used to ignore changing the FileName for the reader when reporting MTime
  unsigned long FileNameMTime;
  // Reader modification time before changing the Reader's FileName
  // Used to ignore changing the FileName for the reader when reporting MTime
  unsigned long BeforeFileNameMTime;
  // Method name used to set the file name for the Reader
  char* FileNameMethod;
  // The index of the file to read.
  vtkIdType _FileIndex;
  unsigned long FileIndexMTime;
  // Range for the file index
  vtkIdType FileIndexRange[2];


  // File name for the meta-reader
  char *_MetaFileName;
  // File name modification time
  unsigned long MetaFileNameMTime;
  // Description:
  // Records the time when the meta-file was read.
  vtkTimeStamp MetaFileReadTime;
private:
  vtkMetaReader(const vtkMetaReader&); // Not implemented
  void operator=(const vtkMetaReader&); // Not implemented
};

#endif
