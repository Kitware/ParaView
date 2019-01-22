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
/**
 * @class   vtkKWAssignment
 * @brief   Manages allocation and freeing for a string list.
 *
 * A vtkStringList holds a list of strings.
 * We might be able to replace it in the future.
*/

#ifndef vtkStringList_h
#define vtkStringList_h

#include "vtkObject.h"
#include "vtkPVCoreModule.h" // needed for export macro

class VTKPVCORE_EXPORT vtkStringList : public vtkObject
{
public:
  static vtkStringList* New();
  vtkTypeMacro(vtkStringList, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Add a simple string.
   */
  void AddString(const char* str);
  void AddUniqueString(const char* str);
  //@}

  /**
   * Add a command and format it any way you like.
   */
  void AddFormattedString(const char* EventString, ...);

  /**
   * Initialize to empty.
   */
  void RemoveAllItems();

  /**
   * Random access.
   */
  void SetString(int idx, const char* str);

  /**
   * Get the length of the list.
   */
  int GetLength() { return this->NumberOfStrings; }

  /**
   * Get the index of a string.
   */
  int GetIndex(const char* str);

  /**
   * Get a command from its index.
   */
  const char* GetString(int idx);

  vtkGetMacro(NumberOfStrings, int);

protected:
  vtkStringList();
  ~vtkStringList() override;

  int NumberOfStrings;
  int StringArrayLength;
  char** Strings;
  void Reallocate(int num);
  void DeleteStrings();

  vtkStringList(const vtkStringList&) = delete;
  void operator=(const vtkStringList&) = delete;
};

#endif
