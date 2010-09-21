/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPrismSurfaceReader.cxx


=========================================================================*/
#include "vtkPrismSurfaceReader.h"

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
#include "vtkCellData.h"
#include "vtkPrismSESAMEReader.h"
#include "vtkRectilinearGridGeometryFilter.h"
#include "vtkSmartPointer.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtractPolyDataGeometry.h"
#include "vtkBox.h"
#include "vtkCleanPolyData.h"
#include "vtkTransformFilter.h"
#include "vtkTransform.h"
#include <vtkstd/algorithm>
#include "vtkMultiBlockDataSet.h"

#include <math.h>

vtkStandardNewMacro(vtkPrismSurfaceReader);

namespace
{
class vtkSESAMEConversionFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSESAMEConversionFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with initial extent (0,100, 0,100, 0,0) (i.e., a k-plane).
  static vtkSESAMEConversionFilter *New();
  void SetConversions(double density,double temperature,double pressure,double energy);


  void SetVariableConversionValues(int i, double value);
  void SetNumberOfVariableConversionValues(int);
  double GetVariableConversionValue(int i);

  void AddVariableConversionNames( char*  value);
  void RemoveAllVariableConversionNames();
  const char * GetVariableConversionName(int i);







protected:
  vtkSESAMEConversionFilter();
  ~vtkSESAMEConversionFilter() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
 // virtual int FillInputPortInformation(int port, vtkInformation *info);



  vtkSmartPointer<vtkStringArray> VariableConversionNames;
  vtkSmartPointer<vtkDoubleArray> VariableConversionValues;

private:
  vtkSESAMEConversionFilter(const vtkSESAMEConversionFilter&);  // Not implemented.
  void operator=(const vtkSESAMEConversionFilter&);  // Not implemented.
};
}
vtkCxxRevisionMacro(vtkSESAMEConversionFilter, "1.11");
vtkStandardNewMacro(vtkSESAMEConversionFilter);

//----------------------------------------------------------------------------
vtkSESAMEConversionFilter::vtkSESAMEConversionFilter()
{



  this->VariableConversionNames = vtkSmartPointer<vtkStringArray>::New();
  this->VariableConversionValues = vtkSmartPointer<vtkDoubleArray>::New();




    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(1);
}

void vtkSESAMEConversionFilter::SetVariableConversionValues(int i, double value)
{
  this->VariableConversionValues->SetValue(i,value);
  this->Modified();
}
void vtkSESAMEConversionFilter::SetNumberOfVariableConversionValues(int v)
{
  this->VariableConversionValues->SetNumberOfValues(v);
}
double vtkSESAMEConversionFilter::GetVariableConversionValue(int i)
{
  return this->VariableConversionValues->GetValue(i);
}

void vtkSESAMEConversionFilter::AddVariableConversionNames(char*  value)
{
  this->VariableConversionNames->InsertNextValue(value);
    this->Modified();
}
void vtkSESAMEConversionFilter::RemoveAllVariableConversionNames()
{
  this->VariableConversionNames->Reset();
    this->Modified();
}
const char * vtkSESAMEConversionFilter::GetVariableConversionName(int i)
{
  return this->VariableConversionNames->GetValue(i).c_str();
}



//----------------------------------------------------------------------------
void vtkSESAMEConversionFilter::PrintSelf(ostream& os, vtkIndent indent)
    {
    this->Superclass::PrintSelf(os,indent);

    os << indent << "Not Implemented: " << "\n";

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
        vtkDebugMacro( << "No input found." );
        return 0;
    }

    vtkInformation *OutInfo = outputVector->GetInformationObject(0);
    vtkPointSet *Output = vtkPointSet::SafeDownCast(
        OutInfo->Get(vtkDataObject::DATA_OBJECT()));


    vtkSmartPointer<vtkPolyData> localOutput= vtkSmartPointer<vtkPolyData>::New();


    vtkPoints *inPts;
    vtkPointData *pd;

    vtkIdType ptId, numPts;

    localOutput->ShallowCopy(input);
    localOutput->GetPointData()->DeepCopy(input->GetPointData());

    inPts = localOutput->GetPoints();
    pd = localOutput->GetPointData();

    numPts = inPts->GetNumberOfPoints();

   vtkIdType numArrays=localOutput->GetPointData()->GetNumberOfArrays();
    vtkSmartPointer<vtkFloatArray> convertArray;

    double conversion;
    for(int i=0;i<numArrays;i++)
    {
      convertArray= vtkFloatArray::SafeDownCast(localOutput->GetPointData()->GetArray(i));
      if(i<this->VariableConversionValues->GetNumberOfTuples())
      {
        conversion = this->VariableConversionValues->GetValue(i+2);
      }
      else
      {
        conversion=1.0;
      }
          for(ptId=0;ptId<numPts;ptId++)
          {
            convertArray->SetValue(ptId,convertArray->GetValue(ptId)*conversion);
          }
     }



     vtkStringArray* xName= vtkStringArray::SafeDownCast(input->GetFieldData()->GetAbstractArray("XAxisName"));
     vtkStringArray* yName= vtkStringArray::SafeDownCast(input->GetFieldData()->GetAbstractArray("YAxisName"));



    vtkSmartPointer<vtkFloatArray> xArray= vtkSmartPointer<vtkFloatArray>::New();
    xArray->SetNumberOfComponents(1);
    xArray->Allocate(numPts);
    if(xName)
    {
      xArray->SetName(xName->GetValue(0).c_str());
    }
    else
    {
      xArray->SetName("Density");
    }
    xArray->SetNumberOfTuples(numPts);

    vtkSmartPointer<vtkFloatArray> yArray= vtkSmartPointer<vtkFloatArray>::New();
    yArray->SetNumberOfComponents(1);
    yArray->Allocate(numPts);
    if(yName)
    {
      yArray->SetName(yName->GetValue(0).c_str());
    }
    else
    {
      yArray->SetName("Temperature");
    }
    yArray->SetNumberOfTuples(numPts);


    vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
    newPts->SetNumberOfPoints(numPts);
    localOutput->SetPoints(newPts);


    double conversionValues[2];
    conversionValues[0]=1.0;
    conversionValues[0]=1.0;
    if(this->VariableConversionValues->GetNumberOfTuples()>=2)
    {
      conversionValues[0]=this->VariableConversionValues->GetValue(0);
      conversionValues[1]=this->VariableConversionValues->GetValue(1);
    }
    for(ptId=0;ptId<numPts;ptId++)
    {
      double coords[3];
      inPts->GetPoint(ptId,coords);
      xArray->InsertValue(ptId,coords[0]*conversionValues[0]);
      yArray->InsertValue(ptId,coords[1]*conversionValues[1]);
    }

    localOutput->GetPointData()->AddArray(xArray);
    localOutput->GetPointData()->AddArray(yArray);


    Output->ShallowCopy(localOutput);
    return 1;

    }


//---------------------------------------------------



class vtkPrismSurfaceReader::MyInternal
    {
    public:
         vtkSmartPointer<vtkPrismSESAMEReader> Reader;
        vtkSmartPointer<vtkSESAMEConversionFilter> ConversionFilter;
         vtkSmartPointer<vtkRectilinearGridGeometryFilter> RectGridGeometry;
        vtkSmartPointer<vtkContourFilter> ContourFilter;
        vtkSmartPointer<vtkExtractPolyDataGeometry > ExtractGeometry;
        vtkSmartPointer<vtkBox> Box;

        vtkSmartPointer<vtkCleanPolyData> CleanPolyData;
        vtkSmartPointer<vtkTransformFilter> ScaleTransform;
        vtkSmartPointer<vtkTransformFilter> ContourScaleTransform;
        vtkstd::string AxisVarName[3];
        vtkSmartPointer<vtkStringArray> ArrayNames;

        bool ArrayLogScaling[3];

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
            this->AxisVarName[0]      = "Density";
            this->AxisVarName[1]      = "Temperature";
            this->AxisVarName[2]      = "Density";


            this->ArrayLogScaling[0]=false;
            this->ArrayLogScaling[1]=false;
            this->ArrayLogScaling[2]=false;


            this->XRangeArray=vtkSmartPointer<vtkDoubleArray>::New();
            this->YRangeArray=vtkSmartPointer<vtkDoubleArray>::New();
             this->ZRangeArray=vtkSmartPointer<vtkDoubleArray>::New();
           this->CRangeArray=vtkSmartPointer<vtkDoubleArray>::New();

           this->ScaleTransform= vtkSmartPointer<vtkTransformFilter>::New(); 
           this->ContourScaleTransform= vtkSmartPointer<vtkTransformFilter>::New(); 


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
            this->RectGridGeometry =  vtkSmartPointer<vtkRectilinearGridGeometryFilter>::New();

            this->RectGridGeometry->SetInput(this->Reader->GetOutput());
            this->ConversionFilter =  vtkSmartPointer<vtkSESAMEConversionFilter>::New();
            this->ConversionFilter->SetInput(this->RectGridGeometry->GetOutput());


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
    this->SetNumberOfOutputPorts(2);


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
    unsigned long t3 = this->Internal->RectGridGeometry->GetMTime();
    unsigned long t4 = this->Internal->ConversionFilter->GetMTime();
    unsigned long ret_time = t1 > t2 ? t1 : t2;
    ret_time= t3 > ret_time ? t3 : ret_time;
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

/*
void vtkPrismSurfaceReader::SetConversions(double dc,double tc,double pc, double ec)
{
   this->Internal->ConversionFilter->SetConversions(dc,tc,pc,ec);
    this->Modified();
}
double* vtkPrismSurfaceReader::GetConversions()
{
    return this->Internal->ConversionFilter->GetConversions();
}

void vtkPrismSurfaceReader::GetConversions (double &_arg1, double &_arg2,double &_arg3,double &_arg4)
{
    return this->Internal->ConversionFilter->GetConversions(_arg1,_arg2,_arg3,_arg4);
}

void vtkPrismSurfaceReader::GetConversions (double _arg[4])
{
    this->Internal->ConversionFilter->GetConversions(_arg);
}
*/
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
  /*  if(str=="Density")
        {
        double bounds[6];
        this->Internal->RectGridGeometry->Update();
        this->Internal->RectGridGeometry->GetOutput()->GetBounds(bounds);
        rangeArray->InsertValue(0,bounds[0]);
        rangeArray->InsertValue(1,bounds[1]);
        return true;

        }
    else if(str=="Temperature")
        {
        double bounds[6];
        this->Internal->RectGridGeometry->Update();
        this->Internal->RectGridGeometry->GetOutput()->GetBounds(bounds);
        rangeArray->InsertValue(0,bounds[2]);
        rangeArray->InsertValue(1,bounds[3]);
        return true;
        }
    else
        {*/
        this->Internal->ConversionFilter->Update();
        vtkIdType numArrays= this->Internal->ConversionFilter->GetOutput()->GetPointData()->GetNumberOfArrays();
        vtkSmartPointer<vtkFloatArray> xArray;
        for(int i=0;i<numArrays;i++)
            {
            vtkStdString name=this->Internal->ConversionFilter->GetOutput()->GetPointData()->GetArrayName(i);
            vtkStdString::size_type pos=name.find_first_of(":");
            if(pos!= vtkStdString::npos)
                {
                name.erase(0,pos+2);
                }
            if(name==str)
                {
                xArray= vtkFloatArray::SafeDownCast(this->Internal->ConversionFilter->GetOutput()->GetPointData()->GetArray(i)); 
                break;
                }
            }

        if(xArray)
            {
            rangeArray->InsertValue(0,xArray->GetRange()[0]);
            rangeArray->InsertValue(1,xArray->GetRange()[1]);
            return true;
            }
        else
            {
            rangeArray->InsertValue(0,0.0);
            rangeArray->InsertValue(1,0.0);
            return false;
            }
       /* }*/

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
    //this->Internal->ArrayNames->InsertNextValue("Density");
    //this->Internal->ArrayNames->InsertNextValue("Temperature");


this->Internal->ArrayNames->InsertNextValue(this->Internal->Reader->GetTableXAxisName());
this->Internal->ArrayNames->InsertNextValue(this->Internal->Reader->GetTableYAxisName());

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



    vtkInformation *contourOutInfo = outputVector->GetInformationObject(1);
    vtkPointSet *contourOutput = vtkPointSet::SafeDownCast(
        contourOutInfo->Get(vtkDataObject::DATA_OBJECT()));




    vtkSmartPointer<vtkPolyData> localOutput= vtkSmartPointer<vtkPolyData>::New();

    vtkPointSet *input = this->Internal->ConversionFilter->GetOutput();

    vtkPoints *inPts;
    vtkPointData *pd;

    vtkIdType ptId, numPts;



    localOutput->ShallowCopy(input);

    inPts = input->GetPoints();
    pd = input->GetPointData();

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
        if(xArray)
          {
            coords[0]=xArray->GetValue(ptId);
            }
        else
            {
            coords[0]=0.0;
            }

        if(yArray)
            {
            coords[1]=yArray->GetValue(ptId);
            }
        else
            {
            coords[1]=0.0;
            }

        if(zArray)
            {
            coords[2]=zArray->GetValue(ptId);
            }
        else
            {
            coords[2]=0.0;
            }


        int tID=this->GetTable();
        if(tID==502 ||
          tID==503 ||
          tID==504 ||
          tID==505 ||
          tID==601 ||
          tID==602 ||
          tID==603 ||
          tID==604 ||
          tID==605)
          {
          if(!this->GetXLogScaling())
            {
            coords[0]=pow(10,coords[0]);
            }

          if(!this->GetYLogScaling())
            {
            coords[1]=pow(10,coords[1]);
            }

          if(!this->GetZLogScaling())
            {
            coords[2]=pow(10,coords[2]);
            }
          }
        else
          {
          if(this->GetXLogScaling())
            {
            if(coords[0]>0)
              {
              coords[0]=log(coords[0]);
              }
            else
              {
              coords[0]=0.0;
              }
            }

          if(this->GetYLogScaling())
            {
            if(coords[1]>0)
              {
              coords[1]=log(coords[1]);
              }
            else
              {
              coords[1]=0.0;
              }
            }

          if(this->GetZLogScaling())
            {
            if(coords[2]>0)
              {
              coords[2]=log(coords[2]);
              }
            else
              {
              coords[2]=0.0;
              }
            }
          }
        newPts->InsertPoint(ptId,coords);

        }
    double bounds[6];
    localOutput->GetBounds(bounds);

    if(!this->Internal->WarpSurface)
        {
        bounds[4]=-10;
        bounds[5]=10;
        }

        if(this->GetXLogScaling())
            {
            if(this->XThresholdBetween[0]>0)
                {
                this->ActualThresholdBounds[0]=log(this->XThresholdBetween[0]);
                }
            else
                {
                this->ActualThresholdBounds[0]=0.0;
                }
            if(this->XThresholdBetween[1]>0)
                {
                this->ActualThresholdBounds[1]=log(this->XThresholdBetween[1]);
                }
            else
              {
              this->ActualThresholdBounds[1]=0.0;
              }
            }
        else
          {
          this->ActualThresholdBounds[0]=this->XThresholdBetween[0];
          this->ActualThresholdBounds[1]=this->XThresholdBetween[1];
          }
        if(this->GetYLogScaling())
            {
            if(this->YThresholdBetween[0]>0)
                {
                this->ActualThresholdBounds[2]=log(this->YThresholdBetween[0]);
                }
            else
                {
                this->ActualThresholdBounds[2]=0.0;
                }
            if(this->YThresholdBetween[1]>0)
                {
                this->ActualThresholdBounds[3]=log(this->YThresholdBetween[1]);
                }
            else
                {
                this->ActualThresholdBounds[3]=0.0;
                }
            }
        else
          {
          this->ActualThresholdBounds[2]=this->YThresholdBetween[0];
          this->ActualThresholdBounds[3]=this->YThresholdBetween[1];
          }
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
    double scaleBounds[6];
    this->Internal->CleanPolyData->GetOutput()->GetPoints()->GetBounds(scaleBounds);

    double delta[3] = {
        scaleBounds[1] - scaleBounds[0],
        scaleBounds[3] - scaleBounds[2],
        scaleBounds[5] - scaleBounds[4]
    };

    if(delta[0]<=1e-6)
    {
      delta[0]=100;
    }
    if(delta[1]<=1e-6)
    {
      delta[1]=100;
    }
    if(delta[2]<=1e-6)
    {
      delta[2]=100;
    }

    this->AspectScale[0]=100/delta[0];
    this->AspectScale[1]=100/delta[1];
    this->AspectScale[2]=100/delta[2];

    vtkSmartPointer<vtkTransform> transform= vtkSmartPointer<vtkTransform>::New();
    transform->Scale(this->AspectScale[0],this->AspectScale[1],this->AspectScale[2]);

    this->Internal->ScaleTransform->SetInput(this->Internal->CleanPolyData->GetOutput());
    this->Internal->ScaleTransform->SetTransform(transform);
    this->Internal->ScaleTransform->Update();
    surfaceOutput->ShallowCopy(this->Internal->ScaleTransform->GetOutput());


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
    }

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

                this->Internal->ContourScaleTransform->SetInput(this->Internal->ContourFilter->GetOutput());
                this->Internal->ContourScaleTransform->SetTransform(transform);
                this->Internal->ContourScaleTransform->Update();
                contourOutput->ShallowCopy(this->Internal->ContourScaleTransform->GetOutput());
            }

        }


    return 1;

    }

//----------------------------------------------------------------------------
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
