/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __ImageDecomp_h
#define __ImageDecomp_h


#include "CartesianDecomp.h" // for CartesianDecomp

class CartesianDataBlock;
class CartesianDataBlockIODescriptor;


/// Splits a cartesian grid into a set of smaller cartesian grids.
/**
Splits a cartesian grid into a set of smaller cartesian grids using
a set of axis aligned planes. Given a point will locate and return
the sub-grid which contains it.
*/
class ImageDecomp : public CartesianDecomp
{
public:
  static ImageDecomp *New(){ return new ImageDecomp; }

  /**
  Get/Set the domain origin.
  */
  void SetOrigin(double x0, double y0, double z0);
  void SetOrigin(const double X0[3]);
  double *GetOrigin(){ return this->X0; }
  const double *GetOrigin() const { return this->X0; }

  /**
  Get/Set the grid spacing.
  */
  void SetSpacing(double dx, double dy, double dz);
  void SetSpacing(const double dX[3]);
  double *GetSpacing(){ return this->DX; }
  const double *GetSpacing() const { return this->DX; }

  /**
  Set the domain bounds based on curren values of
  extent, oring and spacing.
  */
  void ComputeBounds();

  /**
  Decompose the domain in to the requested number of blocks.
  */
  int DecomposeDomain();

protected:
  ImageDecomp();
  ~ImageDecomp();

private:
  void operator=(ImageDecomp &); // not implemented
  ImageDecomp(ImageDecomp &); // not implemented

private:
  double DX[3];                 // grid spacing
  double X0[3];                 // origin
};

#endif

// VTK-HeaderTest-Exclude: ImageDecomp.h
