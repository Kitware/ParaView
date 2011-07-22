/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPrismSurfaceReader.cxx


=========================================================================*/
#include "vtkPrismSurfaceReader.h"
#include "vtkPrismPrivate.h"

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
#include "vtkContourFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkPrismSESAMEReader.h"
#include "vtkSESAMEConversionFilter.h"
#include "vtkSmartPointer.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtractPolyDataGeometry.h"
#include "vtkBox.h"
#include "vtkCleanPolyData.h"
#include <vtkstd/algorithm>
#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/string>
#include "vtkMultiBlockDataSet.h"

#include <math.h>

vtkStandardNewMacro(vtkPrismSurfaceReader);


//---------------------------------------------------
class vtkPrismSurfaceReader::MyInternal
    {
    public:
        vtkSmartPointer<vtkPrismSESAMEReader> Reader;
        vtkSmartPointer<vtkSESAMEConversionFilter> ConversionFilter;

        vtkSmartPointer<vtkPrismSESAMEReader> VaporizationReader;
        vtkSmartPointer<vtkSESAMEConversionFilter> VaporizationConversionFilter;
        vtkSmartPointer<vtkPrismSESAMEReader> ColdReader;
        vtkSmartPointer<vtkSESAMEConversionFilter> ColdConversionFilter;
        vtkSmartPointer<vtkPrismSESAMEReader> SolidMeltReader;
        vtkSmartPointer<vtkSESAMEConversionFilter> SolidMeltConversionFilter;
        vtkSmartPointer<vtkPrismSESAMEReader> LiquidMeltReader;
        vtkSmartPointer<vtkSESAMEConversionFilter> LiquidMeltConversionFilter;



        vtkSmartPointer<vtkContourFilter> ContourFilter;
        vtkSmartPointer<vtkExtractPolyDataGeometry > ExtractGeometry;
        vtkSmartPointer<vtkBox> Box;

        vtkSmartPointer<vtkCleanPolyData> CleanPolyData;
        vtkstd::string AxisVarName[3];
        vtkSmartPointer<vtkStringArray> ArrayNames;

        bool ArrayLogScaling[3];
        bool ShowCold;
        bool ShowVaporization;
        bool ShowSolidMelt;
        bool ShowLiquidMelt;

        bool WarpSurface;
        bool DisplayContours;
        int NumberOfContours;
        vtkstd::string  ContourVarName;
        vtkSmartPointer<vtkDoubleArray> XRangeArray;
        vtkSmartPointer<vtkDoubleArray> YRangeArray;
        vtkSmartPointer<vtkDoubleArray> ZRangeArray;
        vtkSmartPointer<vtkDoubleArray> CRangeArray;


        vtkTimeStamp XRangeTime;
        vtkTimeStamp YRangeTime;
        vtkTimeStamp ZRangeTime;

        vtkTimeStamp CRangeTime;

        void Initialize();

        MyInternal()
            {
            this->AxisVarName[0]      = "";
            this->AxisVarName[1]      = "";
            this->AxisVarName[2]      = "";


            this->ArrayLogScaling[0]=false;
            this->ArrayLogScaling[1]=false;
            this->ArrayLogScaling[2]=false;
            this->ShowCold=false;
            this->ShowVaporization=false;
            this->ShowSolidMelt=false;
            this->ShowLiquidMelt=false;


            this->XRangeArray=vtkSmartPointer<vtkDoubleArray>::New();
            this->YRangeArray=vtkSmartPointer<vtkDoubleArray>::New();
             this->ZRangeArray=vtkSmartPointer<vtkDoubleArray>::New();
           this->CRangeArray=vtkSmartPointer<vtkDoubleArray>::New();

            this->XRangeArray->Initialize();
            this->XRangeArray->SetNumberOfComponents(1);
            this->XRangeArray->InsertNextValue(0.0);
            this->XRangeArray->InsertNextValue(0.0);


            this->YRangeArray->Initialize();
            this->YRangeArray->SetNumberOfComponents(1);
            this->YRangeArray->InsertNextValue(0.0);
            this->YRangeArray->InsertNextValue(0.0);

            this->ZRangeArray->Initialize();
            this->ZRangeArray->SetNumberOfComponents(1);
            this->ZRangeArray->InsertNextValue(0.0);
            this->ZRangeArray->InsertNextValue(0.0);


            this->CRangeArray->Initialize();
            this->CRangeArray->SetNumberOfComponents(1);
            this->CRangeArray->InsertNextValue(0.0);
            this->CRangeArray->InsertNextValue(0.0);

            this->ContourFilter=vtkSmartPointer<vtkContourFilter>::New();

            this->Reader =  vtkSmartPointer<vtkPrismSESAMEReader>::New();
            this->ConversionFilter =  vtkSmartPointer<vtkSESAMEConversionFilter>::New();
            this->ConversionFilter->SetInput(this->Reader->GetOutput());

            this->VaporizationReader =  vtkSmartPointer<vtkPrismSESAMEReader>::New();
            this->VaporizationConversionFilter =  vtkSmartPointer<vtkSESAMEConversionFilter>::New();
            this->VaporizationConversionFilter->SetInput(this->VaporizationReader->GetOutput());

            this->ColdReader =  vtkSmartPointer<vtkPrismSESAMEReader>::New();
            this->ColdConversionFilter =  vtkSmartPointer<vtkSESAMEConversionFilter>::New();
            this->ColdConversionFilter->SetInput(this->ColdReader->GetOutput());


            this->SolidMeltReader =  vtkSmartPointer<vtkPrismSESAMEReader>::New();
            this->SolidMeltConversionFilter =  vtkSmartPointer<vtkSESAMEConversionFilter>::New();
            this->SolidMeltConversionFilter->SetInput(this->SolidMeltReader->GetOutput());

            this->LiquidMeltReader =  vtkSmartPointer<vtkPrismSESAMEReader>::New();
            this->LiquidMeltConversionFilter =  vtkSmartPointer<vtkSESAMEConversionFilter>::New();
            this->LiquidMeltConversionFilter->SetInput(this->LiquidMeltReader->GetOutput());

            this->ExtractGeometry=vtkSmartPointer<vtkExtractPolyDataGeometry >::New();

            this->Box=  vtkSmartPointer<vtkBox>::New();
            this->ExtractGeometry->SetImplicitFunction(this->Box);
            this->ExtractGeometry->ExtractInsideOn();
            this->ExtractGeometry->ExtractBoundaryCellsOn();
            this->CleanPolyData=vtkSmartPointer<vtkCleanPolyData>::New();

            this->ArrayNames =vtkSmartPointer<vtkStringArray>::New();
            this->ArrayNames->Initialize();

            this->WarpSurface=true;
            this->DisplayContours=false;
            this->NumberOfContours=1;
            this->ContourVarName="none";

            }
        ~MyInternal()
            {
            } 
    };





//----------------------------------------------------------------------------
vtkPrismSurfaceReader::vtkPrismSurfaceReader()
    {

    this->Internal = new MyInternal();

    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(3);


    this->XThresholdBetween[0]=0.0;
    this->XThresholdBetween[1]=1.0;
    this->YThresholdBetween[0]=0.0;
    this->YThresholdBetween[1]=1.0;

    this->ActualThresholdBounds[0]=0.0;
    this->ActualThresholdBounds[1]=1.0;
    this->ActualThresholdBounds[2]=0.0;
    this->ActualThresholdBounds[3]=1.0;
    this->ActualThresholdBounds[4]=0.0;
    this->ActualThresholdBounds[5]=1.0;



    }
vtkPrismSurfaceReader::~vtkPrismSurfaceReader()
{
  delete this->Internal;
}

unsigned long vtkPrismSurfaceReader::GetMTime()
{
    unsigned long t1 = this->Superclass::GetMTime();
    unsigned long t2 = this->Internal->Reader->GetMTime();
    unsigned long t4 = this->Internal->ConversionFilter->GetMTime();
    unsigned long ret_time = t1 > t2 ? t1 : t2;
//    ret_time= t3 > ret_time ? t3 : ret_time;
    return t4 > ret_time ? t4 : ret_time;
}

void vtkPrismSurfaceReader::MyInternal::Initialize()
    {





    }

void vtkPrismSurfaceReader::SetWarpSurface(bool b)
    {
    if(this->Internal->WarpSurface!=b)
        {
        this->Internal->WarpSurface=b;
        this->Modified();
        }
    }
void vtkPrismSurfaceReader::SetDisplayContours(bool b)
    {
    if(this->Internal->DisplayContours!=b)
        {
        this->Internal->DisplayContours=b;
        this->Internal->ContourFilter->Modified();

        this->Modified();
        }

    }
void vtkPrismSurfaceReader::SetContourVarName( const char *name )
    {
    if(this->Internal->ContourVarName!=name)
        {
        this->Internal->ContourVarName=name;
        this->Internal->ContourFilter->Modified();

        this->Modified();
        }
    }
const char *vtkPrismSurfaceReader::GetContourVarName()
    {

    return this->Internal->ContourVarName.c_str();


    }
vtkDoubleArray* vtkPrismSurfaceReader::GetContourVarRange()
{
    if(this->Internal->CRangeTime<this->GetMTime())
    {
        this->Internal->CRangeTime.Modified();
        this->GetVariableRange(this->GetContourVarName(),this->Internal->CRangeArray);

    }

    return this->Internal->CRangeArray;
}


void vtkPrismSurfaceReader::SetNumberOfContours(int i)
    {
    if(this->Internal->NumberOfContours!=i)
        {
        this->Internal->NumberOfContours=i;
        this->Internal->ContourFilter->SetNumberOfContours(i);
        this->Modified();
        }
    }

void vtkPrismSurfaceReader::SetContourValue(int i, double value)
{
  this->Internal->ContourFilter->SetValue(i,value);
  this->Modified();
}
double vtkPrismSurfaceReader::GetContourValue(int i)
{
    return this->Internal->ContourFilter->GetValue(i);
}
double *vtkPrismSurfaceReader::GetContourValues()
{
    return this->Internal->ContourFilter->GetValues();
}
void vtkPrismSurfaceReader::GetContourValues(double *contourValues)
{
    this->Internal->ContourFilter->GetValues(contourValues);

}


void vtkPrismSurfaceReader::SetVariableConversionValues(int i, double value)
{
  this->Internal->ConversionFilter->SetVariableConversionValues(i,value);
    this->Modified();
}
void vtkPrismSurfaceReader::SetNumberOfVariableConversionValues(int v)
{
  this->Internal->ConversionFilter->SetNumberOfVariableConversionValues(v);
}
double vtkPrismSurfaceReader::GetVariableConversionValue(int i)
{
  return this->Internal->ConversionFilter->GetVariableConversionValue(i);
}

void vtkPrismSurfaceReader::AddVariableConversionNames(char*  value)
{
  this->Internal->ConversionFilter->AddVariableConversionNames(value);
    this->Modified();
}
void vtkPrismSurfaceReader::RemoveAllVariableConversionNames()
{
  this->Internal->ConversionFilter->RemoveAllVariableConversionNames();
    this->Modified();
}
const char * vtkPrismSurfaceReader::GetVariableConversionName(int i)
{
  return this->Internal->ConversionFilter->GetVariableConversionName(i);
}
void vtkPrismSurfaceReader::SetShowCold(bool b)
{
  this->Internal->ShowCold=b;
  this->Modified();

}
void vtkPrismSurfaceReader::SetShowVaporization(bool b)
{
  this->Internal->ShowVaporization=b;
  this->Modified();

}
void vtkPrismSurfaceReader::SetShowSolidMelt(bool b)
{
  this->Internal->ShowSolidMelt=b;
  this->Modified();
}
void vtkPrismSurfaceReader::SetShowLiquidMelt(bool b)
{
  this->Internal->ShowLiquidMelt=b;
  this->Modified();
}
bool vtkPrismSurfaceReader::GetShowCold()
{
  return this->Internal->ShowCold;
}
bool vtkPrismSurfaceReader::GetShowVaporization()
{
  return this->Internal->ShowVaporization;
}
bool vtkPrismSurfaceReader::GetShowSolidMelt()
{
  return this->Internal->ShowSolidMelt;

}
bool vtkPrismSurfaceReader::GetShowLiquidMelt()
{
  return this->Internal->ShowLiquidMelt;
}



void vtkPrismSurfaceReader::SetXLogScaling(bool b)
    {
    this->Internal->ArrayLogScaling[0]=b;
    this->Modified();
    }
void vtkPrismSurfaceReader::SetYLogScaling(bool b)
    {
    this->Internal->ArrayLogScaling[1]=b;
    this->Modified();
    }
void vtkPrismSurfaceReader::SetZLogScaling(bool b)
    {
    this->Internal->ArrayLogScaling[2]=b;
    this->Modified();
    }
bool vtkPrismSurfaceReader::GetXLogScaling()
    {
    return   this->Internal->ArrayLogScaling[0];

    }
bool vtkPrismSurfaceReader::GetYLogScaling()
    {
    return   this->Internal->ArrayLogScaling[1];
    }
bool vtkPrismSurfaceReader::GetZLogScaling()
    {
    return   this->Internal->ArrayLogScaling[2];
    }


bool vtkPrismSurfaceReader::GetVariableRange (const char *varName,vtkDoubleArray* rangeArray)
    {
    rangeArray->Initialize();
    rangeArray->SetNumberOfComponents(1);
    rangeArray->SetNumberOfValues(2);
    vtkStdString str=varName;

    if(!this->Internal->Reader->IsValidFile() || this->Internal->Reader->GetTable()==-1)
    {
        rangeArray->InsertValue(0,0.0);
        rangeArray->InsertValue(1,0.0);
        return false;

    }
    this->Internal->ConversionFilter->Update();
    vtkIdType numArrays= this->Internal->ConversionFilter->GetOutput()->GetPointData()->GetNumberOfArrays();
    vtkSmartPointer<vtkFloatArray> xArray;
    for(int i=0;i<numArrays;i++)
    {
      vtkStdString name=this->Internal->ConversionFilter->GetOutput()->GetPointData()->GetArrayName(i);
      if(name==str)
      {
        xArray= vtkFloatArray::SafeDownCast(this->Internal->ConversionFilter->GetOutput()->GetPointData()->GetArray(i));
        break;
      }
    }

    if(xArray)
    {
      double temp[2];
      xArray->GetRange(temp);
      rangeArray->InsertValue(0,xArray->GetValueRange()[0]);
      rangeArray->InsertValue(1,xArray->GetValueRange()[1]);
      return true;
    }
    else
    {
      rangeArray->InsertValue(0,0.0);
      rangeArray->InsertValue(1,0.0);
      return false;
    }
}



vtkDoubleArray* vtkPrismSurfaceReader::GetXRange ()
    {
        if(!this->Internal->Reader->IsValidFile())
        {
            return this->Internal->XRangeArray;
        }

        if(this->Internal->XRangeTime<this->GetMTime())
        {
        this->Internal->XRangeTime.Modified();

        this->GetVariableRange(this->GetXAxisVarName(),this->Internal->XRangeArray);
        }

    return this->Internal->XRangeArray;
    }

vtkDoubleArray* vtkPrismSurfaceReader::GetYRange ()
    {

        if(!this->Internal->Reader->IsValidFile())
        {
            return this->Internal->YRangeArray;
        }

    if(this->Internal->YRangeTime<this->GetMTime())
        {
        this->Internal->YRangeTime.Modified();
        this->GetVariableRange(this->GetYAxisVarName(),this->Internal->YRangeArray);
        }

    return this->Internal->YRangeArray;

    }




vtkDoubleArray* vtkPrismSurfaceReader::GetZRange ()
    {
        if(!this->Internal->Reader->IsValidFile())
        {
            return this->Internal->XRangeArray;
        }   
        
        if(this->Internal->ZRangeTime<this->GetMTime())
        {
        this->Internal->ZRangeTime.Modified();
        this->GetVariableRange(this->GetZAxisVarName(),this->Internal->ZRangeArray);
        }

    return this->Internal->ZRangeArray;
    }




void vtkPrismSurfaceReader::GetRanges(vtkDoubleArray* RangeArray)
{

    vtkSmartPointer<vtkDoubleArray> range=vtkSmartPointer<vtkDoubleArray>::New();
    range->Initialize();
    range->SetNumberOfComponents(1);

    range=this->GetXRange();
    RangeArray->InsertValue(0,range->GetValue(0));
    RangeArray->InsertValue(1,range->GetValue(1));
    
    range=this->GetYRange();
    RangeArray->InsertValue(2,range->GetValue(0));
    RangeArray->InsertValue(3,range->GetValue(1));

    range=this->GetZRange();
    RangeArray->InsertValue(4,range->GetValue(0));
    RangeArray->InsertValue(5,range->GetValue(1));
}


double *vtkPrismSurfaceReader::GetXThresholdBetween() 
    { 
    return this->XThresholdBetween; 
    } 
void vtkPrismSurfaceReader::GetXThresholdBetween (double &_arg1, double &_arg2) 
    { 
    _arg1 = this->XThresholdBetween[0]; 
    _arg2 = this->XThresholdBetween[1]; 
    } 
void vtkPrismSurfaceReader::GetXThresholdBetween (double _arg[2]) 
    { 
    this->GetXThresholdBetween (_arg[0], _arg[1]);
    } 

void vtkPrismSurfaceReader::SetThresholdXBetween(double lower, double upper)
    {
    this->XThresholdBetween[0]=lower;
    this->XThresholdBetween[1]=upper;
    this->Modified();
    }
void vtkPrismSurfaceReader::SetThresholdYBetween(double lower, double upper)
    {
    this->YThresholdBetween[0]=lower;
    this->YThresholdBetween[1]=upper;
    this->Modified();

    }



vtkStringArray* vtkPrismSurfaceReader::GetAxisVarNames()
{
  this->Internal->ArrayNames->Reset();
  int numberArrayNames=this->Internal->Reader->GetNumberOfTableArrayNames();
  for(int i=0;i<numberArrayNames;i++)
        {
        vtkStdString str=this->Internal->Reader->GetTableArrayName(i);
        vtkStdString::size_type pos=str.find_first_of(":");
        if(pos!=vtkStdString::npos)
          {
          str.erase(0,pos+2);
          }
        this->Internal->ArrayNames->InsertNextValue(str);

        }
    return this->Internal->ArrayNames;
    }


void vtkPrismSurfaceReader::SetXAxisVarName( const char *name )
    {
    if(this->Internal->AxisVarName[0]!=name)
        {
        this->Internal->AxisVarName[0]=name;
        this->Modified();  

        }
    }
void vtkPrismSurfaceReader::SetYAxisVarName( const char *name )
    {
    if(this->Internal->AxisVarName[1]!=name)
        {

        this->Internal->AxisVarName[1]=name;
        this->Modified();

        }

    }
void vtkPrismSurfaceReader::SetZAxisVarName( const char *name )
    {
    if(this->Internal->AxisVarName[2]!=name)
        {
        this->Internal->AxisVarName[2]=name;
        this->Modified();
        }
    }
const char * vtkPrismSurfaceReader::GetXAxisVarName()
    {
    return this->Internal->AxisVarName[0].c_str();
    }
const char * vtkPrismSurfaceReader::GetYAxisVarName()
    {
    return this->Internal->AxisVarName[1].c_str();
    }
const char * vtkPrismSurfaceReader::GetZAxisVarName()
    {
    return this->Internal->AxisVarName[2].c_str();
    }




int vtkPrismSurfaceReader::IsValidFile()
    {
    if(!this->Internal->Reader)
        {
        return 0;
        }

    return this->Internal->Reader->IsValidFile();

    }

void vtkPrismSurfaceReader::SetFileName(const char* file)
    {
    if(!this->Internal->Reader)
        {
        return;
        }

    this->Internal->Reader->SetFileName(file);
  //  this->Internal->Reader->Update();
    this->Modified();
    }

const char* vtkPrismSurfaceReader::GetFileName()
    {
    if(!this->Internal->Reader)
        {
        return NULL;
        }
    return this->Internal->Reader->GetFileName();
    }



int vtkPrismSurfaceReader::GetNumberOfTableIds()
    {
    if(!this->Internal->Reader)
        {
        return 0;
        }

    return this->Internal->Reader->GetNumberOfTableIds();
    }

int* vtkPrismSurfaceReader::GetTableIds()
    {
    if(!this->Internal->Reader)
        {
        return NULL;
        }

    return this->Internal->Reader->GetTableIds();
    }

vtkIntArray* vtkPrismSurfaceReader::GetTableIdsAsArray()
    {
    if(!this->Internal->Reader)
        {
        return NULL;
        }

    return this->Internal->Reader->GetTableIdsAsArray();
    }

void vtkPrismSurfaceReader::SetTable(int tableId)
    {
    if(!this->Internal->Reader)
        {
        return ;
        }

    if(this->Internal->Reader->GetTable() != tableId)
        {
        this->Internal->Reader->SetTable(tableId);
        }
    }

int vtkPrismSurfaceReader::GetTable()
    {
    if(!this->Internal->Reader)
        {
        return 0;
        }

    return this->Internal->Reader->GetTable();
    }

int vtkPrismSurfaceReader::GetNumberOfTableArrayNames()
    {
    if(!this->Internal->Reader)
        {
        return 0;
        }

    return this->Internal->Reader->GetNumberOfTableArrayNames();
    }

const char* vtkPrismSurfaceReader::GetTableArrayName(int index)
    {
    if(!this->Internal->Reader)
        {
        return NULL;
        }

    return this->Internal->Reader->GetTableArrayName(index);

    }

void vtkPrismSurfaceReader::SetTableArrayToProcess(const char* name)
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

const char* vtkPrismSurfaceReader::GetTableArrayNameToProcess()
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


void vtkPrismSurfaceReader::SetTableArrayStatus(const char* name, int flag)
    {
    if(!this->Internal->Reader)
        {
        return ;
        }

    return this->Internal->Reader->SetTableArrayStatus(name , flag);
    }

int vtkPrismSurfaceReader::GetTableArrayStatus(const char* name)
    {
    if(!this->Internal->Reader)
        {
        return 0 ;
        }
    return this->Internal->Reader->GetTableArrayStatus(name);

    }




//----------------------------------------------------------------------------
int vtkPrismSurfaceReader::RequestData(
                                       vtkInformation *vtkNotUsed(request),
                                       vtkInformationVector **vtkNotUsed(inputVector),
                                       vtkInformationVector *outputVector)
{

  this->Internal->ConversionFilter->Update();
  // get the info objects

  vtkInformation *surfaceOutInfo = outputVector->GetInformationObject(0);
  vtkPointSet *surfaceOutput = vtkPointSet::SafeDownCast(
    surfaceOutInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *curveOutInfo = outputVector->GetInformationObject(1);
  vtkPointSet *curveOutput = vtkPointSet::SafeDownCast(
    curveOutInfo->Get(vtkDataObject::DATA_OBJECT()));


  vtkInformation *contourOutInfo = outputVector->GetInformationObject(2);
  vtkPointSet *contourOutput = vtkPointSet::SafeDownCast(
    contourOutInfo->Get(vtkDataObject::DATA_OBJECT()));




  vtkSmartPointer<vtkPolyData> localOutput= vtkSmartPointer<vtkPolyData>::New();

  vtkPointSet *input = this->Internal->ConversionFilter->GetOutput();

  vtkPoints *inPts;

  vtkIdType ptId, numPts;



  localOutput->ShallowCopy(input);

  inPts = input->GetPoints();

  numPts = inPts->GetNumberOfPoints();

  vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
  newPts->SetNumberOfPoints(numPts);
  localOutput->SetPoints(newPts);


  vtkSmartPointer<vtkFloatArray> xArray;
  vtkSmartPointer<vtkFloatArray> yArray;
  vtkSmartPointer<vtkFloatArray> zArray;

  vtkIdType numArrays=input->GetPointData()->GetNumberOfArrays();

  bool xFound=false;
  bool yFound=false;
  bool zFound=false;
  for(int i=0;i<numArrays;i++)
  {
    vtkStdString name=localOutput->GetPointData()->GetArrayName(i);
    vtkStdString::size_type pos=name.find_first_of(":");
    if(pos!=vtkStdString::npos)
    {
      name.erase(0,pos+2);
    }
    if(name==this->GetXAxisVarName())
    {
      xArray= vtkFloatArray::SafeDownCast(localOutput->GetPointData()->GetArray(i));
      xFound=true;
    }

    if(name==this->GetYAxisVarName())
    {
      yArray= vtkFloatArray::SafeDownCast(localOutput->GetPointData()->GetArray(i));
      yFound=true;
    }

    if(this->Internal->WarpSurface)
    {
      if(name==this->GetZAxisVarName())
      {
        zArray= vtkFloatArray::SafeDownCast(localOutput->GetPointData()->GetArray(i));
        zFound=true;
      }
    }

    if(xFound && yFound && zFound)
    {
      break;
    }
  }

  if(!(xFound && yFound))
  {
    vtkDebugMacro( << "SESAME arrays not found" );
    return 0;
  }
  int tID=this->GetTable();
  bool scalingEnabled[3] = {this->GetXLogScaling(),
                              this->GetYLogScaling(),
                              this->GetZLogScaling()};
  for(ptId=0;ptId<numPts;ptId++)
  {

    if ( ! (ptId % 10000) )
    {
      this->UpdateProgress ((double)ptId/numPts);
      if (this->GetAbortExecute())
      {
        break;
      }
    }


    double coords[3];
    coords[0] = (xArray) ? xArray->GetValue(ptId) : 0.0;
    coords[1] = (yArray) ? yArray->GetValue(ptId) : 0.0;
    coords[2] = (zArray) ? zArray->GetValue(ptId) : 0.0;
    vtkPrismCommon::scalePoint(coords,scalingEnabled,tID);

    newPts->InsertPoint(ptId,coords);

  }

  double bounds[6];
  localOutput->GetBounds(bounds);

  if(!this->Internal->WarpSurface)
  {
    bounds[4]=-10;
    bounds[5]=10;
  }

  //scale the threshold numbers
  vtkPrismCommon::scaleThresholdBounds(scalingEnabled,tID,
    this->XThresholdBetween, this->YThresholdBetween,
    this->ActualThresholdBounds);
  this->ActualThresholdBounds[4]=bounds[4];
  this->ActualThresholdBounds[5]=bounds[5];

  this->Internal->ExtractGeometry->SetInput(localOutput);
  this->Internal->Box->SetBounds(this->ActualThresholdBounds);

  this->Internal->CleanPolyData->SetInput( this->Internal->ExtractGeometry->GetOutput());

  this->Internal->CleanPolyData->Update();

  vtkSmartPointer<vtkFloatArray> newXArray= vtkFloatArray::SafeDownCast(this->Internal->CleanPolyData->GetOutput()->GetPointData()->GetArray(xArray->GetName()));
  vtkSmartPointer<vtkFloatArray> newYArray= vtkFloatArray::SafeDownCast(this->Internal->CleanPolyData->GetOutput()->GetPointData()->GetArray(yArray->GetName()));
  vtkSmartPointer<vtkFloatArray> newZArray;
  if(this->Internal->WarpSurface)
  {
    newZArray= vtkFloatArray::SafeDownCast(this->Internal->CleanPolyData->GetOutput()->GetPointData()->GetArray(zArray->GetName()));
  }
 
  surfaceOutput->ShallowCopy(this->Internal->CleanPolyData->GetOutput());


  //add the arrays needed by the custom cube axes representation
  if(newXArray)
  {
    vtkSmartPointer<vtkFloatArray> xRangeArray= vtkSmartPointer<vtkFloatArray>::New();
    xRangeArray->SetNumberOfComponents(1);
    xRangeArray->Allocate(2);
    xRangeArray->SetName("XRange");
    double* rdb=newXArray->GetRange();
    xRangeArray->InsertNextValue(rdb[0]);
    xRangeArray->InsertNextValue(rdb[1]);
    surfaceOutput->GetFieldData()->AddArray(xRangeArray);

    vtkSmartPointer<vtkStringArray> xNameArray = vtkSmartPointer<vtkStringArray>::New();
    xNameArray->SetName("XTitle");
    xNameArray->SetNumberOfValues(1);
    xNameArray->SetValue(0,xArray->GetName());
    surfaceOutput->GetFieldData()->AddArray(xNameArray);
  }
  if(newYArray)
  {
    vtkSmartPointer<vtkFloatArray> yRangeArray= vtkSmartPointer<vtkFloatArray>::New();
    yRangeArray->SetNumberOfComponents(1);
    yRangeArray->Allocate(2);
    yRangeArray->SetName("YRange");
    double* rdb=newYArray->GetRange();
    yRangeArray->InsertNextValue(rdb[0]);
    yRangeArray->InsertNextValue(rdb[1]);
    surfaceOutput->GetFieldData()->AddArray(yRangeArray);

    vtkSmartPointer<vtkStringArray> yNameArray = vtkSmartPointer<vtkStringArray>::New();
    yNameArray->SetName("YTitle");
    yNameArray->SetNumberOfValues(1);
    yNameArray->SetValue(0,yArray->GetName());
    surfaceOutput->GetFieldData()->AddArray(yNameArray);
  }

  if(newZArray)
  {
    vtkSmartPointer<vtkFloatArray> zRangeArray= vtkSmartPointer<vtkFloatArray>::New();
    zRangeArray->SetNumberOfComponents(1);
    zRangeArray->Allocate(2);
    zRangeArray->SetName("ZRange");
    double* rdb=newZArray->GetRange();
    zRangeArray->InsertNextValue(rdb[0]);
    zRangeArray->InsertNextValue(rdb[1]);
    surfaceOutput->GetFieldData()->AddArray(zRangeArray);

    vtkSmartPointer<vtkStringArray> zNameArray = vtkSmartPointer<vtkStringArray>::New();
    zNameArray->SetName("ZTitle");
    zNameArray->SetNumberOfValues(1);
    zNameArray->SetValue(0,zArray->GetName());
    surfaceOutput->GetFieldData()->AddArray(zNameArray);
  }

  
  //add on the flag to the surface, curves and contours that they
  //should be used to compute the world bounds.
  vtkDoubleArray *prismBounds = vtkDoubleArray::New();
  prismBounds->SetName("PRISM_GEOMETRY_BOUNDS");
  prismBounds->SetNumberOfValues(6);
  //we use the surface bounds as we want the properly scaled dataset including log scaling
  localOutput->GetBounds(prismBounds->GetPointer(0)); //copy the bounds into the prismBounds array

  //add on the flag to the surface, curves and contours that they
  //have a thresholded bounds too
  vtkDoubleArray *prismThresholdBounds = vtkDoubleArray::New();
  prismThresholdBounds->SetName("PRISM_THRESHOLD_BOUNDS");
  prismThresholdBounds->SetNumberOfValues(6);
  //copy the thresholded bounds into the prismBounds array
  this->Internal->Box->GetBounds(prismThresholdBounds->GetPointer(0));

  surfaceOutput->GetFieldData()->AddArray(prismBounds);
  surfaceOutput->GetFieldData()->AddArray(prismThresholdBounds);

  curveOutput->GetFieldData()->AddArray(prismBounds);
  curveOutput->GetFieldData()->AddArray(prismThresholdBounds);

  contourOutput->GetFieldData()->AddArray(prismBounds);
  contourOutput->GetFieldData()->AddArray(prismThresholdBounds);

  prismBounds->FastDelete();
  prismThresholdBounds->FastDelete();


  if(this->Internal->DisplayContours)
  {
    vtkIdType numberArrays=this->Internal->CleanPolyData->GetOutput()->GetPointData()->GetNumberOfArrays();

    vtkSmartPointer<vtkFloatArray> cArray;
    for(int i=0;i<numberArrays;i++)
    {
      vtkStdString name=this->Internal->CleanPolyData->GetOutput()->GetPointData()->GetArrayName(i);
      vtkStdString::size_type pos=name.find_first_of(":");
      if(pos!=vtkStdString::npos)
      {
        name.erase(0,pos+2);
      }
      if(name==this->GetContourVarName())
      {
        cArray= vtkFloatArray::SafeDownCast(this->Internal->CleanPolyData->GetOutput()->GetPointData()->GetArray(i));
        break;
      }
    }

    if(cArray)
    {

      this->Internal->ContourFilter->SetInput(this->Internal->CleanPolyData->GetOutput());

      this->Internal->ContourFilter->SetInputArrayToProcess(
        0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,cArray->GetName());
      this->Internal->ContourFilter->Update();      
      contourOutput->ShallowCopy(this->Internal->ContourFilter->GetOutput());

    }
  }
  else
  {
    vtkSmartPointer<vtkPoints> newContourPts = vtkSmartPointer<vtkPoints>::New();
    contourOutput->SetPoints(newContourPts);
  }

  if(tID==301)
  {

    return this->RequestCurveData(curveOutput);
  }
  else
  {
    vtkSmartPointer<vtkPoints> newCurvePts = vtkSmartPointer<vtkPoints>::New();
    curveOutput->SetPoints(newCurvePts);
  }
  return 1;



}

//----------------------------------------------------------------------------
int vtkPrismSurfaceReader::RequestCurveData(  vtkPointSet *curveOutput)
{
  vtkSmartPointer<vtkPolyData> resultPd[5];
  if(this->Internal->ShowVaporization)
  {
    this->Internal->VaporizationReader->SetFileName(this->Internal->Reader->GetFileName());
    vtkIntArray* tableIds=this->Internal->VaporizationReader->GetTableIdsAsArray();

    bool table401Found=false;
    if(tableIds)
    {
      for(int i=0;i<tableIds->GetSize();i++)
      {
        int id=tableIds->GetValue(i);
        if(id==401)
        {
          table401Found=true;
          break;
        }
      }
    }


    if(table401Found)
    {
      this->Internal->VaporizationReader->SetTable(401);

      //Now we determine the conversion factors for each array in the 401 table;
      this->Internal->VaporizationConversionFilter->SetNumberOfVariableConversionValues(8);
      //Pressure
      this->Internal->VaporizationConversionFilter->SetVariableConversionValues(0,this->Internal->ConversionFilter->GetVariableConversionValue(2));
      //Temperature
      this->Internal->VaporizationConversionFilter->SetVariableConversionValues(1,this->Internal->ConversionFilter->GetVariableConversionValue(1));
      //Vapor Density
      this->Internal->VaporizationConversionFilter->SetVariableConversionValues(2,this->Internal->ConversionFilter->GetVariableConversionValue(0));
      //Density of Liquid or Solid
      this->Internal->VaporizationConversionFilter->SetVariableConversionValues(3,this->Internal->ConversionFilter->GetVariableConversionValue(0));
      //Internal Energy of Vapor
      this->Internal->VaporizationConversionFilter->SetVariableConversionValues(4,this->Internal->ConversionFilter->GetVariableConversionValue(3));
      //Internal Energy of Liquid or Solid
      this->Internal->VaporizationConversionFilter->SetVariableConversionValues(5,this->Internal->ConversionFilter->GetVariableConversionValue(3));
      //Free Energy of Vapor
      this->Internal->VaporizationConversionFilter->SetVariableConversionValues(6,this->Internal->ConversionFilter->GetVariableConversionValue(4));
      //Free Energy of Liquid or Solid
      this->Internal->VaporizationConversionFilter->SetVariableConversionValues(7,this->Internal->ConversionFilter->GetVariableConversionValue(4));

      this->Internal->VaporizationConversionFilter->Update();

      vtkSmartPointer<vtkPolyData> input = this->Internal->VaporizationConversionFilter->GetOutput();





      //Next we match the correct 401 array with the correct 301 axis variables.
      vtkstd::map<vtkstd::string,vtkstd::vector<int> > tableMap;
      vtkstd::vector<int> indexes;
      indexes.resize(2);
      //Density
      indexes[0]=2;//Vapor Density on Coexistence Line
      indexes[1]=3;//Density of Liquid or Solid on Coexistence Line
      tableMap[this->Internal->Reader->GetTableArrayName(0)]=indexes;
      //Temperature
      indexes[0]=1;//Temperature
      indexes[1]=1;
      tableMap[this->Internal->Reader->GetTableArrayName(1)]=indexes;
      //Pressure
      indexes[0]=0;//Vapor Pressure
      indexes[1]=0;
      tableMap[this->Internal->Reader->GetTableArrayName(2)]=indexes;
      //Energy
      indexes[0]=4;//Internal Energy of Vapor
      indexes[1]=5;// Internal Energy of Liquid or Solid
      tableMap[this->Internal->Reader->GetTableArrayName(3)]=indexes;
      //Free Energy
      indexes[0]=6;//Free Energy of Vapor
      indexes[1]=7;//Free Energy of Liquid
      tableMap[this->Internal->Reader->GetTableArrayName(4)]=indexes;



      vtkSmartPointer<vtkFloatArray> xArray[2];
      vtkSmartPointer<vtkFloatArray> yArray[2];
      vtkSmartPointer<vtkFloatArray> zArray[2];

      vtkstd::map<vtkstd::string,vtkstd::vector<int> >::iterator iter;

      for(iter=tableMap.begin();iter!=tableMap.end();iter++)
      {
        //First check for pressure.
        vtkstd::string name=iter->first;
        if(name==this->GetXAxisVarName())
        {
          if(iter->second[0]!=-1)
          {
            xArray[0]=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second[0]));
          }
          if(iter->second[1]!=-1)
          {
            xArray[1]=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second[1]));
          }
        }
        else if(name==this->GetYAxisVarName())
        {
          if(iter->second[0]!=-1)
          {
            yArray[0]=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second[0]));
          }
          if(iter->second[1]!=-1)
          {
            yArray[1]=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second[1]));
          }
        }
        else if(this->Internal->WarpSurface)
        {
          if(name==this->GetZAxisVarName())
          {
            if(iter->second[0]!=-1)
            {
              zArray[0]=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second[0]));
            }
            if(iter->second[1]!=-1)
            {
              zArray[1]=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second[1]));
            }
          }
        }
      }


      vtkPoints *inPts;
      vtkIdType ptId, numPts;
      for(int v=0;v<2;v++)
      {
        vtkSmartPointer<vtkPolyData> localOutput= vtkSmartPointer<vtkPolyData>::New();
        localOutput->ShallowCopy(input);

        inPts = input->GetPoints();

        numPts = inPts->GetNumberOfPoints();

        vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
        newPts->SetNumberOfPoints(numPts);
        localOutput->SetPoints(newPts);



        bool scalingEnabled[3] = {this->GetXLogScaling(),
                              this->GetYLogScaling(),
                              this->GetZLogScaling()};
        double coords[3];
        for(ptId=0;ptId<numPts;ptId++)
        {
          coords[0] = (xArray[v]) ? xArray[v]->GetValue(ptId) : 0.0;
          coords[1] = (yArray[v]) ? yArray[v]->GetValue(ptId) : 0.0;
          coords[2] = (zArray[v]) ? zArray[v]->GetValue(ptId) : 0.0;
          vtkPrismCommon::logScale(coords,scalingEnabled);
          
          newPts->InsertPoint(ptId,coords);
        }


        this->Internal->ExtractGeometry->SetInput(localOutput);
        this->Internal->CleanPolyData->SetInput( this->Internal->ExtractGeometry->GetOutput());
        this->Internal->CleanPolyData->Update();

        vtkSmartPointer<vtkPolyData> output= vtkSmartPointer<vtkPolyData>::New();
        output->DeepCopy( this->Internal->CleanPolyData->GetOutput());
        resultPd[v]=output;


       vtkSmartPointer<vtkIntArray> curveNumber=vtkSmartPointer<vtkIntArray>::New();
        curveNumber->SetName("Curve Number");
        curveNumber->SetNumberOfValues(output->GetNumberOfCells());
        output->GetCellData()->AddArray(curveNumber);
        if(v==0)
        {
          curveNumber->FillComponent(0,0);
        }
        else
        {
          curveNumber->FillComponent(0,1);
        }
       vtkSmartPointer<vtkIntArray> curveType=vtkSmartPointer<vtkIntArray>::New();
       curveType->SetNumberOfValues(output->GetNumberOfCells());
        curveType->SetName("Table Number");
        output->GetCellData()->AddArray(curveType);
        curveType->FillComponent(0,401);

       //vtkSmartPointer<vtkStringArray> tableName=vtkSmartPointer<vtkStringArray>::New();
       // tableName->SetName("Table Name");
       // output->GetCellData()->AddArray(tableName);
       // if(v==0)
       // {
       //   tableName->InsertNextValue("Vaporization - Vapor");
       // }
       // else
       // {
       //   tableName->InsertNextValue("Vaporization - Liquid or Solid");
       // }


      }
    }
  }


  if(this->Internal->ShowCold)
  {
    this->Internal->ColdReader->SetFileName(this->Internal->Reader->GetFileName());
    vtkIntArray* tableIds=this->Internal->ColdReader->GetTableIdsAsArray();

    bool table306Found=false;
    if(tableIds)
    {
      for(int i=0;i<tableIds->GetSize();i++)
      {
        int id=tableIds->GetValue(i);
        if(id==306)
        {
          table306Found=true;
          break;
        }
      }
    }


    if(table306Found)
    {
      this->Internal->ColdReader->SetTable(306);

      //Now we determine the conversion factors for each array in the 306 table;
      this->Internal->ColdConversionFilter->SetNumberOfVariableConversionValues(5);
      //Density
      this->Internal->ColdConversionFilter->SetVariableConversionValues(0,this->Internal->ConversionFilter->GetVariableConversionValue(0));
     //Pressure
      this->Internal->ColdConversionFilter->SetVariableConversionValues(1,this->Internal->ConversionFilter->GetVariableConversionValue(2));
      //Energy
      this->Internal->ColdConversionFilter->SetVariableConversionValues(2,this->Internal->ConversionFilter->GetVariableConversionValue(3));
      //Free Energy
      this->Internal->ColdConversionFilter->SetVariableConversionValues(3,this->Internal->ConversionFilter->GetVariableConversionValue(4));

      this->Internal->ColdConversionFilter->Update();

      vtkSmartPointer<vtkPolyData> input = this->Internal->ColdConversionFilter->GetOutput();

      //Next we match the correct 306 array with the correct 301 axis variables.
      vtkstd::map<vtkstd::string,int > tableMap;
      //Density
      tableMap[this->Internal->Reader->GetTableArrayName(0)]=0;
      //Pressure
      tableMap[this->Internal->Reader->GetTableArrayName(1)]=2;
      //Energy
      tableMap[this->Internal->Reader->GetTableArrayName(2)]=3;
      //Free Energy
      tableMap[this->Internal->Reader->GetTableArrayName(3)]=4;



      vtkSmartPointer<vtkFloatArray> xArray;
      vtkSmartPointer<vtkFloatArray> yArray;
      vtkSmartPointer<vtkFloatArray> zArray;

      vtkstd::map<vtkstd::string,int>::iterator iter;

      for(iter=tableMap.begin();iter!=tableMap.end();iter++)
      {
        vtkstd::string name=iter->first;
        if(name==this->GetXAxisVarName())
        {
            xArray=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second));
        }
        else if(name==this->GetYAxisVarName())
        {
            yArray=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second));
        }
        else if(this->Internal->WarpSurface)
        {
          if(name==this->GetZAxisVarName())
          {
              zArray=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second));
          }
        }
      }




      vtkPoints *inPts;
      vtkIdType ptId, numPts;

      vtkSmartPointer<vtkPolyData> localOutput= vtkSmartPointer<vtkPolyData>::New();
      localOutput->ShallowCopy(input);

      inPts = input->GetPoints();

      numPts = inPts->GetNumberOfPoints();


      vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
      newPts->SetNumberOfPoints(numPts);
      localOutput->SetPoints(newPts);

      bool scalingEnabled[3] = {this->GetXLogScaling(),
                              this->GetYLogScaling(),
                              this->GetZLogScaling()};
      double coords[3];
      for(ptId=0;ptId<numPts;ptId++)
        {
        coords[0] = (xArray) ? xArray->GetValue(ptId) : 0.0;
        coords[1] = (yArray) ? yArray->GetValue(ptId) : 0.0;
        coords[2] = (zArray) ? zArray->GetValue(ptId) : 0.0;
        vtkPrismCommon::logScale(coords,scalingEnabled);
          
        newPts->InsertPoint(ptId,coords);
      }




      this->Internal->ExtractGeometry->SetInput(localOutput);
      this->Internal->CleanPolyData->SetInput( this->Internal->ExtractGeometry->GetOutput());
      this->Internal->CleanPolyData->Update();

      vtkSmartPointer<vtkPolyData> output= vtkSmartPointer<vtkPolyData>::New();
      output->DeepCopy( this->Internal->CleanPolyData->GetOutput());
      resultPd[2]=output;



      vtkSmartPointer<vtkIntArray> curveNumber=vtkSmartPointer<vtkIntArray>::New();
      curveNumber->SetNumberOfValues(output->GetNumberOfCells());
      curveNumber->SetName("Curve Number");
      curveNumber->FillComponent(0,2);
      output->GetCellData()->AddArray(curveNumber);

      vtkSmartPointer<vtkIntArray> curveType=vtkSmartPointer<vtkIntArray>::New();
      curveType->SetNumberOfValues(output->GetNumberOfCells());
      curveType->SetName("Table Number");
      curveType->FillComponent(0,306);
      output->GetCellData()->AddArray(curveType);


      //vtkSmartPointer<vtkIntArray> curveNumber=vtkSmartPointer<vtkIntArray>::New();
      //curveNumber->SetName("Curve Number");
      //output->GetCellData()->AddArray(curveNumber);
      //curveNumber->InsertNextValue(2);

      //vtkSmartPointer<vtkIntArray> curveType=vtkSmartPointer<vtkIntArray>::New();
      //curveType->SetName("Table Number");
      //output->GetCellData()->AddArray(curveType);
      //curveType->InsertNextValue(306);

      //vtkSmartPointer<vtkStringArray> tableName=vtkSmartPointer<vtkStringArray>::New();
      //tableName->SetName("Table Name");
      //output->GetCellData()->AddArray(tableName);
      //tableName->InsertNextValue("Cold Curve (No Zero Point)");

    }
  }

  if(this->Internal->ShowSolidMelt)
  {
    this->Internal->SolidMeltReader->SetFileName(this->Internal->Reader->GetFileName());
    vtkIntArray* tableIds=this->Internal->SolidMeltReader->GetTableIdsAsArray();

    bool table411Found=false;
    if(tableIds)
    {
      for(int i=0;i<tableIds->GetSize();i++)
      {
        int id=tableIds->GetValue(i);
        if(id==411)
        {
          table411Found=true;
          break;
        }
      }
    }


    if(table411Found)
    {
      this->Internal->SolidMeltReader->SetTable(411);

      //Now we determine the conversion factors for each array in the 401 table;
      this->Internal->SolidMeltConversionFilter->SetNumberOfVariableConversionValues(5);
      //Density
      this->Internal->SolidMeltConversionFilter->SetVariableConversionValues(0,this->Internal->ConversionFilter->GetVariableConversionValue(0));
      //Temperature
      this->Internal->SolidMeltConversionFilter->SetVariableConversionValues(1,this->Internal->ConversionFilter->GetVariableConversionValue(1));
      //Pressure
      this->Internal->SolidMeltConversionFilter->SetVariableConversionValues(2,this->Internal->ConversionFilter->GetVariableConversionValue(2));
      //Energy
      this->Internal->SolidMeltConversionFilter->SetVariableConversionValues(3,this->Internal->ConversionFilter->GetVariableConversionValue(3));
      //Free Energy
      this->Internal->SolidMeltConversionFilter->SetVariableConversionValues(4,this->Internal->ConversionFilter->GetVariableConversionValue(4));

      this->Internal->SolidMeltConversionFilter->Update();

      vtkSmartPointer<vtkPolyData> input = this->Internal->SolidMeltConversionFilter->GetOutput();

      //Next we match the correct 401 array with the correct 301 axis variables.
      vtkstd::map<vtkstd::string,int > tableMap;
      //Density
      tableMap[this->Internal->Reader->GetTableArrayName(0)]=0;
      //Temperature
      tableMap[this->Internal->Reader->GetTableArrayName(1)]=1;
      //Pressure
      tableMap[this->Internal->Reader->GetTableArrayName(2)]=2;
      //Energy
      tableMap[this->Internal->Reader->GetTableArrayName(3)]=3;
      //Free Energy
      tableMap[this->Internal->Reader->GetTableArrayName(4)]=4;



      vtkSmartPointer<vtkFloatArray> xArray;
      vtkSmartPointer<vtkFloatArray> yArray;
      vtkSmartPointer<vtkFloatArray> zArray;

      vtkstd::map<vtkstd::string,int>::iterator iter;

      for(iter=tableMap.begin();iter!=tableMap.end();iter++)
      {
        vtkstd::string name=iter->first;
        if(name==this->GetXAxisVarName())
        {
            xArray=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second));
        }
        else if(name==this->GetYAxisVarName())
        {
            yArray=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second));
        }
        else if(this->Internal->WarpSurface)
        {
          if(name==this->GetZAxisVarName())
          {
              zArray=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second));
          }
        }
      }




      vtkPoints *inPts;
      vtkIdType ptId, numPts;

      vtkSmartPointer<vtkPolyData> localOutput= vtkSmartPointer<vtkPolyData>::New();
      localOutput->ShallowCopy(input);

      inPts = input->GetPoints();

      numPts = inPts->GetNumberOfPoints();


      vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
      newPts->SetNumberOfPoints(numPts);
      localOutput->SetPoints(newPts);


      bool scalingEnabled[3] = {this->GetXLogScaling(),
                              this->GetYLogScaling(),
                              this->GetZLogScaling()};
      double coords[3];
      for(ptId=0;ptId<numPts;ptId++)
        {
        coords[0] = (xArray) ? xArray->GetValue(ptId) : 0.0;
        coords[1] = (yArray) ? yArray->GetValue(ptId) : 0.0;
        coords[2] = (zArray) ? zArray->GetValue(ptId) : 0.0;
        vtkPrismCommon::logScale(coords,scalingEnabled);
          
        newPts->InsertPoint(ptId,coords);
      }


      this->Internal->ExtractGeometry->SetInput(localOutput);
      this->Internal->CleanPolyData->SetInput( this->Internal->ExtractGeometry->GetOutput());
      this->Internal->CleanPolyData->Update();

      vtkSmartPointer<vtkPolyData> output= vtkSmartPointer<vtkPolyData>::New();
      output->DeepCopy( this->Internal->CleanPolyData->GetOutput());
      resultPd[3]=output;

      vtkSmartPointer<vtkIntArray> curveNumber=vtkSmartPointer<vtkIntArray>::New();
      curveNumber->SetNumberOfValues(output->GetNumberOfCells());
      curveNumber->SetName("Curve Number");
      curveNumber->FillComponent(0,3);
      output->GetCellData()->AddArray(curveNumber);

      vtkSmartPointer<vtkIntArray> curveType=vtkSmartPointer<vtkIntArray>::New();
      curveType->SetNumberOfValues(output->GetNumberOfCells());
      curveType->SetName("Table Number");
      curveType->FillComponent(0,411);
      output->GetCellData()->AddArray(curveType);

      //vtkSmartPointer<vtkStringArray> tableName=vtkSmartPointer<vtkStringArray>::New();
      //tableName->SetName("Table Name");
      //output->GetCellData()->AddArray(tableName);
      //tableName->InsertNextValue("Solid Melt");

    }
  }

  if(this->Internal->ShowLiquidMelt)
  {
    this->Internal->LiquidMeltReader->SetFileName(this->Internal->Reader->GetFileName());
    vtkIntArray* tableIds=this->Internal->LiquidMeltReader->GetTableIdsAsArray();

    bool table412Found=false;
    if(tableIds)
    {
      for(int i=0;i<tableIds->GetSize();i++)
      {
        int id=tableIds->GetValue(i);
        if(id==412)
        {
          table412Found=true;
          break;
        }
      }
    }


    if(table412Found)
    {
      this->Internal->LiquidMeltReader->SetTable(412);

      //Now we determine the conversion factors for each array in the 412 table;
      this->Internal->LiquidMeltConversionFilter->SetNumberOfVariableConversionValues(5);
      //Density
      this->Internal->LiquidMeltConversionFilter->SetVariableConversionValues(0,this->Internal->ConversionFilter->GetVariableConversionValue(0));
      //Temperature
      this->Internal->LiquidMeltConversionFilter->SetVariableConversionValues(1,this->Internal->ConversionFilter->GetVariableConversionValue(1));
      //Pressure
      this->Internal->LiquidMeltConversionFilter->SetVariableConversionValues(2,this->Internal->ConversionFilter->GetVariableConversionValue(2));
      //Energy
      this->Internal->LiquidMeltConversionFilter->SetVariableConversionValues(3,this->Internal->ConversionFilter->GetVariableConversionValue(3));
      //Free Energy
      this->Internal->LiquidMeltConversionFilter->SetVariableConversionValues(4,this->Internal->ConversionFilter->GetVariableConversionValue(4));

      this->Internal->LiquidMeltConversionFilter->Update();

      vtkSmartPointer<vtkPolyData> input = this->Internal->LiquidMeltConversionFilter->GetOutput();

      //Next we match the correct 412 array with the correct 301 axis variables.
      vtkstd::map<vtkstd::string,int > tableMap;
      //Density
      tableMap[this->Internal->Reader->GetTableArrayName(0)]=0;
      //Temperature
      tableMap[this->Internal->Reader->GetTableArrayName(1)]=1;
      //Pressure
      tableMap[this->Internal->Reader->GetTableArrayName(2)]=2;
      //Energy
      tableMap[this->Internal->Reader->GetTableArrayName(3)]=3;
      //Free Energy
      tableMap[this->Internal->Reader->GetTableArrayName(4)]=4;



      vtkSmartPointer<vtkFloatArray> xArray;
      vtkSmartPointer<vtkFloatArray> yArray;
      vtkSmartPointer<vtkFloatArray> zArray;

      vtkstd::map<vtkstd::string,int>::iterator iter;

      for(iter=tableMap.begin();iter!=tableMap.end();iter++)
      {
        vtkstd::string name=iter->first;
        if(name==this->GetXAxisVarName())
        {
            xArray=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second));
        }
        else if(name==this->GetYAxisVarName())
        {
            yArray=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second));
        }
        else if(this->Internal->WarpSurface)
        {
          if(name==this->GetZAxisVarName())
          {
              zArray=vtkFloatArray::SafeDownCast(input->GetPointData()->GetArray(iter->second));
          }
        }
      }




      vtkPoints *inPts;
      vtkIdType ptId, numPts;

      vtkSmartPointer<vtkPolyData> localOutput= vtkSmartPointer<vtkPolyData>::New();
      localOutput->ShallowCopy(input);

      inPts = input->GetPoints();

      numPts = inPts->GetNumberOfPoints();


      vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
      newPts->SetNumberOfPoints(numPts);
      localOutput->SetPoints(newPts);


      bool scalingEnabled[3] = {this->GetXLogScaling(),
                              this->GetYLogScaling(),
                              this->GetZLogScaling()};
      double coords[3];
      for(ptId=0;ptId<numPts;ptId++)
      {
        coords[0] = (xArray) ? xArray->GetValue(ptId) : 0.0;
        coords[1] = (yArray) ? yArray->GetValue(ptId) : 0.0;
        coords[2] = (zArray) ? zArray->GetValue(ptId) : 0.0;
        vtkPrismCommon::logScale(coords,scalingEnabled);

        newPts->InsertPoint(ptId,coords);
      }

      this->Internal->ExtractGeometry->SetInput(localOutput);
      this->Internal->CleanPolyData->SetInput( this->Internal->ExtractGeometry->GetOutput());
      this->Internal->CleanPolyData->Update();

      vtkSmartPointer<vtkPolyData> output= vtkSmartPointer<vtkPolyData>::New();
      output->DeepCopy( this->Internal->CleanPolyData->GetOutput());
      resultPd[4]=output;



      vtkSmartPointer<vtkIntArray> curveNumber=vtkSmartPointer<vtkIntArray>::New();
      curveNumber->SetNumberOfValues(output->GetNumberOfCells());
      curveNumber->SetName("Curve Number");
      curveNumber->FillComponent(0,4);
      output->GetCellData()->AddArray(curveNumber);

      vtkSmartPointer<vtkIntArray> curveType=vtkSmartPointer<vtkIntArray>::New();
      curveType->SetNumberOfValues(output->GetNumberOfCells());
      curveType->SetName("Table Number");
      curveType->FillComponent(0,412);
      output->GetCellData()->AddArray(curveType);


 /*     vtkSmartPointer<vtkIntArray> curveNumber=vtkSmartPointer<vtkIntArray>::New();
      curveNumber->SetName("Curve Number");
      output->GetCellData()->AddArray(curveNumber);
      curveNumber->InsertNextValue(4);
      vtkSmartPointer<vtkIntArray> curveType=vtkSmartPointer<vtkIntArray>::New();
      curveType->SetName("Table Number");
      output->GetCellData()->AddArray(curveType);
      curveType->InsertNextValue(412);

      vtkSmartPointer<vtkStringArray> tableName=vtkSmartPointer<vtkStringArray>::New();
      tableName->SetName("Table Name");
      output->GetCellData()->AddArray(tableName);
      tableName->InsertNextValue("Liquid Melt");*/

    }
  }

  vtkSmartPointer<vtkAppendPolyData> appendPD= vtkSmartPointer<vtkAppendPolyData>::New();
  bool cOutput=false;
  for(int v=0;v<5;v++)
  {
    if(resultPd[v])
    {
      appendPD->AddInput(resultPd[v]);
      cOutput=true;
    }
  }
  if(cOutput)
  {
    appendPD->Update();

    curveOutput->ShallowCopy(appendPD->GetOutput());
  }
  else
  {

    vtkSmartPointer<vtkPoints> newCurvePts = vtkSmartPointer<vtkPoints>::New();
    curveOutput->SetPoints(newCurvePts);
  }
 return 1;
}




int vtkPrismSurfaceReader::RequestInformation(
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
void vtkPrismSurfaceReader::PrintSelf(ostream& os, vtkIndent indent)
    {
    this->Superclass::PrintSelf(os,indent);

    os << indent << "Not Implemented: " << "\n";

    }
