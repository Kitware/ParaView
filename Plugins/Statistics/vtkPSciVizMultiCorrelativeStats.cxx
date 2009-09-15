#include "vtkPSciVizMultiCorrelativeStats.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPMultiCorrelativeStatistics.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPSciVizMultiCorrelativeStats);
vtkCxxRevisionMacro(vtkPSciVizMultiCorrelativeStats,"1.1");

vtkPSciVizMultiCorrelativeStats::vtkPSciVizMultiCorrelativeStats()
{
}

vtkPSciVizMultiCorrelativeStats::~vtkPSciVizMultiCorrelativeStats()
{
}

void vtkPSciVizMultiCorrelativeStats::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

int vtkPSciVizMultiCorrelativeStats::RequestModelDataObject( vtkInformation* oinfo )
{
  vtkDataObject* ouData = oinfo->Get( vtkDataObject::DATA_OBJECT() );
  if ( ! ouData || ! ouData->IsA( "vtkMultiBlockDataSet" ) )
    {
    vtkMultiBlockDataSet* mbds = vtkMultiBlockDataSet::New();
    mbds->SetPipelineInformation( oinfo );
    oinfo->Set( vtkDataObject::DATA_OBJECT(), mbds );
    oinfo->Set( vtkDataObject::DATA_EXTENT_TYPE(), mbds->GetExtentType() );
    mbds->FastDelete();
    }
  return 1;
}

int vtkPSciVizMultiCorrelativeStats::FitModel( vtkDataObject*& modelDO, vtkInformationVector* output, vtkTable* trainingData )
{
  // Get where we'll store the output statistical model.
  modelDO = vtkDataObject::GetData( output, 0 );
  if ( ! modelDO )
    {
    vtkErrorMacro( "No model output dataset" );
    return 0;
    }
  modelDO->Initialize();

  // Create the statistics filter and run it
  vtkPMultiCorrelativeStatistics* stats = vtkPMultiCorrelativeStatistics::New();
  stats->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, trainingData );
  vtkIdType ncols = trainingData->GetNumberOfColumns();
  for ( vtkIdType i = 0; i < ncols; ++ i )
    {
    stats->SetColumnStatus( trainingData->GetColumnName( i ), 1 );
    }

  stats->SetLearnOption( true );
  stats->SetDeriveOption( true );
  stats->SetAssessOption( false );
  stats->Update();

  // Copy the output of the statistics filter to our output
  modelDO->ShallowCopy( stats->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  stats->Delete();

  return 1;
}

int vtkPSciVizMultiCorrelativeStats::AssessData( vtkTable* observations, vtkDataObject* assessedOut, vtkDataObject* modelOut )
{
  if ( ! assessedOut )
    {
    vtkErrorMacro( "No output data object." );
    return 0;
    }

  vtkFieldData* dataAttrOut = assessedOut->GetAttributesAsFieldData( this->AttributeMode );
  if ( ! dataAttrOut )
    {
    vtkErrorMacro( "No attributes of type " << this->AttributeMode << " on data object " << assessedOut );
    return 0;
    }

  // Shallow-copy the model so we don't create an infinite loop.
  vtkDataObject* modelCopy = modelOut->NewInstance();
  modelCopy->ShallowCopy( modelOut );

  // Create the statistics filter and run it
  vtkPMultiCorrelativeStatistics* stats = vtkPMultiCorrelativeStatistics::New();
  stats->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, observations );
  stats->SetInput( vtkStatisticsAlgorithm::INPUT_MODEL, modelCopy );
  //stats->GetAssessNames()->SetValue( 0, "d2" );
  modelCopy->FastDelete();
  vtkIdType ncols = observations->GetNumberOfColumns();
  for ( vtkIdType i = 0; i < ncols; ++ i )
    {
    stats->SetColumnStatus( observations->GetColumnName( i ), 1 );
    }

  stats->SetLearnOption( false );
  stats->SetDeriveOption( true );
  stats->SetAssessOption( true );
  stats->Update();

  vtkTable* assessTable = vtkTable::SafeDownCast( stats->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA ) );
  vtkIdType ncolsout = assessTable ? assessTable->GetNumberOfColumns() : 0;
  for ( int i = ncols; i < ncolsout; ++ i )
    {
    dataAttrOut->AddArray( assessTable->GetColumn( i ) );
    }
  stats->Delete();

  return 1;
}

