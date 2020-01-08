#include "vtkPSciVizKMeans.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPKMeansStatistics.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPSciVizKMeans);

vtkPSciVizKMeans::vtkPSciVizKMeans()
{
  this->K = 5;
  this->MaxNumIterations = 50;
  this->Tolerance = 0.01;
}

vtkPSciVizKMeans::~vtkPSciVizKMeans()
{
}

void vtkPSciVizKMeans::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "K: " << K << "\n";
  os << indent << "MaxNumIterations: " << this->MaxNumIterations << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}

int vtkPSciVizKMeans::LearnAndDerive(vtkMultiBlockDataSet* modelDO, vtkTable* inData)
{
  // Create the statistics filter and run it
  vtkPKMeansStatistics* stats = vtkPKMeansStatistics::New();
  stats->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, inData);
  stats->SetDefaultNumberOfClusters(this->K);
  stats->SetMaxNumIterations(this->MaxNumIterations);
  stats->SetTolerance(this->Tolerance);
  vtkIdType ncols = inData->GetNumberOfColumns();
  for (vtkIdType i = 0; i < ncols; ++i)
  {
    stats->SetColumnStatus(inData->GetColumnName(i), 1);
  }

  stats->SetLearnOption(true);
  stats->SetDeriveOption(true);
  stats->SetAssessOption(false);
  stats->Update();

  // Copy the output of the statistics filter to our output
  modelDO->ShallowCopy(stats->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
  stats->Delete();

  return 1;
}

int vtkPSciVizKMeans::AssessData(
  vtkTable* observations, vtkDataObject* assessedOut, vtkMultiBlockDataSet* modelOut)
{
  if (!assessedOut)
  {
    vtkErrorMacro("No output data object.");
    return 0;
  }

  vtkFieldData* dataAttrOut = assessedOut->GetAttributesAsFieldData(this->AttributeMode);
  if (!dataAttrOut)
  {
    vtkErrorMacro(
      "No attributes of type " << this->AttributeMode << " on data object " << assessedOut);
    return 0;
  }

  // Shallow-copy the model so we don't create an infinite loop.
  vtkDataObject* modelCopy = modelOut->NewInstance();
  modelCopy->ShallowCopy(modelOut);

  // Create the statistics filter and run it
  vtkPKMeansStatistics* stats = vtkPKMeansStatistics::New();
  stats->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, observations);
  stats->SetInputData(vtkStatisticsAlgorithm::INPUT_MODEL, modelCopy);
  stats->SetDefaultNumberOfClusters(this->K);
  stats->SetMaxNumIterations(this->MaxNumIterations);
  stats->SetTolerance(this->Tolerance);
  modelCopy->FastDelete();
  vtkIdType ncols = observations->GetNumberOfColumns();
  for (vtkIdType i = 0; i < ncols; ++i)
  {
    stats->SetColumnStatus(observations->GetColumnName(i), 1);
    // stats->AddColumn( observations->GetColumnName( i ) );
  }

  stats->SetLearnOption(false);
  stats->SetDeriveOption(true);
  stats->SetAssessOption(true);
  stats->Update();

  vtkTable* assessTable =
    vtkTable::SafeDownCast(stats->GetOutput(vtkStatisticsAlgorithm::OUTPUT_DATA));
  vtkIdType ncolsout = assessTable ? assessTable->GetNumberOfColumns() : 0;
  for (int i = ncols; i < ncolsout; ++i)
  {
    dataAttrOut->AddArray(assessTable->GetColumn(i));
  }
  stats->Delete();

  return 1;
}
