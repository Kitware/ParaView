/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "FlatIndex.h"
#include "CartesianExtent.h"

#include <iostream>
using std::cerr;
using std::endl;

//-----------------------------------------------------------------------------
FlatIndex::FlatIndex(int ni, int nj, int nk, int mode)
{
  switch(mode)
    {
    case CartesianExtent::DIM_MODE_2D_XZ:
      this->A = ni;
      this->B = 0; // ni*nk;
      this->C = 1;
      break;

    case CartesianExtent::DIM_MODE_2D_YZ:
      this->A = nj;
      this->B = 1;
      this->C = 0; // nj*nk;
      break;

    case CartesianExtent::DIM_MODE_2D_XY:
      this->A = 0; // ni*nj;
      this->B = ni;
      this->C = 1;
      break;

    case CartesianExtent::DIM_MODE_3D:
      this->A = ni*nj;
      this->B = ni;
      this->C = 1;
      break;

    default:
      cerr << "Unsupported mode " << mode << endl;
      break;
    }
}

