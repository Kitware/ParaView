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
#include "vtkKWHashTable.h"
#include "vtkKWHashTableIterator.h"
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
  this->SubKey  = 0;
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
  vtkKWUNIXRegisteryEntry( const char *key, const char *value )
    {
      this->Key = MStrDup(key);
      this->Value = MStrDup(value);
      this->Count ++;
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
      this->Count--;
    }
  void SetValue(const char* value)
    {
      if ( this->Value )
	{
	delete [] this->Value;
	}
      this->Value = MStrDup(value);
    }  

  char *Key;
  char *Value;
  static int Count;
};
int vtkKWUNIXRegisteryEntry::Count = 0;

#define BUFFER_SIZE 1024
int vtkKWUNIXRegisteryUtilities::OpenInternal(const char *toplevel,
					       const char *subkey, int readonly)
{  
  int res = 0;
  int cc;
  ostrstream str;
  if ( !getenv("HOME") )
    {
    return 0;
    }
  str << getenv("HOME") << "/." << toplevel << "rc" << ends;
  if ( readonly == vtkKWRegisteryUtilities::READWRITE )
    {
    ofstream ofs( str.str(), ios::out|ios::app );
    if ( ofs.fail() )
      {
      return 0;
      }
    ofs.close();
    }

  ifstream *ifs = new ifstream(str.str(), ios::in VTK_IOS_NOCREATE);
  if ( !ifs )
    {
    return 0;
    }
  if ( ifs->fail())
    {
    delete ifs;
    return 0;
    }
  if ( !this->Entries )
    {
    this->Entries = vtkKWHashTable::New();
    }

  res = 1;
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
    for ( cc = 0; cc< static_cast<int>(strlen(line)); cc++ )
      {
      if ( line[cc] == '=' )
	{
	char *key = new char[ cc+1 ];
	strncpy( key, line, cc );
	key[cc] = 0;
	char *value = line + cc + 1;
	char *nkey = this->Strip(key);
	char *nvalue = this->Strip(value);
	this->Entries->Insert( nkey, 
			       new vtkKWUNIXRegisteryEntry(nkey, nvalue) );
	this->Empty = 0;
	delete [] key;
	found = 1;	
	}
      }
    if ( !found )
      {
      res = 0;
      }
    
    }
  ifs->close();
  this->SetSubKey( subkey );
  delete ifs;
  return res;
}

int vtkKWUNIXRegisteryUtilities::CloseInternal()
{
  int res = 0;
  if ( !this->Changed )
    {
    vtkKWHashTableIterator *it = this->Entries->Iterator();
    if ( it )
      {
      while ( it )
	{
	if ( it->Valid() )
	  {      
	  //unsigned long key = it->GetKey();
	  void *value = it->GetData();
	  if ( value )
	    {
	    vtkKWUNIXRegisteryEntry *ku 
	      = static_cast<vtkKWUNIXRegisteryEntry *>(value);
	    delete ku;	    
	    }	
	  }
	if ( !it->Next() )
	  {
	  break;
	  }
	}
      it->Delete();
      }
    this->Entries->Delete();
    this->Entries = 0;
    this->Empty = 1;
    this->SetSubKey(0);
    return 1;
    }

  ostrstream str;
  if ( !getenv("HOME") )
    {
    return 0;
    }
  str << getenv("HOME") << "/." << this->GetTopLevel() << "rc" << ends;
  ofstream *ofs = new ofstream(str.str(), ios::out);
  if ( !ofs )
    {
    return 0;
    }
  if ( ofs->fail())
    {
    delete ofs;
    return 0;
    }
  *ofs << "# This file is automatically generated by the application" << endl
       << "# If you change any lines or add new lines, note that all" << endl
       << "# coments and empty lines will be deleted. Every line has" << endl
       << "# to be in format: " << endl
       << "# key = value" << endl
       << "#" << endl;

  if ( this->Entries )
    {
    vtkKWHashTableIterator *it = this->Entries->Iterator();
    if ( !it )
      {
      res = 1;
      }
    else 
      {
      while ( it )
	{
	if ( it->Valid() )
	  {      
	  // unsigned long key = it->GetKey();
	  void *value = it->GetData();
	  if ( value )
	    {
	    vtkKWUNIXRegisteryEntry *ku 
	      = static_cast<vtkKWUNIXRegisteryEntry *>(value);
	    *ofs << ku->Key << " = " << ku->Value << endl;
	    delete ku;
	    }	
	  }
	if ( !it->Next() )
	  {
	  break;
	  }
	}
      it->Delete();
      }
    }
  this->Entries->Delete();
  this->Entries=0;
  ofs->close();
  res = 1;
  this->SetSubKey(0);
  this->Empty = 1;
  return res;
}

int vtkKWUNIXRegisteryUtilities::ReadValueInternal(const char *skey,
						   char *value)

{
  int res = 0;
  char *key = this->CreateKey( skey );
  if ( !key )
    {
    return 0;
    }
  if ( this->Entries->Lookup( key ) )
    {
    vtkKWUNIXRegisteryEntry *en = static_cast<vtkKWUNIXRegisteryEntry*>(
      this->Entries->Lookup( key ) );
    strcpy(value, en->Value);
    res = 1;
    }
  delete [] key;
  return res;
}

int vtkKWUNIXRegisteryUtilities::DeleteKeyInternal(const char *key)
{
  int res = 0;
  return res;
}

int vtkKWUNIXRegisteryUtilities::DeleteValueInternal(const char *skey)
{
  int res = 0;
  char *key = this->CreateKey( skey );
  if ( !key )
    {
    return 0;
    }
  if ( this->Entries->Lookup( key ) )
    {
    vtkKWUNIXRegisteryEntry *en = static_cast<vtkKWUNIXRegisteryEntry*>(
      this->Entries->Lookup( key ) );
    if ( strcmp(en->Key, key) == 0 )
      {
      if ( this->Entries->Remove( key ) )
	{
	delete en;
	res = 1;
	}
      }
    }
  delete [] key;
  return res;
}

int vtkKWUNIXRegisteryUtilities::SetValueInternal(const char *skey, 
						   const char *value)
{
  int res = 0;
  vtkKWUNIXRegisteryEntry *en = 0;
  char *key = this->CreateKey( skey );
  if ( !key )
    {
    return 0;
    }
  if ( this->Entries->Lookup( key ) )
    {
    en = static_cast<vtkKWUNIXRegisteryEntry*>( 
      this->Entries->Lookup( key ) );
    en->SetValue(value);
    res = 1;
    this->Empty = 0;
    }
  if ( !en )
    {
    en = new vtkKWUNIXRegisteryEntry(key, value);
    if ( this->Entries->Insert( key, en ) )
      {
      res = 1;
      this->Empty = 0;
      }
    else
      {
      delete en;
      }
    }
  delete [] key;
  return res;
}

char *vtkKWUNIXRegisteryUtilities::CreateKey( const char *key )
{
  char *newkey;
  if ( !this->SubKey || !key )
    {
    return 0;
    }
  int len = strlen(this->SubKey) + strlen(key) + 1;
  newkey = new char[ len+1 ] ;
  sprintf(newkey, "%s\\%s", this->SubKey, key);
  return newkey;
}
