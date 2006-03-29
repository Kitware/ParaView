/*=========================================================================

  Program:   ParaView
  Module:    vtkTestingProcessModuleGUIHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTestingProcessModuleGUIHelper.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLParser.h"
#include "vtkRenderWindow.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkSMTesting.h"
#include "vtkTesting.h"
#include "vtkTestingOptions.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkTestingProcessModuleGUIHelper, "1.3");
vtkStandardNewMacro(vtkTestingProcessModuleGUIHelper);

//----------------------------------------------------------------------------
vtkTestingProcessModuleGUIHelper::vtkTestingProcessModuleGUIHelper()
{
  this->SMApplication = vtkSMApplication::New();
  this->ShowProgress = 0;
  this->Filter = 0;
  this->CurrentProgress = 0;
}

//----------------------------------------------------------------------------
vtkTestingProcessModuleGUIHelper::~vtkTestingProcessModuleGUIHelper()
{
  this->SMApplication->Finalize();
  this->SMApplication->Delete();
  this->SetFilter(0);
}

//----------------------------------------------------------------------------
void vtkTestingProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkTestingProcessModuleGUIHelper::RunGUIStart(int , char **, 
  int numServerProcs, int myId)
{
  (void)myId;
  (void)numServerProcs;
  int res = 0;

  this->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);
  this->SMApplication->ParseConfigurationFiles();

  // Load the state and process it.
  vtkTestingOptions* options = vtkTestingOptions::SafeDownCast(
    this->ProcessModule->GetOptions());
  
  if (!options->GetSMStateXMLName())
    {
    vtkErrorMacro("No state to load.");
    this->ProcessModule->Exit();
    return 1;
    }

  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  ifstream ifp;
  ifp.open(options->GetSMStateXMLName(), ios::in | ios::binary);

  // get length of file.
  ifp.seekg(0, ios::end);
  int length = ifp.tellg();
  ifp.seekg(0, ios::beg);

  char* buffer = new char[length+1];
  ifp.read(buffer, length);
  buffer[length] = 0;
  ifp.close();

  // Replace ${DataDir} with the actual data dir path.
  vtkstd::string str_buffer (buffer);
  delete []buffer;
  buffer = 0;
  if (options->GetDataDir())
    {
    vtksys::SystemTools::ReplaceString(str_buffer,
      "${DataDir}", options->GetDataDir());
    }
  parser->Parse(str_buffer.c_str(), str_buffer.length());

  vtkSMStateLoader *loader = vtkSMStateLoader::New();
  loader->LoadState(parser->GetRootElement());
  loader->Delete();
  parser->Delete();

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->UpdateRegisteredProxies("sources", 0);
  pxm->UpdateRegisteredProxies(0);

  vtkSMRenderModuleProxy* rm = vtkSMRenderModuleProxy::SafeDownCast(
    pxm->GetProxy("rendermodules", "RenderModule0"));
  
  rm->StillRender();

  if (options->GetBaselineImage() && options->GetTempDir())
    {
    vtkSMTesting* testing = vtkSMTesting::New();
    testing->AddArgument("-V");
    testing->AddArgument(options->GetBaselineImage());
    testing->AddArgument("-T");
    testing->AddArgument(options->GetTempDir());
    testing->SetRenderModuleProxy(rm);
    if (testing->RegressionTest(options->GetThreshold()) != vtkTesting::PASSED)
      {
      vtkErrorMacro("Regression Test Failed!");
      res = 1;
      }
    testing->Delete();
    }
  else
    {
    cout << "ERROR: Regression tests not performed since no baseline or temp directory "
      "specified." << endl;
    cout << "Press a key to exit." << endl;
    char c;
    cin >> c;
    }
  this->ProcessModule->Exit();

  // Exiting:  CLean up.
  return res;
}

//----------------------------------------------------------------------------
void vtkTestingProcessModuleGUIHelper::ExitApplication()
{ 
}

//----------------------------------------------------------------------------
void vtkTestingProcessModuleGUIHelper::SendPrepareProgress()
{
}

//----------------------------------------------------------------------------
void vtkTestingProcessModuleGUIHelper::CloseCurrentProgress()
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
void vtkTestingProcessModuleGUIHelper::SendCleanupPendingProgress()
{
  this->CloseCurrentProgress();
  this->ShowProgress = 0;
  this->SetFilter(0);
}

//----------------------------------------------------------------------------
void vtkTestingProcessModuleGUIHelper::SetLocalProgress(const char* filter, int val)
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

