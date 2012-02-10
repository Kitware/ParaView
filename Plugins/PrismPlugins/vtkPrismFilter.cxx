/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPrismFilter.cxx


=========================================================================*/
#include "vtkPrismFilter.h"
#include "vtkPrismPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h" 
#include "vtkCellData.h"
#include "vtkPrismSurfaceReader.h"  
#include "vtkUnstructuredGrid.h"
#include "vtkSmartPointer.h"
#include "vtkPoints.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkExtractGeometry.h"
#include "vtkBox.h"
#include "vtkDataObject.h"

#include <math.h>

vtkStandardNewMacro(vtkPrismFilter);

class vtkPrismFilter::MyInternal
{
public:

    bool SimulationDataThreshold;

    vtkSmartPointer<vtkExtractGeometry > ExtractGeometry;
    vtkSmartPointer<vtkBox> Box;
    vtkPrismSurfaceReader *Reader;
    vtkSmartPointer<vtkDoubleArray> RangeArray;
    std::string AxisVarName[3];
    MyInternal()
    {
        this->SimulationDataThreshold=false;
        this->RangeArray = vtkSmartPointer<vtkDoubleArray>::New();
        this->RangeArray->Initialize();
        this->RangeArray->SetNumberOfComponents(1);


        this->Reader = vtkPrismSurfaceReader::New();
        this->AxisVarName[0]      = "none";
        this->AxisVarName[1]      = "none";
        this->AxisVarName[2]      = "none";

        this->ExtractGeometry=vtkSmartPointer<vtkExtractGeometry >::New();

        this->Box=  vtkSmartPointer<vtkBox>::New();
        this->ExtractGeometry->SetImplicitFunction(this->Box);
        this->ExtractGeometry->ExtractInsideOn();
        this->ExtractGeometry->ExtractBoundaryCellsOn();


    }
    ~MyInternal()
    {
        if(this->Reader)
        {
            this->Reader->Delete();
        }
    } 
};

//----------------------------------------------------------------------------
vtkPrismFilter::vtkPrismFilter()
{

    this->Internal = new MyInternal();

    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(4);

}
//----------------------------------------------------------------------------
vtkPrismFilter::~vtkPrismFilter()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSimulationDataThreshold(bool b)
{
    this->Internal->SimulationDataThreshold=b;
    this->Modified();
}

//----------------------------------------------------------------------------
bool vtkPrismFilter::GetSimulationDataThreshold()
{
    return this->Internal->SimulationDataThreshold;
}

//----------------------------------------------------------------------------
unsigned long vtkPrismFilter::GetMTime()
{
  unsigned long time = this->Superclass::GetMTime();
  unsigned long readertime = this->Internal->Reader->GetMTime();
  return time > readertime ? time : readertime;
}

//----------------------------------------------------------------------------
int vtkPrismFilter::IsValidFile()
{
    if(!this->Internal->Reader)
    {
        return 0;
    }

    return this->Internal->Reader->IsValidFile();

}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetFileName(const char* file)
{
    if(!this->Internal->Reader)
    {
        return;
    }

    this->Internal->Reader->SetFileName(file);
}

//----------------------------------------------------------------------------
const char* vtkPrismFilter::GetFileName()
{
    if(!this->Internal->Reader)
    {
        return NULL;
    }
    return this->Internal->Reader->GetFileName();
}

//----------------------------------------------------------------------------
int vtkPrismFilter::GetNumberOfTableIds()
{
    if(!this->Internal->Reader)
    {
        return 0;
    }

    return this->Internal->Reader->GetNumberOfTableIds();
}

//----------------------------------------------------------------------------
int* vtkPrismFilter::GetTableIds()
{
    if(!this->Internal->Reader)
    {
        return NULL;
    }

    return this->Internal->Reader->GetTableIds();
}

//----------------------------------------------------------------------------
vtkIntArray* vtkPrismFilter::GetTableIdsAsArray()
{
    if(!this->Internal->Reader)
    {
        return NULL;
    }

    return this->Internal->Reader->GetTableIdsAsArray();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetTable(int tableId)
{
    if(!this->Internal->Reader)
    {
        return ;
    }

    this->Internal->Reader->SetTable(tableId);
}

//----------------------------------------------------------------------------
int vtkPrismFilter::GetTable()
{
    if(!this->Internal->Reader)
    {
        return 0;
    }

    return this->Internal->Reader->GetTable();
}

//----------------------------------------------------------------------------
int vtkPrismFilter::GetNumberOfTableArrayNames()
{
    if(!this->Internal->Reader)
    {
        return 0;
    }

    return this->Internal->Reader->GetNumberOfTableArrayNames();
}

//----------------------------------------------------------------------------
const char* vtkPrismFilter::GetTableArrayName(int index)
{
    if(!this->Internal->Reader)
    {
        return NULL;
    }

    return this->Internal->Reader->GetTableArrayName(index);

}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetTableArrayToProcess(const char* name)
{
    if(!this->Internal->Reader)
    {
        return ;
    }


    int numberOfArrays=this->Internal->Reader->GetNumberOfTableArrayNames();
    for(int i=0;i<numberOfArrays;i++)
    {
        this->Internal->Reader->SetTableArrayStatus(this->Internal->Reader->GetTableArrayName(i), 0);
    }
    this->Internal->Reader->SetTableArrayStatus(name, 1);

    this->SetInputArrayToProcess(
        0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
        name );
}

//----------------------------------------------------------------------------
const char* vtkPrismFilter::GetTableArrayNameToProcess()
{
    int numberOfArrays;
    int i;

    numberOfArrays=this->Internal->Reader->GetNumberOfTableArrayNames();
    for(i=0;i<numberOfArrays;i++)
    {
        if(this->Internal->Reader->GetTableArrayStatus(this->Internal->Reader->GetTableArrayName(i)))
        {
            return this->Internal->Reader->GetTableArrayName(i);
        }
    }

    return NULL;
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetTableArrayStatus(const char* name, int flag)
{
    if(!this->Internal->Reader)
    {
        return ;
    }

    return this->Internal->Reader->SetTableArrayStatus(name , flag);
}

//----------------------------------------------------------------------------
int vtkPrismFilter::GetTableArrayStatus(const char* name)
{
    if(!this->Internal->Reader)
    {
        return 0 ;
    }
    return this->Internal->Reader->GetTableArrayStatus(name);
}



//----------------------------------------------------------------------------
int vtkPrismFilter::RequestData(
                                vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector)
{
    this->RequestSESAMEData(request, inputVector,outputVector);
    this->RequestGeometryData(request, inputVector,outputVector);

    return 1;
}

//----------------------------------------------------------------------------
int vtkPrismFilter::RequestSESAMEData(
                                      vtkInformation *vtkNotUsed(request),
                                      vtkInformationVector **vtkNotUsed(inputVector),
                                      vtkInformationVector *outputVector)
{
    std::string filename=this->Internal->Reader->GetFileName();
    if(filename.empty())
    {
        return 1;
    }

    this->Internal->Reader->Update();


    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkPointSet *output = vtkPointSet::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));

    vtkPointSet *input= this->Internal->Reader->GetOutput(0);
    output->ShallowCopy(input);

    vtkInformation *curveOutInfo = outputVector->GetInformationObject(1);
    vtkPointSet *curveOutput = vtkPointSet::SafeDownCast(
        curveOutInfo->Get(vtkDataObject::DATA_OBJECT()));

    vtkPointSet *curveInput= this->Internal->Reader->GetOutput(1);
    curveOutput->ShallowCopy(curveInput);


    vtkInformation *contourOutInfo = outputVector->GetInformationObject(2);
    vtkPointSet *contourOutput = vtkPointSet::SafeDownCast(
        contourOutInfo->Get(vtkDataObject::DATA_OBJECT()));

    contourOutput->ShallowCopy(this->Internal->Reader->GetOutput(2));


    //Copy the PRISM_GEOMETRY_BOUNDS and PRISM_THRESHOLD_BOUNDS field data
    //from output data 1 to 3. This way the points have the same bounds key
    //as the surface
    vtkInformation *geomInfo = outputVector->GetInformationObject(3);
    vtkMultiBlockDataSet *geomOutput = vtkMultiBlockDataSet::SafeDownCast(
        geomInfo->Get(vtkDataObject::DATA_OBJECT()));

    //give it the same prism world bounds as the surface
    //this way we don't scale the world with the points which will cause
    //the reference surface to change size on each time step.
    //we want to copy from the input port 1 so we get if the points have any scaling
    geomOutput->GetFieldData()->PassData(output->GetFieldData());

    return 1;
}

//----------------------------------------------------------------------------
int vtkPrismFilter::CreateGeometry(vtkDataSet *inputData,
                                   unsigned int index,
                                   vtkMultiBlockDataSet *output)
{

  double weight       = 0.0;
  double *weights     = NULL;
  vtkIdType cellId, ptId;
  vtkIdType numCells, numPts;
  vtkIdList *cellPts  = NULL;
  vtkDataArray *inputScalars[3];


  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  // construct new points at the centers of the cells
  vtkPoints *newPoints = vtkPoints::New();


  vtkPointData  *outPD = polydata->GetPointData();
  vtkCellData  *outCD = polydata->GetCellData();
  vtkPointData *inPD  = inputData->GetPointData();
  vtkCellData  *inCD  = inputData->GetCellData();
  int maxCellSize     = inputData->GetMaxCellSize();

  vtkDebugMacro( << "Mapping point data to new cell center point..." );

  bool isCellData[3]={true,true,true};


  inputScalars[0] = inCD->GetScalars( this->GetXAxisVarName() );
  if(!inputScalars[0])
  {
    inputScalars[0] = inPD->GetScalars( this->GetXAxisVarName() );
    if(inputScalars[0])
    {
      isCellData[0]=false;
    }
  }
  inputScalars[1] = inCD->GetScalars( this->GetYAxisVarName() );
  if(!inputScalars[1])
  {
    inputScalars[1] = inPD->GetScalars( this->GetYAxisVarName() );
    if(inputScalars[1])
    {
      isCellData[1]=false;
    }
  }
  inputScalars[2] = inCD->GetScalars( this->GetZAxisVarName() );
  if(!inputScalars[2])
  {
    inputScalars[2] = inPD->GetScalars( this->GetZAxisVarName() );
    if(inputScalars[2])
    {
      isCellData[2]=false;
    }
  }

  vtkIdType newIDs[1] = {0};
  if ( (numCells=inputData->GetNumberOfCells()) < 1 )
  {
    vtkDebugMacro(<< "No input cells, nothing to do." );
    return 0;
  }

  bool scalingEnabled[3] = {this->GetSESAMEXLogScaling(),
                            this->GetSESAMEYLogScaling(),
                            this->GetSESAMEZLogScaling()};

  if(!isCellData[0] && !isCellData[1] && !isCellData[2])
  {
    //All Point Data.
    weights = new double[maxCellSize];
    cellPts = vtkIdList::New();
    cellPts->Allocate( maxCellSize );

    // Pass cell data (note that this passes current cell data through to the
    // new points that will be created at the cell centers)
    outCD->PassData( inCD );

    // create space for the newly interpolated values
    outPD->CopyAllocate( inPD,numCells );

    int abort=0;
    //double funcArgs[3]  = { 0.0, 0.0, 0.0 };
    double newPt[3] = {0.0, 0.0, 0.0};
    vtkIdType progressInterval=numCells/20 + 1;
    polydata->Allocate( numCells );
    for ( cellId=0; cellId < numCells && !abort; cellId++ )
    {
      if ( !(cellId % progressInterval) )
      {
        this->UpdateProgress( (double)cellId/numCells );
        abort = GetAbortExecute();
      }

      inputData->GetCellPoints( cellId, cellPts );
      numPts = cellPts->GetNumberOfIds();
      if ( numPts > 0 )
      {
        weight = 1.0 / numPts;
        for (ptId=0; ptId < numPts; ptId++)
        {
          weights[ptId] = weight;
        }
        outPD->InterpolatePoint(inPD, cellId, cellPts, weights);
      }

      inputScalars[0] = outPD->GetScalars( this->GetXAxisVarName() );
      inputScalars[1] = outPD->GetScalars( this->GetYAxisVarName() );
      inputScalars[2] = outPD->GetScalars( this->GetZAxisVarName() );


      newPt[0] = inputScalars[0]->GetTuple( cellId )[0];
      newPt[1] = inputScalars[1]->GetTuple( cellId )[0];
      newPt[2] = inputScalars[2]->GetTuple( cellId )[0];
      vtkPrismCommon::logScale(newPt,scalingEnabled);
      newIDs[0] = newPoints->InsertNextPoint( newPt );


      polydata->InsertNextCell( VTK_VERTEX, 1, newIDs );
    }
  }
  else if(isCellData[0] && isCellData[1] && isCellData[2])
  {
    //All Cell Data.

    weights = new double[maxCellSize];
    cellPts = vtkIdList::New();
    cellPts->Allocate( maxCellSize );

    // Pass cell data (note that this passes current cell data through to the
    // new points that will be created at the cell centers)
    outCD->PassData( inCD );

    // create space for the newly interpolated values
    outPD->CopyAllocate( inPD,numCells );

    int abort=0;
    //double funcArgs[3]  = { 0.0, 0.0, 0.0 };
    double newPt[3] = {0.0, 0.0, 0.0};
    vtkIdType progressInterval=numCells/20 + 1;
    polydata->Allocate( numCells );
    for ( cellId=0; cellId < numCells && !abort; cellId++ )
    {
      if ( !(cellId % progressInterval) )
      {
        this->UpdateProgress( (double)cellId/numCells );
        abort = GetAbortExecute();
      }

      inputData->GetCellPoints( cellId, cellPts );
      numPts = cellPts->GetNumberOfIds();
      if ( numPts > 0 )
      {
        weight = 1.0 / numPts;
        for (ptId=0; ptId < numPts; ptId++)
        {
          weights[ptId] = weight;
        }
        outPD->InterpolatePoint(inPD, cellId, cellPts, weights);
      }


      newPt[0] = inputScalars[0]->GetTuple( cellId )[0];
      newPt[1] = inputScalars[1]->GetTuple( cellId )[0];
      newPt[2] = inputScalars[2]->GetTuple( cellId )[0];
      vtkPrismCommon::logScale(newPt,scalingEnabled);
      newIDs[0] = newPoints->InsertNextPoint( newPt );
      polydata->InsertNextCell( VTK_VERTEX, 1, newIDs );
    }

  }
  else
  {
    //Mixed input array types between cell and point. We can't handle this right now.
    vtkDebugMacro(<< "Error: Prism can't handle mixed cell and point data" );
    return 0;

  }

  polydata->SetPoints( newPoints );
  newPoints->Delete();
  polydata->Squeeze();
  cellPts->Delete();
  delete [] weights;

  output->SetBlock(index,polydata);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPrismFilter::RequestGeometryData(
                                        vtkInformation *vtkNotUsed(request),
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{

  if( strcmp(this->GetXAxisVarName(), "none") == 0)
    {
    return 1;
    }

  vtkInformation *info = outputVector->GetInformationObject(3);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  if ( !output ) 
    {
    vtkDebugMacro( << "No output found." );
    return 0;
    }

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkMultiBlockDataSet *inputMB = vtkMultiBlockDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (inputMB )
    {
    unsigned int j=0;
    vtkCompositeDataIterator* iter= inputMB->NewIterator();
    iter->SkipEmptyNodesOn();
    iter->TraverseSubTreeOn();
    iter->VisitOnlyLeavesOn();
    iter->GoToFirstItem();
    while(!iter->IsDoneWithTraversal())
      {
      vtkDataSet *inputData=NULL;
      inputData=vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      iter->GoToNextItem();
      if(inputData)
        {
        this->CreateGeometry(inputData,j,output);
        j++;
        }
      }
    iter->Delete();
    return 1;
    }

  vtkDataSet *inputDS = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if(inputDS)
    {
    this->CreateGeometry(inputDS,0,output);
    return 1;
    }
  else
    {
    vtkDebugMacro( << "Incorrect input type." );
    return 0;
    }
  return 1;
}


//----------------------------------------------------------------------------
vtkDoubleArray* vtkPrismFilter::GetRanges()
{
  this->Internal->Reader->GetRanges(this->Internal->RangeArray);

  return this->Internal->RangeArray;
}


//----------------------------------------------------------------------------
void vtkPrismFilter::SetXAxisVarName( const char *name )
{
    this->Internal->AxisVarName[0]=name;
    this->Modified();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetYAxisVarName( const char *name )
{
    this->Internal->AxisVarName[1]=name;
    this->Modified();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetZAxisVarName( const char *name )
{
    this->Internal->AxisVarName[2]=name;
    this->Modified();
}

//----------------------------------------------------------------------------
const char * vtkPrismFilter::GetXAxisVarName()
{
    return this->Internal->AxisVarName[0].c_str();
}

//----------------------------------------------------------------------------
const char * vtkPrismFilter::GetYAxisVarName()
{
    return this->Internal->AxisVarName[1].c_str();
}

//----------------------------------------------------------------------------
const char * vtkPrismFilter::GetZAxisVarName()
{
    return this->Internal->AxisVarName[2].c_str();
}

//----------------------------------------------------------------------------
int vtkPrismFilter::FillOutputPortInformation(
    int port, vtkInformation* info)
{
  if(port==0)
  {
    // now add our info
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  if(port==1)
  {
    // now add our info
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  if(port==2)
  {
    // now add our info
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  if(port==3)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  }

    return 1;
}

//----------------------------------------------------------------------------
int vtkPrismFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}
//----------------------------------------------------------------------------
void vtkPrismFilter::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);

    os << indent << "Not Implemented: " << "\n";
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSESAMEXAxisVarName( const char *name )
{
    this->Internal->Reader->SetXAxisVarName(name);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSESAMEYAxisVarName( const char *name )
{
    this->Internal->Reader->SetYAxisVarName(name);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSESAMEZAxisVarName( const char *name )
{
    this->Internal->Reader->SetZAxisVarName(name);
}

//----------------------------------------------------------------------------
const char* vtkPrismFilter::GetSESAMEXAxisVarName()
{
    return this->Internal->Reader->GetXAxisVarName();
}

//----------------------------------------------------------------------------
const char* vtkPrismFilter::GetSESAMEYAxisVarName()
{
    return this->Internal->Reader->GetYAxisVarName();
}

//----------------------------------------------------------------------------
const char* vtkPrismFilter::GetSESAMEZAxisVarName()
{
    return this->Internal->Reader->GetZAxisVarName();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetShowCold(bool b)
{
  this->Internal->Reader->SetShowCold(b);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetShowVaporization(bool b)
{
  this->Internal->Reader->SetShowVaporization(b);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetShowSolidMelt(bool b)
{
  this->Internal->Reader->SetShowSolidMelt(b);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetShowLiquidMelt(bool b)
{
  this->Internal->Reader->SetShowLiquidMelt(b);
  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkPrismFilter::GetShowCold()
{
  return this->Internal->Reader->GetShowCold();
}

//----------------------------------------------------------------------------
bool vtkPrismFilter::GetShowVaporization()
{
  return this->Internal->Reader->GetShowVaporization();
}

//----------------------------------------------------------------------------
bool vtkPrismFilter::GetShowSolidMelt()
{
  return this->Internal->Reader->GetShowSolidMelt();
}

//----------------------------------------------------------------------------
bool vtkPrismFilter::GetShowLiquidMelt()
{
  return this->Internal->Reader->GetShowLiquidMelt();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSESAMEXLogScaling(bool b)
{
    this->Internal->Reader->SetXLogScaling(b);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSESAMEYLogScaling(bool b)
{
    this->Internal->Reader->SetYLogScaling(b);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSESAMEZLogScaling(bool b)
{
    this->Internal->Reader->SetZLogScaling(b);
}

//----------------------------------------------------------------------------
bool vtkPrismFilter::GetSESAMEXLogScaling()
{
    return this->Internal->Reader->GetXLogScaling();
}

//----------------------------------------------------------------------------
bool vtkPrismFilter::GetSESAMEYLogScaling()
{
    return this->Internal->Reader->GetYLogScaling();
}

//----------------------------------------------------------------------------
bool vtkPrismFilter::GetSESAMEZLogScaling()
{
    return this->Internal->Reader->GetZLogScaling();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSESAMEVariableConversionValues(int i, double value)
{
  this->Internal->Reader->SetVariableConversionValues(i,value);
    this->Modified();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetNumberOfSESAMEVariableConversionValues(int v)
{
  this->Internal->Reader->SetNumberOfVariableConversionValues(v);
}

//----------------------------------------------------------------------------
double vtkPrismFilter::GetSESAMEVariableConversionValue(int i)
{
  return this->Internal->Reader->GetVariableConversionValue(i);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::AddSESAMEVariableConversionNames(char*  value)
{
  this->Internal->Reader->AddVariableConversionNames(value);
    this->Modified();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::RemoveAllSESAMEVariableConversionNames()
{
  this->Internal->Reader->RemoveAllVariableConversionNames();
    this->Modified();
}

//----------------------------------------------------------------------------
const char * vtkPrismFilter::GetSESAMEVariableConversionName(int i)
{
  return this->Internal->Reader->GetVariableConversionName(i);
}

//----------------------------------------------------------------------------
vtkDoubleArray* vtkPrismFilter:: GetSESAMEXRange()
{
    return this->Internal->Reader->GetXRange();
}

//----------------------------------------------------------------------------
vtkDoubleArray* vtkPrismFilter:: GetSESAMEYRange()
{
    return this->Internal->Reader->GetYRange();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetThresholdSESAMEXBetween(double lower, double upper)
{
    this->Internal->Reader->SetThresholdXBetween(lower,upper);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetThresholdSESAMEYBetween(double lower, double upper)
{
    this->Internal->Reader->SetThresholdYBetween(lower,upper);
}

//----------------------------------------------------------------------------
double* vtkPrismFilter::GetSESAMEXThresholdBetween()
{
    return this->Internal->Reader->GetXThresholdBetween();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::GetSESAMEXThresholdBetween (double &_arg1, double &_arg2)
{
    return this->Internal->Reader->GetXThresholdBetween(_arg1,_arg2);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::GetSESAMEXThresholdBetween (double _arg[2])
{
    this->Internal->Reader->GetXThresholdBetween(_arg);
}

//----------------------------------------------------------------------------
double* vtkPrismFilter::GetSESAMEYThresholdBetween()
{
    return this->Internal->Reader->GetYThresholdBetween();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::GetSESAMEYThresholdBetween (double &_arg1, double &_arg2)
{
    return this->Internal->Reader->GetYThresholdBetween(_arg1,_arg2);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::GetSESAMEYThresholdBetween (double _arg[2])
{
    this->Internal->Reader->GetYThresholdBetween(_arg);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetWarpSESAMESurface(bool b)
{
    this->Internal->Reader->SetWarpSurface(b);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetDisplaySESAMEContours(bool b)
{
    this->Internal->Reader->SetDisplayContours(b);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSESAMEContourVarName( const char *name )
{
    this->Internal->Reader->SetContourVarName(name);
}

//----------------------------------------------------------------------------
const char* vtkPrismFilter::GetSESAMEContourVarName()
{
    return this->Internal->Reader->GetContourVarName();
}

//----------------------------------------------------------------------------
vtkDoubleArray* vtkPrismFilter:: GetSESAMEContourVarRange()
{
    return this->Internal->Reader->GetContourVarRange();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetSESAMEContourValue(int i, double value)
{
    this->Internal->Reader->SetContourValue(i,value);
}

//----------------------------------------------------------------------------
double vtkPrismFilter::GetSESAMEContourValue(int i)
{
    return this->Internal->Reader->GetContourValue(i);
}

//----------------------------------------------------------------------------
double* vtkPrismFilter::GetSESAMEContourValues()
{
    return this->Internal->Reader->GetContourValues();
}

//----------------------------------------------------------------------------
void vtkPrismFilter::GetSESAMEContourValues(double *contourValues)
{
    this->Internal->Reader->GetContourValues(contourValues);
}

//----------------------------------------------------------------------------
void vtkPrismFilter::SetNumberOfSESAMEContours(int i)
{
    this->Internal->Reader->SetNumberOfContours(i);
}

//----------------------------------------------------------------------------
vtkStringArray* vtkPrismFilter:: GetSESAMEAxisVarNames()
{
    return this->Internal->Reader->GetAxisVarNames();
}
