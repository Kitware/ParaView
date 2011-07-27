/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "CartesianExtent.h"

#include "Tuple.hxx"
#include "SQMacros.h"
#include "postream.h"

//*****************************************************************************
ostream &operator<<(ostream &os, const CartesianExtent &ext)
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
  if ((inExt[0]<minExt) && (inExt[1]<minExt)
    ||(inExt[0]<minExt) && (inExt[2]<minExt)
    ||(inExt[1]<minExt) && (inExt[2]<minExt))
    {
    sqErrorMacro(pCerr(),"This filter does not support less than 2D.");
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
  else
    {
    return DIM_MODE_3D;
    }

  return DIM_MODE_INVALID;
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
    if (inputExt[i] == problemDomain[i])
      {
      outputExt[i] = problemDomain[i];
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

