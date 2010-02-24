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
#include "ChainingMesh.h"

#ifndef USE_VTK_COSMO
#include "Timings.h"
#endif

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
                        POSVEL_T pMass,
                        POSVEL_T pDist)
{
  this->outFile = outName;

  // Halo finder parameters
  this->boxSize = rL;
  this->deadSize = deadSz;
  this->particleMass = pMass;
  this->bb = pDist;
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

void FOFHaloProperties::FOFHaloCenterMinimumPotential(vector<int>* haloCenter)
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
// Find the index of the most bound particle which is the particle
// closest to every other particle in the halo.
// Use the minimumPotential() algorithm
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloCenterMBP(vector<int>* haloCenter)
{
  int smallHalo = 0;
  int largeHalo = 0;
  int centerIndex;
  POTENTIAL_T minPotential;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {
    if (this->haloCount[halo] < 10000) {
#ifndef USE_VTK_COSMO
      static Timings::TimerRef stimer = Timings::getTimer("N2 MBP");
      Timings::startTimer(stimer);
#endif
      smallHalo++;
      centerIndex = mostBoundParticleN2(halo, &minPotential);
      (*haloCenter).push_back(centerIndex);
#ifndef USE_VTK_COSMO
      Timings::stopTimer(stimer);
#endif
    }

     else {
#ifndef USE_VTK_COSMO
      static Timings::TimerRef ltimer = Timings::getTimer("ASTAR MBP");
      Timings::startTimer(ltimer);
#endif
      largeHalo++;
      centerIndex = mostBoundParticleAStar(halo);
      (*haloCenter).push_back(centerIndex);
#ifndef USE_VTK_COSMO
      Timings::stopTimer(ltimer);
#endif
    }
  }

#ifndef USE_VTK_COSMO
  cout << "MBP Rank " << this->myProc 
       << " #small = " << smallHalo
       << " #large = " << largeHalo << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Find the index of the most connected particle which is the particle
// with the most friends (most particles within halo interparticle distance)
// Use the mostConnectedParticle() algorithm
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloCenterMCP(vector<int>* haloCenter)
{
  int smallHalo = 0;
  int largeHalo = 0;
  int centerIndex;

  for (int halo = 0; halo < this->numberOfHalos; halo++) {

    if (this->haloCount[halo] < 10000) {
#ifndef USE_VTK_COSMO
      static Timings::TimerRef smtimer = Timings::getTimer("N2 MCP");
      Timings::startTimer(smtimer);
#endif
      smallHalo++;
      centerIndex = mostConnectedParticleN2(halo);
#ifndef USE_VTK_COSMO
      Timings::stopTimer(smtimer);
#endif
    }

    else {
#ifndef USE_VTK_COSMO
      static Timings::TimerRef latimer = Timings::getTimer("ChainMesh MCP");
      Timings::startTimer(latimer);
#endif
      largeHalo++;
      centerIndex = mostConnectedParticleChainMesh(halo);
#ifndef USE_VTK_COSMO
      Timings::stopTimer(latimer);
#endif
    }
    (*haloCenter).push_back(centerIndex);
  }

#ifndef USE_VTK_COSMO
  cout << "MCP Rank " << this->myProc 
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
    POSVEL_T vDispersion = (POSVEL_T)sqrt((particleDot - haloDot) / 3.0);

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
// Calculate the most connected particle using (N*(N-1)) / 2 algorithm
// This is the particle with the most friends (most particles within bb)
// Locations of the particles have taken wraparound into account so that
// processors on the low edge of a dimension have particles with negative
// positions and processors on the high edge of a dimension have particles
// with locations greater than the box size
//
/////////////////////////////////////////////////////////////////////////

int FOFHaloProperties::mostConnectedParticleN2(int halo)
{       
  // Arrange in an upper triangular grid of friend counts
  // friendCount will hold number of friends that a particle has
  // actualIndx will hold the particle index matching that halo index
  //
  int* friendCount = new int[this->haloCount[halo]];
  int* actualIndx = new int[this->haloCount[halo]];

  // Initialize friend count and set the actual index of the particle
  // so that at the end we can return a particle index not halo index
  int p = this->halos[halo];
  for (int i = 0; i < this->haloCount[halo]; i++) {
    friendCount[i] = 0;
    actualIndx[i] = p;
    p = this->haloList[p];
  }

  // Iterate on all particles in halo adding to count if friends of each other
  // Iterate in upper triangular fashion
  POSVEL_T cutoff = this->bb * this->bb;
  p = this->halos[halo];
  int indx1 = 0;
  int indx2 = 1;

  while (p != -1) {

    // Get halo particle after the current one
    int q = this->haloList[p];
    indx2 = indx1 + 1;
    while (q != -1) {

      // Calculate distance betweent the two
      POSVEL_T xdist = fabs(this->xx[p] - this->xx[q]);
      POSVEL_T ydist = fabs(this->yy[p] - this->yy[q]);
      POSVEL_T zdist = fabs(this->zz[p] - this->zz[q]);
      
      if ((xdist < this->bb) && (ydist < this->bb) && (zdist < this->bb)) {
        POSVEL_T dist = (xdist * xdist) + (ydist * ydist) + (zdist * zdist);
        if (dist < cutoff) {
          friendCount[indx1]++;
          friendCount[indx2]++;
        }
      }
      // Next inner particle
      q = this->haloList[q];
      indx2++;
    }
    // Next outer particle
    p = this->haloList[p];
    indx1++;
  }

  // Particle with the most friends
  int maxFriends = 0;
  int result = this->halos[halo];

  for (int i = 0; i < this->haloCount[halo]; i++) {
    if (friendCount[i] > maxFriends) {
      maxFriends = friendCount[i];
      result = actualIndx[i];
    }
  }

  delete [] friendCount;
  delete [] actualIndx;
  return result;
}

/////////////////////////////////////////////////////////////////////////
//
// Most connected particle using a chaining mesh of particles in one FOF halo
// Build chaining mesh with a grid size such that all friends will be in
// adjacent mesh grids.
//
/////////////////////////////////////////////////////////////////////////

int FOFHaloProperties::mostConnectedParticleChainMesh(int halo)
{
  // Transfer the locations for this halo into separate vectors
  // for the building of the ChainingMesh
  POSVEL_T* xLocHalo = new POSVEL_T[this->haloCount[halo]];
  POSVEL_T* yLocHalo = new POSVEL_T[this->haloCount[halo]];
  POSVEL_T* zLocHalo = new POSVEL_T[this->haloCount[halo]];

  // Save the number of friends for each particle in the halo
  // Save the actual particle tag corresponding the particle index within halo
  int* friendCount = new int[this->haloCount[halo]];
  int* actualIndx = new int[this->haloCount[halo]];

  // Find the bounding box of this halo
  POSVEL_T* minLoc = new POSVEL_T[DIMENSION];
  POSVEL_T* maxLoc = new POSVEL_T[DIMENSION];

  // Initialize by finding bounding box, moving locations, setting friends
  // to zero and setting the actual particle tag corresponding to halo index
  int p = this->halos[halo];
  minLoc[0] = maxLoc[0] = this->xx[p];
  minLoc[1] = maxLoc[1] = this->yy[p];
  minLoc[2] = maxLoc[2] = this->zz[p];

  for (int i = 0; i < this->haloCount[halo]; i++) {

    xLocHalo[i] = this->xx[p];
    yLocHalo[i] = this->yy[p];
    zLocHalo[i] = this->zz[p];

    if (minLoc[0] > this->xx[p]) minLoc[0] = this->xx[p];
    if (maxLoc[0] < this->xx[p]) maxLoc[0] = this->xx[p];
    if (minLoc[1] > this->yy[p]) minLoc[1] = this->yy[p];
    if (maxLoc[1] < this->yy[p]) maxLoc[1] = this->yy[p];
    if (minLoc[2] > this->zz[p]) minLoc[2] = this->zz[p];
    if (maxLoc[2] < this->zz[p]) maxLoc[2] = this->zz[p];

    friendCount[i] = 0;
    actualIndx[i] = p;
    p = this->haloList[p];
  }

  // Round up and down for chaining mesh creation
  for (int dim = 0; dim < DIMENSION; dim++) {
    minLoc[dim] = floor(minLoc[dim]);
    maxLoc[dim] = ceil(maxLoc[dim]);
  }

  // Build the chaining mesh
  int chainFactor = 5;
  POSVEL_T chainSize = this->bb / chainFactor;

  ChainingMesh haloChain(minLoc, maxLoc, chainSize,
                         this->haloCount[halo], xLocHalo, yLocHalo, zLocHalo); 

  int*** buckets = haloChain.getBuckets();
  int* bucketList = haloChain.getBucketList();
  int* meshSize = haloChain.getMeshSize();

  // Walking window extents and size
  int first[DIMENSION], last[DIMENSION];

  // Walk every bucket in the chaining mesh, processing all particles in bucket
  // Examine particles in a walking window around the current bucket
  for (int bi = 0; bi < meshSize[0]; bi++) {
    for (int bj = 0; bj < meshSize[1]; bj++) {
      for (int bk = 0; bk < meshSize[2]; bk++) {
   
        // Set the walking window around this bucket
        for (int dim = 0; dim < DIMENSION; dim++) {
          first[dim] = bi - chainFactor;
          last[dim] = bi + chainFactor;
          if (first[dim] < 0)
            first[dim] = 0;
          if (last[dim] >= meshSize[dim])
            last[dim] = meshSize[dim] - 1;
        }

        // First particle in the bucket being processed
        int bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // For the current particle in the current bucket
          // compare it against all particles in the walking window buckets
          for (int wi = first[0]; wi <= last[0]; wi++) {
            for (int wj = first[1]; wj <= last[1]; wj++) {
              for (int wk = first[2]; wk <= last[2]; wk++) {
    
                // Iterate on all particles in this bucket
                int wp = buckets[wi][wj][wk];
                while (wp != -1) {
    
                  // Calculate distance between the two
                  POSVEL_T xdist = fabs(this->xx[bp] - this->xx[wp]);
                  POSVEL_T ydist = fabs(this->yy[bp] - this->yy[wp]);
                  POSVEL_T zdist = fabs(this->zz[bp] - this->zz[wp]);
    
                  if ((xdist < this->bb) && 
                      (ydist < this->bb) && 
                      (zdist < this->bb)) {
                        POSVEL_T dist = 
                          (xdist * xdist) + (ydist * ydist) + (zdist * zdist);
                        if (dist < this->bb)
                          friendCount[bp]++;
                  }
                  wp = bucketList[wp];
                }
              }
            }
          }
          bp = bucketList[bp];
        }
      }
    }
  }
  // Particle with the most friends
  int maxFriends = 0;
  int result = this->halos[halo];

  for (int i = 0; i < this->haloCount[halo]; i++) {
    if (friendCount[i] > maxFriends) {
      maxFriends = friendCount[i];
      result = actualIndx[i];
    }
  }
  delete [] xLocHalo;
  delete [] yLocHalo;
  delete [] zLocHalo;
  delete [] minLoc;
  delete [] maxLoc;
  delete [] friendCount;
  delete [] actualIndx;
  return result;
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the most bound particle using (N*(N-1)) / 2 algorithm
// Also minimum potential particle for the halo.
// Locations of the particles have taken wraparound into account so that
// processors on the low edge of a dimension have particles with negative
// positions and processors on the high edge of a dimension have particles
// with locations greater than the box size
//
/////////////////////////////////////////////////////////////////////////

int FOFHaloProperties::mostBoundParticleN2(int halo, POTENTIAL_T* minPotential)
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
        lpot[indx1] = (POTENTIAL_T)(lpot[indx1] - (1.0 / r));
        lpot[indx2] = (POTENTIAL_T)(lpot[indx2] - (1.0 / r));
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

/////////////////////////////////////////////////////////////////////////
//
// Most bound particle using a chaining mesh of particles in one FOF halo
// Build chaining mesh with a grid size such that all friends will be in
// adjacent mesh grids.  Use estimates for distant mesh grids by taking
// the number of particles for the entire grid and applying the distance
// at the center of the grid to add.
//
// A first pass on this should eliminate particles at the edges
// A* algorithm will increasingly refine the number for smaller and
// smallers sets of particles
//
/////////////////////////////////////////////////////////////////////////

int FOFHaloProperties::mostBoundParticleAStar(int halo)
{
  // Transfer the locations for this halo into separate vectors
  // for the building of the ChainingMesh
  POSVEL_T* xLocHalo = new POSVEL_T[this->haloCount[halo]];
  POSVEL_T* yLocHalo = new POSVEL_T[this->haloCount[halo]];
  POSVEL_T* zLocHalo = new POSVEL_T[this->haloCount[halo]];
  int* actualIndx = new int[this->haloCount[halo]];
  POSVEL_T* estimate = new POSVEL_T[this->haloCount[halo]];

  // Find the bounding box of this halo
  POSVEL_T* minLoc = new POSVEL_T[DIMENSION];
  POSVEL_T* maxLoc = new POSVEL_T[DIMENSION];

  // Initialize by finding bounding box, moving locations,
  // and setting the actual particle tag corresponding to halo index
  int p = this->halos[halo];
  minLoc[0] = maxLoc[0] = this->xx[p];
  minLoc[1] = maxLoc[1] = this->yy[p];
  minLoc[2] = maxLoc[2] = this->zz[p];

  for (int i = 0; i < this->haloCount[halo]; i++) {

    xLocHalo[i] = this->xx[p];
    yLocHalo[i] = this->yy[p];
    zLocHalo[i] = this->zz[p];

    if (minLoc[0] > this->xx[p]) minLoc[0] = this->xx[p];
    if (maxLoc[0] < this->xx[p]) maxLoc[0] = this->xx[p];
    if (minLoc[1] > this->yy[p]) minLoc[1] = this->yy[p];
    if (maxLoc[1] < this->yy[p]) maxLoc[1] = this->yy[p];
    if (minLoc[2] > this->zz[p]) minLoc[2] = this->zz[p];
    if (maxLoc[2] < this->zz[p]) maxLoc[2] = this->zz[p];

    estimate[i] = 0.0;
    actualIndx[i] = p;
    p = this->haloList[p];
  }

  // Round up and down for the chaining mesh
  for (int dim = 0; dim < DIMENSION; dim++) {
    minLoc[dim] = floor(minLoc[dim]);
    maxLoc[dim] = ceil(maxLoc[dim]);
  }

  // Build the chaining mesh
  int chainFactor = 1;
  POSVEL_T chainSize = this->bb / chainFactor;

  ChainingMesh haloChain(minLoc, maxLoc, chainSize,
                         this->haloCount[halo], xLocHalo, yLocHalo, zLocHalo); 

  int*** bucketCount = haloChain.getBucketCount();
  int*** buckets = haloChain.getBuckets();
  int* bucketList = haloChain.getBucketList();
  int* meshSize = haloChain.getMeshSize();

  // Walking window extents and size
  int first[DIMENSION], last[DIMENSION];

  // Iterate on all buckets making an initial estimate of distance
  // Distance must be underestimated for A* iterations
  for (int bi = 0; bi < meshSize[0]; bi++) {
    for (int bj = 0; bj < meshSize[1]; bj++) {
      for (int bk = 0; bk < meshSize[2]; bk++) {

        // Set the walking window around this bucket
        for (int dim = 0; dim < DIMENSION; dim++) {
          first[dim] = bi - 2;
          last[dim] = bi + 2;
          if (first[dim] < 0)
            first[dim] = 0;
          if (last[dim] >= meshSize[dim])
            last[dim] = meshSize[dim] - 1;
        }

        int bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // For the current particle in the current bucket calculate
          // the actual part from the surrounding buckets
#ifndef USE_VTK_COSMO
      static Timings::TimerRef a1timer = Timings::getTimer("ASTAR ACTUAL");
      Timings::startTimer(a1timer);
#endif
          for (int wi = first[0]; wi <= last[0]; wi++) {
            for (int wj = first[1]; wj <= last[1]; wj++) {
              for (int wk = first[2]; wk <= last[2]; wk++) {

                // Iterate on all particles in this bucket
                int wp = buckets[wi][wj][wk];
                while (wp != -1) {
                  // Calculate distance between the two
                  POSVEL_T xdist = fabs(this->xx[bp] - this->xx[wp]);
                  POSVEL_T ydist = fabs(this->yy[bp] - this->yy[wp]);
                  POSVEL_T zdist = fabs(this->zz[bp] - this->zz[wp]);

                  POSVEL_T dist =
                          (xdist * xdist) + (ydist * ydist) + (zdist * zdist);
                  estimate[bp] += dist;

                  wp = bucketList[wp];
                }
              }
            }
          }
#ifndef USE_VTK_COSMO
      Timings::stopTimer(a1timer);

      static Timings::TimerRef a2timer = Timings::getTimer("ASTAR ESTIMATE");
      Timings::startTimer(a2timer);
#endif
          // Calculate the estimated part from all other buckets using
          // number of particles in the bucket and the closest corner distance
          for (int wi = 0; wi < meshSize[0]; wi++) {
            for (int wj = 0; wj <= meshSize[1]; wj++) {
              for (int wk = 0; wk <= meshSize[2]; wk++) {
              
                // Don't estimate for buckets that have been calculated in full
                if (wi < first[0] && wi > last[0] &&
                    wj < first[1] && wj > last[1] &&
                    wk < first[2] && wk > last[2]) {

                  // Lower corner of the compared bucket
                  POSVEL_T xCorner = wi * chainSize;
                  POSVEL_T yCorner = wj * chainSize;
                  POSVEL_T zCorner = wk * chainSize;

                  // Alter which corner to compare to depending on this bucket
                  if (this->xx[bp] > xCorner)
                    xCorner += chainSize;
                  if (this->yy[bp] > yCorner)
                    yCorner += chainSize;
                  if (this->zz[bp] > zCorner)
                    zCorner += chainSize;

                  // Distance of this particle to the corner
                  POSVEL_T xdist = fabs(this->xx[bp] - xCorner);
                  POSVEL_T ydist = fabs(this->yy[bp] - yCorner);
                  POSVEL_T zdist = fabs(this->zz[bp] - zCorner);

                  POSVEL_T weightedDist = bucketCount[wi][wj][wk] *
                          (xdist * xdist) + (ydist * ydist) + (zdist * zdist);
                  estimate[bp] += weightedDist;
                }
              }
            }
          }
#ifndef USE_VTK_COSMO
      Timings::stopTimer(a2timer);
#endif
          bp = bucketList[bp];
        }
      }
    }
  }

  // For now return the particle with the lowest estimate
  POSVEL_T minimumDistance = estimate[0];
  int result = actualIndx[0];

  for (int i = 0; i < this->haloCount[halo]; i++) {
    if (estimate[i] < minimumDistance) {
      minimumDistance = estimate[i];
      result = actualIndx[i];
    }
  }

  delete [] xLocHalo;
  delete [] yLocHalo;
  delete [] zLocHalo;
  delete [] estimate;
  delete [] actualIndx;

  return result;
}

#ifndef USE_VTK_COSMO
/////////////////////////////////////////////////////////////////////////
//
// Write the halo catalog file
//
// Output one entry per halo 
// Location (xx,yy,zz) is the location of particle closest to centroid
// Eventually this needs to be the particle with the minimum potential
// Velocity (vx,vy,vz) is the average velocity of all halo particles
// Mass is the #particles in the halo * mass of one particle
// Tag is the unique id of the halo
//
/////////////////////////////////////////////////////////////////////////

void FOFHaloProperties::FOFHaloCatalog(
                        vector<int>* haloCenter,
                        vector<POSVEL_T>* xMeanVel,
                        vector<POSVEL_T>* yMeanVel,
                        vector<POSVEL_T>* zMeanVel)
{
  // Compose ascii and .cosmo binary file names
  ostringstream aname, cname;
  if (this->numProc == 1) {
    aname << this->outFile << ".halocatalog.ascii";
    cname << this->outFile << ".halocatalog.cosmo";
  } else {
    aname << this->outFile << ".halocatalog.ascii." << myProc;
    cname << this->outFile << ".halocatalog.cosmo." << myProc;
  }
  ofstream aStream(aname.str().c_str(), ios::out);
  ofstream cStream(cname.str().c_str(), ios::out|ios::binary);

  char str[1024];
  float fBlock[COSMO_FLOAT];
  int iBlock[COSMO_INT];

  for (int halo = 0; halo < this->numberOfHalos; halo++) {

    int centerIndex = (*haloCenter)[halo];
    int haloTag = this->tag[this->halos[halo]];
    POSVEL_T haloMass = this->haloCount[halo] * particleMass;

    // Write ascii
    sprintf(str, "%12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12d\n", 
      this->xx[centerIndex],
      (*xMeanVel)[halo],
      this->yy[centerIndex],
      (*yMeanVel)[halo],
      this->zz[centerIndex],
      (*zMeanVel)[halo],
      haloMass, haloTag);
      aStream << str;

    fBlock[0] = this->xx[centerIndex];
    fBlock[1] = (*xMeanVel)[halo];
    fBlock[2] = this->yy[centerIndex];
    fBlock[3] = (*yMeanVel)[halo];
    fBlock[4] = this->zz[centerIndex];
    fBlock[5] = (*zMeanVel)[halo];
    fBlock[6] = haloMass;
    cStream.write(reinterpret_cast<char*>(fBlock), COSMO_FLOAT * sizeof(float));

    iBlock[0] = haloTag;
    cStream.write(reinterpret_cast<char*>(iBlock), COSMO_INT * sizeof(int));
  }
}

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
