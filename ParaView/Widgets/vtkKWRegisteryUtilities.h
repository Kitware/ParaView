/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRegisteryUtilities.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef __vtkKWRegisteryUtilities_h
#define __vtkKWRegisteryUtilities_h

#include "vtkKWObject.h"

class VTK_EXPORT vtkKWRegisteryUtilities : public vtkKWObject
{
  public:
  static vtkKWRegisteryUtilities* New();
  vtkTypeMacro(vtkKWRegisteryUtilities, vtkKWObject);

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
  // Set or get the toplevel registry key.
  vtkSetStringMacro(TopLevel);
  vtkGetStringMacro(TopLevel);

  // Description:
  // Return true if registry opened
  vtkGetMacro(Opened, int);

  // Description:
  // Should the registry be locked?
  vtkGetMacro(Locked, int);

  enum {
    READONLY,
    READWRITE
  };

  protected:
  vtkKWRegisteryUtilities();
  virtual ~vtkKWRegisteryUtilities();

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
};

#endif
