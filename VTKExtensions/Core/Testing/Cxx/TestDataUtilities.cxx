/*=========================================================================

  Program:   ParaView
  Module:    TestDataUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkConvertToMultiBlockDataSet.h>
#include <vtkDataSet.h>
#include <vtkLogger.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPVDataUtilities.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPartitionedDataSetCollectionSource.h>

int TestDataUtilities(int, char* [])
{
  vtkNew<vtkPartitionedDataSetCollectionSource> pdcSource;
  pdcSource->SetNumberOfShapes(2);
  pdcSource->Update();
  auto pdc = pdcSource->GetOutput();

  // Test for vtkPartitionedDataSetCollection.
  vtkPVDataUtilities::AssignNamesToBlocks(pdc);
  vtkLogIfF(ERROR,
    vtkPVDataUtilities::GetAssignedNameForBlock(pdc->GetPartitionAsDataObject(0, 0)) != "Boy",
    "Block name mismatch for (0, 0)");
  vtkLogIfF(ERROR,
    vtkPVDataUtilities::GetAssignedNameForBlock(pdc->GetPartitionAsDataObject(1, 0)) != "Cross Cap",
    "Block name mismatch for (1, 0)");

  // Test for vtkMultiBlockDataSet
  vtkNew<vtkConvertToMultiBlockDataSet> converter;
  converter->SetInputConnection(pdcSource->GetOutputPort());
  converter->Update();
  auto mb = converter->GetOutput();

  auto datasets = vtkCompositeDataSet::GetDataSets(mb);
  vtkPVDataUtilities::AssignNamesToBlocks(mb);
  vtkLogIfF(ERROR, vtkPVDataUtilities::GetAssignedNameForBlock(datasets[0]) != "Boy",
    "Block name mismatch for (0, 0)");
  vtkLogIfF(ERROR, vtkPVDataUtilities::GetAssignedNameForBlock(datasets[1]) != "Cross Cap",
    "Block name mismatch for (1, 0)");

  return EXIT_SUCCESS;
}
