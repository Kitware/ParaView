// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInitializationHelper.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkPVDataInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include <iostream>
#include <sstream>

//----------------------------------------------------------------------------
extern int TestProxyAnnotation(int argc, char* argv[])
{
  int ret_val = EXIT_SUCCESS;
  bool success = true;
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT);
  if (!success)
  {
    return -1;
  }

  vtkSMSession* session = vtkSMSession::New();
  std::cout << "Starting..." << endl;

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  // *******************************************************************
  // Test specific code
  // *******************************************************************
  std::cout << endl << endl;
  std::cout << "********************************" << endl;
  std::cout << "*** Testing Proxy annotation ***" << endl;
  std::cout << "********************************" << endl << endl;

  // Create a source proxy
  vtkSMProxy* proxy = pxm->NewProxy("sources", "SphereSource");
  proxy->UpdateVTKObjects();

  std::cout << "--- Add annotation: Color, Tooltip, Owner ---" << endl;
  proxy->SetAnnotation("Color", "#FFAA00");
  proxy->SetAnnotation("Tooltip", "Just a sphere");
  proxy->SetAnnotation("Owner", "Seb");
  if (proxy->GetNumberOfAnnotations() != 3)
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect 3 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Color to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Tooltip"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Tooltip to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Owner"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Owner to be present." << endl;
  }
  else
  {
    std::cout << " -> OK" << endl;
  }

  // -----------------------------------------------------------------------
  vtkNew<vtkPVXMLElement> withAnnotationXML;
  proxy->SaveXMLState(withAnnotationXML.GetPointer());
  std::ostringstream withAnnotationStr;
  withAnnotationXML->PrintXML(withAnnotationStr, vtkIndent());
  // -----------------------------------------------------------------------

  proxy->SetAnnotation("Owner", nullptr);
  std::cout << "--- Remove Owner annotations ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 2)
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect 2 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Color to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Tooltip"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Tooltip to be present." << endl;
  }
  else
  {
    std::cout << " -> OK" << endl;
  }

  proxy->RemoveAnnotation("Tooltip");
  std::cout << "--- Remove Tooltip annotations ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 1)
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect 1 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Color to be present." << endl;
  }
  else
  {
    std::cout << " -> OK" << endl;
  }

  proxy->RemoveAllAnnotations();
  std::cout << "--- Remove all annotations ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 0)
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect 0 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else
  {
    std::cout << " -> OK" << endl;
  }

  std::cout << "--- Add annotation: Color, Tooltip, Owner ---" << endl;
  proxy->SetAnnotation("Color", "#FFAA00");
  proxy->SetAnnotation("Tooltip", "Just a sphere");
  proxy->SetAnnotation("Owner", "Seb");
  if (proxy->GetNumberOfAnnotations() != 3)
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect 3 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Color to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Tooltip"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Tooltip to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Owner"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Owner to be present." << endl;
  }
  else
  {
    std::cout << " -> OK" << endl;
  }

  proxy->RemoveAllAnnotations();
  std::cout << "--- Remove all annotations ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 0)
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect 0 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else
  {
    std::cout << " -> OK" << endl;
  }

  // -----------------------------------------------------------------------
  vtkNew<vtkPVXMLElement> withoutAnnotationXML;
  proxy->SaveXMLState(withoutAnnotationXML.GetPointer());
  std::ostringstream withoutAnnotationStr;
  withoutAnnotationXML->PrintXML(withoutAnnotationStr, vtkIndent());
  // -----------------------------------------------------------------------

  std::cout << "--- Compare XML state ---" << endl;
  std::cout << withAnnotationStr.str().c_str() << endl;
  if (withAnnotationStr.str() == withoutAnnotationStr.str())
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: The xml state should be different when annotation are present." << endl;
  }
  else
  {
    std::cout << " -> OK" << endl;
  }

  std::cout << "--- Load XML state with annotation ---" << endl;
  if (proxy->GetNumberOfAnnotations() != 0)
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect 0 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else
  {
    std::cout << " -> Before load OK no annotation" << endl;
  }
  proxy->LoadXMLState(withAnnotationXML->GetNestedElement(0), nullptr);
  if (proxy->GetNumberOfAnnotations() != 3)
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect 3 annotations and got " << proxy->GetNumberOfAnnotations() << endl;
  }
  else if (!proxy->HasAnnotation("Color"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Color to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Tooltip"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Tooltip to be present." << endl;
  }
  else if (!proxy->HasAnnotation("Owner"))
  {
    ret_val = EXIT_FAILURE;
    std::cout << "Error: Expect annotations Owner to be present." << endl;
  }
  else
  {
    std::cout << " -> After load OK found the expected annotations" << endl;
  }

  // *******************************************************************

  proxy->Delete();
  session->Delete();
  std::cout << "Exiting..." << endl;

  vtkInitializationHelper::Finalize();
  return ret_val;
}
