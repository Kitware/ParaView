/*=========================================================================

  Module:    vtkKWUNIXRegistryUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWRegistryUtilities - A registry class
// .SECTION Description
// This class abstracts the storing of data that can be restored
// when the program executes again. It is implemented as a file in
// the user's home directory.

#ifndef __vtkKWUNIXRegistryUtilities_h
#define __vtkKWUNIXRegistryUtilities_h

#include "vtkKWRegistryUtilities.h"

class vtkKWUNIXRegistryUtilitiesInternals;

class VTK_EXPORT vtkKWUNIXRegistryUtilities : public vtkKWRegistryUtilities
{
public:
  static vtkKWUNIXRegistryUtilities* New();
  vtkTypeRevisionMacro(vtkKWUNIXRegistryUtilities, vtkKWRegistryUtilities);
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
  virtual int OpenInternal(const char *toplevel, const char *subkey, 
                           int readonly);
  
  // Description:
  // Close the registry.
  virtual int CloseInternal();

protected:
  vtkKWUNIXRegistryUtilities();
  virtual ~vtkKWUNIXRegistryUtilities();
  vtkSetStringMacro(SubKey);

  char *CreateKey(const char *key);

private:
  char *SubKey;

  vtkKWUNIXRegistryUtilitiesInternals* Internals;

  vtkKWUNIXRegistryUtilities(const vtkKWUNIXRegistryUtilities&); // Not implemented
  void operator=(const vtkKWUNIXRegistryUtilities&); // Not implemented
};

#endif



