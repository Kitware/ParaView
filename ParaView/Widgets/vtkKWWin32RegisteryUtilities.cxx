/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWin32RegisteryUtilities.cxx
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

#include "vtkKWWin32RegisteryUtilities.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWWin32RegisteryUtilities );

vtkKWWin32RegisteryUtilities::vtkKWWin32RegisteryUtilities()
{
  this->HKey = 0;  
}

vtkKWWin32RegisteryUtilities::~vtkKWWin32RegisteryUtilities()
{
}

int vtkKWWin32RegisteryUtilities::OpenInternal(const char *toplevel,
					       const char *subkey, int readonly)
{
  int res = 0;
  ostrstream str;
  DWORD dwDummy;
  str << "Software\\Kitware\\" << toplevel << "\\" << subkey << ends;
  if ( readonly == vtkKWRegisteryUtilities::READONLY )
    {
    res = ( RegOpenKeyEx(HKEY_CURRENT_USER, str.str(), 
			 0, KEY_READ, &this->HKey) == ERROR_SUCCESS );
    }
  else
    {
    res = ( RegCreateKeyEx(HKEY_CURRENT_USER, str.str(),
			   0, "", REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, 
			   NULL, &this->HKey, &dwDummy) == ERROR_SUCCESS );    
    }
  str.rdbuf()->freeze(0);
  return res;
}

int vtkKWWin32RegisteryUtilities::CloseInternal()
{
  int res;
  res = ( RegCloseKey(this->HKey) == ERROR_SUCCESS );    
  return res;
}

int vtkKWWin32RegisteryUtilities::ReadValueInternal(char *value, 
						    const char *key)
{
  int res = 1;
  DWORD dwType, dwSize;  
  dwType = REG_SZ;
  dwSize = 1023;
  res = ( RegQueryValueEx(this->HKey, key, NULL, &dwType, 
			  (BYTE *)value, &dwSize) == ERROR_SUCCESS );
  return res;
}

int vtkKWWin32RegisteryUtilities::DeleteKeyInternal(const char *key)
{
  int res = 1;
  res = ( RegDeleteKey( this->HKey, key ) == ERROR_SUCCESS );
  return res;
}

int vtkKWWin32RegisteryUtilities::DeleteValueInternal(const char *key)
{
  int res = 1;
  res = ( RegDeleteValue( this->HKey, key ) == ERROR_SUCCESS );
  return res;
}

int vtkKWWin32RegisteryUtilities::SetValueInternal(const char *key, 
						   const char *value)
{
  int res = 1;
  res = ( RegSetValueEx(this->HKey, key, 0, REG_SZ, 
			(CONST BYTE *)(const char *)value, 
			strlen(value)+1) == ERROR_SUCCESS );
  return res;
}
