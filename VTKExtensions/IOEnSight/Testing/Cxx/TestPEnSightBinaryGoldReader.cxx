#include "vtkCellTypes.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPGenericEnSightReader.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

int TestPEnSightBinaryGoldReader(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/EnSight/TEST_bin.case");

  vtkNew<vtkPGenericEnSightReader> reader;
  reader->SetCaseFileName(fname);
  reader->Update();
  vtkMultiBlockDataSet* mb = reader->GetOutput();
  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(mb->GetBlock(0));

  vtkNew<vtkCellTypes> types;
  ug->GetCellTypes(types);

  auto nbOfTypes = types->GetNumberOfTypes();
  if (nbOfTypes != 2)
  {
    std::cerr << "Wrong number of cell types. Expects 2 ( has " << nbOfTypes << ")." << std::endl;
    return EXIT_FAILURE;
  }

  for (vtkIdType i = 0; i < nbOfTypes; i++)
  {
    auto type = ug->GetCellType(i);
    switch (type)
    {
      case VTK_QUAD:
      case VTK_POLYGON:
      case VTK_HEXAHEDRON:
      case VTK_POLYHEDRON:
        continue;
      default:
        std::cerr << "Unexpected cell type (" << vtkCellTypes::GetClassNameFromTypeId(type) << ")."
                  << std::endl;
        return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
