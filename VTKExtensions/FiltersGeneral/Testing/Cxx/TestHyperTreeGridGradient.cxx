/*=========================================================================

  Program:   ParaView
  Module:    TestPolyhedralToSimpleCellsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkErrorObserver.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPVGradientFilter.h"
#include "vtkRandomHyperTreeGridSource.h"

#include <string>

#define vtk_assert(x)                                                                              \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;               \
    return EXIT_FAILURE;                                                                           \
  }

class TestHTGGradient
{
public:
  static int DoTest()
  {
    vtkNew<vtkErrorObserver> obs;

    vtkNew<vtkRandomHyperTreeGridSource> source;
    source->AddObserver(vtkCommand::ErrorEvent, obs);
    source->SetDimensions(3, 3, 2);
    source->SetSeed(42);
    source->SetSplitFraction(0.75);

    vtkNew<vtkPVGradientFilter> grad;
    grad->AddObserver(vtkCommand::ErrorEvent, obs);
    grad->SetInputConnection(source->GetOutputPort());
    grad->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Depth");
    grad->SetResultArrayName("Grad");

    grad->Update();

    vtkHyperTreeGrid* res = vtkHyperTreeGrid::SafeDownCast(grad->GetOutputDataObject(0));

    vtk_assert(res);                              // output type: HTG
    vtk_assert(res->GetCellData()->GetVectors()); // Has gradient array ...
    std::string arrName(res->GetCellData()->GetVectors()->GetName());
    vtk_assert(arrName == "Grad"); // ... with the right name

    source->RemoveObserver(obs);
    grad->RemoveObserver(obs);

    return EXIT_SUCCESS;
  }
};

int TestHyperTreeGridGradient(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // test is inside class such that friend declaration in
  // filter can work
  return TestHTGGradient::DoTest();
}
