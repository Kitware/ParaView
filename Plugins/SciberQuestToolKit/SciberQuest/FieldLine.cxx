/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
