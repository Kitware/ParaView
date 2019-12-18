/*=========================================================================

  Program:   ParaView
  Module:    TestPVDArraySelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkNew.h"
#include "vtkPVDReader.h"
#include "vtkTestUtilities.h"

#define TASSERT(x)                                                                                 \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "ERROR: failed at " << __LINE__ << "!" << endl;                                        \
    return EXIT_FAILURE;                                                                           \
  }

bool HasArray(vtkDataObject* dobj, int association, const char* aname)
{
  if (auto cd = vtkCompositeDataSet::SafeDownCast(dobj))
  {
    bool has_array = false;
    auto iter = cd->NewIterator();
    for (iter->InitTraversal(); !has_array && !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      has_array = has_array || HasArray(iter->GetCurrentDataObject(), association, aname);
    }
    iter->Delete();
    return has_array;
  }
  else
  {
    return dobj->GetAttributes(association)->GetArray(aname) != nullptr;
  }
}

int TestPVDArraySelection(int argc, char* argv[])
{
  vtkNew<vtkPVDReader> reader;

  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/dualSphereAnimation.pvd");
  reader->SetFileName(fname);
  delete[] fname;

  TASSERT(reader->GetCellDataArraySelection()->GetNumberOfArrays() == 0);

  // disable array before first UpdateInformation.
  reader->GetCellDataArraySelection()->DisableArray("cellNormals");
  reader->Update();
  TASSERT(!HasArray(reader->GetOutputDataObject(0), vtkDataObject::CELL, "cellNormals"))

  // enable array
  reader->GetCellDataArraySelection()->EnableAllArrays();
  reader->Update();
  TASSERT(HasArray(reader->GetOutputDataObject(0), vtkDataObject::CELL, "cellNormals"))

  reader->GetCellDataArraySelection()->DisableAllArrays();
  reader->GetPointDataArraySelection()->DisableAllArrays();
  reader->Update();

  TASSERT(!HasArray(reader->GetOutputDataObject(0), vtkDataObject::CELL, "cellNormals"))
  TASSERT(!HasArray(reader->GetOutputDataObject(0), vtkDataObject::POINT, "Normals"))
  return EXIT_SUCCESS;
}
