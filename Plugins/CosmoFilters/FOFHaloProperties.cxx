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

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <math.h>

#include "Partition.h"
#include "FOFHaloProperties.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// FOFHaloProperties uses the results of the CosmoHaloFinder to locate the
// particle within every halo in order to calculate properties on halos
//
/////////////////////////////////////////////////////////////////////////

FOFHaloProperties::FOFHaloProperties()
{
  // Get the number of processors and rank of this processor
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();
}

FOFHaloProperties::~FOFHaloProperties()
{
}

/////////////////////////////////////////////////////////////////////////
//
// Set chaining structure which will locate all particles in a halo
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::setHalos(
                        int numberHalos,
                        int* haloStartIndex,
                        int* haloParticleCount,
                        int* nextParticleIndex)
{
  this->numberOfHalos = numberHalos;
  this->halos = haloStartIndex;
  this->haloCount = haloParticleCount;
  this->haloList = nextParticleIndex;
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for the halo center finder
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::setParameters(
                        const string& outName,
                        POSVEL_T rL,
                        POSVEL_T deadSz,
                        POSVEL_T pMass)
{
  // Particles for this processor output to file
  ostringstream oname, hname;
  if (this->numProc == 1) {
    oname << outName;
    hname << outName;
  } else {
    oname << outName << "." << myProc;
    hname << outName << ".halo." << myProc;
  }
  this->outFile = oname.str();

  // Halo finder parameters
  this->boxSize = rL;
  this->deadSize = deadSz;
  this->particleMass = pMass;
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::setParticles(
                        vector<POSVEL_T>* xLoc,
                        vector<POSVEL_T>* yLoc,
                        vector<POSVEL_T>* zLoc,
                        vector<POSVEL_T>* xVel,
                        vector<POSVEL_T>* yVel,
                        vector<POSVEL_T>* zVel,
                        vector<POTENTIAL_T>* potential,
                        vector<ID_T>* id,
                        vector<MASK_T>* maskData,
                        vector<STATUS_T>* state)
{
  this->particleCount = (long)xLoc->size();

  // Extract the contiguous data block from a vector pointer
  this->xx = &(*xLoc)[0];
  this->yy = &(*yLoc)[0];
  this->zz = &(*zLoc)[0];
  this->vx = &(*xVel)[0];
  this->vy = &(*yVel)[0];
  this->vz = &(*zVel)[0];
  this->pot = &(*potential)[0];
  this->tag = &(*id)[0];
  this->mask = &(*maskData)[0];
  this->status = &(*state)[0];
}

/////////////////////////////////////////////////////////////////////////
//
// Find the index of the particle at the center of every FOF halo which is the
// particle with the minimum value in the potential array.
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloCenter(vector<int>* haloCenter)
{
  for (int halo = 0; halo < this->numberOfHalos; halo++) {
                        
    // First particle in halo
    int p = this->halos[halo];
    POTENTIAL_T minPotential = this->pot[p];
    int centerIndex = p;
  
    // Next particle
    p = this->haloList[p];
  
    // Search for minimum
    while (p != -1) {
      if (minPotential > this->pot[p]) {
        minPotential = this->pot[p];
        centerIndex = p;
      }
      p = this->haloList[p];
    }

    // Save the minimum potential index for this halo
    (*haloCenter).push_back(centerIndex);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Find the index of the particle at the center of every FOF halo and return
// in the given vector.  Use the minimumPotential() algorithm
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloCenterN2_2(vector<int>* haloCenter)
{
  int smallHalo = 0;
  int largeHalo = 0;
  POTENTIAL_T minPotential;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    if (this->haloCount[halo] <= 100) {
      smallHalo++;
      int centerIndex = minimumPotentialN2_2(halo, &minPotential);
      (*haloCenter).push_back(centerIndex);
    } else {
      largeHalo++;
      (*haloCenter).push_back(-1);
    }
  }

#ifndef USE_VTK_COSMO
  cout << "Rank " << this->myProc 
       << " #small = " << smallHalo
       << " #large = " << largeHalo << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the mass of every FOF halo
//
// m_FOF = m_P * n_FOF
//    m_FOF is the mass of an FOF halo
//    n_FOF is the number of particles in the halo
//    m_P is (Omega_m * rho_c * L^3) / N_p
//       Omega_m is ratio of mass density to critical density
//       rho_c is 2.7755E11
//       L is length of one side of the simulation box
//       N_p is total number of particles in the simulation (n_p^3)
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloMass(
                        vector<POSVEL_T>* haloMass)
{
  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    POSVEL_T mass = this->particleMass * this->haloCount[halo];
    (*haloMass).push_back(mass);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the average position of particles of every FOF halo
//
// x_FOF = ((Sum i=1 to n_FOF) x_i) / n_FOF
//    x_FOF is the average position vector
//    n_FOF is the number of particles in the halo
//    x_i is the position vector of particle i
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFPosition(
                        vector<POSVEL_T>* xMeanPos,
                        vector<POSVEL_T>* yMeanPos,
                        vector<POSVEL_T>* zMeanPos)
{
  POSVEL_T xMean, yMean, zMean;
  double xKahan, yKahan, zKahan;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    xKahan = KahanSummation(halo, this->xx);
    yKahan = KahanSummation(halo, this->yy);
    zKahan = KahanSummation(halo, this->zz);

    xMean = (POSVEL_T) (xKahan / this->haloCount[halo]);
    yMean = (POSVEL_T) (yKahan / this->haloCount[halo]);
    zMean = (POSVEL_T) (zKahan / this->haloCount[halo]);

    (*xMeanPos).push_back(xMean);
    (*yMeanPos).push_back(yMean);
    (*zMeanPos).push_back(zMean);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the average velocity of particles of every FOF halo
//
// v_FOF = ((Sum i=1 to n_FOF) v_i) / n_FOF
//    v_FOF is the average velocity vector
//    n_FOF is the number of particles in the halo
//    v_i is the velocity vector of particle i
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFVelocity(
                        vector<POSVEL_T>* xMeanVel,
                        vector<POSVEL_T>* yMeanVel,
                        vector<POSVEL_T>* zMeanVel)
{
  POSVEL_T xMean, yMean, zMean;
  double xKahan, yKahan, zKahan;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    xKahan = KahanSummation(halo, this->vx);
    yKahan = KahanSummation(halo, this->vy);
    zKahan = KahanSummation(halo, this->vz);

    xMean = (POSVEL_T) (xKahan / this->haloCount[halo]);
    yMean = (POSVEL_T) (yKahan / this->haloCount[halo]);
    zMean = (POSVEL_T) (zKahan / this->haloCount[halo]);

    (*xMeanVel).push_back(xMean);
    (*yMeanVel).push_back(yMean);
    (*zMeanVel).push_back(zMean);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the velocity dispersion of every FOF halo
//
// o_FOF = sqrt((avg_part_vel_dot_prod - dot_prod_halo_vel) / 3)
//    avg_part_vel_dot_prod = ((Sum i=1 to n_FOF) v_i dot v_i) / n_FOF
//       n_FOF is the number of particles in the halo
//       v_i is the velocity vector of particle i
//    dot_prod_halo_vel = v_FOF dot v_FOF
//       v_FOF is the average velocity vector of all particles in the halo
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFVelocityDispersion(
                        vector<POSVEL_T>* xAvgVel,
                        vector<POSVEL_T>* yAvgVel,
                        vector<POSVEL_T>* zAvgVel,
                        vector<POSVEL_T>* velDisp)
{
  for (int halo = 0; halo < this->numberOfHalos; halo++) {

    // First particle in the halo
    int p = this->halos[halo];
    POSVEL_T particleDot = 0.0;

    // Iterate over all particles in the halo collecting dot products
    while (p != -1) {
      particleDot += dotProduct(this->vx[p], this->vy[p], this->vz[p]);
      p = this->haloList[p];
    }

    // Average of all the dot products
    particleDot /= this->haloCount[halo];

    // Dot product of the average velocity for the entire halo
    POSVEL_T haloDot = dotProduct((*xAvgVel)[halo], 
                                  (*yAvgVel)[halo], (*zAvgVel)[halo]);

    // Velocity dispersion
    POSVEL_T vDispersion = sqrt((particleDot - haloDot) / 3.0);

    // Save onto supplied vector
    velDisp->push_back(vDispersion);
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Dot product of a vector
//
/////////////////////////////////////////////////////////////////////////

POSVEL_T FOFHaloProperties::dotProduct(POSVEL_T x, POSVEL_T y, POSVEL_T z)
{
  POSVEL_T dotProd = x * x + y * y + z * z;
  return dotProd;
}

/////////////////////////////////////////////////////////////////////////
//                      
// Calculate the Kahan summation
// Reduces roundoff error in floating point arithmetic
//                      
/////////////////////////////////////////////////////////////////////////
                        
POSVEL_T FOFHaloProperties::KahanSummation(int halo, POSVEL_T* data)
{                       
  POSVEL_T dataSum, dataRem, v, w;
                        
  // First particle in halo and first step in Kahan summation
  int p = this->halos[halo];
  dataSum = data[p];
  dataRem = 0.0;
  
  // Next particle
  p = this->haloList[p];
  
  // Remaining steps in Kahan summation
  while (p != -1) {
    v = data[p] - dataRem;
    w = dataSum + v;
    dataRem = (w - dataSum) - v;
    dataSum = w;

    p = this->haloList[p];
  }
  return dataSum;
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the incremental mean using Kahan summation
//
/////////////////////////////////////////////////////////////////////////

POSVEL_T FOFHaloProperties::incrementalMean(int halo, POSVEL_T* data)
{
  double dataMean, dataRem, diff, value, v, w;

  // First particle in halo and first step in incremental mean
  int p = this->halos[halo];
  dataMean = data[p];
  dataRem = 0.0;
  int count = 1;

  // Next particle
  p = this->haloList[p];
  count++;

  // Remaining steps in incremental mean
  while (p != -1) {
    diff = data[p] - dataMean;
    value = diff / count;
    v = value - dataRem;
    w = dataMean + v;
    dataRem = (w - dataMean) - v;
    dataMean = w;

    p = this->haloList[p];
    count++;
  }
  return (POSVEL_T) dataMean;
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the mimimum potential or halo center using (N*(N-1)) / 2 algorithm
// Locations of the particles have taken wraparound into account so that
// processors on the low edge of a dimension have particles with negative
// positions and processors on the high edge of a dimension have particles
// with locations greater than the box size
//
/////////////////////////////////////////////////////////////////////////

int FOFHaloProperties::minimumPotentialN2_2(int halo, POTENTIAL_T* minPotential)
{
  // Arrange in an upper triangular grid to save computation
  POTENTIAL_T* lpot = new POTENTIAL_T[this->haloCount[halo]];
  int* actualIndx = new int[this->haloCount[halo]];

  // Initialize potential and set the actual index of the particle
  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    lpot[i] = 0.0;
    actualIndx[i] = p;
    p = this->haloList[p];
  }

  // First particle in halo to calculate minimum potential on
  p = this->halos[halo];
  int indx1 = 0;
  int indx2 = 1;

  while (p != -1 && indx1 < this->haloCount[halo]) {

    // Next particle in halo in minimum potential loop
    int q = this->haloList[p];
    indx2 = indx1 + 1;

    while (q != -1) {

      POSVEL_T xdist = fabs(this->xx[p] - this->xx[q]);
      POSVEL_T ydist = fabs(this->yy[p] - this->yy[q]);
      POSVEL_T zdist = fabs(this->zz[p] - this->zz[q]);

      POSVEL_T r = sqrt((xdist * xdist) + (ydist * ydist) + (zdist * zdist));
      if (r != 0.0) {
        lpot[indx1] = lpot[indx1] - (1.0 / r);
        lpot[indx2] = lpot[indx2] - (1.0 / r);
      }

      // Next particle
      q = this->haloList[q];
      indx2++;
    }

    // Next particle
    p = this->haloList[p];
    indx1++;
  }

  *minPotential = MAX_FLOAT;
  int minIndex = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    if (lpot[i] < *minPotential) {
      *minPotential = lpot[i];
      minIndex = i;
    }
  } 
  int result = actualIndx[minIndex];
  delete [] lpot;
  delete [] actualIndx;

  return result;
}

#ifndef USE_VTK_COSMO
/////////////////////////////////////////////////////////////////////////
//
// For each processor print the halo index and size for debugging
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::printHaloSizes(int minSize)
{
  for (int i = 0; i < this->numberOfHalos; i++)
    if (this->haloCount[i] > minSize)
      cout << "Rank " << Partition::getMyProc() 
           << " Halo " << i 
           << " size = " << this->haloCount[i] << endl;
}
 
/////////////////////////////////////////////////////////////////////////
//
// For the requested processor and halo index output locations for
// a scatter plot for debugging
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::printLocations(int halo)
{
  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    cout << "FOF INFO " << this->myProc << " " << halo
         << " INDEX " << p << " TAG " << this->tag[p] << " LOCATION " 
         << this->xx[p] << " " << this->yy[p] << " " << this->zz[p] << endl;
    p = this->haloList[p];
  }
}

/////////////////////////////////////////////////////////////////////////
//
// For the requested processor and halo index output bounding box
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::printBoundingBox(int halo)
{
  POSVEL_T minBox[DIMENSION], maxBox[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    minBox[dim] = this->boxSize;
    maxBox[dim] = 0.0;
  }

  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {

    if (minBox[0] > this->xx[p])
      minBox[0] = this->xx[p];
    if (maxBox[0] < this->xx[p])
      maxBox[0] = this->xx[p];

    if (minBox[1] > this->yy[p])
      minBox[1] = this->yy[p];
    if (maxBox[1] < this->yy[p])
      maxBox[1] = this->yy[p];

    if (minBox[2] > this->zz[p])
      minBox[2] = this->zz[p];
    if (maxBox[2] < this->zz[p])
      maxBox[2] = this->zz[p];

    p = this->haloList[p];
  }
  cout << "FOF BOUNDING BOX " << this->myProc << " " << halo << ": " 
         << minBox[0] << ":" << maxBox[0] << "  "
         << minBox[1] << ":" << maxBox[1] << "  "
         << minBox[2] << ":" << maxBox[2] << "  " << endl;
}
#endif
