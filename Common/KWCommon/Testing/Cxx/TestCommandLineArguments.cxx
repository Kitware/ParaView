/*=========================================================================

  Program:   ParaView
  Module:    TestCommandLineArguments.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWArguments.h"

void* random_ptr = (void*)0x123;

int argument(const char* arg, const char* value, void* call_data)
{
  cout << "Got argument: \"" << arg << "\" value: \"" << (value?value:"(null)") << "\"" << endl;
  if ( call_data != random_ptr )
    {
    cerr << "Problem processing call_data" << endl;
    return 0;
    }
  return 1;
}

int unknown_argument(const char* argument, void* call_data)
{
  cout << "Got unknown argument: \"" << argument << "\"" << endl;
  if ( call_data != random_ptr )
    {
    cerr << "Problem processing call_data" << endl;
    return 0;
    }
  return 1;
}

vtkKWArguments::CallbackStructure callbacks[] = {
    { "-A", vtkKWArguments::NO_ARGUMENT, argument, random_ptr, "Some option -A. This option has a multiline comment. It should demonstrate how the code splits lines." },
    { "-B", vtkKWArguments::SPACE_ARGUMENT, argument, random_ptr, "Option -B takes argument with space" },
    { "-C", vtkKWArguments::EQUAL_ARGUMENT, argument, random_ptr, "Option -C takes argument after =" },
    { "-D", vtkKWArguments::CONCAT_ARGUMENT, argument, random_ptr, "This option takes concatinated argument" },
    { "--long1", vtkKWArguments::NO_ARGUMENT, argument, random_ptr, "-A"},
    { "--long2", vtkKWArguments::SPACE_ARGUMENT, argument, random_ptr, "-B" },
    { "--long3", vtkKWArguments::EQUAL_ARGUMENT, argument, random_ptr, "Same as -C but a bit different" },
    { "--long4", vtkKWArguments::CONCAT_ARGUMENT, argument, random_ptr, "-C" },
    { 0, 0, 0, 0, 0 }
    };

int main(int argc, char* argv[])
{
  int res = 0;
  vtkKWArguments *arg = vtkKWArguments::New();
  arg->Initialize(argc, argv);
  arg->AddCallbacks(callbacks);
  arg->SetClientData(random_ptr);
  arg->SetUnknownArgumentCallback(unknown_argument);
  if ( !arg->Parse() )
    {
    cerr << "Problem parsing arguments" << endl;
    res = 1;
    }
  cout << "Help: " << arg->GetHelp() << endl;

  int cc;
  for ( cc = 0; callbacks[cc].Argument; cc ++ )
    {
    const char* carg = callbacks[cc].Argument;
    cout << "Argument " << carg << " is ";
    if ( arg->IsSpecified(carg) )
      {
      cout << "specified value is: " << arg->GetValue(carg);
      }
    else
      {
      cout << "not specified";
      }
    cout << " - Help: [" << arg->GetHelp(carg) << "]";
    cout << endl;
    }
  arg->Delete();
  return res;
}
