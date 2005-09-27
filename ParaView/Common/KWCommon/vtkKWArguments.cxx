/*=========================================================================

  Module:    vtkKWArguments.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWArguments.h"

#include "vtkObjectFactory.h"

#ifdef _MSC_VER
#pragma warning (push, 1)
#pragma warning (disable: 4702)
#pragma warning (disable: 4503)
#endif

#include <vector>
#include <map>
#include <string>
#include <set>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

//----------------------------------------------------------------------------
//============================================================================
class vtkKWArgumentsString : public vtkstd::string 
{
public:
  typedef vtkstd::string StdString;
  vtkKWArgumentsString(): StdString() {}
  vtkKWArgumentsString(const value_type* s): StdString(s) {}
  vtkKWArgumentsString(const value_type* s, size_type n): StdString(s, n) {}
  vtkKWArgumentsString(const StdString& s, size_type pos=0, size_type n=npos):
    StdString(s, pos, n) {}
};

class vtkKWArgumentsVectorOfStrings : 
  public vtkstd::vector<vtkKWArgumentsString> {};
class vtkKWArgumentsSetOfStrings :
  public vtkstd::set<vtkKWArgumentsString> {};
class vtkKWArgumentsMapOfStrucs : 
  public vtkstd::map<vtkKWArgumentsString,
    vtkKWArguments::CallbackStructure> {};
class vtkKWArgumentsMapOfStrings:
  public vtkstd::map<vtkKWArgumentsString,
    vtkKWArgumentsString> {};

class vtkKWArgumentsInternal
{
public:
  vtkKWArgumentsInternal()
    {
    this->UnknownArgumentCallback = 0;
    this->ClientData = 0;
    this->LastArgument = 0;
    }

  typedef vtkKWArgumentsVectorOfStrings VectorOfStrings;
  typedef vtkKWArgumentsMapOfStrucs CallbacksMap;
  typedef vtkKWArgumentsString String;
  typedef vtkKWArgumentsSetOfStrings SetOfStrings;
  typedef vtkKWArgumentsMapOfStrings MapOfStrings;

  VectorOfStrings Argv;
  CallbacksMap Callbacks;
  MapOfStrings ArgumentValues;

  vtkKWArguments::ErrorCallbackType UnknownArgumentCallback;
  void*             ClientData;

  VectorOfStrings::size_type LastArgument;
};
//============================================================================
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWArguments );
vtkCxxRevisionMacro(vtkKWArguments, "1.20");

//----------------------------------------------------------------------------
vtkKWArguments::vtkKWArguments()
{
  this->Internals = new vtkKWArguments::Internal;
  this->Help = 0;
  this->LineLength = 80;
}

//----------------------------------------------------------------------------
vtkKWArguments::~vtkKWArguments()
{
  delete this->Internals;
  this->SetHelp(0);
}

//----------------------------------------------------------------------------
void vtkKWArguments::Initialize(int argc, char* argv[])
{
  int cc;

  this->Initialize();
  for ( cc = 1; cc < argc; cc ++ )
    {
    this->AddArgument(argv[cc]);
    }
}

//----------------------------------------------------------------------------
void vtkKWArguments::Initialize()
{
  this->Internals->Argv.clear();
  this->Internals->LastArgument = 0;
}

//----------------------------------------------------------------------------
void vtkKWArguments::AddArgument(const char* arg)
{
  this->Internals->Argv.push_back(arg);
}

//----------------------------------------------------------------------------
int vtkKWArguments::Parse()
{
  vtkKWArguments::Internal::VectorOfStrings::size_type cc;
  vtkKWArguments::Internal::VectorOfStrings matches;
  for ( cc = 0; cc < this->Internals->Argv.size(); cc ++ )
    {
    matches.clear();
    vtkKWArguments::Internal::String& arg = this->Internals->Argv[cc];
    vtkKWArguments::Internal::CallbacksMap::iterator it;

    // Does the argument match to any we know about?
    for ( it = this->Internals->Callbacks.begin();
      it != this->Internals->Callbacks.end();
      it ++ )
      {
      const vtkKWArguments::Internal::String& parg = it->first;
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
      // Ok, we found one or more arguments that match what user specified.
      // Let's find the longest one.
      vtkKWArguments::Internal::VectorOfStrings::size_type kk;
      vtkKWArguments::Internal::VectorOfStrings::size_type maxidx = 0;
      vtkKWArguments::Internal::String::size_type maxlen = 0;
      for ( kk = 0; kk < matches.size(); kk ++ )
        {
        if ( matches[kk].size() > maxlen )
          {
          maxlen = matches[kk].size();
          maxidx = kk;
          }
        }
      // So, the longest one is probably the right one. Now see if it has any
      // additional value
      const char* value = 0;
      vtkKWArguments::CallbackStructure *cs 
        = &this->Internals->Callbacks[matches[maxidx]];
      const vtkKWArguments::Internal::String& sarg = matches[maxidx];
      if ( cs->ArgumentType == NO_ARGUMENT )
        {
        // No value
        }
      else if ( cs->ArgumentType == SPACE_ARGUMENT )
        {
        if ( cc == this->Internals->Argv.size()-1 )
          {
          return 0;
          }
        // Value is the next argument
        value = this->Internals->Argv[cc+1].c_str();
        cc ++;
        }
      else if ( cs->ArgumentType == EQUAL_ARGUMENT )
        {
        if ( arg.size() == sarg.size() || *(arg.c_str() + sarg.size()) != '=' )
          {
          return 0;
          }
        // Value is everythng followed the '=' sign
        value = arg.c_str() + sarg.size()+1;
        }
      else if ( cs->ArgumentType == CONCAT_ARGUMENT )
        {
        // Value is whatever follows the argument
        value = arg.c_str() + sarg.size();
        }

      // Store the value in the map
      this->Internals->ArgumentValues[sarg.c_str()] = (value?value:"1");
      // Call the callback
      if ( cs->Callback )
        {
        if ( !cs->Callback(sarg.c_str(), value, cs->CallData) )
          {
          return 0;
          }
        }
      if ( cs->Variable )
        {
        if ( !value )
          {
          value = "1";
          }
        if ( cs->VariableType == vtkKWArguments::INT_TYPE )
          {
          int* variable = static_cast<int*>(cs->Variable);
          char* res = 0;
          *variable = strtol(value, &res, 10);
          //if ( res && *res )
          //  {
          //  Can handle non-int
          //  }
          }
        else if ( cs->VariableType == vtkKWArguments::DOUBLE_TYPE )
          {
          double* variable = static_cast<double*>(cs->Variable);
          char* res = 0;
          *variable = strtod(value, &res);
          //if ( res && *res )
          //  {
          //  Can handle non-int
          //  }
          }
        else if ( cs->VariableType == vtkKWArguments::STRING_TYPE )
          {
          char** variable = static_cast<char**>(cs->Variable);
          if ( *variable )
            {
            delete [] *variable;
            *variable = 0;
            }
          *variable = new char[ strlen(value) + 1 ];
          strcpy(*variable, value);
          }
        }
      }
    else
      {
      // Handle unknown arguments
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
        cerr << "Got unknown argument: \"" << arg.c_str() << "\"" << endl;
        return 0;
        }
      }
    }
  // We are done parsing, so remember what was the last argument
  this->Internals->LastArgument = cc;
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWArguments::GetRemainingArguments(int* argc, char*** argv)
{
  vtkKWArguments::Internal::VectorOfStrings::size_type size 
    = this->Internals->Argv.size() - this->Internals->LastArgument + 1;
  vtkKWArguments::Internal::VectorOfStrings::size_type cc;

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
void vtkKWArguments::AddCallback(const char* argument, ArgumentTypeEnum type, 
  CallbackType callback, void* call_data, const char* help)
{
  vtkKWArguments::CallbackStructure s;
  s.Argument     = argument;
  s.ArgumentType = type;
  s.Callback     = callback;
  s.CallData     = call_data;
  s.VariableType = vtkKWArguments::NO_VARIABLE_TYPE;
  s.Variable     = 0;
  s.Help         = help;

  this->Internals->Callbacks[argument] = s;
  this->GenerateHelp();
}

//----------------------------------------------------------------------------
void vtkKWArguments::AddHandler(const char* argument, ArgumentTypeEnum type,
  VariableTypeEnum vtype, void* variable, const char* help)
{
  vtkKWArguments::CallbackStructure s;
  s.Argument     = argument;
  s.ArgumentType = type;
  s.Callback     = 0;
  s.CallData     = 0;
  s.VariableType = vtype;
  s.Variable     = variable;
  s.Help         = help;

  this->Internals->Callbacks[argument] = s;
  this->GenerateHelp();
}

//----------------------------------------------------------------------------
void vtkKWArguments::AddHandler(const char* argument, ArgumentTypeEnum type,
  int* variable, const char* help)
{
  this->AddHandler(argument, type, vtkKWArguments::INT_TYPE, variable, help);
}

//----------------------------------------------------------------------------
void vtkKWArguments::AddHandler(const char* argument, ArgumentTypeEnum type,
  double* variable, const char* help)
{
  this->AddHandler(argument, type, vtkKWArguments::DOUBLE_TYPE, variable, help);
}

//----------------------------------------------------------------------------
void vtkKWArguments::AddHandler(const char* argument, ArgumentTypeEnum type,
  char** variable, const char* help)
{
  this->AddHandler(argument, type, vtkKWArguments::STRING_TYPE, variable, help);
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
  this->GenerateHelp();
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
const char* vtkKWArguments::GetHelp(const char* arg)
{
  vtkKWArguments::Internal::CallbacksMap::iterator it 
    = this->Internals->Callbacks.find(arg);
  if ( it == this->Internals->Callbacks.end() )
    {
    return 0;
    }

  // Since several arguments may point to the same argument, find the one this
  // one point to if this one is pointing to another argument.
  vtkKWArguments::CallbackStructure *cs = &(it->second);
  while ( 1 )
    {
    vtkKWArguments::Internal::CallbacksMap::iterator hit 
      = this->Internals->Callbacks.find(cs->Help);
    if ( hit == this->Internals->Callbacks.end() )
      {
      return cs->Help;
      }
    cs = &(hit->second);
    }
  // Should never happened
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWArguments::GenerateHelp()
{
  ostrstream str;

  // Collapse all arguments into the map of vectors of all arguments that do
  // the same thing.
  vtkKWArguments::Internal::CallbacksMap::iterator it;
  typedef vtkstd::map<vtkKWArguments::Internal::String, 
     vtkKWArguments::Internal::SetOfStrings > MapArgs;
  MapArgs mp;
  MapArgs::iterator mpit, smpit;
  for ( it = this->Internals->Callbacks.begin();
    it != this->Internals->Callbacks.end();
    it ++ )
    {
    vtkKWArguments::CallbackStructure *cs = &(it->second);
    mpit = mp.find(cs->Help);
    if ( mpit != mp.end() )
      {
      mpit->second.insert(it->first);
      mp[it->first].insert(it->first);
      }
    else
      {
      mp[it->first].insert(it->first);
      }
    }
  for ( it = this->Internals->Callbacks.begin();
    it != this->Internals->Callbacks.end();
    it ++ )
    {
    vtkKWArguments::CallbackStructure *cs = &(it->second);
    mpit = mp.find(cs->Help);
    if ( mpit != mp.end() )
      {
      mpit->second.insert(it->first);
      smpit = mp.find(it->first);
      vtkKWArguments::Internal::SetOfStrings::iterator sit;
      for ( sit = smpit->second.begin(); sit != smpit->second.end(); sit++ )
        {
        mpit->second.insert(*sit);
        }
      mp.erase(smpit);
      }
    else
      {
      mp[it->first].insert(it->first);
      }
    }
 
  // Find the length of the longest string
  vtkKWArguments::Internal::String::size_type maxlen = 0;
  for ( mpit = mp.begin();
    mpit != mp.end();
    mpit ++ )
    {
    vtkKWArguments::Internal::SetOfStrings::iterator sit;
    for ( sit = mpit->second.begin(); sit != mpit->second.end(); sit++ )
      {
      vtkKWArguments::Internal::String::size_type clen = sit->size();
      switch ( this->Internals->Callbacks[*sit].ArgumentType )
        {
        case vtkKWArguments::NO_ARGUMENT:     clen += 0; break;
        case vtkKWArguments::CONCAT_ARGUMENT: clen += 6; break;
        case vtkKWArguments::SPACE_ARGUMENT:  clen += 7; break;
        case vtkKWArguments::EQUAL_ARGUMENT:  clen += 7; break;
        }
      if ( clen > maxlen )
        {
        maxlen = clen;
        }
      }
    }

  // Create format for that string
  char format[80];
  sprintf(format, "%%%ds", static_cast<unsigned int>(maxlen));


  // Print help for each option
  for ( mpit = mp.begin();
    mpit != mp.end();
    mpit ++ )
    {
    vtkKWArguments::Internal::SetOfStrings::iterator sit;
    for ( sit = mpit->second.begin(); sit != mpit->second.end(); sit++ )
      {
      str << endl;
      char argument[100];
      sprintf(argument, sit->c_str());
      switch ( this->Internals->Callbacks[*sit].ArgumentType )
        {
        case vtkKWArguments::NO_ARGUMENT: break;
        case vtkKWArguments::CONCAT_ARGUMENT: strcat(argument, "option"); break;
        case vtkKWArguments::SPACE_ARGUMENT:  strcat(argument, " option"); break;
        case vtkKWArguments::EQUAL_ARGUMENT:  strcat(argument, "=option"); break;
        }
      char buffer[80];
      sprintf(buffer, format, argument);
      str << buffer;
      }
    str << "\t";
    const char* ptr = this->Internals->Callbacks[mpit->first].Help;
    int len = strlen(ptr);
    int cnt = 0;
    while ( len > 0)
      {
      // If argument with help is longer than line length, split it on previous
      // space (or tab) and continue on the next line
      vtkKWArguments::Internal::String::size_type cc;
      for ( cc = 0; ptr[cc]; cc ++ )
        {
        if ( *ptr == ' ' || *ptr == '\t' )
          {
          ptr ++;
          len --;
          }
        }
      if ( cnt > 0 )
        {
        for ( cc = 0; cc < maxlen; cc ++ )
          {
          str << " ";
          }
        str << "\t";
        }
      vtkKWArguments::Internal::String::size_type skip = len;
      if ( skip > this->LineLength - maxlen )
        {
        skip = this->LineLength - maxlen;
        for ( cc = skip-1; cc > 0; cc -- )
          {
          if ( ptr[cc] == ' ' || ptr[cc] == '\t' )
            {
            break;
            }
          }
        if ( cc != 0 )
          {
          skip = cc;
          }
        }
      str.write(ptr, skip);
      str << endl;
      ptr += skip;
      len -= skip;
      cnt ++;
      }
    }
  str << ends;
  this->SetHelp(str.str());
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
int vtkKWArguments::IsSpecified(const char* name)
{
  vtkKWArguments::Internal::MapOfStrings::iterator it
    = this->Internals->ArgumentValues.find(name);
  if ( it == this->Internals->ArgumentValues.end() )
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkKWArguments::GetValue(const char* name)
{
  vtkKWArguments::Internal::MapOfStrings::iterator it
    = this->Internals->ArgumentValues.find(name);
  if ( it == this->Internals->ArgumentValues.end() )
    {
    return 0;
    }
  return it->second.c_str();
}

//----------------------------------------------------------------------------
void vtkKWArguments::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if ( this->Help )
    {
    os << indent << "Help: " << endl << this->Help << endl;
    }
  else
    {
    os << indent << "No help" << endl;
    }
  os << "Linelength: " << this->LineLength << endl;
}



