/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPrismFilter.cxx


=========================================================================*/
#include "vtkPrismFilter.h"

#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"
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
#include <math.h>

vtkCxxRevisionMacro(vtkPrismFilter, "1.8");
vtkStandardNewMacro(vtkPrismFilter);

class vtkPrismFilter::MyInternal
    {
    public:

        vtkPrismSurfaceReader *Reader;
        vtkDoubleArray* RangeArray;
        vtkstd::string AxisVarName[3];
        double Scale[3];
        MyInternal()
            {
            this->RangeArray = vtkDoubleArray::New();

            this->Reader = vtkPrismSurfaceReader::New();
            this->AxisVarName[0]      = "none";
            this->AxisVarName[1]      = "none";
            this->AxisVarName[2]      = "none";


            this->Scale[0]=1.0;
            this->Scale[1]=1.0;
            this->Scale[2]=1.0;
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
    this->SetNumberOfOutputPorts(2);
    }

int vtkPrismFilter::IsValidFile()
    {
    if(!this->Internal->Reader)
        {
        return 0;
        }

    return this->Internal->Reader->IsValidFile();

    }

void vtkPrismFilter::SetFileName(const char* file)
    {
    if(!this->Internal->Reader)
        {
        return;
        }

    this->Internal->Reader->SetFileName(file);
    }

const char* vtkPrismFilter::GetFileName()
    {
    if(!this->Internal->Reader)
        {
        return NULL;
        }
    return this->Internal->Reader->GetFileName();
    }

int vtkPrismFilter::GetNumberOfTableIds()
    {
    if(!this->Internal->Reader)
        {
        return 0;
        }

    return this->Internal->Reader->GetNumberOfTableIds();
    }

int* vtkPrismFilter::GetTableIds()
    {
    if(!this->Internal->Reader)
        {
        return NULL;
        }

    return this->Internal->Reader->GetTableIds();
    }

vtkIntArray* vtkPrismFilter::GetTableIdsAsArray()
    {
    if(!this->Internal->Reader)
        {
        return NULL;
        }

    return this->Internal->Reader->GetTableIdsAsArray();
    }

void vtkPrismFilter::SetTable(int tableId)
    {
    if(!this->Internal->Reader)
        {
        return ;
        }

    this->Internal->Reader->SetTable(tableId);
    this->Modified();
    }

int vtkPrismFilter::GetTable()
    {
    if(!this->Internal->Reader)
        {
        return 0;
        }

    return this->Internal->Reader->GetTable();
    }

int vtkPrismFilter::GetNumberOfTableArrayNames()
    {
    if(!this->Internal->Reader)
        {
        return 0;
        }

    return this->Internal->Reader->GetNumberOfTableArrayNames();
    }

const char* vtkPrismFilter::GetTableArrayName(int index)
    {
    if(!this->Internal->Reader)
        {
        return NULL;
        }

    return this->Internal->Reader->GetTableArrayName(index);

    }

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

void vtkPrismFilter::SetTableArrayStatus(const char* name, int flag)
    {
    if(!this->Internal->Reader)
        {
        return ;
        }

    return this->Internal->Reader->SetTableArrayStatus(name , flag);
    }

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
    vtkstd::string filename=this->Internal->Reader->GetFileName();
    if(filename.empty())
        {
        return 1;
        }

    this->Internal->Reader->Update();

    double *scale=this->Internal->Reader->GetScale();
    this->Internal->Scale[0]=scale[0];
    this->Internal->Scale[1]=scale[1];
    this->Internal->Scale[2]=scale[2];

    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkPointSet *output = vtkPointSet::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));

    vtkPointSet *input= this->Internal->Reader->GetOutput();
    output->ShallowCopy(input);

    return 1;
    }

int vtkPrismFilter::RequestGeometryData(
                                        vtkInformation *vtkNotUsed(request),
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
    {

    if( strcmp(this->GetXAxisVarName(), "none") == 0)
        {
        return 1;
        }



    vtkInformation *info = outputVector->GetInformationObject(1);
    vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
        info->Get(vtkDataObject::DATA_OBJECT()));
    if ( !output ) 
        {
        vtkDebugMacro( << "No output found." );
        return 0;
        }

    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkMultiBlockDataSet *input = vtkMultiBlockDataSet::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if ( !input ) 
        {
        vtkDebugMacro( << "No input found." );
        return 0;
        }


    double weight       = 0.0;
    double *weights     = NULL;
    double x[3], newX[3];
    vtkIdType cellId, ptId;
    vtkIdType numCells, numPts;
    vtkIdList *cellPts  = NULL;
    vtkDataArray *inputScalars[3];

    unsigned int j=0;
    vtkCompositeDataIterator* iter= input->NewIterator();
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
            vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New(); 
            output->SetBlock(j,polydata);
            j++;


            vtkPointData  *outPD = polydata->GetPointData();
            vtkCellData  *outCD = polydata->GetCellData();
            vtkPointData *inPD  = inputData->GetPointData();
            vtkCellData  *inCD  = inputData->GetCellData();
            int maxCellSize     = inputData->GetMaxCellSize();

            vtkDebugMacro( << "Mapping point data to new cell center point..." );

            // construct new points at the centers of the cells 
            vtkPoints *newPoints = vtkPoints::New();

            inputScalars[0] = inCD->GetScalars( this->GetXAxisVarName() );
            inputScalars[1] = inCD->GetScalars( this->GetYAxisVarName() );
            inputScalars[2] = inCD->GetScalars( this->GetZAxisVarName() );

            vtkIdType newIDs[1] = {0};
            if ( (numCells=inputData->GetNumberOfCells()) < 1 )
                {
                vtkDebugMacro(<< "No input cells, nothing to do." );
                return 0;
                }

            weights = new double[maxCellSize];
            cellPts = vtkIdList::New();
            cellPts->Allocate( maxCellSize );

            // Pass cell data (note that this passes current cell data through to the
            // new points that will be created at the cell centers)
            outCD->PassData( inCD );

            // create space for the newly interpolated values
            outPD->CopyAllocate( inPD,numCells );

            int abort=0;
            double funcArgs[3]  = { 0.0, 0.0, 0.0 };
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

                // calculate the position for the new point at the cell center
                funcArgs[0] = inputScalars[0]->GetTuple1( cellId );
                funcArgs[1] = inputScalars[1]->GetTuple1( cellId );
                funcArgs[2] = inputScalars[2]->GetTuple1( cellId );
                this->CalculateValues( funcArgs, newPt );
                newIDs[0] = newPoints->InsertNextPoint( newPt );
                polydata->InsertNextCell( VTK_VERTEX, 1, newIDs );
                }

            // pass the new points to the output data, etc.

            for (ptId=0; ptId < numCells; ptId++)
                {

                newPoints->GetPoint(ptId, x);

                newX[0] = x[0]*this->Internal->Scale[0];
                newX[1] = x[1]*this->Internal->Scale[1];
                newX[2] = x[2]*this->Internal->Scale[2];

                newPoints->SetPoint(ptId, newX);

                }

            polydata->SetPoints( newPoints );
            newPoints->Delete();
            polydata->Squeeze();

            cellPts->Delete();
            delete [] weights;
            }
        }

    iter->Delete();

    return 1;
    }


vtkDoubleArray* vtkPrismFilter::GetRanges()
    {
    double *r=this->Internal->Reader->GetRange();
    this->Internal->RangeArray->Initialize();
    this->Internal->RangeArray->SetNumberOfComponents(1);
    this->Internal->RangeArray->InsertNextValue(r[0]);
    this->Internal->RangeArray->InsertNextValue(r[1]);
    this->Internal->RangeArray->InsertNextValue(r[2]);
    this->Internal->RangeArray->InsertNextValue(r[3]);
    this->Internal->RangeArray->InsertNextValue(r[4]);
    this->Internal->RangeArray->InsertNextValue(r[5]);





    return this->Internal->RangeArray;
    }



void vtkPrismFilter::SetXAxisVarName( const char *name )
    {
    this->Internal->AxisVarName[0]=name;
    this->Modified();
    }
void vtkPrismFilter::SetYAxisVarName( const char *name )
    {
    this->Internal->AxisVarName[1]=name;
    this->Modified();
    }
void vtkPrismFilter::SetZAxisVarName( const char *name )
    {
    this->Internal->AxisVarName[2]=name;
    this->Modified();
    }
const char * vtkPrismFilter::GetXAxisVarName()
    {
    return this->Internal->AxisVarName[0].c_str();
    }
const char * vtkPrismFilter::GetYAxisVarName()
    {
    return this->Internal->AxisVarName[1].c_str();
    }
const char * vtkPrismFilter::GetZAxisVarName()
    {
    return this->Internal->AxisVarName[2].c_str();
    }

int vtkPrismFilter::CalculateValues( double *x, double *f )
    {
    // convert units
    int retVal = 1; 

    // only performing this for table 602
    if ( this->GetTable() == 602 )
        {
        for ( int i=0;i<3; i++ )
            {
            if ( x[i] <= 0.0 )
                {
                x[i] = 0.0;
                }
            else 
                {
                switch ( i )
                    {
                    case 0:
                        f[i] = log10( x[i]/1.0e3 );
                        break;
                    case 1:
                        f[i] = log10( x[i]/11604.5 );
                        break;
                    case 2:
                        f[i] = log10( x[i] );
                        break;
                    }
                }
            }
        }
    else if ( this->GetTable() == 301 || this->GetTable() == 304 )
        {
        for ( int i=0;i<3; i++ )
            {
            switch ( i )
                {
                case 0:
                    f[i] = x[i]/1.0e3;
                    break;
                case 1:
                    f[i]=x[i];
                    break;
                case 2:
                    f[i]=x[i]/1.0e9 ;
                    break;
                }
            }
        }
    else
        {
        for ( int i=0;i<3; i++ )
            {
            f[i]=x[i];
            }
        }
    return retVal;
    }
//----------------------------------------------------------------------------
int vtkPrismFilter::RequestInformation(
                                       vtkInformation *vtkNotUsed(request),
                                       vtkInformationVector **vtkNotUsed(inputVector),
                                       vtkInformationVector *outputVector)
    {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
        -1);

    outInfo = outputVector->GetInformationObject(1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
        -1);

    return 1;
    }

//----------------------------------------------------------------------------
int vtkPrismFilter::ProcessRequest(vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
    {
    // generate the data
    if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
        {
        return this->RequestData(request, inputVector, outputVector);
        }

    if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
        {
        return this->RequestUpdateExtent(request, inputVector, outputVector);
        }

    // execute information
    if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
        {
        return this->RequestInformation(request, inputVector, outputVector);
        }

    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
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
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
        }

    return 1;
    }

//----------------------------------------------------------------------------
int vtkPrismFilter::FillInputPortInformation(
    int port, vtkInformation* info)
    {
    if(port==0)
        {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
        }

    return 1;
    }

//----------------------------------------------------------------------------
int vtkPrismFilter::RequestUpdateExtent(
                                        vtkInformation* vtkNotUsed(request),
                                        vtkInformationVector** inputVector,
                                        vtkInformationVector* vtkNotUsed(outputVector))
    {
    int numInputPorts = this->GetNumberOfInputPorts();
    for (int i=0; i<numInputPorts; i++)
        {
        int numInputConnections = this->GetNumberOfInputConnections(i);
        for (int j=0; j<numInputConnections; j++)
            {
            vtkInformation* inputInfo = inputVector[i]->GetInformationObject(j);
            inputInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
            }
        }
    return 1;
    }

//----------------------------------------------------------------------------
void vtkPrismFilter::PrintSelf(ostream& os, vtkIndent indent)
    {
    this->Superclass::PrintSelf(os,indent);

    os << indent << "Not Implemented: " << "\n";
    }




