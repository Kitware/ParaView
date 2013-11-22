/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef BOVSpaceTimerInterpolator_h
#define BOVSpaceTimerInterpolator_h

#include "RefCountedPointer.h"

class BOVReader;
class BOVOOCReader;

class BOVSpaceTimeInterpolator : public RefCountedPointer
{
public:
  static BOVSpaceTimeInterpolator *New(){ return new BOVSpaceTimeInterpolator; }

  /**
  Open/close a dataset.
  */
  int Open(const char *file);
  void Close();

  /**
  Enable/disable an array to be read.
  */
  void ActivateArray(const char *name);
  void DeActivateArray(const char *name);

  /**
  Set the array to be interpolated.
  */
  void SetActiveArray(const char *name){ this->ActiveArrayName=name; }

  /**
  Interpolate the active array to the given time and position.
  */
  template<typename T>
  int Interpolate(
        double *X,
        double t,
        T *W);

  /**
  Given a time and position interpolate all active
  arrays and append results to the output.
  */
  template<typename T>
  int Interpolate(
        double *X,
        double t,
        vtkDataSet *output);

protected:
  BOVSpaceTimeInterpolator();
  ~BOVSpaceTimeInterpolator();

  int UpdateCache(double *x, double t);

  void IndexOf(double *X, int *I);

  void ParametricCoordinates(
        double t,
        double &tau);

  void ParametricCoordinates(
        int *I,
        double *X,
        double *Xi);

  template<typename T>
  int Interpolate(
        int *I,
        double *Xi,
        double tau,
        int nComp,
        T *V0,
        T *V1,
        T *W);

  template<typename T>
  int Interpolate(
        int *I,
        double *Xi,
        double tau,
        vtkDataSet *output);

  BOVSpaceTimeInterpolator(const BOVSpaceTimeInterpolator&); // not implemented
  void operator=(const BOVSpaceTimeInterpolator&); // not implemented

private:
  int DecompDims[3];             // subset split into an LxMxN cartesian decomposition
  int BlockCacheSize;            // number of blocks to cache during ooc oepration
  CartesianExtent Subset;

  CartesianBounds WorkingDomain; // working block bounds
  CartesianExtent WorkingExtent; // working block extents

  double T0;                     // time at lower bracket
  double Dt;                     // time interval
  double X0[3];                  // lower left of working block
  double Dx[3];                  // grid spacing on working block

  int DimMode;                   // caretesian dimension modee, 2d,3d
  int FastDim;                   // indices for 2d modes
  int SlowDim;
  int Ni;                        // row size
  int Nij;                       // slab size

  BOVReader *Reader;
  BOVOCCReader *Cache[2];
  vtkDataSet *Data[2];
  std::string ActiveArrayName;   // name of array to be interpolated
  vtkDataArray *ActiveArray[2];  // reference to the active array at bracketing times

};

#endif

// VTK-HeaderTest-Exclude: BOVSpaceTimeInterpolator.h
