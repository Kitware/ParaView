/*=========================================================================

  Program:   ParaView
  Module:    TestMPI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCollection.h"
#include "vtkPVFileInformation.h"
#include "vtkPVFileInformationHelper.h"

#include <iostream>

int TestSpecialDirectories(int, char* [])
{
  vtkPVFileInformationHelper* helper = vtkPVFileInformationHelper::New();
  vtkPVFileInformation* info = vtkPVFileInformation::New();

  helper->SetSpecialDirectories(true);

  info->CopyFromObject(helper);

  vtkCollection* coll = info->GetContents();
  coll->InitTraversal();
  vtkObject* obj;
  while ((obj = coll->GetNextItemAsObject()))
  {
    vtkPVFileInformation* finfo = vtkPVFileInformation::SafeDownCast(obj);
    std::cerr << "name: " << finfo->GetName() << std::endl;
    std::cerr << "path: " << finfo->GetFullPath() << std::endl;
    std::cerr << std::endl;
  }

  info->Delete();
  helper->Delete();

  return 0;
}
