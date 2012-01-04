/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrismGeometryFilter.cxx


=========================================================================*/
#include "vtkPrismGeometryFilter.h"

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
#include "vtkGlyph3D.h"
#include "vtkCellData.h"
#include "vtkSESAMEReader.h"
#include "vtkUnstructuredGrid.h"

#include <math.h>

vtkStandardNewMacro(vtkPrismGeometryFilter);

class vtkPrismGeometryFilter::MyInternal
{
public:
  vtkGlyph3D *Glyph;
  vtkIdType TableId;
  std::string AxisVarName[3];

  MyInternal()
    {
      this->AxisVarName[0]      = "none";
      this->AxisVarName[1]      = "none";
      this->AxisVarName[2]      = "none";
      this->TableId = -1;
    }
  ~MyInternal()
    {
    } 
};

//----------------------------------------------------------------------------
vtkPrismGeometryFilter::vtkPrismGeometryFilter()
{

  this->Internal = new MyInternal();

  this->SetNumberOfInputPorts(1);
}

void vtkPrismGeometryFilter::SetTable(int tableId)
{
  if(this->Internal->TableId != tableId)
    {
      this->Internal->TableId = tableId;
      this->Modified();
    }
}

int vtkPrismGeometryFilter::GetTable()
{
  return this->Internal->TableId;
}


//----------------------------------------------------------------------------
int vtkPrismGeometryFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{

  vtkInformation *info = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if ( !output ) 
    {
    vtkDebugMacro( << "No output found." );
    return 0;
    }

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( !input ) 
    {
    vtkDebugMacro( << "No input found." );
    return 0;
    }

  vtkIdType cellId, ptId;
  vtkIdType numCells, numPts;
  vtkPointData *inPD  = input->GetPointData();
  vtkCellData  *inCD  = input->GetCellData();
  vtkPointData  *outPD = output->GetPointData();
  int maxCellSize     = input->GetMaxCellSize();
  vtkIdList *cellPts  = NULL;
  double weight       = 0.0;
  double *weights     = NULL;
  double x[3], newX[3];
 
  vtkDebugMacro( << "Mapping point data to new cell center point..." );

  // construct new points at the centers of the cells 
  vtkPoints *newPoints = vtkPoints::New();
  vtkDataArray *inputScalars[3];

  inputScalars[0] = inCD->GetScalars( this->GetXAxisVarName() );
  inputScalars[1] = inCD->GetScalars( this->GetYAxisVarName() );
  inputScalars[2] = inCD->GetScalars( this->GetZAxisVarName() );

  vtkIdType newIDs[1] = {0};
  if ( (numCells=input->GetNumberOfCells()) < 1 )
    {
    vtkDebugMacro(<< "No input cells, nothing to do." );
    return 0;
    }

  weights = new double[maxCellSize];
  cellPts = vtkIdList::New();
  cellPts->Allocate( maxCellSize );

  // Pass cell data (note that this passes current cell data through to the
  // new points that will be created at the cell centers)
  outPD->PassData( inCD );

  // create space for the newly interpolated values
  outPD->CopyAllocate( inPD,numCells );

  int abort=0;
  double funcArgs[3]  = { 0.0, 0.0, 0.0 };
  double newPt[3] = {0.0, 0.0, 0.0};
  vtkIdType progressInterval=numCells/20 + 1;
  output->Allocate( numCells ); 
  for ( cellId=0; cellId < numCells && !abort; cellId++ )
    {
    if ( !(cellId % progressInterval) )
      {
      this->UpdateProgress( (double)cellId/numCells );
      abort = GetAbortExecute();
      }

    input->GetCellPoints( cellId, cellPts );
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
    output->InsertNextCell( VTK_VERTEX, 1, newIDs );
    }

  // pass the new points to the output data, etc.


   double scale[3];
   for (ptId=0; ptId < numPts; ptId++)
      {

      newPoints->GetPoint(ptId, x);

      newX[0] = x[0]*scale[0];
      newX[1] = x[1]*scale[1];
      newX[2] = x[2]*scale[2];

      newPoints->SetPoint(ptId, newX);

      }




  output->SetPoints( newPoints );
  newPoints->Delete();
  output->Squeeze();

  cellPts->Delete();
  delete [] weights;
  
  return 1;

}



void vtkPrismGeometryFilter::SetXAxisVarName( const char *name )
{
  this->Internal->AxisVarName[0]=name;
  this->Modified();
}
void vtkPrismGeometryFilter::SetYAxisVarName( const char *name )
{
  this->Internal->AxisVarName[1]=name;
  this->Modified();
}
void vtkPrismGeometryFilter::SetZAxisVarName( const char *name )
{
  this->Internal->AxisVarName[2]=name;
  this->Modified();
}
const char * vtkPrismGeometryFilter::GetXAxisVarName()
{
  return this->Internal->AxisVarName[0].c_str();
}
const char * vtkPrismGeometryFilter::GetYAxisVarName()
{
  return this->Internal->AxisVarName[1].c_str();
}
const char * vtkPrismGeometryFilter::GetZAxisVarName()
{
  return this->Internal->AxisVarName[2].c_str();
}

int vtkPrismGeometryFilter::CalculateValues( double *x, double *f )
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
void vtkPrismGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Not Implemented: " << "\n";
  
}




