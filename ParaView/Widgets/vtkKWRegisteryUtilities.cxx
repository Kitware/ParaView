/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRegisteryUtilities.cxx
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

#include "vtkKWRegisteryUtilities.h"
#ifdef _WIN32
#  include "vtkKWWin32RegisteryUtilities.h"
#else // _WIN32
#  include "vtkKWUNIXRegisteryUtilities.h"
#endif // _WIN32

#include <ctype.h>
#include "vtkObjectFactory.h"

vtkKWRegisteryUtilities *vtkKWRegisteryUtilities::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWRegisteryUtilities");
  if(ret)
    {
      return static_cast<vtkKWRegisteryUtilities*>(ret);
    }
#ifdef _WIN32
  return vtkKWWin32RegisteryUtilities::New();
#else // _WIN32
  return vtkKWUNIXRegisteryUtilities::New();
#endif // _WIN32
}

vtkKWRegisteryUtilities::vtkKWRegisteryUtilities()
{
  this->TopLevel = 0;
  this->Opened   = 0;
  this->Locked   = 0;
  this->Changed  = 0;
  this->Empty    = 1;
}

vtkKWRegisteryUtilities::~vtkKWRegisteryUtilities()
{
  this->SetTopLevel(0);
  if ( this->Opened )
    {
    vtkErrorMacro("vtkKWRegisteryUtilities::Close should be "
		  "called here. The registry is not closed.");
    }
}

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
       this->IsSpace(toplevel[strlen(toplevel)-1]) )
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

int vtkKWRegisteryUtilities::ReadValue(const char *subkey, 
				       const char *key, 
				       char *value)
{
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
  *value = 0;
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

int vtkKWRegisteryUtilities::IsSpace(char c)
{
  return isspace(c);
}

char *vtkKWRegisteryUtilities::Strip(char *str)
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
