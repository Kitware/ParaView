/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTestUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTestUtilities.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkType.h"
#include "vtkType.h" //
#include <string>    //
#include <vector>    //
using std::string;
using std::vector;

vtkStandardNewMacro(vtkPVTestUtilities);

//-----------------------------------------------------------------------------
void vtkPVTestUtilities::Initialize(int argc, char** argv)
{
  this->DataRoot = NULL;
  this->TempRoot = NULL;
  this->Argc = argc;
  this->Argv = argv;
  if (!((argc == 0) || (argv == 0)))
  {
    this->DataRoot = this->GetDataRoot();
    this->TempRoot = this->GetTempRoot();
  }
}
//-----------------------------------------------------------------------------
char* vtkPVTestUtilities::GetDataRoot()
{
  return this->GetCommandTailArgument("-D");
}
//-----------------------------------------------------------------------------
char* vtkPVTestUtilities::GetTempRoot()
{
  return this->GetCommandTailArgument("-T");
}
//-----------------------------------------------------------------------------
char* vtkPVTestUtilities::GetCommandTailArgument(const char* tag)
{
  for (int i = 1; i < this->Argc; ++i)
  {
    if (string(this->Argv[i]) == string(tag))
    {
      if ((i + 1) < this->Argc)
      {
        return this->Argv[i + 1];
      }
      else
      {
        break;
      }
    }
  }
  return 0;
}
//-----------------------------------------------------------------------------
// int vtkPVTestUtilities::CheckForCommandTailArgument(const char *tag)
// {
//   for (int i=1; i<this->Argc; ++i)
//     {
//     if (string(this->Argv[i])==string(tag))
//       {
//       return this->Argv[i+1];
//       }
//     }
//   return 0;
// }
//-----------------------------------------------------------------------------
char vtkPVTestUtilities::GetPathSep()
{
#if defined _WIN32 && !defined __CYGWIN__
  return '\\';
#elif defined _WIN64 && !defined __CYGWIN__
  return '\\';
#else
  return '/';
#endif
}
//-----------------------------------------------------------------------------
// Concat the data root path to the name supplied.
// The return is a c string that has the correct
// path separators.
char* vtkPVTestUtilities::GetFilePath(const char* base, const char* name)
{
  int baseLen = static_cast<int>(strlen(base));
  int nameLen = static_cast<int>(strlen(name));
  int pathLen = baseLen + 1 + nameLen + 1;
  char* filePath = new char[pathLen];
  int i = 0;
  for (; i < baseLen; ++i)
  {
    if (this->GetPathSep() == '\\' && base[i] == '/')
    {
      filePath[i] = '\\';
    }
    else
    {
      filePath[i] = base[i];
    }
  }
  filePath[i] = this->GetPathSep();
  ++i;
  for (int j = 0; j < nameLen; ++j, ++i)
  {
    if (this->GetPathSep() == '\\' && name[j] == '/')
    {
      filePath[i] = '\\';
    }
    else
    {
      filePath[i] = name[j];
    }
  }
  filePath[i] = '\0';
  return filePath;
}
//-----------------------------------------------------------------------------
void vtkPVTestUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "argc=" << this->Argc << endl;
  os << indent << "argv=" << this->Argv << endl;
  if (this->DataRoot)
  {
    os << indent << "DataRoot=" << this->DataRoot << endl;
  }
  if (this->TempRoot)
  {
    os << indent << "TempRoot=" << this->TempRoot << endl;
  }
}
