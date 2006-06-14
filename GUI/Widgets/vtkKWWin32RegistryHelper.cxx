/*=========================================================================

  Module:    vtkKWWin32RegistryHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWWin32RegistryHelper.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkKWWin32RegistryHelper, "1.3");
vtkStandardNewMacro( vtkKWWin32RegistryHelper );

#define BUFFER_SIZE 8192

//----------------------------------------------------------------------------
vtkKWWin32RegistryHelper::vtkKWWin32RegistryHelper()
{
  this->HKey = 0;  
  this->Organization = 0;
  this->SetOrganization("Kitware");
}

//----------------------------------------------------------------------------
vtkKWWin32RegistryHelper::~vtkKWWin32RegistryHelper()
{
  this->SetOrganization(0);
}

//----------------------------------------------------------------------------
int vtkKWWin32RegistryHelper::OpenInternal(const char *toplevel,
                                           const char *subkey, 
                                           int readonly)
{
  HKEY scope = HKEY_CURRENT_USER;
  if ( this->GetGlobalScope() )
    {
    scope = HKEY_LOCAL_MACHINE;
    }
  int res = 0;
  ostrstream str;
  DWORD dwDummy;
  str << "Software\\";
  if (this->Organization)
    {
    str << this->Organization << "\\";
    }
  str << toplevel << "\\" << subkey << ends;
  if ( readonly == vtkKWRegistryHelper::ReadOnly )
    {
    res = ( RegOpenKeyEx(scope, str.str(), 
                         0, KEY_READ, &this->HKey) == ERROR_SUCCESS );
    }
  else
    {
    res = ( RegCreateKeyEx(scope, str.str(),
                           0, "", REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, 
                           NULL, &this->HKey, &dwDummy) == ERROR_SUCCESS );    
    }
  str.rdbuf()->freeze(0);
  return res;
}

//----------------------------------------------------------------------------
int vtkKWWin32RegistryHelper::CloseInternal()
{
  int res;
  res = ( RegCloseKey(this->HKey) == ERROR_SUCCESS );    
  return res;
}

//----------------------------------------------------------------------------
int vtkKWWin32RegistryHelper::ReadValueInternal(const char *key,
                                                char *value)
{
  int res = 1;
  DWORD dwType, dwSize;  
  dwType = REG_SZ;
  dwSize = BUFFER_SIZE;
  res = ( RegQueryValueEx(this->HKey, key, NULL, &dwType, 
                          (BYTE *)value, &dwSize) == ERROR_SUCCESS );
  return res;
}

//----------------------------------------------------------------------------
int vtkKWWin32RegistryHelper::DeleteKeyInternal(const char *key)
{
  int res = 1;
  res = ( RegDeleteKey( this->HKey, key ) == ERROR_SUCCESS );
  return res;
}

//----------------------------------------------------------------------------
int vtkKWWin32RegistryHelper::DeleteValueInternal(const char *key)
{
  int res = 1;
  res = ( RegDeleteValue( this->HKey, key ) == ERROR_SUCCESS );
  return res;
}

//----------------------------------------------------------------------------
int vtkKWWin32RegistryHelper::SetValueInternal(const char *key, 
                                               const char *value)
{
  int res = 1;
  DWORD len = (DWORD)(value ? strlen(value) : 0);
  res = ( RegSetValueEx(this->HKey, key, 0, REG_SZ, 
                        (CONST BYTE *)(const char *)value, 
                        len+1) == ERROR_SUCCESS );
  return res;
}

//----------------------------------------------------------------------------
void vtkKWWin32RegistryHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}



