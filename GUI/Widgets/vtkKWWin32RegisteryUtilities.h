/*=========================================================================

  Module:    vtkKWWin32RegisteryUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWRegisteryUtilities - A Win32 implementation of the registery
// .SECTION Description
// This class abstracts the storing of data that can be restored
// when the program executes again. It is designed specifically for 
// Win32 platform.

#ifndef __vtkKWWin32RegisteryUtilities_h
#define __vtkKWWin32RegisteryUtilities_h

#include "vtkKWRegisteryUtilities.h"
#include "vtkWindows.h" // needed for HKEY

class VTK_EXPORT vtkKWWin32RegisteryUtilities : public vtkKWRegisteryUtilities
{
public:
  static vtkKWWin32RegisteryUtilities* New();
  vtkTypeRevisionMacro(vtkKWWin32RegisteryUtilities, vtkKWRegisteryUtilities);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Read a value from the registry.
  virtual int ReadValueInternal(const char *key, char *value); 

  // Description:
  // Delete a key from the registry.
  virtual int DeleteKeyInternal(const char *key);

  // Description:
  // Delete a value from a given key.
  virtual int DeleteValueInternal(const char *key);

  // Description:
  // Set value in a given key.
  virtual int SetValueInternal(const char *key, const char *value);

  // Description:
  // Open the registry at toplevel/subkey.
  virtual int OpenInternal(const char *toplevel, const char *subkey, int readonly);
  
  // Description:
  // Close the registry.
  virtual int CloseInternal();

protected:
  vtkKWWin32RegisteryUtilities();
  virtual ~vtkKWWin32RegisteryUtilities();

private:
  HKEY HKey;
  vtkKWWin32RegisteryUtilities(const vtkKWWin32RegisteryUtilities&); // Not implemented
  void operator=(const vtkKWWin32RegisteryUtilities&); // Not implemented
};

#endif



