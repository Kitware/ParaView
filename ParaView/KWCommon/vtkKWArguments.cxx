/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWArguments.cxx
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
#include "vtkKWArguments.h"

#include "vtkObjectFactory.h"

#include <vector>
#include <map>
#include <string>

//----------------------------------------------------------------------------
//============================================================================
class vtkKWArgumentsInternal
{
public:
  vtkKWArgumentsInternal()
    {
    this->UnknownArgumentCallback = 0;
    this->ClientData = 0;
    this->LastArgument = 0;
    }

  typedef vtkstd::vector<std::string> VectorOfStrings;
  typedef vtkstd::map<std::string,
    vtkKWArguments::CallbackStructure> CallbacksMap;

  VectorOfStrings Argv;
  CallbacksMap Callbacks;

  vtkKWArguments::ErrorCallbackType UnknownArgumentCallback;
  void*             ClientData;

  VectorOfStrings::size_type LastArgument;
};
//============================================================================
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWArguments );
vtkCxxRevisionMacro(vtkKWArguments, "1.1");

//----------------------------------------------------------------------------
vtkKWArguments::vtkKWArguments()
{
  this->Internals = new vtkKWArgumentsInternal;
}

//----------------------------------------------------------------------------
vtkKWArguments::~vtkKWArguments()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkKWArguments::Initialize(int argc, char* argv[])
{
  int cc;

  this->Internals->Argv.clear();
  for ( cc = 1; cc < argc; cc ++ )
    {
    this->Internals->Argv.push_back(argv[cc]);
    }
  this->Internals->LastArgument = 0;
}

//----------------------------------------------------------------------------
int vtkKWArguments::Parse()
{
  vtkKWArgumentsInternal::VectorOfStrings::size_type cc;
  vtkKWArgumentsInternal::VectorOfStrings matches;
  for ( cc = 0; cc < this->Internals->Argv.size(); cc ++ )
    {
    matches.clear();
    std::string& arg = this->Internals->Argv[cc];
    vtkKWArgumentsInternal::CallbacksMap::iterator it;

    for ( it = this->Internals->Callbacks.begin();
      it != this->Internals->Callbacks.end();
      it ++ )
      {
      const std::string& parg = it->first;
      vtkKWArguments::CallbackStructure *cs = &it->second;
      if (cs->ArgumentType == vtkKWArguments::NO_ARGUMENT ||
        cs->ArgumentType == vtkKWArguments::SPACE_ARGUMENT) 
        {
        if ( arg == parg )
          {
          matches.push_back(parg);
          }
        }
      else if ( arg.find( parg ) == 0 )
        {
        matches.push_back(parg);
        }
      }
    if ( matches.size() > 0 )
      {
      vtkKWArgumentsInternal::VectorOfStrings::size_type kk;
      vtkKWArgumentsInternal::VectorOfStrings::size_type maxidx = 0;
      std::string::size_type maxlen = 0;
      for ( kk = 0; kk < matches.size(); kk ++ )
        {
        //cout << "Possible argument: " << matches[kk] << endl;
        if ( matches[kk].size() > maxlen )
          {
          maxlen = matches[kk].size();
          maxidx = kk;
          }
        }
      //cout << "This argument is: " << matches[maxidx] << endl;
      const char* value = 0;
      vtkKWArguments::CallbackStructure *cs 
        = &this->Internals->Callbacks[matches[maxidx]];
      const vtkstd::string& sarg = matches[maxidx];
      if ( cs->ArgumentType == NO_ARGUMENT )
        {
        }
      else if ( cs->ArgumentType == SPACE_ARGUMENT )
        {
        if ( cc == this->Internals->Argv.size()-1 )
          {
          return 0;
          }
        value = this->Internals->Argv[cc+1].c_str();
        cc ++;
        }
      else if ( cs->ArgumentType == EQUAL_ARGUMENT )
        {
        if ( arg.size() == sarg.size() )
          {
          return 0;
          }
        value = arg.c_str() + sarg.size()+1;
        }
      else if ( cs->ArgumentType == CONCAT_ARGUMENT )
        {
        value = arg.c_str() + sarg.size();
        }
      if ( !cs->Callback(sarg.c_str(), value, cs->CallData) )
        {
        return 0;
        }
      }
    else
      {
      if ( this->Internals->UnknownArgumentCallback )
        {
        if ( !this->Internals->UnknownArgumentCallback(arg.c_str(), 
            this->Internals->ClientData) )
          {
          return 0;
          }
        return 1;
        }
      else
        {
        cerr << "Got unknown argument: \"" << arg << "\"" << endl;
        return 0;
        }
      }
    }
  this->Internals->LastArgument = cc;
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWArguments::GetRemainingArguments(int* argc, char*** argv)
{
  vtkKWArgumentsInternal::VectorOfStrings::size_type size 
    = this->Internals->Argv.size() - this->Internals->LastArgument + 1;
  vtkKWArgumentsInternal::VectorOfStrings::size_type cc;

  char** args = new char*[ size ];
  args[0] = new char[ this->Internals->Argv[0].size() + 1 ];
  strcpy(args[0], this->Internals->Argv[0].c_str());
  int cnt = 1;
  for ( cc = this->Internals->LastArgument; 
    cc < this->Internals->Argv.size(); cc ++ )
    {
    args[cnt] = new char[ this->Internals->Argv[cc].size() + 1];
    strcpy(args[cnt], this->Internals->Argv[cc].c_str());
    cnt ++;
    }
  *argc = cnt;
  *argv = args;
}

//----------------------------------------------------------------------------
void vtkKWArguments::AddCallback(const char* argument, int type, 
  CallbackType callback, void* call_data)
{
  vtkKWArguments::CallbackStructure s;
  s.Argument     = argument;
  s.ArgumentType = type;
  s.Callback     = callback;
  s.CallData     = call_data;

  this->Internals->Callbacks[argument] = s;
}

//----------------------------------------------------------------------------
void vtkKWArguments::AddCallbacks(CallbackStructure* callbacks)
{
  if ( !callbacks )
    {
    return;
    }
  int cc;
  for ( cc = 0; callbacks[cc].Argument; cc ++ )
    {
    this->Internals->Callbacks[callbacks[cc].Argument] = callbacks[cc];
    }
}

//----------------------------------------------------------------------------
void vtkKWArguments::SetClientData(void* client_data)
{
  this->Internals->ClientData = client_data;
}

//----------------------------------------------------------------------------
void vtkKWArguments::SetUnknownArgumentCallback(
  vtkKWArguments::ErrorCallbackType callback)
{
  this->Internals->UnknownArgumentCallback = callback;
}

//----------------------------------------------------------------------------
void vtkKWArguments::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
