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

// .NAME FOFDistribute - distribute halos to processors
//
// .SECTION Description
// FOFDistribute takes a series of data files containing RECORD style
// .cosmo data or Gadget style BLOCK data
// along with parameters defining the box size for the data and for
// determining halos within the halo data.  It distributes the data
// across processors including a healthy dead zone of halos belonging
// to neighbor processors.  By definition all halos can be determined
// completely for any processor because of this dead zone.  The serial
// halo finder is called on each processor.
//

#ifndef FOFDistribute_h
#define FOFDistribute_h

#include "Message.h"

#include <cstdlib>


#include "Definition.h"
#include <string>
#include <vector>


using std::string;
using std::ifstream;
using std::vector;

namespace cosmotk {


class FOFDistribute {

public:
  FOFDistribute();
  ~FOFDistribute();

  // Set parameters halo distribution
  void setParameters(
        const string& inName,   // Base file name to read from
        POSVEL_T rL);           // Box size of the physical problem

  // Set neighbor processor numbers and calculate dead regions
  void initialize();

#ifndef USE_SERIAL_COSMO
  struct CosmoHalo
  {
    float floatData[11];
    long  intData[2];
  };

  void readHalosGIO(int reserveQ = 0, bool useAlltoallv = true,
                    bool redistCenter = true);
#endif

  // Return data needed by other software
  int     getHaloCount()    { return this->haloCount; }

  void setHalos(vector<POSVEL_T>* xx,
                vector<POSVEL_T>* yy,
                vector<POSVEL_T>* zz,
                vector<POSVEL_T>* vx,
                vector<POSVEL_T>* vy,
                vector<POSVEL_T>* vz,
                vector<POSVEL_T>* mass,
                vector<POSVEL_T>* cx,
                vector<POSVEL_T>* cy,
                vector<POSVEL_T>* cz,
                vector<POSVEL_T>* vdisp,
                vector<ID_T>* tag,
                vector<int>* count);

  vector<POSVEL_T>* getXLocation()       { return this->xx; }
  vector<POSVEL_T>* getYLocation()       { return this->yy; }
  vector<POSVEL_T>* getZLocation()       { return this->zz; }
  vector<POSVEL_T>* getXVelocity()       { return this->vx; }
  vector<POSVEL_T>* getYVelocity()       { return this->vy; }
  vector<POSVEL_T>* getZVelocity()       { return this->vz; }
  vector<POSVEL_T>* getMass()            { return this->ms; }
  vector<POSVEL_T>* getCenterXLocation() { return this->cx; }
  vector<POSVEL_T>* getCenterYLocation() { return this->cy; }
  vector<POSVEL_T>* getCenterZLocation() { return this->cz; }
  vector<POSVEL_T>* getVDisp()           { return this->vdisp; }
  vector<ID_T>* getTag()                 { return this->tag; }
  vector<int>*  getCount()               { return this->count; }

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  string baseFile;              // Base name of input halo files

  int    layoutSize[DIMENSION]; // Decomposition of processors
  int    layoutPos[DIMENSION];  // Position of this processor in decomposition

  POSVEL_T boxSize;             // Physical box size (rL)

  long   numberOfAliveHalos;

  long   haloCount;         // Running index used to store data
                                // Ends up as the number of alive plus dead

  POSVEL_T minAlive[DIMENSION]; // Minimum alive halo location on processor
  POSVEL_T maxAlive[DIMENSION]; // Maximum alive halo location on processor

  int    neighbor[NUM_OF_NEIGHBORS];            // Neighbor processor ids

  vector<POSVEL_T>* xx;         // X location for halos on this processor
  vector<POSVEL_T>* yy;         // Y location for halos on this processor
  vector<POSVEL_T>* zz;         // Z location for halos on this processor
  vector<POSVEL_T>* vx;         // X velocity for halos on this processor
  vector<POSVEL_T>* vy;         // Y velocity for halos on this processor
  vector<POSVEL_T>* vz;         // Z velocity for halos on this processor
  vector<POSVEL_T>* ms;         // Mass for halos on this processor
  vector<POSVEL_T>* cx;         // center X location for halos on this processor
  vector<POSVEL_T>* cy;         // center Y location for halos on this processor
  vector<POSVEL_T>* cz;         // center Z location for halos on this processor
  vector<POSVEL_T>* vdisp;      // velocity dispersion for halos on this processor
  vector<int>*  count;          // Particle count for halos on this processor
  vector<ID_T>* tag;            // Id tag for halos on this processor
};

}
#endif
