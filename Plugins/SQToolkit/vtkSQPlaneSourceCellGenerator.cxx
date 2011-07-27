/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSQPlaneSourceCellGenerator.h"

#include "vtkObjectFactory.h"

#include "Numerics.hxx"
#include "Tuple.hxx"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSQPlaneSourceCellGenerator,"$Revision: 1.0$");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQPlaneSourceCellGenerator);

//-----------------------------------------------------------------------------
vtkSQPlaneSourceCellGenerator::vtkSQPlaneSourceCellGenerator()
{
  this->Resolution[0]=
  this->Resolution[1]=
  this->Resolution[2]=1;

  this->Origin[0]=
  this->Origin[1]=
  this->Origin[2]=0.0;

  this->Point1[0]=0.0;
  this->Point1[1]=1.0;
  this->Point1[2]=0.0;

  this->Point2[0]=1.0;
  this->Point2[1]=0.0;
  this->Point2[2]=0.0;

  this->Dx[0]=0.0;
  this->Dx[1]=1.0;
  this->Dx[2]=0.0;

  this->Dy[0]=1.0;
  this->Dy[1]=0.0;
  this->Dy[2]=0.0;
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::SetResolution(int *r)
{
  this->Resolution[0]=r[0];   // ncx
  this->Resolution[1]=r[1];   // ncy
  this->Resolution[2]=r[0]+1; // npx
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::SetResolution(int r1, int r2)
{
  this->Resolution[0]=r1;    // ncx
  this->Resolution[1]=r2;    // ncy
  this->Resolution[2]=r1+1;  // npx
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::SetOrigin(double *x)
{
  this->Origin[0]=x[0];
  this->Origin[1]=x[1];
  this->Origin[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::SetOrigin(double x, double y, double z)
{
  this->Origin[0]=x;
  this->Origin[1]=y;
  this->Origin[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::SetPoint1(double *x)
{
  this->Point1[0]=x[0];
  this->Point1[1]=x[1];
  this->Point1[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::SetPoint1(double x, double y, double z)
{
  this->Point1[0]=x;
  this->Point1[1]=y;
  this->Point1[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::SetPoint2(double *x)
{
  this->Point2[0]=x[0];
  this->Point2[1]=x[1];
  this->Point2[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::SetPoint2(double x, double y, double z)
{
  this->Point2[0]=x;
  this->Point2[1]=y;
  this->Point2[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::ComputeDeltas()
{
  if (this->Resolution[0]<1 || this->Resolution[1]<1 )
    {
    vtkErrorMacro(
      << "Invalid resolution " << Tuple<int>(this->Resolution,2) << ".");
    return;
    }

  this->Dx[0]=(this->Point1[0]-this->Origin[0])/this->Resolution[0];
  this->Dx[1]=(this->Point1[1]-this->Origin[1])/this->Resolution[0];
  this->Dx[2]=(this->Point1[2]-this->Origin[2])/this->Resolution[0];

  this->Dy[0]=(this->Point2[0]-this->Origin[0])/this->Resolution[1];
  this->Dy[1]=(this->Point2[1]-this->Origin[1])/this->Resolution[1];
  this->Dy[2]=(this->Point2[2]-this->Origin[2])/this->Resolution[1];
}

//-----------------------------------------------------------------------------
int vtkSQPlaneSourceCellGenerator::GetCellPointIndexes(
      vtkIdType cid,
      vtkIdType *idx)
{
  int i,j;
  indexToIJ(cid,this->Resolution[0],i,j);

  // vertex indices
  int I[12]={
      i  ,j  ,0,
      i+1,j  ,0,
      i+1,j+1,0,
      i  ,j+1,0
      };

  for (int q=0; q<4; ++q)
    {
    int qq=q*3;
    idx[q]=I[qq+1]*this->Resolution[2]+I[qq];
    }

  return 4;
}

//-----------------------------------------------------------------------------
int vtkSQPlaneSourceCellGenerator::GetCellPoints(vtkIdType cid, float *pts)
{
  // cell index (also lower left pt index)
  int i,j;
  indexToIJ(cid,this->Resolution[0],i,j);

  // vertex indices
  int I[12]={
      i  ,j  ,0,
      i+1,j  ,0,
      i+1,j+1,0,
      i  ,j+1,0
      };

  for (int q=0; q<4; ++q)
    {
    int qq=q*3;
    pts[qq  ]=this->Origin[0]+I[qq]*this->Dx[0]+I[qq+1]*this->Dy[0];
    pts[qq+1]=this->Origin[1]+I[qq]*this->Dx[1]+I[qq+1]*this->Dy[1];
    pts[qq+2]=this->Origin[2]+I[qq]*this->Dx[2]+I[qq+1]*this->Dy[2];
    }

  return 4;
}


//-----------------------------------------------------------------------------
void vtkSQPlaneSourceCellGenerator::PrintSelf(ostream& os, vtkIndent indent)
{

}

