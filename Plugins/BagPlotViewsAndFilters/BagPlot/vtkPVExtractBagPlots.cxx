#include "vtkPVExtractBagPlots.h"

#include "vtkAbstractArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtractFunctionalBagPlot.h"
#include "vtkHighestDensityRegionsStatistics.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPCAStatistics.h"
#include "vtkPSciVizPCAStats.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTransposeTable.h"

#include <algorithm>
#include <set>
#include <sstream>
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

  bool Has(const std::string& v) { return this->Columns.find(v) != this->Columns.end(); }

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
  this->TransposeTable = true;
  this->RobustPCA = false;
  this->KernelWidth = 1.;
  this->UseSilvermanRule = false;
  this->GridSize = 100;
  this->UserQuantile = 95;
  this->Internal = new PVExtractBagPlotsInternal();

  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVExtractBagPlots::~vtkPVExtractBagPlots()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
int vtkPVExtractBagPlots::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
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

  os << "TransposeTable: " << this->TransposeTable << std::endl;
  os << "RobustPCA: " << this->RobustPCA << std::endl;
  os << "KernelWidth: " << this->KernelWidth << std::endl;
  os << "UseSilvermanRule: " << this->UseSilvermanRule << std::endl;
  os << "GridSize: " << this->GridSize << std::endl;
  os << "UserQuantile: " << this->UserQuantile << std::endl;
}

// ----------------------------------------------------------------------
void vtkPVExtractBagPlots::GetEigenvalues(
  vtkMultiBlockDataSet* outputMetaDS, vtkDoubleArray* eigenvalues)
{
  vtkTable* outputMeta = vtkTable::SafeDownCast(outputMetaDS->GetBlock(1));

  if (!outputMeta)
  {
    vtkErrorMacro(<< "NULL table pointer!");
    return;
  }

  vtkDoubleArray* meanCol = vtkDoubleArray::SafeDownCast(outputMeta->GetColumnByName("Mean"));
  vtkStringArray* rowNames = vtkStringArray::SafeDownCast(outputMeta->GetColumnByName("Column"));

  eigenvalues->SetNumberOfComponents(1);

  // Get values
  for (vtkIdType i = 0, eval = 0; i < meanCol->GetNumberOfTuples(); i++)
  {
    std::stringstream ss;
    ss << "PCA " << eval;

    std::string rowName = rowNames->GetValue(i);
    if (rowName.compare(ss.str()) == 0)
    {
      eigenvalues->InsertNextValue(meanCol->GetValue(i));
      eval++;
    }
  }
}

// ----------------------------------------------------------------------
void vtkPVExtractBagPlots::GetEigenvectors(
  vtkMultiBlockDataSet* outputMetaDS, vtkDoubleArray* eigenvectors, vtkDoubleArray* eigenvalues)
{
  // Count eigenvalues
  this->GetEigenvalues(outputMetaDS, eigenvalues);
  vtkIdType numberOfEigenvalues = eigenvalues->GetNumberOfTuples();

  if (!outputMetaDS)
  {
    vtkErrorMacro(<< "NULL dataset pointer!");
  }

  vtkTable* outputMeta = vtkTable::SafeDownCast(outputMetaDS->GetBlock(1));

  if (!outputMeta)
  {
    vtkErrorMacro(<< "NULL table pointer!");
  }

  vtkDoubleArray* meanCol = vtkDoubleArray::SafeDownCast(outputMeta->GetColumnByName("Mean"));
  vtkStringArray* rowNames = vtkStringArray::SafeDownCast(outputMeta->GetColumnByName("Column"));

  eigenvectors->SetNumberOfComponents(numberOfEigenvalues);

  // Get vectors
  for (vtkIdType i = 0, eval = 0; i < meanCol->GetNumberOfTuples(); i++)
  {
    std::stringstream ss;
    ss << "PCA " << eval;

    std::string rowName = rowNames->GetValue(i);
    if (rowName.compare(ss.str()) == 0)
    {
      std::vector<double> eigenvector;
      for (int val = 0; val < numberOfEigenvalues; val++)
      {
        // The first two columns will always be "Column" and "Mean",
        // so start with the next one
        vtkDoubleArray* currentCol = vtkDoubleArray::SafeDownCast(outputMeta->GetColumn(val + 2));
        eigenvector.push_back(currentCol->GetValue(i));
      }

      eigenvectors->InsertNextTypedTuple(&eigenvector.front());
      eval++;
    }
  }
}

//----------------------------------------------------------------------------
int vtkPVExtractBagPlots::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkTable* inTable = vtkTable::GetData(inputVector[0]);
  vtkMultiBlockDataSet* outTables = vtkMultiBlockDataSet::GetData(outputVector);

  if (inTable->GetNumberOfColumns() == 0)
  {
    return 1;
  }

  if (!outTables)
  {
    return 0;
  }
  outTables->SetNumberOfBlocks(2);

  // Construct a table that holds only the selected columns
  vtkNew<vtkTable> subTable;
  std::set<std::string>::iterator iter = this->Internal->Columns.begin();
  for (; iter != this->Internal->Columns.end(); ++iter)
  {
    if (vtkAbstractArray* arr = inTable->GetColumnByName(iter->c_str()))
    {
      subTable->AddColumn(arr);
    }
  }

  vtkTable* inputTable = subTable.GetPointer();
  vtkTable* outTable = subTable.GetPointer();

  vtkNew<vtkTransposeTable> transpose;
  if (this->TransposeTable)
  {
    transpose->SetInputData(subTable.GetPointer());
    transpose->SetAddIdColumn(true);
    transpose->SetIdColumnName("ColName");
    transpose->Update();

    inputTable = transpose->GetOutput();
  }

  vtkTable* outTable2 = inputTable;

  // Compute the PCA on the provided input functions
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
  pca->SetFixedBasisSize(
    this->NumberOfProjectionAxes); // Number of PCA projection axis - 10 is the max we will allow
  pca->SetTrainingFraction(1.0);
  pca->SetRobustPCA(this->RobustPCA);
  pca->Update();

  vtkTable* outputPCATable =
    vtkTable::SafeDownCast(pca->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));

  outTable2 = outputPCATable;

  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast(
    pca->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_DATA));

  // Compute the explained variance
  vtkNew<vtkDoubleArray> eigenVectors;
  vtkNew<vtkDoubleArray> eigenValues;
  this->GetEigenvectors(outputMetaDS, eigenVectors.Get(), eigenValues.Get());

  double sumOfEigenValues = 0.;
  double partialSumOfEigenValues = 0.;
  for (vtkIdType i = 0; i < eigenValues->GetNumberOfTuples(); i++)
  {
    double val = eigenValues->GetValue(i);
    sumOfEigenValues += val;
    partialSumOfEigenValues += i < this->NumberOfProjectionAxes ? val : 0.;
  }
  double explainedVariance = 100. * (partialSumOfEigenValues / sumOfEigenValues);

  // Compute HDR
  vtkNew<vtkHighestDensityRegionsStatistics> hdr;
  hdr->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, outputPCATable);

  // Fetch and rename the 2 PCA coordinates components arrays
  std::vector<vtkDataArray*> cArrays;
  vtkDataArray* hdrArrays[2];
  vtkIdType iArray = 0;
  for (vtkIdType i = 0; i < outputPCATable->GetNumberOfColumns(); i++)
  {
    vtkAbstractArray* arr = outputPCATable->GetColumn(i);
    char* str = arr->GetName();
    if (strstr(str, "PCA"))
    {
      std::string name = "x" + std::to_string(iArray);
      arr->SetName(name.c_str());
      cArrays.push_back(vtkDataArray::SafeDownCast(arr));
      if (iArray < 2)
      {
        // select the first two arrays for the hdr computation
        hdrArrays[iArray] = vtkDataArray::SafeDownCast(arr);
      }
      iArray++;
    }
  }

  double bounds[4] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  hdrArrays[0]->GetRange(&bounds[0], 0);
  hdrArrays[1]->GetRange(&bounds[2], 0);

  double sigma = this->KernelWidth;
  if (this->UseSilvermanRule)
  {
    vtkIdType len = hdrArrays[0]->GetNumberOfTuples();
    double xMean = 0.;
    for (vtkIdType i = 0; i < len; i++)
    {
      xMean += hdrArrays[0]->GetTuple1(i);
    }
    xMean /= len;

    sigma = 0.0;
    for (vtkIdType i = 0; i < len; i++)
    {
      sigma += (hdrArrays[0]->GetTuple1(i) - xMean) * (hdrArrays[0]->GetTuple1(i) - xMean);
    }
    sigma /= len;
    sigma = sqrt(sigma) * pow(len, -1. / 6.);
  }

  hdr->SetSigma(sigma);
  hdr->AddColumnPair("x0", "x1");
  hdr->SetLearnOption(true);
  hdr->SetDeriveOption(true);
  hdr->SetAssessOption(false);
  hdr->SetTestOption(false);
  hdr->Update();

  // Compute Grid
  vtkNew<vtkDoubleArray> inObs;
  inObs->SetNumberOfComponents(2);
  inObs->SetNumberOfTuples(hdrArrays[0]->GetNumberOfTuples());

  inObs->CopyComponent(0, hdrArrays[0], 0);
  inObs->CopyComponent(1, hdrArrays[1], 0);

  // Add border to grid
  const double borderSize = 0.15;
  bounds[0] -= (bounds[1] - bounds[0]) * borderSize;
  bounds[1] += (bounds[1] - bounds[0]) * borderSize;
  bounds[2] -= (bounds[3] - bounds[2]) * borderSize;
  bounds[3] += (bounds[3] - bounds[2]) * borderSize;

  const int gridWidth = this->GetGridSize();
  const int gridHeight = this->GetGridSize();
  const double spaceX = (bounds[1] - bounds[0]) / gridWidth;
  const double spaceY = (bounds[3] - bounds[2]) / gridHeight;
  vtkNew<vtkDoubleArray> inPOI;
  inPOI->SetNumberOfComponents(2);
  inPOI->SetNumberOfTuples(gridWidth * gridHeight);

  vtkIdType pointId = 0;
  for (int j = 0; j < gridHeight; j++)
  {
    for (int i = 0; i < gridWidth; i++)
    {
      double x = bounds[0] + i * spaceX;
      double y = bounds[2] + j * spaceY;
      inPOI->SetTuple2(pointId++, x, y);
    }
  }

  vtkDataArray* outDens = vtkDataArray::CreateDataArray(inObs->GetDataType());
  outDens->SetNumberOfComponents(1);
  outDens->SetNumberOfTuples(gridWidth * gridHeight);

  // Evaluate the HDR on every pixel of the grid
  hdr->ComputeHDR(inObs.Get(), inPOI.Get(), outDens);

  vtkNew<vtkImageData> grid;
  grid->SetDimensions(gridWidth, gridHeight, 1);
  grid->SetOrigin(bounds[0], bounds[2], 0.);
  grid->SetSpacing(spaceX, spaceY, 1.);
  grid->GetPointData()->SetScalars(outDens);

  outDens->Delete();

  // Compute the integral of the density and save the position of the
  // highest density in the grid for further evaluation of the mean.
  vtkDataArray* densities = outDens;
  double totalSumOfDensities = 0.;
  std::vector<double> sortedDensities;
  sortedDensities.reserve(densities->GetNumberOfTuples());
  for (vtkIdType pixel = 0; pixel < densities->GetNumberOfTuples(); pixel++)
  {
    double density = densities->GetTuple1(pixel);
    sortedDensities.push_back(density);
    totalSumOfDensities += density;
  }

  // Sort the densities and save the densities associated to the quantiles.
  std::sort(sortedDensities.begin(), sortedDensities.end());
  double sumOfDensities = 0.;
  double sumForP50 = totalSumOfDensities * 0.5;
  double sumForPUser = totalSumOfDensities * ((100. - this->UserQuantile) / 100.);
  double p50 = 0.;
  double pUser = 0.;
  for (double d : sortedDensities)
  {
    sumOfDensities += d;
    if (sumOfDensities >= sumForP50 && p50 == 0.)
    {
      p50 = d;
    }
    if (sumOfDensities >= sumForPUser && pUser == 0.)
    {
      pUser = d;
    }
  }

  // Save information on the quantiles (% and density) in a specific table.
  // It will be used downstream by the bag plot representation for instance
  // to generate the contours at the provided values.
  vtkNew<vtkTable> thresholdTable;
  vtkNew<vtkDoubleArray> tValues;
  tValues->SetName("TValues");
  tValues->SetNumberOfValues(6);
  tValues->SetValue(0, 50);
  tValues->SetValue(1, p50);
  tValues->SetValue(2, this->UserQuantile);
  tValues->SetValue(3, pUser);
  tValues->SetValue(4, explainedVariance);
  tValues->SetValue(5, sigma);
  thresholdTable->AddColumn(tValues.Get());

  // Bag plot
  vtkMultiBlockDataSet* outputHDR = vtkMultiBlockDataSet::SafeDownCast(
    hdr->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
  vtkTable* outputHDRTable = vtkTable::SafeDownCast(outputHDR->GetBlock(0));
  outTable2 = outputHDRTable;

  for (auto* arr : cArrays)
  {
    outTable2->AddColumn(arr);
  }

  vtkAbstractArray* cname = inputTable->GetColumnByName("ColName");
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
    for (vtkIdType i = 0; i < len; i++)
    {
      colNameArray->SetValue(i, inputTable->GetColumn(i)->GetName());
    }
    outputHDRTable->AddColumn(colNameArray.GetPointer());
  }

  // Extract the bag plot columns for functional bag plots
  vtkNew<vtkExtractFunctionalBagPlot> ebp;
  ebp->SetInputData(0, outTable);
  ebp->SetInputData(1, outputHDRTable);
  ebp->SetInputArrayToProcess(0, 1, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "HDR (x1,x0)");
  ebp->SetInputArrayToProcess(1, 1, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "ColName");
  ebp->SetDensityForP50(p50);
  ebp->SetDensityForPUser(pUser);
  ebp->SetPUser(this->UserQuantile);
  ebp->Update();

  outTable = ebp->GetOutput();

  double maxHdr = VTK_DOUBLE_MIN;
  std::string maxHdrCName = "";
  vtkDataArray* seriesHdr =
    vtkDataArray::SafeDownCast(outputHDRTable->GetColumnByName("HDR (x1,x0)"));
  vtkStringArray* seriesColName =
    vtkStringArray::SafeDownCast(outputHDRTable->GetColumnByName("ColName"));
  assert(seriesHdr && seriesColName);
  for (vtkIdType i = 0; i < seriesHdr->GetNumberOfTuples(); i++)
  {
    double v = seriesHdr->GetTuple1(i);
    if (v > maxHdr)
    {
      maxHdr = v;
      maxHdrCName = seriesColName->GetValue(i);
    }
  }

  // Compute the mean function by back-projecting the point of the
  // highest-density with the PCA eigen-vectors and the mean
  vtkDoubleArray* medianFunction =
    vtkDoubleArray::SafeDownCast(outTable->GetColumnByName("QMedianLine"));
  if (medianFunction)
  {
    outTable->RemoveColumnByName(medianFunction->GetName());
  }

  // We need to copy the median array before renaming it as it is a shallow
  // copy from the input.
  vtkDataArray* maxHdrColumn =
    vtkDataArray::SafeDownCast(outTable->GetColumnByName(maxHdrCName.c_str()));
  assert(maxHdrColumn);
  vtkSmartPointer<vtkDataArray> medianArray;
  medianArray.TakeReference(vtkDataArray::CreateDataArray(maxHdrColumn->GetDataType()));
  medianArray->DeepCopy(maxHdrColumn);
  outTable->RemoveColumnByName(medianArray->GetName());
  outTable->AddColumn(medianArray);
  std::stringstream medianColumnName;
  medianColumnName << medianArray->GetName() << "_median";
  medianArray->SetName(medianColumnName.str().c_str());

  // Inject non-selected columns in the first output block.
  // This can be useful to select those columns as X-axis in the plot.
  vtkIdType inNbCols = inTable->GetNumberOfColumns();
  for (vtkIdType i = 0; i < inNbCols; i++)
  {
    vtkAbstractArray* col = inTable->GetColumn(i);
    if (!this->Internal->Has(col->GetName()) && col->GetName() != medianColumnName.str())
    {
      outTable->AddColumn(col);
    }
  }

  // Finally setup the output multi-block dataset
  unsigned int blockID = 0;
  outTables->SetBlock(blockID, outTable);
  outTables->GetMetaData(blockID++)->Set(vtkCompositeDataSet::NAME(), "Functional Bag Plot Data");
  outTables->SetBlock(blockID, outTable2);
  outTables->GetMetaData(blockID++)->Set(vtkCompositeDataSet::NAME(), "Bag Plot Data");
  outTables->SetBlock(blockID, grid.Get());
  outTables->GetMetaData(blockID++)->Set(vtkCompositeDataSet::NAME(), "Grid Data");
  outTables->SetBlock(blockID, thresholdTable.Get());
  outTables->GetMetaData(blockID++)->Set(vtkCompositeDataSet::NAME(), "Threshold Data");

  return 1;
}
