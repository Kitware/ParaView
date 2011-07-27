#include "IntersectionSet.h"

#include <sstream>
#include <iostream>
using namespace std;
//*****************************************************************************
inline bool InRange(int a, int b, int v){ return v>=a && v<=b; }

//*****************************************************************************
inline bool ValidId(int id){ return id>=0; }

//*****************************************************************************
inline int InvalidId(){ return -1; }

//-----------------------------------------------------------------------------
IntersectData::IntersectData(const IntersectData &other)
{
  *this=other;
}

//-----------------------------------------------------------------------------
const IntersectData &IntersectData::operator=(const IntersectData &other)
{
  if (this==&other)
    {
    return *this;
    }

  this->seedPointId=other.seedPointId;
  this->fwdSurfaceId=other.fwdSurfaceId;
  this->fwdIntersectTime=other.fwdIntersectTime;
  this->bwdSurfaceId=other.bwdSurfaceId;
  this->bwdIntersectTime=other.bwdIntersectTime;

  return *this;
}

//-----------------------------------------------------------------------------
int IntersectData::CommitType(MPI_Datatype *classType)
{
  #define nBlocks 2

  int blockLen[nBlocks]={3, 2};
  MPI_Datatype blockType[nBlocks]={MPI_INT, MPI_DOUBLE};
  MPI_Aint blockDispl[nBlocks];
  MPI_Address(&this->seedPointId, &blockDispl[0]);
  MPI_Address(&this->fwdIntersectTime, &blockDispl[1]);
  blockDispl[1]-=blockDispl[0];
  blockDispl[0]=0;

  MPI_Type_struct(nBlocks,blockLen,blockDispl,blockType,classType);

  return MPI_Type_commit(classType)==MPI_SUCCESS;
  
  #undef nBlocks
}

//-----------------------------------------------------------------------------
string IntersectData::Print()
{
  ostringstream os;
  os << "SeedPointId:      " << this->seedPointId << endl
     << "fwdSurfaceId:     " << this->fwdSurfaceId << endl
     << "fwdIntersectTime: " << this->fwdIntersectTime << endl
     << "bwdSurfaceId:     " << this->bwdSurfaceId << endl
     << "bwdIntersectTime: " << this->bwdIntersectTime << endl;
  return os.str();
}

















//-----------------------------------------------------------------------------
IntersectionSet::IntersectionSet(
    const IntersectionSet &other)
{
  *this=other;
  this->Data.CommitType(&this->DataType);
}

//-----------------------------------------------------------------------------
const IntersectionSet &IntersectionSet::operator=(
    const IntersectionSet &other)
{
  if (this==&other)
    {
    return *this;
    }

  this->Data=other.Data;

  return *this;
}

//-----------------------------------------------------------------------------
IntersectionSet::IntersectionSet()
{
  this->Data.CommitType(&this->DataType);
}

//-----------------------------------------------------------------------------
IntersectionSet::~IntersectionSet()
{
  MPI_Type_free(&this->DataType);
}

//-----------------------------------------------------------------------------
string IntersectionSet::Print()
{
  return this->Data.Print();
}

//-----------------------------------------------------------------------------
void IntersectionSet::Reduce(IntersectData &other)
{
  IntersectData &local=this->Data;

  // This reduction we take the surface id for the earliest/first 
  // intersection of the two based on the integration time. We have to
  // do it twice, once for the forward running stream line and once for 
  // the backward running stream line.
  // Start forward running.
  if ( !ValidId(local.fwdSurfaceId) 
    && ValidId(other.fwdSurfaceId) )
    {
    // I don't have the intersection data but he does, take his data.
    local.fwdSurfaceId=other.fwdSurfaceId;
    local.fwdIntersectTime=other.fwdIntersectTime;
    }
  else
  if ( ValidId(local.fwdSurfaceId) 
    && ValidId(other.fwdSurfaceId) 
    && local.fwdIntersectTime>other.fwdIntersectTime )
    {
    // We both have intersection data but his is the first, take his data.
    local.fwdSurfaceId=other.fwdSurfaceId;
    local.fwdIntersectTime=other.fwdIntersectTime;
    }
  // else
  // if niether have valid data, or mine is the first one, in both case
  // nothing to do.

  // Same thing backward running.
  if ( !ValidId(local.bwdSurfaceId) 
    && ValidId(other.bwdSurfaceId) )
    {
    // I don't have the intersection data but he does, take his data.
    local.bwdSurfaceId=other.bwdSurfaceId;
    local.bwdIntersectTime=other.bwdIntersectTime;
    }
  else
  if ( ValidId(local.bwdSurfaceId) 
    && ValidId(other.bwdSurfaceId) 
    && local.bwdIntersectTime>other.bwdIntersectTime )
    {
    // We both have intersection data but his is the first, take his data.
    local.bwdSurfaceId=other.bwdSurfaceId;
    local.bwdIntersectTime=other.bwdIntersectTime;
    }
  // else
  // if niether have valid data, or mine is the first one, in both case
  // nothing to do.
}

//-----------------------------------------------------------------------------
int IntersectionSet::AllReduce()
{
  // Get our identities
  MPI_Status stat;
  int procId;
  int nProcs;
  MPI_Comm_size(MPI_COMM_WORLD, &nProcs);
  MPI_Comm_rank(MPI_COMM_WORLD, &procId);

  // Construct the familly tree.
  int lcid,rcid,pid;
  lcid=procId*2+1;
  lcid=InRange(0,nProcs-1,lcid)?lcid:InvalidId();

  rcid=procId*2+2;
  rcid=InRange(0,nProcs-1,rcid)?rcid:InvalidId();

  pid=procId!=0?(procId-1)/2:InvalidId();

  // everybody find what their children have.
  if (ValidId(lcid))
    {
    // cerr << "proc " << procId << " recv from left " << lcid << endl;
    IntersectData lcd;
    MPI_Recv(&lcd,1,this->DataType,lcid,lcid,MPI_COMM_WORLD,&stat);
    this->Reduce(lcd);
    }
  if (ValidId(rcid))
    {
    // cerr << "proc " << procId << " recv from right " << rcid << endl;
    IntersectData rcd;
    MPI_Recv(&rcd,1,this->DataType,rcid,rcid,MPI_COMM_WORLD,&stat);
    this->Reduce(rcd);
    }
  // everybody tell their paraent what they have, and listen to 
  // the parent for the reduction.
  if (ValidId(pid))
    {
    // cerr << "proc " << procId << " send to, recv from parent " << pid << endl;
    MPI_Send(&this->Data,1,this->DataType,pid,procId,MPI_COMM_WORLD);
    IntersectData pd;
    MPI_Recv(&pd,1,this->DataType,pid,pid,MPI_COMM_WORLD,&stat);
    this->Reduce(pd);
    }
  // everybody pass the reduction on to their children.
  if (ValidId(lcid))
    {
    // cerr << "proc " << procId << " send to left " << lcid << endl;
    MPI_Send(&this->Data,1,this->DataType,lcid,procId,MPI_COMM_WORLD);
    }
  if (ValidId(rcid))
    {
    // cerr << "proc " << procId << " send to right " << rcid << endl;
    MPI_Send(&this->Data,1,this->DataType,rcid,procId,MPI_COMM_WORLD);
    }

  return 1;
}
