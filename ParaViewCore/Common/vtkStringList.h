/*=========================================================================

  Program:   ParaView
  Module:    vtkStringList.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWAssignment - Manages allocation and freeing for a string list.
// .SECTION Description
// A vtkStringList holds a list of strings.  
// We might be able to replace it in the future.

#ifndef __vtkStringList_h
#define __vtkStringList_h

#include "vtkObject.h"


class VTK_EXPORT vtkStringList : public vtkObject
{
public:
  static vtkStringList* New();
  vtkTypeMacro(vtkStringList,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Add a simple string.
  void AddString(const char* str);
  void AddUniqueString(const char* str);
  
  //BTX
  // Description:
  // Add a command and format it any way you like.
  void AddFormattedString(const char* EventString, ...);
  //ETX
  
  // Description:
  // Initialize to empty.
  void RemoveAllItems();

  // Description:
  // Random access.
  void SetString(int idx, const char *str);

  // Description:
  // Get the length of the list.
  int GetLength() { return this->NumberOfStrings;}

  // Description:
  // Get the index of a string.
  int GetIndex(const char* str);
  
  // Description:
  // Get a command from its index.
  const char *GetString(int idx);
  
  vtkGetMacro(NumberOfStrings, int);
  
protected:
  vtkStringList();
  ~vtkStringList();
  
  int NumberOfStrings;
  int StringArrayLength;
  char **Strings;
  void Reallocate(int num);
  void DeleteStrings();

  vtkStringList(const vtkStringList&); // Not implemented
  void operator=(const vtkStringList&); // Not implemented
};

#endif
