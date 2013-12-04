/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "CartesianDataBlock.h"

#include "vtkDataSet.h"
#include "Tuple.hxx"


//-----------------------------------------------------------------------------
CartesianDataBlock::CartesianDataBlock()
{
  this->Id[0]=
  this->Id[1]=
  this->Id[2]=
  this->Id[3]=0;

  this->SetBounds(0,1,0,1,0,1);
  this->SetExtent(0,1,0,1,0,1);

  this->Data=0;
}

//-----------------------------------------------------------------------------
CartesianDataBlock::~CartesianDataBlock()
{
  this->SetData(0);
}

//-----------------------------------------------------------------------------
void CartesianDataBlock::SetId(int i, int j, int k, int idx)
{
  int I[4]={i,j,k,idx};
  this->SetId(I);
}

//-----------------------------------------------------------------------------
void CartesianDataBlock::SetId(int *I)
{
  this->Id[0]=I[0];
  this->Id[1]=I[1];
  this->Id[2]=I[2];
  this->Id[3]=I[3];
}

//-----------------------------------------------------------------------------
void CartesianDataBlock::SetBounds(
      double xlo,
      double xhi,
      double ylo,
      double yhi,
      double zlo,
      double zhi)
{
  this->Bounds.Set(xlo,xhi,ylo,yhi,zlo,zhi);
}

//-----------------------------------------------------------------------------
void CartesianDataBlock::SetBounds(double *bounds)
{
  this->Bounds.Set(bounds);
}

//-----------------------------------------------------------------------------
void CartesianDataBlock::SetExtent(
      int ilo,
      int ihi,
      int jlo,
      int jhi,
      int klo,
      int khi)
{
  this->Extent.Set(ilo,ihi,jlo,jhi,klo,khi);
}

//-----------------------------------------------------------------------------
void CartesianDataBlock::SetExtent(int *ext)
{
  this->Extent.Set(ext);
}

//-----------------------------------------------------------------------------
void CartesianDataBlock::SetData(vtkDataSet *data)
{
  if (this->Data==data) return;
  if (this->Data) this->Data->Delete();
  this->Data=data;
  if (this->Data) this->Data->Register(0);
}

//-----------------------------------------------------------------------------
vtkDataSet *CartesianDataBlock::GetData()
{
  return this->Data;
}

//*****************************************************************************
std::ostream &operator<<(std::ostream &os, CartesianDataBlock &b)
{
  os
    << Tuple<int>(b.Id,4) << " "
    << b.Extent << " "
    << b.Bounds << " "
    << b.Data;

  return os;
}


// //-----------------------------------------------------------------------------
// // There are 26 neighbor blocks surrounding this block that together
// // compose a cube. We can think of them as aranged in a ternary tree.
// // By passing planes coincident with the center blocks faces, first
// // in the x, then in the y then in the z. This arangement lest us
// // locate the block conatining a point (point is assumed to be insize
// // one of the 27 blocks) with in the worst case 6 comparisions, and
// // no storage overhead. something similar to a kd-tree.
// void CartesianDataBlock::GetNeighborId(double *pt, int *I)
// {
//   // start at this block.
//   I[0]=this->Id[0];
//   I[1]=this->Id[1];
//   I[2]=this->Id[2];
//
//   // walk a ternary tree.
//   for (int q=0; q<3; ++q)
//     {
//     if (pt[q]<this->BlockBounds[2*q])
//       {
//       // left branch
//       --I[q];
//       }
//     else
//     if (pt[q]>this->BlockBounds[2*q+1])
//       {
//       // right branch
//       ++I[q];
//       }
//     }
// }


// note: the above is more efficient
// // these are the leaf nodes (13-39) in a ternary
// // tree constructed by cutting a 3x3x3 neighbor cube
// // first in twice x then in twice y then twice in z.
// // Each cut plane is defined by the center and normal
// // of one face of the center cube.
// static
// const
// int neighborIds[27]={
//       0 , 9 , 18,
//       3 , 12, 21,
//       6 , 15, 24,
//
//       1 , 10, 19,
//       4 , 13, 22,
//       7 , 16, 25,
//
//       2 , 11, 20,
//       5 , 14, 23,
//       8 , 17, 26
//       };
//
// //-----------------------------------------------------------------------------
// int Block::GetNeighborId(float *pt)
// {
//   // walk a ternary tree.
//   idx=0;
//   for (int q=0; q<3; ++q)
//     {
//     if (pt[q]<this->BlockBounds[2*q])
//       {
//       // left branch
//       idx=3*idx+1;
//       }
//     else
//     if (pt[q]>this->BlockBounds[2*q+1])
//       {
//       // right branch
//       idx=3*idx+3;
//       }
//     else
//       {
//       // middle branch
//       idx=3*idx+2;
//       }
//     }
//
//   return neighborIds[idx-13];
// }
