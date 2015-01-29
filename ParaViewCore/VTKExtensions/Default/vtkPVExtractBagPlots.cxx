#include "vtkPVExtractBagPlots.h"

#include "vtkAbstractArray.h"
#include "vtkExtractFunctionalBagPlot.h"
#include "vtkHighestDensityRegionsStatistics.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPCAStatistics.h"
#include "vtkPSciVizPCAStats.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTransposeTable.h"

#include <set>
#include <string>

//----------------------------------------------------------------------------
// Internal class that holds selected columns
class PVExtractBagPlotsInternal
{
public:
  bool Clear()
  {
    if (this->Columns.empty())
      {
      return false;
      }
    this->Columns.clear();
    return true;
  }

  bool Has(const std::string& v)
  {
    return this->Columns.find(v) != this->Columns.end();
  }

  bool Set(const std::string& v)
  {
  if (this->Has(v))
    {
    return false;
    }
  this->Columns.insert(v);
  return true;
  }

  std::set<std::string> Columns;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtractBagPlots);

//----------------------------------------------------------------------------
vtkPVExtractBagPlots::vtkPVExtractBagPlots()
{
  this->TransposeTable = false;
  this->RobustPCA = false;
  this->Sigma = 1.;
  this->Internal = new PVExtractBagPlotsInternal();
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVExtractBagPlots::~vtkPVExtractBagPlots()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVExtractBagPlots::EnableAttributeArray(const char* arrName)
{
  if (arrName)
    {
    if (this->Internal->Set(arrName))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractBagPlots::ClearAttributeArrays()
{
  if (this->Internal->Clear())
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractBagPlots::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVExtractBagPlots::RequestData(vtkInformation*,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);

  vtkTable* inTable = vtkTable::GetData(inputVector[0]);
  vtkMultiBlockDataSet* outTables =
    vtkMultiBlockDataSet::SafeDownCast(
      outputInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkTable* outTable = 0;
  vtkTable* outTable2 = 0;

  if (inTable->GetNumberOfColumns() == 0)
    {
    return 1;
    }

  if (!outTables)
    {
    return 0;
    }
  outTables->SetNumberOfBlocks(2);

  vtkNew<vtkTransposeTable> transpose;

  // Construct a table that holds only the selected columns
  vtkNew<vtkTable> subTable;
  std::set<std::string>::iterator iter = this->Internal->Columns.begin();
  for (; iter!=this->Internal->Columns.end(); ++iter)
    {
    if (vtkAbstractArray* arr = inTable->GetColumnByName(iter->c_str()))
      {
      subTable->AddColumn(arr);
      }
    }

  vtkTable *inputTable = subTable.GetPointer();

  outTable = subTable.GetPointer();

  if (this->TransposeTable)
    {
    transpose->SetInputData(subTable.GetPointer());
    transpose->SetAddIdColumn(true);
    transpose->SetIdColumnName("ColName");
    transpose->Update();

    inputTable = transpose->GetOutput();
    }

  outTable2 = inputTable;

  // Compute PCA

  vtkNew<vtkPSciVizPCAStats> pca;
  pca->SetInputData(inputTable);
  pca->SetAttributeMode(vtkDataObject::ROW);
  for (vtkIdType i = 0; i < inputTable->GetNumberOfColumns(); i++)
    {
    vtkAbstractArray* arr = inputTable->GetColumn(i);
    if (strcmp(arr->GetName(), "ColName"))
      {
      pca->EnableAttributeArray(arr->GetName());
      }
    }

  pca->SetBasisScheme(vtkPCAStatistics::FIXED_BASIS_SIZE);
  pca->SetFixedBasisSize(2);
  pca->SetTrainingFraction(1.0);
  pca->SetRobustPCA(this->RobustPCA);
  pca->Update();

  vtkTable* outputPCATable = vtkTable::SafeDownCast(
    pca->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));

  outTable2 = outputPCATable;

  // Compute HDR

  vtkNew<vtkHighestDensityRegionsStatistics> hdr;
  hdr->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, outputPCATable);
  hdr->SetSigma(this->Sigma);

  std::string x, y;
  for (vtkIdType i = 0; i < outputPCATable->GetNumberOfColumns(); i++)
    {
    vtkAbstractArray* arr = outputPCATable->GetColumn(i);
    char* str = arr->GetName();
    if (strstr(str, "PCA"))
      {
      if (strstr(str, "(0)"))
        {
        x = std::string(str);
        arr->SetName("x");
        }
      else
        {
        y = std::string(str);
        arr->SetName("y");
        }
      }
    }
  hdr->AddColumnPair("x", "y");
  hdr->SetLearnOption(true);
  hdr->SetDeriveOption(true);
  hdr->SetAssessOption(false);
  hdr->SetTestOption(false);
  hdr->Update();

  vtkMultiBlockDataSet* outputHDR = vtkMultiBlockDataSet::SafeDownCast(
    hdr->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
  vtkTable* outputHDRTable = vtkTable::SafeDownCast(outputHDR->GetBlock(0));
  outTable2 = outputHDRTable;
  vtkAbstractArray *cname = inputTable->GetColumnByName("ColName");
  if (cname)
    {
    outputHDRTable->AddColumn(cname);
    }
  else
    {
    vtkNew<vtkStringArray> colNameArray;
    colNameArray->SetName("ColName");
    vtkIdType len = inputTable->GetNumberOfColumns();
    colNameArray->SetNumberOfValues(len);
    for (vtkIdType i  = 0 ; i < len; i++)
      {
      colNameArray->SetValue(i, inputTable->GetColumn(i)->GetName());
      }
    outputHDRTable->AddColumn(colNameArray.GetPointer());
    }

  // Extract the bag plot columns for functional bag plots
  vtkNew<vtkExtractFunctionalBagPlot> ebp;
  ebp->SetInputData(0, outTable);
  ebp->SetInputData(1, outputHDRTable);
  ebp->SetInputArrayToProcess(0, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, "HDR (y,x)");
  ebp->SetInputArrayToProcess(1, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, "ColName");
  ebp->Update();

  vtkTable* outBPTable = ebp->GetOutput();
  outTable = outBPTable;

  unsigned int blockID = 0;
  outTables->SetBlock(blockID, outTable);
  outTables->GetMetaData(blockID)->Set(vtkCompositeDataSet::NAME(), "Functional Bag Plot Data");
  blockID = 1;
  outTables->SetBlock(blockID, outTable2);
  outTables->GetMetaData(blockID)->Set(vtkCompositeDataSet::NAME(), "Bag Plot Data");

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractBagPlots
::FillInputPortInformation( int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}
