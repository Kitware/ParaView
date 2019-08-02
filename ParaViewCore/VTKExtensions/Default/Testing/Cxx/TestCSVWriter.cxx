/*=========================================================================

  Program:   ParaView
  Module:    TestCSVWriter.cxx

  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkCSVWriter.h>
#include <vtkDelimitedTextReader.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkLogger.h>
#include <vtkMPIController.h>
#include <vtkNew.h>
#include <vtkTable.h>
#include <vtkTesting.h>

#include <string>

namespace
{

// ensure that the writer works when the columns are not in the same order on all ranks.
// also ensures partial arrays don't mess things up.
bool WriteCSV(const std::string& fname, int rank)
{
  vtkNew<vtkTable> table;
  vtkNew<vtkDoubleArray> col1;
  col1->SetName("Column1");
  col1->SetNumberOfTuples(10);

  vtkNew<vtkIntArray> col2;
  col2->SetName("Column2");
  col2->SetNumberOfTuples(10);

  vtkNew<vtkIntArray> col3;
  col3->SetName("Column3-Partial");
  col3->SetNumberOfTuples(10);

  for (int cc = 0; cc < 10; ++cc)
  {
    const auto row = cc + rank * 10;
    col1->SetValue(cc, row + 1.5);
    col2->SetValue(cc, row * 100);
    col3->SetValue(cc, 20);
  }

  if (rank == 0)
  {
    table->AddColumn(col1);
    table->AddColumn(col3);
    table->AddColumn(col2);
  }
  else
  {
    table->AddColumn(col2);
    table->AddColumn(col1);
  }

  vtkNew<vtkCSVWriter> writer;
  writer->SetFileName(fname.c_str());
  writer->SetInputDataObject(table);
  writer->Update();
  return true;
}

#define VERITFY_EQ(x, y, txt)                                                                      \
  if ((x) != (y))                                                                                  \
  {                                                                                                \
    std::cerr << "ERROR: " << txt << endl                                                          \
              << "expected (" << (x) << "), got (" << (y) << ")" << endl;                          \
    return false;                                                                                  \
  }

bool ReadAndVerifyCSV(const std::string& fname, int rank, int numRanks)
{
  if (rank != 0)
  {
    return true;
  }

  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(fname.c_str());
  reader->SetHaveHeaders(true);
  reader->SetDetectNumericColumns(true);
  reader->Update();

  auto table = reader->GetOutput();
  VERITFY_EQ(table->GetNumberOfRows(), 10 * numRanks, "incorrect row count");
  VERITFY_EQ(table->GetNumberOfColumns(), 2, "incorrect column count");

  for (int irank = 0; irank < numRanks; ++irank)
  {
    for (vtkIdType cc = 0; cc < 10; ++cc)
    {
      const auto row = cc + irank * 10;
      auto value1 = table->GetValueByName(row, "Column1");
      auto value2 = table->GetValueByName(row, "Column2");
      VERITFY_EQ(value1.ToDouble(), row + 1.5,
        std::string("incorrect column1  values at row ") + std::to_string(row));
      VERITFY_EQ(value2.ToInt(), row * 100,
        std::string("incorrect column2  values at row ") + std::to_string(row));
    }
  }
  return true;
}

} // end of namespace

int TestCSVWriter(int argc, char* argv[])
{
  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  const int myRank = contr->GetLocalProcessId();
  const int numRanks = contr->GetNumberOfProcesses();

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  if (!testing->GetTempDirectory())
  {
    vtkLogF(ERROR, "no temp directory specified!");
    contr->Finalize();
    contr->Delete();
    return EXIT_FAILURE;
  }

  std::string tname{ testing->GetTempDirectory() };
  int success = WriteCSV(tname + "/TestCSVWriter.csv", myRank) &&
      ReadAndVerifyCSV(tname + "/TestCSVWriter.csv", myRank, numRanks)
    ? 1
    : 0;

  int all_success;
  contr->AllReduce(&success, &all_success, 1, vtkCommunicator::LOGICAL_AND_OP);

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  contr->Delete();
  return all_success ? EXIT_SUCCESS : EXIT_FAILURE;
}
