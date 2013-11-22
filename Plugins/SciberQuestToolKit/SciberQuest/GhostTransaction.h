/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __GhostTransaction_h
#define __GhostTransaction_h

#include "CartesianExtent.h" // for CartesianExtent
#include "SQMacros.h" // for sqErrorMacro

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Request;
typedef void * MPI_Comm;
#else
#include "SQMPICHWarningSupression.h" // for suppressing MPI warnings
#include <mpi.h> // for MPI_Request and MPI_Comm
#include "MPIRawArrayIO.hxx" // for CreateCartesianView
#endif

#include <sstream> // for ostringstream
#include <vector> // for vector

// #define GhostTransactionDEBUG

/// GhostTransaction - Helper class to handle communication of ghost data
/**
Extents are always ionternally described in cell space. During
execution point data can be specified and extents are converted
to point based extents.
*/
class GhostTransaction
{
public:
  GhostTransaction()
      :
    SrcRank(0),
    DestRank(0)
  {}

  GhostTransaction(
        int srcRank,
        const CartesianExtent &srcExt,
        int destRank,
        const CartesianExtent &destExt,
        const CartesianExtent &intExt,
        int id)
      :
    Id(id),
    SrcRank(srcRank),
    SrcExt(srcExt),
    DestRank(destRank),
    DestExt(destExt),
    IntExt(intExt)
  {}

  ~GhostTransaction(){}

  void SetSourceRank(int rank){ this->SrcRank=rank; }
  int GetSourceRank() const { return this->SrcRank; }

  void SetSourceExtent(CartesianExtent &srcExt){ this->SrcExt=srcExt; }
  CartesianExtent &GetSourceExtent(){ return this->SrcExt; }
  const CartesianExtent &GetSourceExtent() const { return this->SrcExt; }

  void SetDestinationRank(int rank){ this->DestRank=rank; }
  int GetDestinationRank() const { return this->DestRank; }

  void SetDestinationExtent(CartesianExtent &destExt){ this->DestExt=destExt; }
  CartesianExtent &GetDestinationExtent(){ return this->DestExt; }
  const CartesianExtent &GetDestinationExtent() const { return this->DestExt; }

  void SetIntersectionExtent(CartesianExtent &intExt){ this->IntExt=intExt; }
  CartesianExtent &GetIntersectionExtent(){ return this->IntExt; }
  const CartesianExtent &GetIntersectionExtent() const { return this->IntExt; }

  void SetTransactionId(int id){ this->Id=id; }
  int GetTransactionId(){ return this->Id; }

  template<typename T>
  int Execute(
        MPI_Comm comm,
        int rank,
        int nComps,
        T *srcData,
        T *destData,
        bool pointData,
        int dimMode,
        std::vector<MPI_Request> &req,
        int tag);

private:
  int Id;

  int SrcRank;
  CartesianExtent SrcExt;

  int DestRank;
  CartesianExtent DestExt;

  CartesianExtent IntExt;
};

std::ostream &operator<<(std::ostream &os, const GhostTransaction &gt);

//-----------------------------------------------------------------------------
template<typename T>
int GhostTransaction::Execute(
       MPI_Comm comm,
       int rank,
       int nComps,
       T *srcData,
       T *destData,
       bool pointData,
       int dimMode,
       std::vector<MPI_Request> &req,
       int tag)
{
  int iErr=0;

  #ifdef SQTK_WITHOUT_MPI
  (void)comm;
  (void)rank;
  (void)nComps;
  (void)srcData;
  (void)destData;
  (void)pointData;
  (void)dimMode;
  (void)req;
  (void)tag;
  sqErrorMacro(std::cerr,"Attempting to execute MPI code in a serial build.");
  #else
  std::ostringstream oss;

  if (rank==this->SrcRank)
    {
    #ifdef GhostTransactionDEBUG
    oss
      << this->Id << " "
      << rank << " sending " //  << this->IntExt
      << " to " << this->DestRank << std::endl;
    std::cerr << oss.str();
    #endif

    // sender
    CartesianExtent srcExt=this->SrcExt;
    srcExt.Shift(0,-this->SrcExt[0]);
    srcExt.Shift(1,-this->SrcExt[2]);
    srcExt.Shift(2,-this->SrcExt[4]);

    CartesianExtent intExt=this->IntExt;
    intExt.Shift(0,-this->SrcExt[0]);
    intExt.Shift(1,-this->SrcExt[2]);
    intExt.Shift(2,-this->SrcExt[4]);

    if (pointData)
      {
      srcExt = CartesianExtent::CellToNode(srcExt,dimMode);
      intExt = CartesianExtent::CellToNode(intExt,dimMode);
      }

    MPI_Datatype subarray;
    CreateCartesianView<T>(
          srcExt,
          intExt,
          nComps,
          subarray);

    req.push_back(MPI_REQUEST_NULL);

    iErr=MPI_Isend(
          srcData,
          1,
          subarray,
          this->DestRank,
          tag,
          comm,
          &req.back());

    MPI_Type_free(&subarray);

    #ifdef GhostTransactionDEBUG
    oss.str("");
    oss << this->Id << " " << rank << " ok." << std::endl;
    std::cerr << oss.str();
    #endif
    }
  else
  if (rank==this->DestRank)
    {
    #ifdef GhostTransactionDEBUG
    oss
      << this->Id << " "
      << rank << " receiving " // << this->IntExt
      << " from " << this->SrcRank << std::endl;
    std::cerr << oss.str();
    #endif

    // reciever
    CartesianExtent destExt=this->DestExt;
    destExt.Shift(0,-this->DestExt[0]);
    destExt.Shift(1,-this->DestExt[2]);
    destExt.Shift(2,-this->DestExt[4]);

    CartesianExtent intExt=this->IntExt;
    intExt.Shift(0,-this->DestExt[0]);
    intExt.Shift(1,-this->DestExt[2]);
    intExt.Shift(2,-this->DestExt[4]);

    if (pointData)
      {
      destExt = CartesianExtent::CellToNode(destExt,dimMode);
      intExt = CartesianExtent::CellToNode(intExt,dimMode);
      }

    MPI_Datatype subarray;
    CreateCartesianView<T>(
          destExt,
          intExt,
          nComps,
          subarray);

    req.push_back(MPI_REQUEST_NULL);

    iErr=MPI_Irecv(
          destData,
          1,
          subarray,
          this->SrcRank,
          tag,
          comm,
          &req.back());

    MPI_Type_free(&subarray);

    #ifdef GhostTransactionDEBUG
    oss.str("");
    oss << this->Id << " " << rank << " ok." << std::endl;
    std::cerr << oss.str();
    #endif
    }
  #endif

  return iErr;
}

#endif

// VTK-HeaderTest-Exclude: GhostTransaction.h
