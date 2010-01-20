/*=========================================================================
                                                                                
Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
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

// .NAME CosmoHaloFinder - find halos within a cosmology data file
// .SECTION Description
// CosmoHaloFinder is a filter object that operates on the unstructured 
// grid created when a CosmoReader reads a .cosmo data file. 
// It operates by finding clusters of neighbors.
//
// .SECTION Note
// this finder implements a recursive algorithm.
// linked lists are used for halos.
// merge is done recursively.
// each interval has its bounding box calculated in Reorder()
// JimRecOptList

#ifndef CosmoHaloFinder_h
#define CosmoHaloFinder_h

#include <string>

#include "Definition.h"

#define numDataDims 3
#define dataX 0
#define dataY 1
#define dataZ 2

using namespace std;

/****************************************************************************/
typedef POSVEL_T* floatptr;

//
// Particle information for reordering the particles according to position
// Value is either the X, Y or Z position depending on the recursion
// Id in the standalone serial version is the particle tag
// Id in the parallel version is the index of that particle on a
// particular processor which is why it can be int and not ID_T
//
struct ValueIdPair {
  POSVEL_T value;
  int id;
};

class ValueIdPairLT {
public:
  bool operator() (const ValueIdPair& p, const ValueIdPair& q) const
  {
  return p.value < q.value;
  }
};

/****************************************************************************/

class CosmoHaloFinder
{
public:
  // create a finder
  CosmoHaloFinder();
  ~CosmoHaloFinder();

  void Finding();

  // Read alive particles
#ifndef USE_VTK_COSMO
  void Reading();
  void Writing();

  // execute the finder
  void Execute();
#endif

  void setInFile(string inFile)         { infile = inFile.c_str(); }
  void setOutFile(string outFile)       { outfile = outFile.c_str(); }

  void setParticleLocations(POSVEL_T** d) { data = d; }
  void setNumberOfParticles(int n)      { npart = n; }
  void setMyProc(int r)                 { myProc = r; }

  int* getHaloTag()                     { return ht; }

  POSVEL_T* getXLoc()                   { return xx; }
  POSVEL_T* getYLoc()                   { return yy; }
  POSVEL_T* getZLoc()                   { return zz; }
  POSVEL_T* getXVel()                   { return vx; }
  POSVEL_T* getYVel()                   { return vy; }
  POSVEL_T* getZVel()                   { return vz; }
  POSVEL_T* getMass()                   { return ms; }
  int*   getTag()                       { return pt; }
  
  // np.in
  int np;
  POSVEL_T rL;
  POSVEL_T bb;
  int pmin;
  bool periodic;
  const char *infile;
  const char *outfile;
  const char *textmode;

private:

  // input/output interface
  POSVEL_T *xx, *yy, *zz, *vx, *vy, *vz, *ms;
  int   *pt, *ht;

  // internal state
  int npart, nhalo, nhalopart;
  int myProc;

  // data[][] stores xx[], yy[], zz[].
  POSVEL_T **data;

  // scale factor
  POSVEL_T xscal, vscal;

  int *halo, *nextp, *hsize;

  ValueIdPair *v;
  int *seq;
  void Reorder
    (int first, 
     int last, 
     int flag);

  POSVEL_T **lb, **ub;
  void ComputeLU(int, int);

  void myFOF(int, int, int);
  void Merge(int, int, int, int, int);
};

#endif
