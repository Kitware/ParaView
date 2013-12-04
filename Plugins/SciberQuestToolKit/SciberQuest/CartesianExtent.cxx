/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "CartesianExtent.h"

#include "Tuple.hxx"
#include "SQMacros.h"
#include "postream.h"

//*****************************************************************************
std::ostream &operator<<(std::ostream &os, const CartesianExtent &ext)
{
  os << Tuple<int>(ext.GetData(),6);

  return os;
}

//-----------------------------------------------------------------------------
int CartesianExtent::GetDimensionMode(
      const CartesianExtent &problemDomain,
      int nGhosts)
{
  // Identify lower dimensional input and handle special cases.
  // Everything but 3D is a special case.
  int minExt = 2*nGhosts+1;
  int inExt[3];
  problemDomain.Size(inExt);
  // 0D and 1D are disallowed
  if (((inExt[0]<minExt) && (inExt[1]<minExt))
    ||((inExt[0]<minExt) && (inExt[2]<minExt))
    ||((inExt[1]<minExt) && (inExt[2]<minExt)))
    {
    sqErrorMacro(
      pCerr(),
      << "The extent is too small." << std::endl
      << "minExt=" << minExt << std::endl
      << "problemDomain=" << problemDomain << std::endl
      << "problemDomainSize=" << Tuple<int>(inExt,3) << std::endl
      << "NOTE: This filter does not support less than 2D.");
    return DIM_MODE_INVALID;
    }
  //  Identify 2D cases
  if (inExt[0]<minExt)
    {
    return DIM_MODE_2D_YZ;
    }
  else
  if (inExt[1]<minExt)
    {
    return DIM_MODE_2D_XZ;
    }
  else
  if (inExt[2]<minExt)
    {
    return DIM_MODE_2D_XY;
    }
  // It's 3D
  return DIM_MODE_3D;
}

//-----------------------------------------------------------------------------
int CartesianExtent::GetDimensionMode(const CartesianExtent &problemDomain)
{
  // Identify lower dimensional input and handle special cases.
  // Everything but 3D is a special case.
  const int minExt = 1;
  int inExt[3];
  problemDomain.Size(inExt);

  // 0D and 1D
  // ambiguous: can we treat the trivial case as 3D?
  // TODO: handle the trivial cases.
  if (((inExt[0]<=minExt) && (inExt[1]<=minExt))
    ||((inExt[0]<=minExt) && (inExt[2]<=minExt))
    ||((inExt[1]<=minExt) && (inExt[2]<=minExt)))
    {
    return DIM_MODE_3D;
    }
  //  Identify 2D cases
  if (inExt[0]<=minExt)
    {
    return DIM_MODE_2D_YZ;
    }
  else
  if (inExt[1]<=minExt)
    {
    return DIM_MODE_2D_XZ;
    }
  else
  if (inExt[2]<=minExt)
    {
    return DIM_MODE_2D_XY;
    }
  // It's 3D
  return DIM_MODE_3D;
}

//-----------------------------------------------------------------------------
CartesianExtent CartesianExtent::Grow(
      const CartesianExtent &inputExt,
      const CartesianExtent &problemDomain,
      int nGhosts,
      int mode)
{
  CartesianExtent outputExt
    = CartesianExtent::Grow(inputExt,nGhosts,mode);

  outputExt &= problemDomain;

  return outputExt;
}

//-----------------------------------------------------------------------------
CartesianExtent CartesianExtent::Grow(
      const CartesianExtent &inputExt,
      int nGhosts,
      int mode)
{
  CartesianExtent outputExt(inputExt);

  switch(mode)
    {
    case DIM_MODE_2D_XY:
      outputExt.Grow(0,nGhosts);
      outputExt.Grow(1,nGhosts);
      break;
    case DIM_MODE_2D_XZ:
      outputExt.Grow(0,nGhosts);
      outputExt.Grow(2,nGhosts);
      break;
    case DIM_MODE_2D_YZ:
      outputExt.Grow(1,nGhosts);
      outputExt.Grow(2,nGhosts);
      break;
    case DIM_MODE_3D:
      outputExt.Grow(nGhosts);
      break;
    }

  return outputExt;
}

//-----------------------------------------------------------------------------
CartesianExtent CartesianExtent::GrowLow(
      const CartesianExtent &inputExt,
      int q,
      int n,
      int mode)
{
  CartesianExtent outputExt(inputExt);

  switch(mode)
    {
    case DIM_MODE_2D_XY:
      if (q==2)
        {
        return outputExt;
        }
      break;
    case DIM_MODE_2D_XZ:
      if (q==1)
        {
        return outputExt;
        }
      break;
    case DIM_MODE_2D_YZ:
      if (q==0)
        {
        return outputExt;
        }
      break;
    }

  outputExt[2*q]-=n;

  return outputExt;
}

//-----------------------------------------------------------------------------
CartesianExtent CartesianExtent::GrowHigh(
      const CartesianExtent &inputExt,
      int q,
      int n,
      int mode)
{
  CartesianExtent outputExt(inputExt);

  switch(mode)
    {
    case DIM_MODE_2D_XY:
      if (q==2)
        {
        return outputExt;
        }
      break;
    case DIM_MODE_2D_XZ:
      if (q==1)
        {
        return outputExt;
        }
      break;
    case DIM_MODE_2D_YZ:
      if (q==0)
        {
        return outputExt;
        }
      break;
    }

  outputExt[2*q+1]+=n;

  return outputExt;
}

//-----------------------------------------------------------------------------
CartesianExtent CartesianExtent::Shrink(
      const CartesianExtent &inputExt,
      int nGhosts,
      int mode)
{
  return CartesianExtent::Grow(inputExt,-nGhosts,mode);
}

//-----------------------------------------------------------------------------
CartesianExtent CartesianExtent::Shrink(
      const CartesianExtent &inputExt,
      const CartesianExtent &problemDomain,
      int nGhosts,
      int mode)
{
  CartesianExtent outputExt(inputExt);

  switch(mode)
    {
    case DIM_MODE_2D_XY:
      outputExt.Grow(0,-nGhosts);
      outputExt.Grow(1,-nGhosts);
      break;
    case DIM_MODE_2D_XZ:
      outputExt.Grow(0,-nGhosts);
      outputExt.Grow(2,-nGhosts);
      break;
    case DIM_MODE_2D_YZ:
      outputExt.Grow(1,-nGhosts);
      outputExt.Grow(2,-nGhosts);
      break;
    case DIM_MODE_3D:
      outputExt.Grow(-nGhosts);
      break;
    }

  // don't shrink at the problem domain.
  for (int i=0; i<6; ++i)
    {
    if (inputExt[i]==problemDomain[i])
      {
      outputExt[i]=problemDomain[i];
      }
    }

  return outputExt;
}

//-----------------------------------------------------------------------------
CartesianExtent CartesianExtent::CellToNode(
      const CartesianExtent &inputExt,
      int mode)
{
  CartesianExtent outputExt(inputExt);

  switch(mode)
    {
    case DIM_MODE_2D_XY:
      ++outputExt[1];
      ++outputExt[3];
      break;
    case DIM_MODE_2D_XZ:
      ++outputExt[1];
      ++outputExt[5];
      break;
    case DIM_MODE_2D_YZ:
      ++outputExt[3];
      ++outputExt[5];
      break;
    case DIM_MODE_3D:
      outputExt.CellToNode();
      break;
    }

  return outputExt;
}

//-----------------------------------------------------------------------------
CartesianExtent CartesianExtent::NodeToCell(
      const CartesianExtent &inputExt,
      int mode)
{
  CartesianExtent outputExt(inputExt);

  switch(mode)
    {
    case DIM_MODE_2D_XY:
      --outputExt[1];
      --outputExt[3];
      break;
    case DIM_MODE_2D_XZ:
      --outputExt[1];
      --outputExt[5];
      break;
    case DIM_MODE_2D_YZ:
      --outputExt[3];
      --outputExt[5];
      break;
    case DIM_MODE_3D:
      outputExt.NodeToCell();
      break;
    }

  return outputExt;
}

//-----------------------------------------------------------------------------
void CartesianExtent::Shift(
      int *ijk,
      int n,
      int mode)
{
  switch(mode)
    {
    case DIM_MODE_2D_XY:
      ijk[0]+=n;
      ijk[1]+=n;
      break;
    case DIM_MODE_2D_XZ:
      ijk[0]+=n;
      ijk[2]+=n;
      break;
    case DIM_MODE_2D_YZ:
      ijk[1]+=n;
      ijk[2]+=n;
      break;
    case DIM_MODE_3D:
      ijk[0]+=n;
      ijk[1]+=n;
      ijk[2]+=n;
      break;
    }
}

//-----------------------------------------------------------------------------
void CartesianExtent::Shift(
      int *ijk,
      int *n,
      int mode)
{
  switch(mode)
    {
    case DIM_MODE_2D_XY:
      ijk[0]+=n[0];
      ijk[1]+=n[1];
      break;
    case DIM_MODE_2D_XZ:
      ijk[0]+=n[0];
      ijk[2]+=n[2];
      break;
    case DIM_MODE_2D_YZ:
      ijk[1]+=n[1];
      ijk[2]+=n[2];
      break;
    case DIM_MODE_3D:
      ijk[0]+=n[0];
      ijk[1]+=n[1];
      ijk[2]+=n[2];
      break;
    }
}

//-----------------------------------------------------------------------------
void CartesianExtent::GetLowerBound(
      const CartesianExtent &ext,
      const double X0[3],
      const double DX[3],
      double lowerBound[3])
{
  lowerBound[0]=X0[0]+ext[0]*DX[0];
  lowerBound[1]=X0[1]+ext[2]*DX[1];
  lowerBound[2]=X0[2]+ext[4]*DX[2];
}

//-----------------------------------------------------------------------------
void CartesianExtent::GetLowerBound(
      const CartesianExtent &ext,
      const float *X,
      const float *Y,
      const float *Z,
      double lowerBound[3])
{
  lowerBound[0]=X[ext[0]];
  lowerBound[1]=Y[ext[2]];
  lowerBound[2]=Z[ext[4]];
}

//-----------------------------------------------------------------------------
void CartesianExtent::GetBounds(
      const CartesianExtent &ext,
      const double X0[3],
      const double DX[3],
      int mode,
      double bounds[6])
{
  int nCells[3];
  CartesianExtent::Size(ext,nCells);

  double extX0[3];
  CartesianExtent::GetLowerBound(ext,X0,DX,extX0);

  switch(mode)
    {
    case DIM_MODE_2D_XY:
      bounds[0]=extX0[0];
      bounds[1]=extX0[0]+nCells[0]*DX[0];
      bounds[2]=extX0[1];
      bounds[3]=extX0[1]+nCells[1]*DX[1];
      bounds[4]=extX0[2];
      bounds[5]=extX0[2];
      break;
    case DIM_MODE_2D_XZ:
      bounds[0]=extX0[0];
      bounds[1]=extX0[0]+nCells[0]*DX[0];
      bounds[2]=extX0[1];
      bounds[3]=extX0[1];
      bounds[4]=extX0[2];
      bounds[5]=extX0[2]+nCells[2]*DX[2];
      break;
    case DIM_MODE_2D_YZ:
      bounds[0]=extX0[0];
      bounds[1]=extX0[0];
      bounds[2]=extX0[1];
      bounds[3]=extX0[1]+nCells[1]*DX[1];
      bounds[4]=extX0[2];
      bounds[5]=extX0[2]+nCells[2]*DX[2];
      break;
    case DIM_MODE_3D:
      bounds[0]=extX0[0];
      bounds[1]=extX0[0]+nCells[0]*DX[0];
      bounds[2]=extX0[1];
      bounds[3]=extX0[1]+nCells[1]*DX[1];
      bounds[4]=extX0[2];
      bounds[5]=extX0[2]+nCells[2]*DX[2];
      break;
    }
}

//-----------------------------------------------------------------------------
void CartesianExtent::GetBounds(
      const CartesianExtent &ext,
      const float *X,
      const float *Y,
      const float *Z,
      int mode,
      double bounds[6])
{
  switch(mode)
    {
    case DIM_MODE_2D_XY:
      bounds[0]=X[ext[0]];
      bounds[1]=X[ext[1]+1];
      bounds[2]=Y[ext[2]];
      bounds[3]=Y[ext[3]+1];
      bounds[4]=Z[ext[4]];
      bounds[5]=Z[ext[4]];
      break;
    case DIM_MODE_2D_XZ:
      bounds[0]=X[ext[0]];
      bounds[1]=X[ext[1]+1];
      bounds[2]=Y[ext[2]];
      bounds[3]=Y[ext[2]];
      bounds[4]=Z[ext[4]];
      bounds[5]=Z[ext[5]+1];
      break;
    case DIM_MODE_2D_YZ:
      bounds[0]=X[ext[0]];
      bounds[1]=X[ext[0]];
      bounds[2]=Y[ext[2]];
      bounds[3]=Y[ext[3]+1];
      bounds[4]=Z[ext[4]];
      bounds[5]=Z[ext[5]+1];
      break;
    case DIM_MODE_3D:
      bounds[0]=X[ext[0]];
      bounds[1]=X[ext[1]+1];
      bounds[2]=Y[ext[2]];
      bounds[3]=Y[ext[3]+1];
      bounds[4]=Z[ext[4]];
      bounds[5]=Z[ext[5]+1];
      break;
    }
}
