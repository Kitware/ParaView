 /*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSESAMEConversionFilter.h

=========================================================================*/
// .NAME vtkSESAMEConversionFilter
// .SECTION Description

#include "vtkSESAMEConversionFilter.h"

#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"


vtkStandardNewMacro(vtkSESAMEConversionFilter);


//----------------------------------------------------------------------------
vtkSESAMEConversionFilter::vtkSESAMEConversionFilter()
{
  this->VariableConversionNames = vtkSmartPointer<vtkStringArray>::New();
  this->VariableConversionValues = vtkSmartPointer<vtkDoubleArray>::New();
  
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
void vtkSESAMEConversionFilter::SetVariableConversionValues(int i, double value)
{
  this->VariableConversionValues->SetValue(i,value);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSESAMEConversionFilter::SetNumberOfVariableConversionValues(int v)
{
  this->VariableConversionValues->SetNumberOfValues(v);
}

//----------------------------------------------------------------------------
double vtkSESAMEConversionFilter::GetVariableConversionValue(int i)
{
  return this->VariableConversionValues->GetValue(i);
}

//----------------------------------------------------------------------------
void vtkSESAMEConversionFilter::AddVariableConversionNames(char*  value)
{
  this->VariableConversionNames->InsertNextValue(value);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSESAMEConversionFilter::RemoveAllVariableConversionNames()
{
  this->VariableConversionNames->Reset();
  this->Modified();
}

//----------------------------------------------------------------------------
const char * vtkSESAMEConversionFilter::GetVariableConversionName(int i)
{
  return this->VariableConversionNames->GetValue(i).c_str();
}

//----------------------------------------------------------------------------
int vtkSESAMEConversionFilter::RequestData(
                                       vtkInformation *vtkNotUsed(request),
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
    {


    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkPolyData *input = vtkPolyData::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if ( !input ) 
      {
      vtkErrorMacro( << "No input found." );
      return 0;
      }

    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkPointSet *output = vtkPointSet::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));

    vtkSmartPointer<vtkPolyData> localOutput= vtkSmartPointer<vtkPolyData>::New();

    vtkPoints *inPts;

    vtkIdType ptId, numPts;

    localOutput->ShallowCopy(input);
    localOutput->GetPointData()->DeepCopy(input->GetPointData());

    inPts = localOutput->GetPoints();

    numPts = inPts->GetNumberOfPoints();

    vtkIdType numConversionValues = this->VariableConversionValues->GetNumberOfTuples();
    vtkFloatArray* convertArray = NULL;

    double conversion;    
    for(vtkIdType i=0;i<numConversionValues;i++)
      {
      convertArray = vtkFloatArray::SafeDownCast(localOutput->GetPointData()->GetArray(i));
      conversion = this->VariableConversionValues->GetValue(i);
      for(ptId=0;ptId<numPts;ptId++)
        {
        convertArray->SetValue(ptId,convertArray->GetValue(ptId)*conversion);
        }
     }
     
    output->ShallowCopy(localOutput);
    return 1;

    }

//----------------------------------------------------------------------------
void vtkSESAMEConversionFilter::PrintSelf(ostream& os, vtkIndent indent)
    {
    this->Superclass::PrintSelf(os,indent);
    if ( this->VariableConversionNames )
      {
      this->VariableConversionNames->PrintSelf(os,indent);
      }
    if ( this->VariableConversionValues )
      {
      this->VariableConversionValues->PrintSelf(os,indent);
      }
    }
