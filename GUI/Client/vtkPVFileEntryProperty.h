/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFileEntryProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVFileEntryProperty - a specific property for the file entry
// .SECTION Description

#ifndef __vtkPVFileEntryProperty_h
#define __vtkPVFileEntryProperty_h

#include "vtkPVStringWidgetProperty.h"

class vtkPVFileEntryPropertyList;

class VTK_EXPORT vtkPVFileEntryProperty : public vtkPVStringWidgetProperty
{
public:
  static vtkPVFileEntryProperty* New();
  vtkTypeRevisionMacro(vtkPVFileEntryProperty, vtkPVStringWidgetProperty);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  // Description:
  // Set the animation time for this proeprty.  This sets the modified flag on
  // the widget, and then calls Reset on it.
  virtual void SetAnimationTime(float time);
  virtual void SetAnimationTimeInBatch(ofstream *file, float val);
  
  // Description:
  // Set/get the time step
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Add a file to the list of files.
  void AddFile(const char* file);

  // Description:
  // Remove all files.
  void RemoveAllFiles();

  // Description:
  // Get the number of files.
  int GetNumberOfFiles();

  // Description:
  // Get the file with given index.
  const char* GetFile(int idx);
  
  // Description:
  // Get and set the directory name.
  vtkSetStringMacro(DirectoryName);
  vtkGetStringMacro(DirectoryName);
  
protected:
  vtkPVFileEntryProperty();
  ~vtkPVFileEntryProperty();
  
  int TimeStep;
  vtkPVFileEntryPropertyList *Files;
  char* DirectoryName;
  
private:
  vtkPVFileEntryProperty(const vtkPVFileEntryProperty&); // Not implemented
  void operator=(const vtkPVFileEntryProperty&); // Not implemented
};

#endif
