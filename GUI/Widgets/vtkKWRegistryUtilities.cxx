/*=========================================================================

  Module:    vtkKWRegistryUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWRegistryUtilities.h"

#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"

#ifdef _WIN32
#  include "vtkKWWin32RegistryUtilities.h"
#else // _WIN32
#  include "vtkKWUNIXRegistryUtilities.h"
#endif // _WIN32

#include <ctype.h>

vtkCxxRevisionMacro(vtkKWRegistryUtilities, "1.1");

//----------------------------------------------------------------------------
vtkKWRegistryUtilities *vtkKWRegistryUtilities::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWRegistryUtilities");
  if(ret)
    {
    return static_cast<vtkKWRegistryUtilities*>(ret);
    }
  vtkDebugLeaks::DestructClass("vtkKWRegistryUtilities");
#ifdef _WIN32
  return vtkKWWin32RegistryUtilities::New();
#else // _WIN32
  return vtkKWUNIXRegistryUtilities::New();
#endif // _WIN32
}

//----------------------------------------------------------------------------
vtkKWRegistryUtilities::vtkKWRegistryUtilities()
{
  this->TopLevel    = 0;
  this->Opened      = 0;
  this->Locked      = 0;
  this->Changed     = 0;
  this->Empty       = 1;
  this->GlobalScope = 0;
}

//----------------------------------------------------------------------------
vtkKWRegistryUtilities::~vtkKWRegistryUtilities()
{
  this->SetTopLevel(0);
  if ( this->Opened )
    {
    vtkErrorMacro("vtkKWRegistryUtilities::Close should be "
                  "called here. The registry is not closed.");
    }
}

//----------------------------------------------------------------------------
int vtkKWRegistryUtilities::Open(const char *toplevel,
                                  const char *subkey, int readonly)
{
  int res = 0;
  if ( this->GetLocked() )
    {
    return 0;
    }
  if ( this->Opened )
    {
    if ( !this->Close() )
      {
      return 0;
      }
    }
  if ( !toplevel )
    {
    vtkErrorMacro("vtkKWRegistryUtilities::Opened() Toplevel not defined");
    return 0;
    }

  if ( this->IsSpace(toplevel[0]) || 
       this->IsSpace(toplevel[strlen(toplevel)-1]) )
    {
    vtkErrorMacro("Toplevel has to start with letter or number and end"
                  " with one");
    return 0;
    }

  if ( readonly == vtkKWRegistryUtilities::READONLY )
    {
    res = this->OpenInternal(toplevel, subkey, readonly);
    }
  else
    {
    res = this->OpenInternal(toplevel, subkey, readonly);
    this->SetLocked(1);
    }
  
  if ( res )
    {
    this->Opened = 1;
    this->SetTopLevel( toplevel );
        }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRegistryUtilities::Close()
{
  int res = 0;
  if ( this->Opened )
    {
    res = this->CloseInternal();
    }

  if ( res )
    {
    this->Opened = 0;
    this->SetLocked(0);
    this->Changed = 0;
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRegistryUtilities::ReadValue(const char *subkey, 
                                       const char *key, 
                                       char *value)
{  
  *value = 0;
  int res = 1;
  int open = 0;  
  if ( ! value )
    {
    return 0;
    }
  if ( !this->Opened )
    {
    if ( !this->Open(this->GetTopLevel(), subkey, 
                     vtkKWRegistryUtilities::READONLY) )
      {
      return 0;
      }
    open = 1;
    }
  res = this->ReadValueInternal(key, value);
  
  if ( open )
    {
    if ( !this->Close() )
      {
      res = 0;
      }
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRegistryUtilities::DeleteKey(const char *subkey, 
                                       const char *key)
{
  int res = 1;
  int open = 0;
  if ( !this->Opened )
    {
    if ( !this->Open(this->GetTopLevel(), subkey, 
                     vtkKWRegistryUtilities::READWRITE) )
      {
      return 0;
      }
    open = 1;
    }

  res = this->DeleteKeyInternal(key);
  this->Changed = 1;

  if ( open )
    {
    if ( !this->Close() )
      {
      res = 0;
      }
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRegistryUtilities::DeleteValue(const char *subkey, const char *key)
{
  int res = 1;
  int open = 0;
  if ( !this->Opened )
    {
    if ( !this->Open(this->GetTopLevel(), subkey, 
                     vtkKWRegistryUtilities::READWRITE) )
      {
      return 0;
      }
    open = 1;
    }

  res = this->DeleteValueInternal(key);
  this->Changed = 1;

  if ( open )
    {
    if ( !this->Close() )
      {
      res = 0;
      }
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRegistryUtilities::SetValue(const char *subkey, const char *key, 
                                      const char *value)
{
  int res = 1;
  int open = 0;
  if ( !this->Opened )
    {
    if ( !this->Open(this->GetTopLevel(), subkey, 
                     vtkKWRegistryUtilities::READWRITE) )
      {
      return 0;
      }
    open = 1;
    }

  res = this->SetValueInternal( key, value );
  this->Changed = 1;
  
  if ( open )
    {
    if ( !this->Close() )
      {
      res = 0;
      }
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRegistryUtilities::IsSpace(char c)
{
  return isspace(c);
}

//----------------------------------------------------------------------------
char *vtkKWRegistryUtilities::Strip(char *str)
{
  int cc;
  int len;
  char *nstr;
  if ( !str )
    {
    return NULL;
    }  
  len = strlen(str);
  nstr = str;
  for( cc=0; cc<len; cc++ )
    {
    if ( !this->IsSpace( *nstr ) )
      {
      break;
      }
    nstr ++;
    }
  for( cc=(strlen(nstr)-1); cc>=0; cc-- )
    {
    if ( !this->IsSpace( nstr[cc] ) )
      {
      nstr[cc+1] = 0;
      break;
      }
    }
  return nstr;
}

//----------------------------------------------------------------------------
void vtkKWRegistryUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if( this->TopLevel )
    {
    os << indent << "TopLevel: " << this->TopLevel << "\n";
    }
  else
    {
    os << indent << "TopLevel: (none)\n";
    }
   
  os << indent << "Locked: " << (this->Locked ? "On" : "Off") << "\n";
  os << indent << "Opened: " << (this->Opened ? "On" : "Off") << "\n";
  os << indent << "GlobalScope: " << (this->GlobalScope ? "On" : "Off") << "\n";
}


