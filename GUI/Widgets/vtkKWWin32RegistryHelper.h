/*=========================================================================

  Module:    vtkKWWin32RegistryHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWin32RegistryHelper - A Win32 implementation of the registry
// .SECTION Description
// This class abstracts the storing of data that can be restored
// when the program executes again. It is designed specifically for 
// Win32 platform.

#ifndef __vtkKWWin32RegistryHelper_h
#define __vtkKWWin32RegistryHelper_h

#include "vtkKWRegistryHelper.h"
#include "vtkWindows.h" // needed for HKEY

class KWWidgets_EXPORT vtkKWWin32RegistryHelper : public vtkKWRegistryHelper
{
public:
  static vtkKWWin32RegistryHelper* New();
  vtkTypeRevisionMacro(vtkKWWin32RegistryHelper, vtkKWRegistryHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set or get the organization registry key.
  // This is valid for the Win32 registry only. Keys are placed under
  // HKEY_CURRENT_USER\Software\Organization\TopLevel\Subkey\Key,
  // where TopLevel can be set a the superclass level (vtkKWRegistryHelper)
  // and Subkey and Key are specified through the SetValue API. 
  vtkSetStringMacro(Organization);
  vtkGetStringMacro(Organization);

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
  vtkKWWin32RegistryHelper();
  virtual ~vtkKWWin32RegistryHelper();

private:
  char *Organization;
  HKEY HKey;
  vtkKWWin32RegistryHelper(const vtkKWWin32RegistryHelper&); // Not implemented
  void operator=(const vtkKWWin32RegistryHelper&); // Not implemented
};

#endif



