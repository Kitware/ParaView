/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWin32RegisteryUtilities.h
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

#ifndef __vtkKWWin32RegisteryUtilities_h
#define __vtkKWWin32RegisteryUtilities_h

#include "vtkKWObject.h"
#include "vtkKWRegisteryUtilities.h"

class VTK_EXPORT vtkKWWin32RegisteryUtilities : public vtkKWRegisteryUtilities
{
  public:
  static vtkKWWin32RegisteryUtilities* New();
  vtkTypeMacro(vtkKWWin32RegisteryUtilities, vtkKWRegisteryUtilities);

  // Description:
  // Read a value from the registry.
  virtual int ReadValueInternal(char *value, const char *key); 

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
};

#endif
