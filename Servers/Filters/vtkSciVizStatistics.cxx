#include "vtkSciVizStatistics.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>
#include <vtksys/ios/sstream>

vtkCxxRevisionMacro(vtkSciVizStatistics,"1.1");

vtkSciVizStatistics::vtkSciVizStatistics()
{
  this->P = new vtkSciVizStatisticsP;
  this->AttributeMode = vtkDataObject::POINT;
  this->TrainingFraction = 0.1;
  this->Task = MODEL_AND_ASSESS;
  this->SetNumberOfInputPorts( 2 ); // data + optional model
  this->SetNumberOfOutputPorts( 2 ); // model + assessed input
}

vtkSciVizStatistics::~vtkSciVizStatistics()
{
  delete this->P;
}

void vtkSciVizStatistics::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Task: " << this->Task << "\n";
  os << indent << "AttributeMode: " << this->AttributeMode << "\n";
  os << indent << "TrainingFraction: " << this->TrainingFraction << "\n";
}

int vtkSciVizStatistics::GetNumberOfAttributeArrays()
{
  vtkDataObject* dobj = this->GetInputDataObject( 0, 0 ); // First input is always the leader
  if ( ! dobj )
    {
    return 0;
    }

  vtkFieldData* fdata = dobj->GetAttributesAsFieldData( this->AttributeMode );
  if ( ! fdata )
    {
    return 0;
    }

  return fdata->GetNumberOfArrays();
}

const char* vtkSciVizStatistics::GetAttributeArrayName( int n )
{
  vtkDataObject* dobj = this->GetInputDataObject( 0, 0 ); // First input is always the leader
  if ( ! dobj )
    {
    return 0;
    }

  vtkFieldData* fdata = dobj->GetAttributesAsFieldData( this->AttributeMode );
  if ( ! fdata )
    {
    return 0;
    }

  int numArrays = fdata->GetNumberOfArrays();
  if ( n < 0 || n > numArrays )
    {
    return 0;
    }

  vtkAbstractArray* arr = fdata->GetAbstractArray( n );
  if ( ! arr )
    {
    return 0;
    }

  return arr->GetName();
}

int vtkSciVizStatistics::GetAttributeArrayStatus( const char* arrName )
{
  return this->P->Has( arrName ) ? 1 : 0;
}

void vtkSciVizStatistics::SetAttributeArrayStatus( const char* arrName, int stat )
{
  if ( arrName )
    {
    if ( this->P->SetBufferColumnStatus( arrName, stat ) )
      {
      this->Modified();
      }
    }
}

void vtkSciVizStatistics::EnableAttributeArray( const char* arrName )
{
  if ( arrName )
    {
    if ( this->P->SetBufferColumnStatus( arrName, 1 ) )
      {
      this->Modified();
      }
    }
}

void vtkSciVizStatistics::ClearAttributeArrays()
{
  if ( this->P->ResetBuffer() )
    {
    this->Modified();
    }
}

int vtkSciVizStatistics::FillInputPortInformation( int port, vtkInformation* info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject" );
  if ( port == 0 )
    {
    return 1;
    }
  else if ( port >= 1 && port <= 2 )
    {
    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1 );
    return 1;
    }
  return 0;
}

int vtkSciVizStatistics::FillOutputPortInformation( int port, vtkInformation* info )
{
  if ( port == 0 )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), this->GetModelDataTypeName() );
    return 1;
    }
  else if ( port == 1 )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject" );
    return 1;
    }
  return 0;
}

int vtkSciVizStatistics::ProcessRequest( vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output )
{
  if ( request && request->Has( vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT() ) )
    {
    return this->RequestDataObject( request, input, output );
    }
  return this->Superclass::ProcessRequest( request, input, output );
}

int vtkSciVizStatistics::RequestDataObject(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** input, vtkInformationVector* output )
{
  vtkInformation* iinfo = input[0]->GetInformationObject( 0 );
  vtkDataObject* inData = iinfo->Get( vtkDataObject::DATA_OBJECT() );

  vtkInformation* oinfo = output->GetInformationObject( 0 );
  this->RequestModelDataObject( oinfo );

  oinfo = output->GetInformationObject( 1 );
  vtkDataObject* ouData = oinfo->Get( vtkDataObject::DATA_OBJECT() );
  if ( ! ouData || ! ouData->IsA( inData->GetClassName() ) )
    {
    ouData = inData->NewInstance();
    ouData->SetPipelineInformation( oinfo );
    oinfo->Set( vtkDataObject::DATA_OBJECT(), ouData );
    //oinfo->Set( vtkDataObject::DATA_EXTENT_TYPE(), ouData->GetExtentType() );
    ouData->FastDelete();
    this->GetOutputPortInformation( 1 )->Set( vtkDataObject::DATA_EXTENT_TYPE(), ouData->GetExtentType() );
    }
  return 1;
}

int vtkSciVizStatistics::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** input, vtkInformationVector* output )
{
  vtkDataObject* dataObjIn = vtkDataObject::GetData( input[0], 0 );
  if ( ! dataObjIn )
    {
    // Silently ignore missing data.
    return 1;
    }

  vtkFieldData* dataAttrIn = dataObjIn->GetAttributesAsFieldData( this->AttributeMode );
  if ( ! dataAttrIn )
    {
    // Silently ignore missing attributes.
    return 1;
    }

  if ( ! this->P->Buffer.size() )
    {
    // Silently ignore empty requests.
    return 1;
    }

  // Create a table with all the data
  vtkTable* tableIn = vtkTable::New();
  int stat = this->PrepareFullDataTable( tableIn, dataAttrIn );
  if ( stat < 1 )
    { // return an error (stat=0) or success (stat=-1)
    tableIn->FastDelete();
    return -stat;
    }

  // Either create or retrieve the model, depending on the task at hand
  vtkDataObject* modelOut = 0;
  if ( this->Task != ASSESS_INPUT )
    {
    // We are creating a model by fitting the input data
    // Create a table to hold the training data (unless the TrainingFraction is exactly 1.0)
    vtkTable* train = 0;
    vtkIdType N = tableIn->GetNumberOfRows();
    vtkIdType M = this->Task == FULL_STATISTICS ? N : this->GetNumberOfObservationsForTraining( tableIn );
    if ( M == N )
      {
      train = tableIn;
      train->Register( this );
      if ( this->Task != FULL_STATISTICS  && this->TrainingFraction < 1. )
        {
        vtkWarningMacro(
          << "Either TrainingFraction (" << this->TrainingFraction << ") is high enough to include all observations after rounding"
          << " or the minimum number of observations required for training is at least the size of the entire input."
          << " Any assessment will not be able to detect overfitting." );
        }
      }
    else
      {
      train = vtkTable::New();
      this->PrepareTrainingTable( train, tableIn, M );
      }
    stat = this->FitModel( modelOut, output, train ); // creates modelOut

    if ( train )
      {
      train->Delete();
      }
    }
  else
    {
    // We are using an input model specified by the user
    stat = this->FetchModel( modelOut, input[1] ); // retrieves modelOut from input[1]
    // FIXME: Initialize modelOut with the input model or freak out if it's not there.
    }

  if ( stat < 1 )
    { // Exit on failure (0) or early success (-1)
    tableIn->Delete();
    return -stat;
    }

  if ( this->Task != CREATE_MODEL && this->Task != FULL_STATISTICS )
    { // we need to assess the data using the input or the just-created model
    vtkInformation* oinfo = output->GetInformationObject( 1 );
    vtkDataObject* dataObjOut = oinfo->Get( vtkDataObject::DATA_OBJECT() );
    dataObjOut->ShallowCopy( dataObjIn );
    stat = this->AssessData( tableIn, dataObjOut, modelOut );
    }
  tableIn->Delete();

  return stat ? 1 : 0;
}

int vtkSciVizStatistics::PrepareFullDataTable( vtkTable* tableIn, vtkFieldData* dataAttrIn )
{
  vtkstd::set<vtkStdString>::iterator colIt;
  for ( colIt = this->P->Buffer.begin(); colIt != this->P->Buffer.end(); ++ colIt )
    {
    vtkAbstractArray* arr = dataAttrIn->GetAbstractArray( colIt->c_str() );
    if ( arr )
      {
      vtkIdType ntup = arr->GetNumberOfTuples();
      int ncomp = arr->GetNumberOfComponents();
      if ( ncomp > 1 )
        {
        // Create a column in the table for each component of non-scalar arrays requested.
        // FIXME: Should we add a "norm" column when arr is a vtkDataArray? It would make sense.
        vtkstd::vector<vtkAbstractArray*> comps;
        int i;
        for ( i = 0; i < ncomp; ++ i )
          {
          vtksys_ios::ostringstream os;
          os << arr->GetName() << "_" << i;
          vtkAbstractArray* arrCol = vtkAbstractArray::CreateArray( arr->GetDataType() );
          arrCol->SetName( os.str().c_str() );
          arrCol->SetNumberOfComponents( 1 );
          arrCol->SetNumberOfTuples( ntup );
          comps.push_back( arrCol );
          tableIn->AddColumn( arrCol );
          arrCol->FastDelete();
          }
        vtkIdType vidx = 0;
        vtkDataArray* darr = vtkDataArray::SafeDownCast( arr );
        vtkStringArray* sarr = vtkStringArray::SafeDownCast( arr );
        if ( darr )
          {
          for ( i = 0; i < ncomp; ++ i )
            {
            vtkDataArray::SafeDownCast( comps[i] )->CopyComponent( 0, darr, i );
            }
          }
        else if ( sarr )
          {
          vtkstd::vector<vtkStringArray*> scomps;
          for ( i = 0; i < ncomp; ++ i, ++vidx )
            {
            scomps[i] = vtkStringArray::SafeDownCast( comps[i] );
            }
          for ( vtkIdType j = 0; j < ntup; ++ j )
            {
            for ( i = 0; i < ncomp; ++ i, ++vidx )
              {
              scomps[i]->SetValue( j, sarr->GetValue( vidx ) );
              }
            }
          }
        else
          {
          // Inefficient, but works for any array type.
          for ( vtkIdType j = 0; j < ntup; ++ j )
            {
            for ( i = 0; i < ncomp; ++ i, ++vidx )
              {
              comps[i]->InsertVariantValue( j, arr->GetVariantValue( vidx ) );
              }
            }
          }
        }
      else
        {
        tableIn->AddColumn( arr );
        }
      }
    }

  vtkIdType ncols = tableIn->GetNumberOfColumns();
  if ( ncols < 1 )
    {
    tableIn->Delete();
    vtkWarningMacro( "Every requested array wasn't a scalar or wasn't present." )
    return -1;
    }

  return 1;
}

int vtkSciVizStatistics::PrepareTrainingTable( vtkTable* trainingTable, vtkTable* fullDataTable, vtkIdType M )
{
  // FIXME: this should eventually eliminate duplicate points as well as subsample...
  //        but will require the original ugrid/polydata/graph.
  vtkstd::set<vtkIdType> trainRows;
  vtkIdType N = fullDataTable->GetNumberOfRows();
  double frac = static_cast<double>( M ) / static_cast<double>( N );
  for ( vtkIdType i = 0; i < N; ++ i )
    {
    if ( vtkMath::Random() < frac )
      {
      trainRows.insert( i );
      }
    }
  // Now add or subtract entries as required.
  N = N - 1;
  while ( static_cast<vtkIdType>(trainRows.size()) > M )
    {
    vtkIdType rec = static_cast<vtkIdType>( vtkMath::Random( 0, N ) );
    trainRows.erase( rec );
    }
  while ( static_cast<vtkIdType>(trainRows.size()) < M )
    {
    vtkIdType rec = static_cast<vtkIdType>( vtkMath::Random( 0, N ) );
    trainRows.insert( rec );
    }
  // Finally, copy the subset into the training table
  trainingTable->Initialize();
  for ( int i = 0; i < fullDataTable->GetNumberOfColumns(); ++ i )
    {
    vtkAbstractArray* srcCol = fullDataTable->GetColumn( i );
    vtkAbstractArray* dstCol = vtkAbstractArray::CreateArray( srcCol->GetDataType() );
    dstCol->SetName( srcCol->GetName() );
    trainingTable->AddColumn( dstCol );
    dstCol->FastDelete();
    }
  trainingTable->SetNumberOfRows( M );
  vtkVariantArray* row = vtkVariantArray::New();
  vtkIdType dstRow = 0;
  for ( vtkstd::set<vtkIdType>::iterator it = trainRows.begin(); it != trainRows.end(); ++ it, ++ dstRow )
    {
    fullDataTable->GetRow( *it, row );
    trainingTable->SetRow( dstRow, row );
    }
  row->Delete();
  return 1;
}

int vtkSciVizStatistics::RequestModelDataObject( vtkInformation* oinfo )
{
  vtkDataObject* ouData = oinfo->Get( vtkDataObject::DATA_OBJECT() );
  if ( ! ouData || ! ouData->IsA( "vtkTable" ) )
    {
    vtkTable* tab = vtkTable::New();
    tab->SetPipelineInformation( oinfo );
    oinfo->Set( vtkDataObject::DATA_OBJECT(), tab );
    oinfo->Set( vtkDataObject::DATA_EXTENT_TYPE(), tab->GetExtentType() );
    tab->FastDelete();
    }
  return 1;
}

int vtkSciVizStatistics::FetchModel( vtkDataObject*& model, vtkInformationVector* input )
{
  model = vtkDataObject::GetData( input, 0 );
  return 1;
}

vtkIdType vtkSciVizStatistics::GetNumberOfObservationsForTraining( vtkTable* observations )
{
  vtkIdType N = observations->GetNumberOfRows();
  vtkIdType M = static_cast<vtkIdType>( N * this->TrainingFraction );
  return M < 100 ? ( N < 100 ? N : 100 ) : M;
}

