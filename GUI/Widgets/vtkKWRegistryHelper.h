/*=========================================================================

  Module:    vtkKWRegistryHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWRegistryHelper - A registry class
// .SECTION Description
// This class abstracts the storing of data that can be restored
// when the program executes again. On Win32 platform it is 
// implemented using the registry and on unix as a file in
// the user's home directory.

#ifndef __vtkKWRegistryHelper_h
#define __vtkKWRegistryHelper_h

#include "vtkObject.h"

class VTK_EXPORT vtkKWRegistryHelper : public vtkObject
{
public:
  // Description:
  // Standard New and type methods
  static vtkKWRegistryHelper* New();
  vtkTypeRevisionMacro(vtkKWRegistryHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Read a value from the registry.
  int ReadValue(const char *subkey, 
                const char *key, char *value);

  // Description:
  // Delete a key from the registry.
  int DeleteKey(const char *subkey, const char *key);

  // Description:
  // Delete a value from a given key.
  int DeleteValue(const char *subkey, const char *key);

  // Description:
  // Set value in a given key.
  int SetValue(const char *subkey, const char *key, 
               const char *value);

  // Description:
  // Open the registry at toplevel/subkey.
  int Open(const char *toplevel, const char *subkey, 
           int readonly);
  
  // Description:
  // Close the registry. 
  int Close();

  // Description:
  // Read from local or global scope. On Windows this mean from local machine
  // or local user. On unix this will read from $HOME/.Projectrc or 
  // /etc/Project
  vtkSetClampMacro(GlobalScope, int, 0, 1);
  vtkBooleanMacro(GlobalScope, int);
  vtkGetMacro(GlobalScope, int);
  
  // Description:
  // Set or get the toplevel registry key.
  vtkSetStringMacro(TopLevel);
  vtkGetStringMacro(TopLevel);

  // Description:
  // Return true if registry opened
  vtkGetMacro(Opened, int);

  // Description:
  // Should the registry be locked?
  vtkGetMacro(Locked, int);

//BTX
  enum {
    READONLY,
    READWRITE
  };
//ETX

  protected:
  vtkKWRegistryHelper();
  virtual ~vtkKWRegistryHelper();

  // Description:
  // Should the registry be locked?
  vtkSetClampMacro(Locked, int, 0, 1);
  vtkBooleanMacro(Locked, int);

  
  // Description:
  // Read a value from the registry.
  virtual int ReadValueInternal(const char *key, char *value) = 0;
  
  // Description:
  // Delete a key from the registry.
  virtual int DeleteKeyInternal(const char *key) = 0;

  // Description:
  // Delete a value from a given key.
  virtual int DeleteValueInternal(const char *key) = 0;

  // Description:
  // Set value in a given key.
  virtual int SetValueInternal(const char *key, 
                               const char *value) = 0;

  // Description:
  // Open the registry at toplevel/subkey.
  virtual int OpenInternal(const char *toplevel, const char *subkey, 
                           int readonly) = 0;
  
  // Description:
  // Close the registry. 
  virtual int CloseInternal() = 0;

  // Description:
  // Return true if the character is space.
  int IsSpace(char c);

  // Description:
  // Strip trailing and ending spaces.
  char *Strip(char *str);

  int Opened;
  int Changed;
  int Empty;
   
private:
  char *TopLevel;  
  int Locked;
  int GlobalScope;

  vtkKWRegistryHelper(const vtkKWRegistryHelper&); // Not implemented
  void operator=(const vtkKWRegistryHelper&); // Not implemented
};

#endif


