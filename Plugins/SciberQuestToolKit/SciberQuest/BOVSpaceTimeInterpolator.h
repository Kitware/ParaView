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
