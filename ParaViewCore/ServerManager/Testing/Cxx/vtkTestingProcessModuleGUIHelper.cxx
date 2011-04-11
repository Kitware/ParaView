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
#include "vtkOutputWindow.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLParser.h"
#include "vtkRenderWindow.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkSMTesting.h"
#include "vtkTesting.h"
#include "vtkTestingOptions.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/string>


//----------------------------------------------------------------------------
// Output window which prints out the process id
// with the error or warning messages
class VTK_EXPORT vtkTestingOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkTestingOutputWindow,vtkOutputWindow);
  static vtkTestingOutputWindow* New();

  void DisplayDebugText(const char* t)
    {
    this->PVDisplayText(t);
    }

  void DisplayWarningText(const char* t)
    {
    this->PVDisplayText(t);
    }

  void DisplayErrorText(const char* t)
    {
    this->PVDisplayText(t, 1);
    }

  void DisplayGenericWarningText(const char* t)
    {
    this->PVDisplayText(t);
    }
  void DisplayText(const char* t)
    {
    this->PVDisplayText(t, 0);
    }
  void PVDisplayText(const char* t, int error = 0)
    {
    (void)error;
    cerr << t << endl;
    }

protected:
  vtkTestingOutputWindow() {}
  ~vtkTestingOutputWindow() {}

private:
  vtkTestingOutputWindow(const vtkTestingOutputWindow&);
  void operator=(const vtkTestingOutputWindow&);
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTestingOutputWindow);



vtkStandardNewMacro(vtkTestingProcessModuleGUIHelper);

//----------------------------------------------------------------------------
vtkTestingProcessModuleGUIHelper::vtkTestingProcessModuleGUIHelper()
{
  vtkTestingOutputWindow* win = vtkTestingOutputWindow::New();
  vtkOutputWindow::SetInstance(win);
  win->Delete();
}

//----------------------------------------------------------------------------
vtkTestingProcessModuleGUIHelper::~vtkTestingProcessModuleGUIHelper()
{
}

//----------------------------------------------------------------------------
void vtkTestingProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkTestingProcessModuleGUIHelper::Run()
{
  int res = 0;
  // Load the state and process it.
  vtkTestingOptions* options = vtkTestingOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());

  if (!options->GetSMStateXMLName())
    {
    vtkErrorMacro("No state to load.");
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

  vtkSMProxyManager::GetProxyManager()->LoadXMLState(parser->GetRootElement());
  parser->Delete();

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->UpdateRegisteredProxiesInOrder(0);

  vtkSMRenderViewProxy* rm = vtkSMRenderViewProxy::SafeDownCast(
    pxm->GetProxy("rendermodules", "RenderModule0"));
  
  rm->StillRender();

  if (options->GetBaselineImage() && options->GetTempDir())
    {
    vtkSMTesting* testing = vtkSMTesting::New();
    testing->AddArgument("-V");
    testing->AddArgument(options->GetBaselineImage());
    testing->AddArgument("-T");
    testing->AddArgument(options->GetTempDir());
    testing->SetRenderViewProxy(rm);
    if (testing->RegressionTest(options->GetThreshold()) != vtkTesting::PASSED)
      {
      vtkErrorMacro("Regression Test Failed!");
      res = 1;
      }
    testing->Delete();
    pxm->SaveXMLState("/tmp/foo.pvsm");
    }
  else
    {
    vtkErrorMacro("Regression tests not performed since no baseline or temp directory "
      "specified.")
    cout << "Press a key to exit." << endl;
    char c;
    cin >> c;
    }
  
  // Exiting:  CLean up.
  return res;
}
