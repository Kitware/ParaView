/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQVolumeSourceCellGenerator.h"

#include "vtkObjectFactory.h"

#include "Numerics.hxx"
#include "Tuple.hxx"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQVolumeSourceCellGenerator);

//-----------------------------------------------------------------------------
vtkSQVolumeSourceCellGenerator::vtkSQVolumeSourceCellGenerator()
{
  this->Resolution[0]=
  this->Resolution[1]=
  this->Resolution[2]=
  this->Resolution[3]=
  this->Resolution[4]=
  this->Resolution[5]=1;

  this->Origin[0]=
  this->Origin[1]=
  this->Origin[2]=0.0;

  this->Point1[0]=0.0;
  this->Point1[1]=1.0;
  this->Point1[2]=0.0;

  this->Point2[0]=1.0;
  this->Point2[1]=0.0;
  this->Point2[2]=0.0;

  this->Point3[0]=0.0;
  this->Point3[1]=0.0;
  this->Point3[2]=1.0;

  this->Dx[0]=0.0;
  this->Dx[1]=1.0;
  this->Dx[2]=0.0;

  this->Dy[0]=1.0;
  this->Dy[1]=0.0;
  this->Dy[2]=0.0;

  this->Dz[0]=0.0;
  this->Dz[1]=0.0;
  this->Dz[2]=1.0;
}


//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetResolution(int *r)
{
  this->Resolution[0]=r[0];                 // ncx
  this->Resolution[1]=r[1];                 // ncy
  this->Resolution[2]=r[2];                 // ncz
  this->Resolution[3]=r[0]*r[1];            // ncx*ncy
  this->Resolution[4]=r[0]+1;               // npx
  this->Resolution[5]=(r[0]+1)*(r[1]+1);    // npx*npy
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetResolution(int r1, int r2, int r3)
{
  this->Resolution[0]=r1;             // ncx
  this->Resolution[1]=r2;             // ncy
  this->Resolution[2]=r3;             // ncz
  this->Resolution[3]=r1*r2;          // ncx*ncy
  this->Resolution[4]=r1+1;           // npx
  this->Resolution[5]=(r1+1)*(r2+1);  // npx*npy
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetOrigin(double *x)
{
  this->Origin[0]=x[0];
  this->Origin[1]=x[1];
  this->Origin[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetOrigin(double x, double y, double z)
{
  this->Origin[0]=x;
  this->Origin[1]=y;
  this->Origin[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint1(double *x)
{
  this->Point1[0]=x[0];
  this->Point1[1]=x[1];
  this->Point1[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint1(double x, double y, double z)
{
  this->Point1[0]=x;
  this->Point1[1]=y;
  this->Point1[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint2(double *x)
{
  this->Point2[0]=x[0];
  this->Point2[1]=x[1];
  this->Point2[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint2(double x, double y, double z)
{
  this->Point2[0]=x;
  this->Point2[1]=y;
  this->Point2[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint3(double *x)
{
  this->Point3[0]=x[0];
  this->Point3[1]=x[1];
  this->Point3[2]=x[2];
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::SetPoint3(double x, double y, double z)
{
  this->Point3[0]=x;
  this->Point3[1]=y;
  this->Point3[1]=z;
  this->ComputeDeltas();
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::ComputeDeltas()
{
  if ( (this->Resolution[0]<1)
    || (this->Resolution[1]<1)
    || (this->Resolution[2]<1) )
    {
    vtkErrorMacro(
        << "Invalid resolution "
        << Tuple<int>(this->Resolution,3)
        << ".");
    return;
    }

  this->Dx[0]=(this->Point1[0]-this->Origin[0])/this->Resolution[0];
  this->Dx[1]=(this->Point1[1]-this->Origin[1])/this->Resolution[0];
  this->Dx[2]=(this->Point1[2]-this->Origin[2])/this->Resolution[0];

  this->Dy[0]=(this->Point2[0]-this->Origin[0])/this->Resolution[1];
  this->Dy[1]=(this->Point2[1]-this->Origin[1])/this->Resolution[1];
  this->Dy[2]=(this->Point2[2]-this->Origin[2])/this->Resolution[1];

  this->Dz[0]=(this->Point3[0]-this->Origin[0])/this->Resolution[2];
  this->Dz[1]=(this->Point3[1]-this->Origin[1])/this->Resolution[2];
  this->Dz[2]=(this->Point3[2]-this->Origin[2])/this->Resolution[2];
}

//-----------------------------------------------------------------------------
int vtkSQVolumeSourceCellGenerator::GetCellPointIndexes(vtkIdType cid, vtkIdType *idx)
{
  int i,j,k;
  indexToIJK(
      ((int)cid),
      this->Resolution[0],
      this->Resolution[3],
      i,
      j,
      k);

  // point indices in VTK order.
  int I[24]={
      i  ,j  ,k  ,
      i+1,j  ,k  ,
      i+1,j+1,k  ,
      i  ,j+1,k  ,
      i  ,j  ,k+1,
      i+1,j  ,k+1,
      i+1,j+1,k+1,
      i  ,j+1,k+1
      };

  for (int q=0; q<8; ++q)
    {
    int qq=q*3;
    idx[q]=I[qq+2]*this->Resolution[5]+I[qq+1]*this->Resolution[4]+I[qq];
    }

  return 4;
}

//-----------------------------------------------------------------------------
int vtkSQVolumeSourceCellGenerator::GetCellPoints(vtkIdType cid, float *pts)
{
  int i,j,k;
  indexToIJK(
      ((int)cid),
      this->Resolution[0],
      this->Resolution[3],
      i,
      j,
      k);

  // point indices in VTK order.
  int I[24]={
      i  ,j  ,k  ,
      i+1,j  ,k  ,
      i+1,j+1,k  ,
      i  ,j+1,k  ,
      i  ,j  ,k+1,
      i+1,j  ,k+1,
      i+1,j+1,k+1,
      i  ,j+1,k+1
      };

  float dx[3]={
      ((float)this->Dx[0]),
      ((float)this->Dx[1]),
      ((float)this->Dx[2])};

  float dy[3]={
      ((float)this->Dy[0]),
      ((float)this->Dy[1]),
      ((float)this->Dy[2])};

  float dz[3]={
      ((float)this->Dz[0]),
      ((float)this->Dz[1]),
      ((float)this->Dz[2])};

  float O[3]={
      ((float)this->Origin[0]),
      ((float)this->Origin[1]),
      ((float)this->Origin[2])};

  for (int q=0; q<8; ++q)
    {
    int qq=q*3;
    pts[qq  ]=O[0]+((float)I[qq])*dx[0]+((float)I[qq+1])*dy[0]+((float)I[qq+2])*dz[0];
    pts[qq+1]=O[1]+((float)I[qq])*dx[1]+((float)I[qq+1])*dy[1]+((float)I[qq+2])*dz[1];
    pts[qq+2]=O[2]+((float)I[qq])*dx[2]+((float)I[qq+1])*dy[2]+((float)I[qq+2])*dz[2];
    }

  return 8;
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceCellGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
 (void)os;
 (void)indent;
}
