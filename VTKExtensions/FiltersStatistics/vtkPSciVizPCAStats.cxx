#include "vtkPSciVizPCAStats.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPPCAStatistics.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPSciVizPCAStats);

vtkPSciVizPCAStats::vtkPSciVizPCAStats()
{
  this->NormalizationScheme = vtkPCAStatistics::NONE;
  this->BasisScheme = vtkPCAStatistics::FULL_BASIS;
  this->FixedBasisSize = 10;
  this->FixedBasisEnergy = 1.;
  this->RobustPCA = false;
}

vtkPSciVizPCAStats::~vtkPSciVizPCAStats()
{
}

void vtkPSciVizPCAStats::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NormalizationScheme: " << this->NormalizationScheme << "\n";
  os << indent << "BasisScheme: " << this->BasisScheme << "\n";
  os << indent << "FixedBasisSize: " << this->FixedBasisSize << "\n";
  os << indent << "FixedBasisEnergy: " << this->FixedBasisEnergy << "\n";
  os << indent << "RobustPCA:" << this->RobustPCA << "\n";
}

int vtkPSciVizPCAStats::LearnAndDerive(vtkMultiBlockDataSet* modelDO, vtkTable* inData)
{
  // Create the statistics filter and run it
  vtkPPCAStatistics* stats = vtkPPCAStatistics::New();
  stats->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, inData);
  vtkIdType ncols = inData->GetNumberOfColumns();
  for (vtkIdType i = 0; i < ncols; ++i)
  {
    stats->SetColumnStatus(inData->GetColumnName(i), 1);
  }
  stats->SetNormalizationScheme(this->NormalizationScheme);
  stats->SetMedianAbsoluteDeviation(this->RobustPCA);

  stats->SetLearnOption(true);
  stats->SetDeriveOption(true);
  stats->SetAssessOption(false);
  stats->Update();

  // Copy the output of the statistics filter to our output
  modelDO->ShallowCopy(stats->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
  stats->Delete();

  return 1;
}

int vtkPSciVizPCAStats::AssessData(
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
  vtkPPCAStatistics* stats = vtkPPCAStatistics::New();
  stats->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, observations);
  stats->SetInputData(vtkStatisticsAlgorithm::INPUT_MODEL, modelCopy);
  // stats->GetAssessNames()->SetValue( 0, "d2" );
  modelCopy->FastDelete();
  vtkIdType ncols = observations->GetNumberOfColumns();
  for (vtkIdType i = 0; i < ncols; ++i)
  {
    stats->SetColumnStatus(observations->GetColumnName(i), 1);
  }
  stats->SetNormalizationScheme(this->NormalizationScheme);
  stats->SetBasisScheme(this->BasisScheme);
  stats->SetFixedBasisSize(this->FixedBasisSize);
  stats->SetFixedBasisEnergy(this->FixedBasisEnergy);
  stats->SetMedianAbsoluteDeviation(this->RobustPCA);

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
