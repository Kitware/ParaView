/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWUNIXRegisteryUtilities.cxx
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

#include "vtkKWUNIXRegisteryUtilities.h"
#include "vtkObjectFactory.h"
#include <ctype.h>

#ifdef VTK_USE_ANSI_STDLIB
#define VTK_IOS_NOCREATE 
#else
#define VTK_IOS_NOCREATE | ios::nocreate
#endif

vtkStandardNewMacro( vtkKWUNIXRegisteryUtilities );

vtkKWUNIXRegisteryUtilities::vtkKWUNIXRegisteryUtilities()
{
  this->Entries = 0;
}

vtkKWUNIXRegisteryUtilities::~vtkKWUNIXRegisteryUtilities()
{
  if ( this->Entries )
    {
    this->Entries->Delete();
    }
}

#define MStrDup(x) \
   x ? strcpy( new char[ strlen(x) + 1 ], x ) : 0

class vtkKWUNIXRegisteryEntry
{
public:
  vtkKWUNIXRegisteryEntry( char *key, char *value )
    {
      this->Key = MStrDup(key);
      this->Value = MStrDup(value);
    }
  ~vtkKWUNIXRegisteryEntry()
    {
      if ( this->Key )
	{
	delete [] this->Key;
	}
      if ( this->Value )
	{
	delete [] this->Value;
	}
    }

  char *Key;
  char *Value;
}

#define BUFFER_SIZE 1024
int vtkKWUNIXRegisteryUtilities::OpenInternal(const char *toplevel,
					       const char *subkey, int readonly)
{  
  int res = 0;
  int cc;
  ostrstream str;
  if ( !getenv("HOME") )
    {
    cout << "Environment variable $HOME is not defined.\n"
      "Examine your shell environment variables and try again" << endl;
    return 0;
    }
  str << getenv("HOME") << "/." << toplevel << "rc" << ends;
  ifstream *ifs = new ifstream(str.str(), ios::in VTK_IOS_NOCREATE);
  if ( !ifs )
    {
    vtkErrorMacro("Cannot create file object");
    return 0;
    }
  if ( ifs->fail())
    {
    vtkErrorMacro("Problem opening file: " << str.str());
    return 0;
    }

  if ( !this->Entries )
    {
    this->Entries = vtkKWHashTable::New();
    }
  
  char buffer[BUFFER_SIZE];
  while( !ifs->fail() )
    {
    int found = 0;
    ifs->getline(buffer, BUFFER_SIZE);
    if ( ifs->fail() || ifs->eof() )
      {
      break;
      }
    char *line = this->Strip(buffer);
    if ( *line == '#'  || *line == 0 )
      {
      // Comment
      continue;
      }   
    //cout << "Checking line: [" << line << "]" << endl;
    for ( cc = 0; cc< strlen(line); cc++ )
      {
      if ( line[cc] == '=' )
	{
	char *key = new char[ cc+1 ];
	strncpy( key, line, cc );
	key[cc] = 0;
	char *value = line + cc + 1;
	char *nkey = this->Strip(key);
	char *nvalue = this->Strip(value);
	cout << "Found : \"" << nkey << "\" => \"" << nvalue << "\"" << endl;
	this->Entries->Insert( key, 
	delete [] key;
	found = 1;	
	}
      }
    if ( !found )
      {
      cout << "What is this line: \"" << line << "\"" << endl;
      }
    
    }
  ifs->close();
  delete ifs;
  return res;
}

int vtkKWUNIXRegisteryUtilities::CloseInternal()
{
  int res;
  
  return res;
}

int vtkKWUNIXRegisteryUtilities::ReadValueInternal(char *value, 
						    const char *key)

{
  int res = 0;
  return res;
}

int vtkKWUNIXRegisteryUtilities::DeleteKeyInternal(const char *key)
{
  int res = 0;
  return res;
}

int vtkKWUNIXRegisteryUtilities::DeleteValueInternal(const char *key)
{
  int res = 0;
  return res;
}

int vtkKWUNIXRegisteryUtilities::SetValueInternal(const char *key, 
						   const char *value)
{
  int res = 0;
  return res;
}
