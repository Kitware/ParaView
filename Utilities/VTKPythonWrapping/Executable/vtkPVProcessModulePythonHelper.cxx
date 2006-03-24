/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModulePythonHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h"

#include "vtkPVProcessModulePythonHelper.h"

#include "vtkProcessModule.h"
#include "vtkObjectFactory.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkWindows.h"

#include <vtksys/SystemTools.hxx>

#include "vtkPythonAppInitConfigure.h"

#if defined(CMAKE_INTDIR)
# define VTK_PYTHON_LIBRARY_DIR VTK_PYTHON_LIBRARY_DIR_BUILD "/" CMAKE_INTDIR
#else
# define VTK_PYTHON_LIBRARY_DIR VTK_PYTHON_LIBRARY_DIR_BUILD
#endif

extern "C" {
  extern DL_IMPORT(int) Py_Main(int, char **);
}


vtkCxxRevisionMacro(vtkPVProcessModulePythonHelper, "1.6");
vtkStandardNewMacro(vtkPVProcessModulePythonHelper);

//----------------------------------------------------------------------------
vtkPVProcessModulePythonHelper::vtkPVProcessModulePythonHelper()
{
  this->SMApplication = vtkSMApplication::New();
  this->ShowProgress = 0;
  this->Filter = 0;
  this->CurrentProgress = 0;
}

//----------------------------------------------------------------------------
vtkPVProcessModulePythonHelper::~vtkPVProcessModulePythonHelper()
{
  this->SMApplication->Finalize();
  this->SMApplication->Delete();
  this->SetFilter(0);
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVProcessModulePythonHelper::RunGUIStart(int argc, char **argv, int numServerProcs, int myId)
{
  (void)myId;
  (void)numServerProcs;

  this->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);

  vtkPVPythonOptions* boptions = vtkPVPythonOptions::SafeDownCast(this->ProcessModule->GetOptions());
  int res = 0; 

  // The following code will hack in the path for running VTK/Python
  // from the build tree. Do not try this at home. We are
  // professionals.

  // Set the program name, so that we can ask python to provide us
  // full path.
  Py_SetProgramName(argv[0]);

  // Initialize interpreter.
  Py_Initialize();

  // If the location of the library path and wrapping path exist, add
  // them to the list.
  
  // Get the pointer to path list object, append both paths, and
  // make sure to decrease reference counting for both path strings.
  char tmpPath[5];
  sprintf(tmpPath,"path");
  PyObject* path = PySys_GetObject(tmpPath);
  PyObject* newpath;
  if ( vtksys::SystemTools::FileExists(VTK_PYTHON_LIBRARY_DIR) )
    {
    newpath = PyString_FromString(VTK_PYTHON_LIBRARY_DIR);
    PyList_Insert(path, 0, newpath);
    Py_DECREF(newpath);
    }
  if ( vtksys::SystemTools::FileExists(VTK_PYTHON_PACKAGE_DIR) )
    {
    newpath = PyString_FromString(VTK_PYTHON_PACKAGE_DIR);
    PyList_Insert(path, 0, newpath);
    Py_DECREF(newpath);
    }

  // Ok, all done, now enter python main.
  vtkstd::vector<char*> vArg;
#define vtkPVStrDup(x) \
  strcpy(new char[ strlen(x) + 1], x)

  vArg.push_back(vtkPVStrDup(argv[0]));
  if ( boptions->GetPythonScriptName() )
    {
    vArg.push_back(vtkPVStrDup(boptions->GetPythonScriptName()));
    }
  else if (argc > 1)
    {
    vArg.push_back(vtkPVStrDup("-"));
    }
  for (int cc=1; cc < argc; cc++)
    {
    vArg.push_back(vtkPVStrDup(argv[cc]));
    }

  res = Py_Main(vArg.size(), &*vArg.begin());

  vtkstd::vector<char*>::iterator it;
  for ( it = vArg.begin(); it != vArg.end(); ++ it )
    {
    delete [] *it;
    }

  this->ProcessModule->Exit();

  // Exiting:  CLean up.
  return res;
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::ExitApplication()
{ 
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::SendPrepareProgress()
{
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::CloseCurrentProgress()
{
  if ( this->ShowProgress )
    {
    while ( this->CurrentProgress <= 10 )
      {
      cout << ".";
      this->CurrentProgress ++;
      }
    cout << "]" << endl;
    }
  this->CurrentProgress = 0;
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::SendCleanupPendingProgress()
{
  this->CloseCurrentProgress();
  this->ShowProgress = 0;
  this->SetFilter(0);
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::SetLocalProgress(const char* filter, int val)
{
  val /= 10;
  int new_progress = 0;
  if ( !filter || !this->Filter || strcmp(filter, this->Filter) != 0 )
    {
    this->CloseCurrentProgress();
    this->SetFilter(filter);
    new_progress = 1;
    }
  if ( !this->ShowProgress )
    {
    new_progress = 1;
    this->ShowProgress = 1;
    }
  if ( new_progress )
    {
    if ( filter[0] == 'v' && filter[1] == 't' && filter[2] == 'k' )
      {
      filter += 3;
      }
    cout << "Process " << filter << " [";
    cout.flush();
    }
  while ( this->CurrentProgress <= val )
    {
    cout << ".";
    cout.flush();
    this->CurrentProgress ++;
    }
}

