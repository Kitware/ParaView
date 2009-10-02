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
#include "vtkTransform.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkContourFilter.h"
#include "vtkCellData.h"
#include "vtkSESAMEReader.h"
#include "vtkRectilinearGridGeometryFilter.h"
#include "vtkSmartPointer.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtractPolyDataGeometry.h"
#include "vtkBox.h"
#include "vtkCleanPolyData.h"

#include <math.h>

vtkCxxRevisionMacro(vtkPrismSurfaceReader, "1.9");
vtkStandardNewMacro(vtkPrismSurfaceReader);

class vtkPrismSurfaceReader::MyInternal
    {
    public:
        vtkSESAMEReader *Reader;
        vtkRectilinearGridGeometryFilter *RectGridGeometry;
        vtkSmartPointer<vtkContourFilter> ContourFilter;
        vtkSmartPointer<vtkExtractPolyDataGeometry > ExtractGeometry;
        vtkSmartPointer<vtkBox> Box;

        vtkSmartPointer<vtkCleanPolyData> CleanPolyData;

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

            this->Reader = vtkSESAMEReader::New();
            this->RectGridGeometry = vtkRectilinearGridGeometryFilter::New();

            this->RectGridGeometry->SetInput(this->Reader->GetOutput());


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
    this->Conversions[0]=1.0;
    this->Conversions[1]=1.0;
    this->Conversions[2]=1.0;
    this->Conversions[3]=1.0;

    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(2);


    this->XThresholdBetween[0]=0.0;
    this->XThresholdBetween[1]=1.0;
    this->YThresholdBetween[0]=0.0;
    this->YThresholdBetween[1]=1.0;



    }

unsigned long vtkPrismSurfaceReader::GetMTime()
{
    unsigned long t1 = this->Superclass::GetMTime();
    unsigned long t2 = this->Internal->Reader->GetMTime();
    unsigned long t3 = this->Internal->RectGridGeometry->GetMTime();
    unsigned long ret_time = t1 > t2 ? t1 : t2;
    return t3 > ret_time ? t3 : ret_time;
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
        this->Internal->CRangeArray->SetValue(0,(this->Internal->CRangeArray->GetValue(0)*this->Conversions[3]));
        this->Internal->CRangeArray->SetValue(1,(this->Internal->CRangeArray->GetValue(1)*this->Conversions[3]));

    }

    return this->Internal->CRangeArray;
}


void vtkPrismSurfaceReader::SetNumberOfContours(int i)
    {
    if(this->Internal->NumberOfContours!=i)
        {
        this->Internal->NumberOfContours=i;
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
void vtkPrismSurfaceReader::SetConversions(double xc,double yc,double zc, double cc)
{
    this->Conversions[0]=xc;
    this->Conversions[1]=yc;
    this->Conversions[2]=zc;
    this->Conversions[3]=cc;
    this->Modified();
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
    if(str=="Density")
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
        {
        this->Internal->RectGridGeometry->Update();
        vtkIdType numArrays= this->Internal->RectGridGeometry->GetOutput()->GetPointData()->GetNumberOfArrays();
        vtkSmartPointer<vtkFloatArray> xArray;
        for(int i=0;i<numArrays;i++)
            {
            vtkStdString name=this->Internal->RectGridGeometry->GetOutput()->GetPointData()->GetArrayName(i);
            vtkStdString::size_type pos=name.find_first_of(":");
            if(pos!= vtkStdString::npos)
                {
                name.erase(0,pos+2);
                }
            if(name==str)
                {
                xArray= vtkFloatArray::SafeDownCast(this->Internal->RectGridGeometry->GetOutput()->GetPointData()->GetArray(i)); 
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
        this->Internal->XRangeArray->SetValue(0,(this->Internal->XRangeArray->GetValue(0)*this->Conversions[0]));
        this->Internal->XRangeArray->SetValue(1,(this->Internal->XRangeArray->GetValue(1)*this->Conversions[0]));

        if(this->Internal->ArrayLogScaling[0])
            {
            if(this->Internal->XRangeArray->GetValue(0)>0)
                {
                this->Internal->XRangeArray->SetValue(0,log(this->Internal->XRangeArray->GetValue(0)));
                }
            else
                {
                this->Internal->XRangeArray->SetValue(0,0.0);
                }


            if(this->Internal->XRangeArray->GetValue(1)>0)
                {
                this->Internal->XRangeArray->SetValue(1,log(this->Internal->XRangeArray->GetValue(1)));
                }
            else
                {
                this->Internal->XRangeArray->SetValue(1,0.0);
                }
            }
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
       
        this->Internal->YRangeArray->SetValue(0,(this->Internal->YRangeArray->GetValue(0)*this->Conversions[1]));
        this->Internal->YRangeArray->SetValue(1,(this->Internal->YRangeArray->GetValue(1)*this->Conversions[1]));

        
        if(this->Internal->ArrayLogScaling[1])
            {
            if(this->Internal->YRangeArray->GetValue(0)>0)
                {
                this->Internal->YRangeArray->SetValue(0,log(this->Internal->YRangeArray->GetValue(0)));
                }
            else
                {
                this->Internal->YRangeArray->SetValue(0,0.0);
                }


            if(this->Internal->YRangeArray->GetValue(1)>0)
                {
                this->Internal->YRangeArray->SetValue(1,log(this->Internal->YRangeArray->GetValue(1)));
                }
            else
                {
                this->Internal->YRangeArray->SetValue(1,0.0);
                }
            }
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
        this->Internal->ZRangeArray->SetValue(0,(this->Internal->ZRangeArray->GetValue(0)*this->Conversions[2]));
        this->Internal->ZRangeArray->SetValue(1,(this->Internal->ZRangeArray->GetValue(1)*this->Conversions[2]));

        if(this->Internal->ArrayLogScaling[2])
            {
            if(this->Internal->ZRangeArray->GetValue(0)>0)
                {
                this->Internal->ZRangeArray->SetValue(0,log(this->Internal->ZRangeArray->GetValue(0)));
                }
            else
                {
                this->Internal->ZRangeArray->SetValue(0,0.0);
                }


            if(this->Internal->ZRangeArray->GetValue(1)>0)
                {
                this->Internal->ZRangeArray->SetValue(1,log(this->Internal->ZRangeArray->GetValue(1)));
                }
            else
                {
                this->Internal->ZRangeArray->SetValue(1,0.0);
                }
            }
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
    this->Internal->ArrayNames->InsertNextValue("Density");
    this->Internal->ArrayNames->InsertNextValue("Temperature");

    int numberArrayNames=this->Internal->Reader->GetNumberOfTableArrayNames();
    for(int i=0;i<numberArrayNames;i++)
        {
        vtkStdString str=this->Internal->Reader->GetTableArrayName(i);
        vtkStdString::size_type pos=str.find_first_of(":");
        str.erase(0,pos+2);
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

    this->Internal->RectGridGeometry->Update();
    // get the info objects

    vtkInformation *surfaceOutInfo = outputVector->GetInformationObject(0);
    vtkPointSet *surfaceOutput = vtkPointSet::SafeDownCast(
        surfaceOutInfo->Get(vtkDataObject::DATA_OBJECT()));



    vtkInformation *contourOutInfo = outputVector->GetInformationObject(1);
    vtkPointSet *contourOutput = vtkPointSet::SafeDownCast(
        contourOutInfo->Get(vtkDataObject::DATA_OBJECT()));

    vtkSmartPointer<vtkPolyData> localOutput= vtkSmartPointer<vtkPolyData>::New();

    vtkPointSet *input = this->Internal->RectGridGeometry->GetOutput();

    vtkPoints *inPts;
    vtkPointData *pd;

    vtkIdType ptId, numPts;



    localOutput->ShallowCopy(input);

    inPts = input->GetPoints();
    pd = input->GetPointData();

    numPts = inPts->GetNumberOfPoints();

    vtkSmartPointer<vtkFloatArray> densityArray= vtkSmartPointer<vtkFloatArray>::New();
    densityArray->SetNumberOfComponents(1);
    densityArray->Allocate(numPts);
    densityArray->SetName("Density");
    densityArray->SetNumberOfTuples(numPts);

    vtkSmartPointer<vtkFloatArray> temperatureArray= vtkSmartPointer<vtkFloatArray>::New();
    temperatureArray->SetNumberOfComponents(1);
    temperatureArray->Allocate(numPts);
    temperatureArray->SetName("Temperature");
    temperatureArray->SetNumberOfTuples(numPts);




    vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
    newPts->SetNumberOfPoints(numPts);
    localOutput->SetPoints(newPts);


    for(ptId=0;ptId<numPts;ptId++)
        {
        double coords[3];
        inPts->GetPoint(ptId,coords);
        densityArray->InsertValue(ptId,coords[0]);
        temperatureArray->InsertValue(ptId,coords[1]);

        }

    localOutput->GetPointData()->AddArray(densityArray);
    localOutput->GetPointData()->AddArray(temperatureArray);

    vtkSmartPointer<vtkFloatArray> xArray;
    vtkSmartPointer<vtkFloatArray> yArray;
    vtkSmartPointer<vtkFloatArray> zArray;

    vtkIdType numArrays=localOutput->GetPointData()->GetNumberOfArrays();

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


        coords[0]*=this->Conversions[0];
        coords[1]*=this->Conversions[1];
        coords[2]*=this->Conversions[2];






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

        newPts->InsertPoint(ptId,coords);

        }
    double bounds[6];
    localOutput->GetBounds(bounds);

    if(!this->Internal->WarpSurface)
        {
        bounds[4]=-10;
        bounds[5]=10;
        }


    this->Internal->ExtractGeometry->SetInput(localOutput);
    this->Internal->Box->SetBounds(
        this->XThresholdBetween[0],
        this->XThresholdBetween[1],
        this->YThresholdBetween[0],
        this->YThresholdBetween[1],
        bounds[4],
        bounds[5]
    );

    this->Internal->CleanPolyData->SetInput( this->Internal->ExtractGeometry->GetOutput());

    this->Internal->CleanPolyData->Update();
    surfaceOutput->ShallowCopy(this->Internal->CleanPolyData->GetOutput());
 //surfaceOutput->ShallowCopy(this->Internal->ExtractGeometry->GetOutput());


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

             
                vtkSmartPointer<vtkFloatArray> newContourArray;

                newContourArray= vtkFloatArray::SafeDownCast(this->Internal->CleanPolyData->GetOutput()->GetPointData()->GetArray("PrismContours"));

                if(!newContourArray)
                {
                    newContourArray= vtkSmartPointer<vtkFloatArray>::New();
                    this->Internal->CleanPolyData->GetOutput()->GetPointData()->AddArray(newContourArray);
                 } 
                newContourArray->SetNumberOfComponents(1);
                newContourArray->Allocate(numPts);
                newContourArray->SetName("PrismContours");
                newContourArray->SetNumberOfTuples(numPts);
               
                for(int p=0;p<numPts;p++)
                {
                    newContourArray->InsertNextValue(cArray->GetValue(p)*this->Conversions[3]);
                }


                this->Internal->ContourFilter->SetInput(this->Internal->CleanPolyData->GetOutput());


                this->Internal->ContourFilter->SetInputArrayToProcess(
                    0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,newContourArray->GetName());
                this->Internal->ContourFilter->Update();
                contourOutput->ShallowCopy(this->Internal->ContourFilter->GetOutput());

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




