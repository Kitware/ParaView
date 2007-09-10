/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrismSurfaceFilter.cxx


=========================================================================*/
#include "vtkPrismSurfaceFilter.h"

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
#include "vtkSESAMEReader.h"
#include "vtkRectilinearGridGeometryFilter.h"


#include <math.h>

vtkCxxRevisionMacro(vtkPrismSurfaceFilter, "1.2");
vtkStandardNewMacro(vtkPrismSurfaceFilter);

class vtkPrismSurfaceFilter::MyInternal
{
public:
  vtkSESAMEReader *Reader;
  vtkRectilinearGridGeometryFilter *RectGridGeometry;

  MyInternal()
    {
      this->Reader = vtkSESAMEReader::New();
      this->RectGridGeometry = vtkRectilinearGridGeometryFilter::New();

      this->RectGridGeometry->SetInput(this->Reader->GetOutput());
    }
  ~MyInternal()
    {
    } 
};





//----------------------------------------------------------------------------
vtkPrismSurfaceFilter::vtkPrismSurfaceFilter()
{

  this->Internal = new MyInternal();

  this->SetNumberOfInputPorts(0);
}

int vtkPrismSurfaceFilter::IsValidFile()
{
  if(!this->Internal->Reader)
    {
    return 0;
    }
  
  return this->Internal->Reader->IsValidFile();

}

void vtkPrismSurfaceFilter::SetFileName(const char* file)
{
 if(!this->Internal->Reader)
    {
    return;
    }

 this->Internal->Reader->SetFileName(file);
}

const char* vtkPrismSurfaceFilter::GetFileName()
{
  if(!this->Internal->Reader)
    {
    return NULL;
    }
  return this->Internal->Reader->GetFileName();
}
  


int vtkPrismSurfaceFilter::GetNumberOfTableIds()
{
  if(!this->Internal->Reader)
    {
    return 0;
    }

  return this->Internal->Reader->GetNumberOfTableIds();
}

int* vtkPrismSurfaceFilter::GetTableIds()
{
  if(!this->Internal->Reader)
    {
    return NULL;
    }

  return this->Internal->Reader->GetTableIds();
}

vtkIntArray* vtkPrismSurfaceFilter::GetTableIdsAsArray()
{
   if(!this->Internal->Reader)
    {
    return NULL;
    }

   return this->Internal->Reader->GetTableIdsAsArray();
}

void vtkPrismSurfaceFilter::SetTable(int tableId)
{
 if(!this->Internal->Reader)
    {
    return ;
    }

  this->Internal->Reader->SetTable(tableId);
}

int vtkPrismSurfaceFilter::GetTable()
{
 if(!this->Internal->Reader)
    {
    return 0;
    }

 return this->Internal->Reader->GetTable();
}

int vtkPrismSurfaceFilter::GetNumberOfTableArrayNames()
{
 if(!this->Internal->Reader)
    {
    return 0;
    }

 return this->Internal->Reader->GetNumberOfTableArrayNames();
}

const char* vtkPrismSurfaceFilter::GetTableArrayName(int index)
{
 if(!this->Internal->Reader)
    {
    return NULL;
    }

 return this->Internal->Reader->GetTableArrayName(index);

}

void vtkPrismSurfaceFilter::SetTableArrayToProcess(const char* name)
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

const char* vtkPrismSurfaceFilter::GetTableArrayNameToProcess()
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


void vtkPrismSurfaceFilter::SetTableArrayStatus(const char* name, int flag)
{
   if(!this->Internal->Reader)
    {
    return ;
    }

   return this->Internal->Reader->SetTableArrayStatus(name , flag);
}

int vtkPrismSurfaceFilter::GetTableArrayStatus(const char* name)
{
   if(!this->Internal->Reader)
    {
    return 0 ;
    }
      return this->Internal->Reader->GetTableArrayStatus(name);

}


//----------------------------------------------------------------------------
int vtkPrismSurfaceFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{

  this->Internal->RectGridGeometry->Update();
  // get the info objects

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointSet *input = this->Internal->RectGridGeometry->GetOutput();


  vtkPoints *inPts;
  vtkDataArray *inScalars;
  vtkDataArray *outScalars;
  vtkPoints *newPts;
  vtkPointData *pd;
  vtkIdType ptId, numPts;
  double x[3], newX[3];
  double s;
  double bounds[6];
  int tableID;
 
  
  output->CopyStructure( input );

  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());




  inPts = input->GetPoints();
  pd = input->GetPointData();

  numPts = inPts->GetNumberOfPoints();
  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);

 // Loop over all points, adjusting locations
  //

  inScalars = input->GetPointData()->GetArray(this->GetTableArrayNameToProcess());
  outScalars = output->GetPointData()->GetArray(this->GetTableArrayNameToProcess());

  tableID=this->Internal->Reader->GetTable();
  if(tableID==602)
    {
    for (ptId=0; ptId < numPts; ptId++)
      {
      if ( ! (ptId % 10000) ) 
        {
        this->UpdateProgress ((double)ptId/numPts);
        if (this->GetAbortExecute())
          {
          break;
          }
        }

 
      double sca = inScalars->GetComponent(ptId,0);
      s=sca;
      s= s- log10(9.0e9);
      
      inPts->GetPoint(ptId, x);
      
      newX[0]=x[0];
      newX[1]=x[1];
      newX[2]=s;
      newPts->SetPoint(ptId, newX);

      outScalars->SetComponent(ptId,0,s);
      }
    }
  else if(tableID== 301 || tableID == 304)
    {
    for (ptId=0; ptId < numPts; ptId++)
      {
      if ( ! (ptId % 10000) ) 
        {
        this->UpdateProgress ((double)ptId/numPts);
        if (this->GetAbortExecute())
          {
          break;
          }
        }
      inPts->GetPoint(ptId, x);
       s = inScalars->GetComponent(ptId,0);

      newX[0] = x[0];
      newX[1] = x[1];
      newX[2] = s;

      newPts->SetPoint(ptId, newX);
      }
    }
  else
    {
    for (ptId=0; ptId < numPts; ptId++)
      {
      if ( ! (ptId % 10000) ) 
        {
        this->UpdateProgress ((double)ptId/numPts);
        if (this->GetAbortExecute())
          {
          break;
          }
        }
      inPts->GetPoint(ptId, x);

      newX[0] = x[0] ;
      newX[1] = x[1] ;
      newX[2] = x[2] ;

      newPts->SetPoint(ptId, newX);
      }
    }



  newPts->GetBounds(bounds);


  double delta[3] = {
    bounds[1] - bounds[0],
    bounds[3] - bounds[2],
    bounds[5] - bounds[4]
    };

  double smVal = delta[0];
  if ( delta[1] < smVal )
    {
    smVal = delta[1];
    }
  if ( delta[2] < smVal )
    {
    smVal = delta[2];
    }
  if ( smVal != 0.0 )
    {

    double scale[3] = {
      smVal/delta[0],
      smVal/delta[1],
      smVal/delta[2]
      };


    for (ptId=0; ptId < numPts; ptId++)
      {

      newPts->GetPoint(ptId, x);

      newX[0] = x[0]*scale[0];
      newX[1] = x[1]*scale[1];
      newX[2] = x[2]*scale[2];

      newPts->SetPoint(ptId, newX);

      }

    }




 

  // Update ourselves and release memory
  //

  output->SetPoints(newPts);
  newPts->Delete();

  return 1;

}

//----------------------------------------------------------------------------
int vtkPrismSurfaceFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);
  return 1;

}

//----------------------------------------------------------------------------
void vtkPrismSurfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Not Implemented: " << "\n";
  
}




