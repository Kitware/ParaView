// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCollection.h"
#include "vtkPVFileInformation.h"
#include "vtkPVFileInformationHelper.h"

#include <iostream>

extern int TestSpecialDirectories(int, char*[])
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
