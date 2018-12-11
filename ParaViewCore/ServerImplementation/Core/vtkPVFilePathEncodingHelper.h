/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFilePathEncodingHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVFilePathEncodingHelper
 * @brief   server side object used to check if manipulate a directory
 *
 * Server side object to list, create and remove directory
 * the main reason for this helper to exist is to convert
 * the file path from utf8 to locale encoding
*/

#ifndef vtkPVFilePathEncodingHelper_h
#define vtkPVFilePathEncodingHelper_h

#include "vtkObject.h"
#include "vtkPVServerImplementationCoreModule.h" //needed for exports

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkPVFilePathEncodingHelper : public vtkObject
{
public:
  static vtkPVFilePathEncodingHelper* New();
  vtkTypeMacro(vtkPVFilePathEncodingHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the path that will be used by the helper
   */
  vtkSetStringMacro(Path);
  vtkGetStringMacro(Path);
  //@}

  //@{
  /**
   * Get/Set the secondary path that will potentially be used by the helper
   */
  vtkSetStringMacro(SecondaryPath);
  vtkGetStringMacro(SecondaryPath);

  //@}
  //@{
  /**
   * Get/Set the globalId of the vtkDirectory present of the server
   * that we will use to manipulate directories.
   */
  vtkGetMacro(ActiveGlobalId, int);
  vtkSetMacro(ActiveGlobalId, int);
  //@}

  /**
   * Create a directory named Path
   */
  bool MakeDirectory();

  /**
   * Delete a directory named Path
   */
  bool DeleteDirectory();

  /**
   * Open a directory named Path
   */
  bool OpenDirectory();

  /**
   * Rename a directory named Path to SecondaryPath
   */
  bool RenameDirectory();

  /**
   * Check is directory named Path exists.
   */
  bool IsDirectory();

  /**
   * Returns if this->Path is a readable file.
   */
  bool GetActiveFileIsReadable();

protected:
  vtkPVFilePathEncodingHelper();
  ~vtkPVFilePathEncodingHelper() override;

  bool CallObjectMethod(const char* method, bool ignoreError = false);

  char* Path;
  char* SecondaryPath;
  int ActiveGlobalId;

private:
  vtkPVFilePathEncodingHelper(const vtkPVFilePathEncodingHelper&) = delete;
  void operator=(const vtkPVFilePathEncodingHelper&) = delete;
};

#endif
