/*=========================================================================

Program:   ParaView
Module:    TestProxyAnnotation.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include <sstream>

//----------------------------------------------------------------------------
int TestProxyAnnotation(int argc, char* argv[])
{
  int ret_val = EXIT_SUCCESS;
  vtkPVOptions* options = vtkPVOptions::New();
  bool success = true;
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT, options);
  if (!success)
  {
    return -1;
  }

  vtkSMSession* session = vtkSMSession::New();
  cout << "Starting..." << endl;

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  // *******************************************************************
  // Test specific code
  // *******************************************************************
  cout << endl << endl;
  cout << "********************************" << endl;
  cout << "*** Testing Proxy annotation ***" << endl;
  cout << "********************************" << endl << endl;

  // Create a source proxy
  vtkSMProxy* proxy = pxm->NewProxy("sources", "SphereSource");
  proxy->UpdateVTKObjects();

  cout << "--- Add annotation: Color, Tooltip, Owner ---" << endl;
  proxy->SetAnnotation("Color", "#FFAA00");
  proxy->SetAnnotation("Tooltip", "Just a sphere");
  proxy->SetAnnotation("Owner", "Seb");
  if (proxy->GetNumberOfAnnotations() != 3)
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect 3 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Color to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Tooltip"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Tooltip to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Owner"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Owner to be present." << endl;
  }
  else
  {
    cout << " -> OK" << endl;
  }

  // -----------------------------------------------------------------------
  vtkNew<vtkPVXMLElement> withAnnotationXML;
  proxy->SaveXMLState(withAnnotationXML.GetPointer());
  std::ostringstream withAnnotationStr;
  withAnnotationXML->PrintXML(withAnnotationStr, vtkIndent());
  // -----------------------------------------------------------------------

  proxy->SetAnnotation("Owner", nullptr);
  cout << "--- Remove Owner annotations ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 2)
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect 2 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Color to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Tooltip"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Tooltip to be present." << endl;
  }
  else
  {
    cout << " -> OK" << endl;
  }

  proxy->RemoveAnnotation("Tooltip");
  cout << "--- Remove Tooltip annotations ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 1)
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect 1 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Color to be present." << endl;
  }
  else
  {
    cout << " -> OK" << endl;
  }

  proxy->RemoveAllAnnotations();
  cout << "--- Remove all annotations ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 0)
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect 0 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else
  {
    cout << " -> OK" << endl;
  }

  cout << "--- Add annotation: Color, Tooltip, Owner ---" << endl;
  proxy->SetAnnotation("Color", "#FFAA00");
  proxy->SetAnnotation("Tooltip", "Just a sphere");
  proxy->SetAnnotation("Owner", "Seb");
  if (proxy->GetNumberOfAnnotations() != 3)
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect 3 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Color to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Tooltip"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Tooltip to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Owner"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Owner to be present." << endl;
  }
  else
  {
    cout << " -> OK" << endl;
  }

  proxy->RemoveAllAnnotations();
  cout << "--- Remove all annotations ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 0)
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect 0 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else
  {
    cout << " -> OK" << endl;
  }

  // -----------------------------------------------------------------------
  vtkNew<vtkPVXMLElement> withoutAnnotationXML;
  proxy->SaveXMLState(withoutAnnotationXML.GetPointer());
  std::ostringstream withoutAnnotationStr;
  withoutAnnotationXML->PrintXML(withoutAnnotationStr, vtkIndent());
  // -----------------------------------------------------------------------

  cout << "--- Compare XML state ---" << endl;
  cout << withAnnotationStr.str().c_str() << endl;
  if (withAnnotationStr.str() == withoutAnnotationStr.str())
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: The xml state should be different when annotation are present." << endl;
  }
  else
  {
    cout << " -> OK" << endl;
  }

  cout << "--- Load XML state with annotation ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 0)
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect 0 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else
  {
    cout << " -> Before load OK no annotation" << endl;
  }
  proxy->LoadXMLState(withAnnotationXML->GetNestedElement(0), nullptr);
  if (proxy->GetNumberOfAnnotations() != 3)
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect 3 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Color to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Tooltip"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Tooltip to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Owner"))
  {
    ret_val = EXIT_FAILURE;
    cout << "Error: Expect annotations Owner to be present." << endl;
  }
  else
  {
    cout << " -> After load OK found the expected annotations" << endl;
  }

  // *******************************************************************

  proxy->Delete();
  session->Delete();
  cout << "Exiting..." << endl;

  vtkInitializationHelper::Finalize();
  options->Delete();
  return ret_val;
}
