/*=========================================================================

  Module:    vtkKWRegisteryUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWRegisteryUtilities.h"

#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"

#ifdef _WIN32
#  include "vtkKWWin32RegisteryUtilities.h"
#else // _WIN32
#  include "vtkKWUNIXRegisteryUtilities.h"
#endif // _WIN32

#include <ctype.h>

vtkCxxRevisionMacro(vtkKWRegisteryUtilities, "1.10");

//----------------------------------------------------------------------------
vtkKWRegisteryUtilities *vtkKWRegisteryUtilities::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWRegisteryUtilities");
  if(ret)
    {
    return static_cast<vtkKWRegisteryUtilities*>(ret);
    }
  vtkDebugLeaks::DestructClass("vtkKWRegisteryUtilities");
#ifdef _WIN32
  return vtkKWWin32RegisteryUtilities::New();
#else // _WIN32
  return vtkKWUNIXRegisteryUtilities::New();
#endif // _WIN32
}

//----------------------------------------------------------------------------
vtkKWRegisteryUtilities::vtkKWRegisteryUtilities()
{
  this->TopLevel    = 0;
  this->Opened      = 0;
  this->Locked      = 0;
  this->Changed     = 0;
  this->Empty       = 1;
  this->GlobalScope = 0;
}

//----------------------------------------------------------------------------
vtkKWRegisteryUtilities::~vtkKWRegisteryUtilities()
{
  this->SetTopLevel(0);
  if ( this->Opened )
    {
    vtkErrorMacro("vtkKWRegisteryUtilities::Close should be "
                  "called here. The registry is not closed.");
    }
}

//----------------------------------------------------------------------------
int vtkKWRegisteryUtilities::Open(const char *toplevel,
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
    vtkErrorMacro("vtkKWRegisteryUtilities::Opened() Toplevel not defined");
    return 0;
    }

  if ( this->IsSpace(toplevel[0]) || 
       this->IsSpace(toplevel[vtkString::Length(toplevel)-1]) )
    {
    vtkErrorMacro("Toplevel has to start with letter or number and end"
                  " with one");
    return 0;
    }

  if ( readonly == vtkKWRegisteryUtilities::READONLY )
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
int vtkKWRegisteryUtilities::Close()
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
int vtkKWRegisteryUtilities::ReadValue(const char *subkey, 
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
                     vtkKWRegisteryUtilities::READONLY) )
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
int vtkKWRegisteryUtilities::DeleteKey(const char *subkey, 
                                       const char *key)
{
  int res = 1;
  int open = 0;
  if ( !this->Opened )
    {
    if ( !this->Open(this->GetTopLevel(), subkey, 
                     vtkKWRegisteryUtilities::READWRITE) )
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
int vtkKWRegisteryUtilities::DeleteValue(const char *subkey, const char *key)
{
  int res = 1;
  int open = 0;
  if ( !this->Opened )
    {
    if ( !this->Open(this->GetTopLevel(), subkey, 
                     vtkKWRegisteryUtilities::READWRITE) )
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
int vtkKWRegisteryUtilities::SetValue(const char *subkey, const char *key, 
                                      const char *value)
{
  int res = 1;
  int open = 0;
  if ( !this->Opened )
    {
    if ( !this->Open(this->GetTopLevel(), subkey, 
                     vtkKWRegisteryUtilities::READWRITE) )
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
int vtkKWRegisteryUtilities::IsSpace(char c)
{
  return isspace(c);
}

//----------------------------------------------------------------------------
char *vtkKWRegisteryUtilities::Strip(char *str)
{
  int cc;
  int len;
  char *nstr;
  if ( !str )
    {
    return NULL;
    }  
  len = vtkString::Length(str);
  nstr = str;
  for( cc=0; cc<len; cc++ )
    {
    if ( !this->IsSpace( *nstr ) )
      {
      break;
      }
    nstr ++;
    }
  for( cc=(vtkString::Length(nstr)-1); cc>=0; cc-- )
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
void vtkKWRegisteryUtilities::PrintSelf(ostream& os, vtkIndent indent)
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


