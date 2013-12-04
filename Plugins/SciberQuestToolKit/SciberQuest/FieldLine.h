/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __FieldLine_h
#define __FieldLine_h

#include "vtkFloatArray.h"

// Data type to collect streamline points during a trace. The
// streamline is expected mapped in two steps, a backward and
// forward trace.
//=============================================================================
class FieldLine
{
public:

  FieldLine();
  FieldLine(float p[3], unsigned long long seedId=0);
  FieldLine(double p[3], unsigned long long seedId=0);
  FieldLine(const FieldLine &other) { *this=other; }
  ~FieldLine() { this->DeleteTrace(); }

  // Decscription:
  // Internal allocate/free data structures for stream line trace,
  void AllocateTrace();
  void DeleteTrace();

  // Description:
  // Copy the field line. Does a shallow copy of internal data
  // structures.
  const FieldLine &operator=(const FieldLine &other);

  // Description:
  // Set seed point for coming trace and clear out internal data
  // structures.
  void Initialize(double p[3], unsigned long long seedId);
  void Initialize(float p[3], unsigned long long seedId);
  // Description:
  // Get seed point and it's id.
  unsigned long long GetSeedId() const { return this->SeedId; }
  float *GetSeedPoint() { return this->Seed; }
  void GetSeedPoint(double p[3]) const;
  void GetSeedPoint(float p[3]) const;

  // Description:
  // Add a point to the trace in the given direction.
  void PushPoint(int dir,float *p)
    {
    (dir==0?BwdTrace:FwdTrace)->InsertNextTuple(p);
    }

  void PushPoint(int dir,double *p)
    {
    (dir==0?BwdTrace:FwdTrace)->InsertNextTuple(p);
    }

  // Description:
  // Set/Get the code indicating why the trace in the given direction
  // terminated.
  void SetTerminator(int dir, int code)
    {
    *(dir==0?&this->BwdTerminator:&this->FwdTerminator)=code;
    }
  int GetForwardTerminator() const { return this->FwdTerminator; }
  int GetBackwardTerminator() const { return this->BwdTerminator; }

  // Description:
  // Return the number of points in the trace data.
  vtkIdType GetNumberOfPoints();

  // Description:
  // Copy the trace data into the supplied bufffer. The buffer must be
  // big enough to hold them all. In the Line variant the the backward trace
  // data is reveresed.
  vtkIdType CopyPoints(float *pts);

  // Description:
  // Compute the displacement between the first and last point
  // in the Line.
  void GetDisplacement(float *d);

  // Description:
  // Get end points on the filed line.
  void GetForwardEndPoint(float *d);
  void GetBackwardEndPoint(float *d);

private:
  vtkFloatArray *FwdTrace;    // streamline trace along V
  vtkFloatArray *BwdTrace;    // streamline trace along -V
  float Seed[3];              // seed point
  unsigned long long SeedId;  // cell id in origniating dataset
  int FwdTerminator;          // code indicating how fwd trace ended
  int BwdTerminator;          // code indicating how bwd trace ended
};

//-----------------------------------------------------------------------------
inline
FieldLine::FieldLine()
    :
  FwdTrace(0),
  BwdTrace(0),
  SeedId(0),
  FwdTerminator(0),
  BwdTerminator(0)
{
  this->Seed[0]=0.0f;
  this->Seed[1]=0.0f;
  this->Seed[2]=0.0f;
}

//-----------------------------------------------------------------------------
inline
FieldLine::FieldLine(float p[3], unsigned long long seedId)
    :
  FwdTrace(0),
  BwdTrace(0),
  SeedId(seedId),
  FwdTerminator(0),
  BwdTerminator(0)
{
  this->Seed[0]=p[0];
  this->Seed[1]=p[1];
  this->Seed[2]=p[2];
}

//-----------------------------------------------------------------------------
inline
FieldLine::FieldLine(double p[3], unsigned long long seedId)
    :
  FwdTrace(0),
  BwdTrace(0),
  SeedId(seedId),
  FwdTerminator(0),
  BwdTerminator(0)
{
  this->Seed[0]=(float)p[0];
  this->Seed[1]=(float)p[1];
  this->Seed[2]=(float)p[2];
}

//-----------------------------------------------------------------------------
inline
void FieldLine::AllocateTrace()
{
  this->FwdTrace=vtkFloatArray::New();
  this->FwdTrace->SetNumberOfComponents(3);
  this->FwdTrace->Allocate(128);
  this->BwdTrace=vtkFloatArray::New();
  this->BwdTrace->SetNumberOfComponents(3);
  this->BwdTrace->Allocate(128);
}

//-----------------------------------------------------------------------------
inline
void FieldLine::DeleteTrace()
{
  if (this->FwdTrace){ this->FwdTrace->Delete(); }
  if (this->BwdTrace){ this->BwdTrace->Delete(); }
  this->FwdTrace=0;
  this->BwdTrace=0;
}

//-----------------------------------------------------------------------------
inline
void FieldLine::Initialize(double p[3], unsigned long long seedId)
{
  this->Seed[0]=(float)p[0];
  this->Seed[1]=(float)p[1];
  this->Seed[2]=(float)p[2];
  this->SeedId=seedId;
  if (this->FwdTrace) this->FwdTrace->SetNumberOfTuples(0);
  if (this->BwdTrace) this->BwdTrace->SetNumberOfTuples(0);
  this->BwdTerminator=this->FwdTerminator=0;
}

//-----------------------------------------------------------------------------
inline
void FieldLine::Initialize(float p[3], unsigned long long seedId)
{
  this->Seed[0]=p[0];
  this->Seed[1]=p[1];
  this->Seed[2]=p[2];
  this->SeedId=seedId;
  if (this->FwdTrace) this->FwdTrace->SetNumberOfTuples(0);
  if (this->BwdTrace) this->BwdTrace->SetNumberOfTuples(0);
  this->BwdTerminator=this->FwdTerminator=0;
}

//-----------------------------------------------------------------------------
inline
void FieldLine::GetSeedPoint(float p[3]) const
{
  p[0]=this->Seed[0];
  p[1]=this->Seed[1];
  p[2]=this->Seed[2];
}

//-----------------------------------------------------------------------------
inline
void FieldLine::GetSeedPoint(double p[3]) const
{
  p[0]=this->Seed[0];
  p[1]=this->Seed[1];
  p[2]=this->Seed[2];
}

//-----------------------------------------------------------------------------
inline
vtkIdType FieldLine::GetNumberOfPoints()
{
  vtkIdType total=0;

  total+=(this->FwdTrace?this->FwdTrace->GetNumberOfTuples():0);
  total+=(this->BwdTrace?this->BwdTrace->GetNumberOfTuples():0);

  return total;
}

#endif

// VTK-HeaderTest-Exclude: FieldLine.h
