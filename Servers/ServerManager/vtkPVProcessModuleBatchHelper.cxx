/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModuleBatchHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProcessModuleBatchHelper.h"

#include "vtkProcessModule.h"
#include "vtkPVProcessModuleBatchHelperConfig.h"
#include "vtkObjectFactory.h"
#include "vtkPVBatchOptions.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkTclUtil.h"
#include "vtkWindows.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPVProcessModuleBatchHelper);

EXTERN void TclSetLibraryPath _ANSI_ARGS_((Tcl_Obj * pathPtr));
extern "C" int Vtkcommontcl_Init(Tcl_Interp *interp);
extern "C" int Vtkpvservermanagertcl_Init(Tcl_Interp *interp); 
extern "C" int Vtkpvservercommontcl_Init(Tcl_Interp *interp); 

//----------------------------------------------------------------------------
static Tcl_Interp *vtkPVProcessModuleBatchHelperInitializeTcl(int argc, 
                                            char *argv[], 
                                            ostream *err)
{
  Tcl_Interp *interp;
  char *args;
  char buf[100];

  (void)err;

  // The call to Tcl_FindExecutable has to be executed *now*, it does more 
  // than just finding the executable (for ex:, it will set variables 
  // depending on the value of TCL_LIBRARY, TK_LIBRARY)

  vtkTclApplicationInitExecutable(argc, argv);

  // Create the interpreter

  interp = Tcl_CreateInterp();
  args = Tcl_Merge(argc - 1, argv + 1);
  Tcl_SetVar(interp, (char *)"argv", args, TCL_GLOBAL_ONLY);
  ckfree(args);
  sprintf(buf, "%d", argc-1);
  Tcl_SetVar(interp, (char *)"argc", buf, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, (char *)"argv0", argv[0], TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, (char *)"tcl_interactive", 
             (char *)"0", TCL_GLOBAL_ONLY);

  const char* relative_dirs[] =
    {
      "../lib/TclTk/lib",
      "TclTk/lib",
      ".." VTK_PV_TclTk_INSTALL_DIR,     // for exe in PREFIX/bin
      "../.." VTK_PV_TclTk_INSTALL_DIR,  // for exe in PREFIX/lib/paraview-V.v
      0
    };
  vtkTclApplicationInitTclTk(interp, relative_dirs);

  // Init Tcl

  int status;

  status = Tcl_Init(interp);
  if (status != TCL_OK)
    {
    if (err)
      {
      *err << "Tcl_Init error: " << Tcl_GetStringResult(interp) << endl;
      }
    return NULL;
    }

  // Initialize VTK

  Vtkcommontcl_Init(interp);
  Vtkpvservermanagertcl_Init(interp); 
  Vtkpvservercommontcl_Init(interp); 

  return interp;
}

//----------------------------------------------------------------------------
vtkPVProcessModuleBatchHelper::vtkPVProcessModuleBatchHelper()
{
  this->SMApplication = vtkSMApplication::New();
  this->ShowProgress = 0;
  this->Filter = 0;
  this->CurrentProgress = 0;
}

//----------------------------------------------------------------------------
vtkPVProcessModuleBatchHelper::~vtkPVProcessModuleBatchHelper()
{
  this->SMApplication->Finalize();
  this->SMApplication->Delete();
  this->SetFilter(0);
}

//----------------------------------------------------------------------------
void vtkPVProcessModuleBatchHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVProcessModuleBatchHelper::RunGUIStart(int argc, char **argv, int numServerProcs, int myId)
{
  (void)myId;
  vtksys_ios::ostringstream err;
  Tcl_Interp *interp = vtkPVProcessModuleBatchHelperInitializeTcl(argc, argv, &err);
  if (!interp)
    {
#ifdef _WIN32
    ::MessageBox(0, err.str().c_str(),
                 "ParaView error: InitializeTcl failed", MB_ICONERROR|MB_OK);
#else
    cerr << "ParaView error: InitializeTcl failed" << endl 
         << err.str().c_str() << endl;
#endif
    return 1;
    }
  (void)numServerProcs;

  this->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);

  vtkSMProxyManager* proxm = vtkSMObject::GetProxyManager();
  proxm->InstantiateGroupPrototypes("filters");

  vtkPVBatchOptions* boptions = vtkPVBatchOptions::SafeDownCast(this->ProcessModule->GetOptions());
  char* file = 
    vtksys::SystemTools::DuplicateString(boptions->GetBatchScriptName());
  int res = 0; 
  // make exit do nothing in batch scripts
  if(Tcl_GlobalEval(interp, "proc exit {} {}") != TCL_OK)
    {
    cerr << "\n    Script: \n" << "proc exit {} {}"
         << "\n    Returned Error on line "
         << interp->errorLine << ": \n"  
         << Tcl_GetStringResult(interp) << endl;
    res = 1;
    }
  // add this window as a variable
  if ( Tcl_EvalFile(interp, file) != TCL_OK )
    {
    cerr << "Script: \n" << boptions->GetBatchScriptName() 
      << "\n    Returned Error on line "
      << interp->errorLine << ": \n      "  
      << Tcl_GetStringResult(interp) << endl;
    res = 1;
    }
  delete [] file;

  Tcl_DeleteInterp(interp);
  Tcl_Finalize();

  this->ProcessModule->Exit();

  // Exiting:  CLean up.
  return res;
}

//----------------------------------------------------------------------------
void vtkPVProcessModuleBatchHelper::ExitApplication()
{ 
}

//----------------------------------------------------------------------------
void vtkPVProcessModuleBatchHelper::SendPrepareProgress()
{
}

//----------------------------------------------------------------------------
void vtkPVProcessModuleBatchHelper::CloseCurrentProgress()
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
void vtkPVProcessModuleBatchHelper::SendCleanupPendingProgress()
{
  this->CloseCurrentProgress();
  this->ShowProgress = 0;
  this->SetFilter(0);
}

//----------------------------------------------------------------------------
void vtkPVProcessModuleBatchHelper::SetLocalProgress(const char* filter, int val)
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

