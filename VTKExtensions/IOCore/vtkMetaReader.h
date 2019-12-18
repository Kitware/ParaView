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

/**
 * @class   vtkMetaReader
 * @brief   Common functionality for meta-readers.
 *
 *
 * to another Reader.  The Reader reads from a file selected from a
 * list of files using a FileIndex.
*/

#ifndef vtkMetaReader_h
#define vtkMetaReader_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsIOCoreModule.h" //needed for exports

#include <string> // for std::string

class VTKPVVTKEXTENSIONSIOCORE_EXPORT vtkMetaReader : public vtkDataObjectAlgorithm
{
public:
  static vtkMetaReader* New();
  vtkTypeMacro(vtkMetaReader, vtkDataObjectAlgorithm);

  vtkMetaReader();
  ~vtkMetaReader() override;

  //@{
  /**
   * Set/get the internal reader.
   */
  vtkSetObjectMacro(Reader, vtkAlgorithm);
  vtkGetObjectMacro(Reader, vtkAlgorithm);
  //@}

  //@{
  /**
   * Get/set the filename for the meta-file.
   * Description:
   * Get/Set the meta-file name
   */
  void SetMetaFileName(const char* name)
  {
    Set_MetaFileName(name);
    this->MetaFileNameMTime = this->vtkDataObjectAlgorithm::GetMTime();
  }
  char* GetMetaFileName() { return Get_MetaFileName(); }
  //@}

  //@{
  /**
   * Returns the available range of file indexes. It is
   * 0, ..., GetNumberOfFiles () - 1.
   */
  vtkGetVector2Macro(FileIndexRange, vtkIdType);
  //@}

  //@{
  /**
   * Get/set the index of the file to read.
   */
  vtkIdType GetFileIndex() { return this->Get_FileIndex(); }
  void SetFileIndex(vtkIdType i)
  {
    this->Set_FileIndex(i);
    this->FileIndexMTime = this->vtkDataObjectAlgorithm::GetMTime();
  }
  //@}

  /**
   * Return the MTime when also considering the internal reader.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Name of the method used to set the file name of the internal
   * reader. By default, this is SetFileName.
   */
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);
  //@}

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  vtkSetStringMacro(_MetaFileName);
  vtkGetStringMacro(_MetaFileName);

  vtkSetMacro(_FileIndex, vtkIdType);
  vtkGetMacro(_FileIndex, vtkIdType);

  void ReaderSetFileName(const char* filename);
  int ReaderCanReadFile(const char* filename);

  /**
   * 'metaFileName' to either a file path that is relative to the
   * current working directory (CWD) or to an absolute file path.
   * The choice is made based on if 'metaFileName' is relative or absolute.
   * Return the original if 'fileName' is already absolute.
   */
  std::string FromRelativeToMetaFile(const char* metaFileName, const char* fileName);

protected:
  // Reader that handles requests for the meta-reader
  vtkAlgorithm* Reader;
  // Reader modification time after changing the Reader's FileName
  // Used to ignore changing the FileName for the reader when reporting MTime
  vtkMTimeType FileNameMTime;
  // Reader modification time before changing the Reader's FileName
  // Used to ignore changing the FileName for the reader when reporting MTime
  vtkMTimeType BeforeFileNameMTime;
  // Method name used to set the file name for the Reader
  char* FileNameMethod;
  // The index of the file to read.
  vtkIdType _FileIndex;
  vtkMTimeType FileIndexMTime;
  // Range for the file index
  vtkIdType FileIndexRange[2];

  // File name for the meta-reader
  char* _MetaFileName;
  // File name modification time
  vtkMTimeType MetaFileNameMTime;
  //@{
  /**
   * Records the time when the meta-file was read.
   */
  vtkTimeStamp MetaFileReadTime;

private:
  vtkMetaReader(const vtkMetaReader&) = delete;
  void operator=(const vtkMetaReader&) = delete;
};
//@}

#endif
