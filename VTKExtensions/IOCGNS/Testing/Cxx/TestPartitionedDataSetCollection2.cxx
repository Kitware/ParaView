/*=========================================================================

  Program:   ParaView
  Module:    TestPartitionedDataSetCollection2.cxx

  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkIOSSReader.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPVTestUtilities.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

int TestPartitionedDataSetCollection2(int argc, char* argv[])
{
  // read can.ex2 using ioss reader
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/can.ex2");
  vtkNew<vtkIOSSReader> readerIOSS;
  readerIOSS->AddFileName(fileNameC);
  delete[] fileNameC;
  readerIOSS->Update();

  // check that can should have 3 empty and 2 non-empty partitions
  auto can = vtkPartitionedDataSetCollection::SafeDownCast(readerIOSS->GetOutputDataObject(0));
  unsigned int emptyPartitions = 0;
  unsigned int nonEmptyPartitions = 0;
  for (unsigned int i = 0; i < can->GetNumberOfPartitionedDataSets(); ++i)
  {
    if (can->GetPartitionedDataSet(i)->GetNumberOfPartitions() == 0)
    {
      emptyPartitions++;
    }
    else
    {
      nonEmptyPartitions++;
    }
  }

  vtkLogIfF(ERROR, emptyPartitions != 3 || nonEmptyPartitions != 2,
    "Expected 3 empty and 2 non-empty partitions");

  // remove writerCGNS file if it already exists
  const vtkNew<vtkPVTestUtilities> utilities;
  utilities->Initialize(argc, argv);
  const char* filename = utilities->GetTempFilePath("can.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  // write can.ex2 as can.cgns
  const vtkNew<vtkCGNSWriter> writerCGNS;
  writerCGNS->SetInputConnection(readerIOSS->GetOutputPort());
  writerCGNS->SetFileName(filename);

  int result = writerCGNS->Write();

  if (result == 1)
  {
    vtkLogIfF(ERROR, !vtksys::SystemTools::FileExists(filename), "File %s not found", filename);
    const vtkNew<vtkCGNSReader> readerCGNS;
    readerCGNS->SetFileName(filename);
    // update information first to get all bases in the information
    readerCGNS->UpdateInformation();
    // then enable all bases get both bases (volume, surface) into the output
    readerCGNS->EnableAllBases();
    readerCGNS->Update();

    const unsigned long err = readerCGNS->GetErrorCode();
    vtkLogIfF(ERROR, err != 0, "CGNS reading failed.");

    vtkMultiBlockDataSet* output = readerCGNS->GetOutput();
    vtkLogIfF(ERROR, nullptr == output, "No CGNS readerCGNS output.");

    result = err == 0 ? 1 : 0;
  }

  delete[] filename;
  return result == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
