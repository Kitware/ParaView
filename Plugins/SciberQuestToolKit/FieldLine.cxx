/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "FieldLine.h"

//-----------------------------------------------------------------------------
const FieldLine &FieldLine::operator=(const FieldLine &other)
{
  if (&other==this)
    {
    return *this;
    }

  this->Seed[0]=other.Seed[0];
  this->Seed[1]=other.Seed[1];
  this->Seed[2]=other.Seed[2];

  this->SeedId=other.SeedId;

  this->FwdTerminator=other.FwdTerminator;
  this->BwdTerminator=other.BwdTerminator;

  this->DeleteTrace();

  this->FwdTrace=other.FwdTrace;
  if (this->FwdTrace) this->FwdTrace->Register(0);

  this->BwdTrace=other.BwdTrace;
  if (this->BwdTrace) this->BwdTrace->Register(0);

  return *this;
}

//-----------------------------------------------------------------------------
vtkIdType FieldLine::CopyPoints(float *pts)
{
  // Copy the bwd running field line, reversing order
  // so it ends at the seed point.
  vtkIdType nPtsBwd=this->BwdTrace->GetNumberOfTuples();
  float *pbtr=this->BwdTrace->GetPointer(0);

  // start at the end.
  pbtr+=3*nPtsBwd-3;
  for (vtkIdType i=0; i<nPtsBwd; ++i,pts+=3,pbtr-=3)
    {
    pts[0]=pbtr[0];
    pts[1]=pbtr[1];
    pts[2]=pbtr[2];
    }

  // Copy the forward running field line.
  vtkIdType nPtsFwd=this->FwdTrace->GetNumberOfTuples();
  float *pftr=this->FwdTrace->GetPointer(0);
  for (vtkIdType i=0; i<nPtsFwd; ++i,pts+=3,pftr+=3)
    {
    pts[0]=pftr[0];
    pts[1]=pftr[1];
    pts[2]=pftr[2];
    }

  return nPtsBwd+nPtsFwd;
}

//-----------------------------------------------------------------------------
void FieldLine::GetDisplacement(float *d)
{
  float s[3]={
      this->Seed[0],
      this->Seed[1],
      this->Seed[2]};

  vtkIdType np;
  float *p0=s;
  if (this->BwdTrace && (np=this->BwdTrace->GetNumberOfTuples()))
    {
    p0=this->BwdTrace->GetPointer(0);
    p0+=3*np-3;
    }

  float *p1=s;
  if (this->FwdTrace && (np=this->FwdTrace->GetNumberOfTuples()))
    {
    p1=this->FwdTrace->GetPointer(0);
    p1+=3*np-3;
    }

  d[0]=p1[0]-p0[0];
  d[1]=p1[1]-p0[1];
  d[2]=p1[2]-p0[2];
}

//-----------------------------------------------------------------------------
void FieldLine::GetForwardEndPoint(float *d)
{
  float s[3]={
      this->Seed[0],
      this->Seed[1],
      this->Seed[2]
      };

  vtkIdType np;

  float *p1=s;
  if (this->FwdTrace && (np=this->FwdTrace->GetNumberOfTuples()))
    {
    p1=this->FwdTrace->GetPointer(0);
    p1+=3*np-3;
    }

  d[0]=p1[0];
  d[1]=p1[1];
  d[2]=p1[2];
}

//-----------------------------------------------------------------------------
void FieldLine::GetBackwardEndPoint(float *d)
{
  float s[3]={
      this->Seed[0],
      this->Seed[1],
      this->Seed[2]
      };

  vtkIdType np;
  float *p0=s;
  if (this->BwdTrace && (np=this->BwdTrace->GetNumberOfTuples()))
    {
    p0=this->BwdTrace->GetPointer(0);
    p0+=3*np-3;
    }

  d[0]=p0[0];
  d[1]=p0[1];
  d[2]=p0[2];
}
