/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoHaloFinder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoHaloFinder.h

Copyright (c) 2007, 2009, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007, 2009. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkCosmoHaloFinder - find halos within a cosmology data file
// .SECTION Description
// vtkCosmoHaloFinder is a filter object that operates on the unstructured 
// grid of all particles and assigns each particle a halo id.
//
// .SECTION Thanks
// Katrin Heitmann (heitmann@lanl.gov) provided the original code.
// Lee Ankeny (laa@lanl.gov) and James Ahrens (ahrens@lanl.gov) adapted
// that code for VTK.
//
// .SECTION Note
// This finder implements a recursive algorithm.
// Linked lists are used for halos.
// Merge is done recursively.
// Each interval has its bounding box calculated in Reorder().
// Non-power-of-two case can be handled.


#ifndef __vtkCosmoHaloFinder_h
#define __vtkCosmoHaloFinder_h

#include "vtkUnstructuredGridAlgorithm.h"

struct ValueIdPair
{
  float value;
  int id;
};

typedef struct ValueIdPair ValueIdPair;

class VTK_EXPORT vtkCosmoHaloFinder : public vtkUnstructuredGridAlgorithm
{
 public:
  // Description:
  static vtkCosmoHaloFinder *New();

  vtkTypeRevisionMacro(vtkCosmoHaloFinder,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /*
  // Number of particles seeded in each dim of the original simulation app. From the source.
  vtkSetMacro(np,int);
  vtkGetMacro(np,int);
  */

  // Minimal number of particles needed before a group is called a halo.
  vtkSetMacro(pmin,int);
  vtkGetMacro(pmin,int);

  // Linking length measured in units of interparticle spacing is dimensionless.
  vtkSetMacro(bb,double);
  vtkGetMacro(bb,double);

  // Physical length of the box.
  vtkSetMacro(rL,double);
  vtkGetMacro(rL,double);

  // If the box is periodic on the boundaries.
  vtkSetMacro(Periodic,bool);
  vtkGetMacro(Periodic,bool);

  // Process the complete time series
  vtkSetMacro(BatchMode, bool);
  vtkGetMacro(BatchMode, bool);

  void SetOutputDirectory(const char* dir);

 protected:
  // np.in
  int np;
  double rL;
  double bb;
  int pmin;
  bool Periodic;

  // if true, the filter will process all the time steps, otherwise only the current time step
  bool BatchMode;
  // output path for the new dataset

  // internal state
  int npart;
  int nhalo;
  int nhalopart;
  int realnpart;

  //  float *xx, *yy, *zz, *vx, *vy, *vz, *pm;
  int *pt;
  int *ht;

  //  vtkDataArray *velocity, *mass, *tag, *hID;

  //  float xscal;

  int *halo;
  int *nextp;

  int *seq;

  ValueIdPair *v;
  float **data;

  float **lb;
  float **ub;

  vtkCosmoHaloFinder();
  ~vtkCosmoHaloFinder();

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  int CurrentTimeIndex;
  int NumberOfTimeSteps;

  char* outputDir;
  
 private:

  vtkCosmoHaloFinder(const vtkCosmoHaloFinder&);  // Not implemented.
  void operator=(const vtkCosmoHaloFinder&);  // Not implemented.

  //void Reading(vtkDataSet *);
  //void Finding();
  //void Writing(vtkUnstructuredGrid *);

  void Reorder(int, int, int);

  void myFOF(int, int, int);

  void Merge(int, int, int, int, int);
  void basicMerge(int, int);

  void WritePVDFile(vtkInformationVector** inputVector);
};

#endif //  __vtkCosmoHaloFinder_h
