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
#include <algorithm>

#include <math.h>
#include <stdio.h>

#include "Partition.h"
#include "CosmoHaloFinderP.h"
#include "GenericIO.h"

using namespace std;
using namespace gio;

namespace cosmotk {


/////////////////////////////////////////////////////////////////////////
//
// Parallel manager for serial CosmoHaloFinder
// Particle data space is partitioned for the number of processors
// which currently is a factor of two but is easily extended.  Particles
// are read in from files where each processor reads one file into a buffer,
// extracts the particles which really belong on the processor (ALIVE) and
// those in a buffer region around the edge (DEAD).  The buffer is then
// passed round robin to every other processor so that all particles are
// examined by all processors.  All dead particles are tagged with the
// neighbor zone (26 neighbors in 3D) so that later halos can be associated
// with zones.
//
// The serial halo finder is called on each processor and returns enough
// information so that it can be determined if a halo is completely ALIVE,
// completely DEAD, or mixed.  A mixed halo that is shared between on two
// processors is kept by the processor that contains it in one of its
// high plane neighbors, and is given up if contained in a low plane neighbor.
//
// Mixed halos that cross more than two processors are bundled up and sent
// to the MASTER processor which decides the processor that should own it.
//
/////////////////////////////////////////////////////////////////////////

CosmoHaloFinderP::CosmoHaloFinderP()
{
  // Get the number of processors and rank of this processor
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();

  // Get the number of processors in each dimension
  Partition::getDecompSize(this->layoutSize);

  // Get my position within the Cartesian topology
  Partition::getMyPosition(this->layoutPos);

  // Get the neighbors of this processor
  Partition::getNeighbors(this->neighbor);

  // For each neighbor zone, how many dead particles does it contain to start
  // and how many dead halos does it contain after the serial halo finder
  // For analysis but not necessary to run the code
  //
  for (int n = 0; n < NUM_OF_NEIGHBORS; n++) {
    this->deadParticle[n] = 0;
    this->deadHalo[n] = 0;
  }
  this->haloTag = 0;
  this->haloStart = 0;
  this->haloList = 0;
  this->haloSize = 0;
}

CosmoHaloFinderP::~CosmoHaloFinderP()
{
  clearHaloTag();
  clearHaloStart();
  clearHaloList();
  clearHaloSize();
}

//
// Halo structure information is allocated here and passed to serial halo
// finder for filling and then some is passed to the calling simulator
// for other analysis.  So memory is not allocated and freed nicely.
//
void CosmoHaloFinderP::clearHaloTag()
{
  // haloTag holds the index of the particle which is the first in the halo
  // so if haloTag[p] != p then this particle is in a halo
  // may be released after tagged particles are written or after
  // all halos are collected for merging
  if (this->haloTag != 0) {
    delete [] this->haloTag;
    this->haloTag = 0;
  }
}

void CosmoHaloFinderP::clearHaloStart()
{
  // haloStart holds the index of the first particle in a halo
  // used with haloList to locate all particles in a halo
  // may be released after merged halos because info is put in halos vector
  if (this->haloStart != 0) {
    delete [] this->haloStart;
    this->haloStart = 0;
  }
}

void CosmoHaloFinderP::clearHaloList()
{
  // haloList is used with haloStart or with halos vector for locating all
  // particles in a halo.  It must stay around through all analysis.
  // may be released only on next call to executeHaloFinder
  if (this->haloList != 0) {
    delete [] this->haloList;
    this->haloList = 0;
  }
}

void CosmoHaloFinderP::clearHaloSize()
{
  // haloSize holds the size of the halo associated with any particle
  // may be released after tagged particles are written or after
  // all halos are collected for merging
  if (this->haloSize != 0) {
    delete [] this->haloSize;
    this->haloSize = 0;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for the serial halo finder
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::setParameters(const string& outName,
                                     POSVEL_T _rL,
                                     POSVEL_T _deadSz,
                                     long _np,
                                     int _pmin,
                                     POSVEL_T _bb,
                                     int _nmin)
{
  // Particles for this processor output to file
  this->outFile = outName;

  // Halo finder parameters
  this->np = _np;
  this->pmin = _pmin;
  this->bb = _bb;
  this->nmin = _nmin;
  this->boxSize = _rL;
  this->deadSize = _deadSz;

  // Unnormalize bb so that it will work with box size distances
  this->haloFinder.bb = _bb * (POSVEL_T) ((1.0 * _rL) / _np);;

  this->haloFinder.np = _np;
  this->haloFinder.pmin = _pmin;
  this->haloFinder.nmin = _nmin;
  this->haloFinder.rL = _rL;
  this->haloFinder.periodic = false;
  this->haloFinder.textmode = "ascii";

  if (this->myProc == MASTER) {
    cout << endl << "------------------------------------" << endl;
    //cout << "np:       " << this->np << endl;
    //cout << "bb:       " << this->bb << endl;
    //cout << "nmin:     " << this->nmin << endl;
    //cout << "pmin:     " << this->pmin << endl << endl;
    printf("np:       %ld\n", this->np);
    printf("bb:       %f\n",  this->bb);
    printf("nmin:     %d\n",  this->nmin);
    printf("pmin:     %d\n",  this->pmin);
    cout << endl;
  }

}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::setParticles(long count,
                                    POSVEL_T* xLoc,
                                    POSVEL_T* yLoc,
                                    POSVEL_T* zLoc,
                                    POSVEL_T* xVel,
                                    POSVEL_T* yVel,
                                    POSVEL_T* zVel,
                                    POTENTIAL_T* potential,
                                    ID_T* id,
                                    MASK_T* maskData,
                                    STATUS_T* state)
{
  this->particleCount = count;

  // Extract the contiguous data block from a vector pointer
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;
  this->vx = xVel;
  this->vy = yVel;
  this->vz = zVel;
  this->pot = potential;
  this->tag = id;
  this->mask = maskData;
  this->status = state;
}

/////////////////////////////////////////////////////////////////////////
//
// Execute the serial halo finder on all particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::executeHaloFinder()
{
  // Clear old halo structure and allocate new
  clearHaloTag();
  clearHaloStart();
  clearHaloList();
  clearHaloSize();

  this->haloTag = new int[this->particleCount];
  this->haloStart = new int[this->particleCount];
  this->haloList = new int[this->particleCount];
  this->haloSize = new int[this->particleCount];

  // Set the input locations for the serial halo finder
  this->haloFinder.setParticleLocations(this->xx, this->yy, this->zz);

  // Set the output locations for the serial halo finder
  this->haloFinder.setHaloLocations(this->haloTag,
                                    this->haloStart,
                                    this->haloList);

  this->haloFinder.setNumberOfParticles(this->particleCount);
  this->haloFinder.setMyProc(this->myProc);
  this->haloFinder.setOutFile(this->outFile);


#ifdef HALO_FINDER_VERBOSE
  cout << "Rank " << setw(3) << this->myProc
       << " RUNNING SERIAL HALO FINDER on "
       << particleCount << " particles" << endl;
#endif // HALO_FINDER_VERBOSE


#ifndef USE_SERIAL_COSMO
  MPI_Barrier(Partition::getComm());
#endif

  if (this->particleCount > 0)
    this->haloFinder.Finding();

#ifndef USE_SERIAL_COSMO
  MPI_Barrier(Partition::getComm());
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// At this point each serial halo finder ran and the particles handed to it
// included alive and dead.  Structure to locate all particles in a halo
// were returned in haloTag, haloStart and haloList
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::collectHalos(bool clearTag)
{
  // Record the halo size of each particle on this processor
  this->haloAliveSize = new int[this->particleCount];
  for (int p = 0; p < this->particleCount; p++) {
    this->haloSize[p] = 0;
    this->haloAliveSize[p] = 0;
  }

  // Build the chaining mesh of particles in all the halos and count particles
  buildHaloStructure();

  // Mixed halos are saved separately so that they can be merged
  processMixedHalos();

  // Clear the data associated with tag and size which won't be needed
  if (clearTag) {
    //clearHaloTag();
    clearHaloSize();
  }
  delete [] this->haloAliveSize;
}

/////////////////////////////////////////////////////////////////////////
//
// Examine every particle on this processor, both ALIVE and DEAD
// For that particle increment the count for the corresponding halo
// which is indicated by the lowest particle index in that halo
// Also build the haloList so that we can find all particles in any halo
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::buildHaloStructure()
{
  // Count particles in the halos
  for (int p = 0; p < this->particleCount; p++) {
    if (this->status[p] == ALIVE)
      this->haloAliveSize[this->haloTag[p]]++;
    this->haloSize[this->haloTag[p]]++;
  }

  // Iterate over particles and create a CosmoHalo for halos with size > pmin
  // only for the mixed halos, not for those completely alive or dead
  this->numberOfAliveHalos = 0;
  this->numberOfDeadHalos = 0;
  this->numberOfMixedHalos = 0;

  // Only the first particle id for a halo records the size
  // Succeeding particles which are members of a halo have a size of 0
  // Record the start index of any legal halo which will allow the
  // following of the chaining mesh to identify all particles in a halo
  this->numberOfHaloParticles = 0;
  for (ID_T p = 0; p < this->particleCount; p++) {

    if (this->haloSize[p] >= this->pmin) {

      if (this->haloAliveSize[p] == this->haloSize[p]) {
        this->numberOfAliveHalos++;
        this->numberOfHaloParticles += this->haloAliveSize[p];

        // Save start of legal alive halo for halo properties
        this->halos.push_back(this->haloStart[p]);
        this->haloCount.push_back(this->haloAliveSize[p]);
      }
      else if (this->haloAliveSize[p] == 0) {
        this->numberOfDeadHalos++;
      }
      else {
        this->numberOfMixedHalos++;
        CosmoHalo* halo = new CosmoHalo(
                               p,
                               this->haloAliveSize[p],
                               this->haloSize[p] - this->haloAliveSize[p]);
        this->myMixedHalos.push_back(halo);
      }
    }
  }

#ifdef DEBUG
  cout << "Rank " << this->myProc
       << " #alive halos = " << this->numberOfAliveHalos
       << " #dead halos = " << this->numberOfDeadHalos
       << " #mixed halos = " << this->numberOfMixedHalos << endl;
#endif

}

/////////////////////////////////////////////////////////////////////////
//
// Mixed halos (which cross several processors) have been collected
// By applying a high/low rule most mixed halos are assigned immediately
// to one processor or another.  This requires extra processing so that
// it is known which neighbor processors share the halo.
//
/////////////////////////////////////////////////////////////////////////

void CosmoHaloFinderP::processMixedHalos()
{
  // Iterate over all particles and add tags to large mixed halos
  for (ID_T p = 0; p < this->particleCount; p++) {

    // All particles in the same halo have the same haloTag
    if (this->haloSize[this->haloTag[p]] >= pmin &&
        this->haloAliveSize[this->haloTag[p]] > 0 &&
        this->haloAliveSize[this->haloTag[p]] < this->haloSize[this->haloTag[p]]) {

      // Check all each mixed halo to see which this particle belongs to
      for (unsigned int h = 0; h < this->myMixedHalos.size(); h++) {

        // If the tag of the particle matches the halo ID it belongs
        if (this->haloTag[p] == this->myMixedHalos[h]->getHaloID()) {

          // Add the index to that mixed halo.  Also record which neighbor
          // the dead particle is associated with for merging
          this->myMixedHalos[h]->addParticle(p,
                                             this->tag[p],
                                             this->status[p]);

          // For debugging only
          if (this->status[p] > 0)
            this->deadHalo[this->status[p]]++;

          // Do some bookkeeping for the final output
          // This processor should output all ALIVE particles, unless they
          // are in a mixed halo that ends up being INVALID
          // This processor should output none of the DEAD particles,
          // unless they are in a mixed halo that ends up being VALID

          // So since this particle is in a mixed halo set it to MIXED
          // which is going to be one less than ALIVE.  Later when we
          // determine we have a VALID mixed, we'll add one to the status
          // for every particle turning all into ALIVE

          // Now when we output we only do the ALIVE particles
          this->status[p] = MIXED;
        }
      }
    }
  }

  // just in case didn't want heap allocation in loops
  vector<int64_t> arbiter;
  arbiter.reserve(NUM_OF_NEIGHBORS+1);

  // Now iterate over mixed halos, which have access to particle indexes & tags
  for (unsigned int h = 0; h < this->myMixedHalos.size(); h++) {

    // start by assuming a halo is not mine
    bool mine = false;
    int hneighbors[NUM_OF_NEIGHBORS+1];

    size_t nAlive = this->myMixedHalos[h]->getAliveCount();
    size_t nDead = this->myMixedHalos[h]->getDeadCount();
    size_t nTotal = nAlive + nDead;
    if( nAlive > nTotal/2) {
      // if more than half of the particles are alive then it will be mine
      mine = true;
    } else {
      // more complicated arbitration required
      // unsure if this current algorithm works when overloading from self

      // count halo particles in neighbors
      for(int i=0; i<NUM_OF_NEIGHBORS+1; i++)
        hneighbors[i] = 0;
      vector<int>* hstatus = this->myMixedHalos[h]->getStatus();

      // debug statement
      if(hstatus->size() != nTotal)
        cout << "rank " << this->myProc << " mixed halo index " << h << " hstatus->size() " << hstatus->size() << " != nTotal " << nTotal << endl;

      for(size_t i=0; i<hstatus->size(); i++)
        if((*hstatus)[i] >= 0)
          hneighbors[(*hstatus)[i]]++;
        else
          hneighbors[NUM_OF_NEIGHBORS]++;

      arbiter.clear();
      // hash rank into low bits and neighbor particle counts into high bits
      arbiter.push_back(myProc + (this->numProc)*hneighbors[NUM_OF_NEIGHBORS]);
      for(int i=0; i<NUM_OF_NEIGHBORS; i++)
        arbiter.push_back(this->neighbor[i] + (this->numProc)*hneighbors[i]);
      // sort in ascending order
      sort(arbiter.begin(), arbiter.end());
      // grab the last/largest element and mod by numProc to get winning rank
      int winner = arbiter[NUM_OF_NEIGHBORS]%(this->numProc);
      if(winner == this->myProc)
        mine = true;
    }

    // Halo is kept by this processor and is marked as VALID
    // May be in multiple neighbor zones, but all the same processor neighbor
    //int hid = this->myMixedHalos[h]->getHaloID();
    if (mine == true) {
      this->numberOfAliveHalos++;
      this->numberOfMixedHalos--;
      this->myMixedHalos[h]->setValid(VALID);
      int id = this->myMixedHalos[h]->getHaloID();
      int newAliveParticles = this->myMixedHalos[h]->getAliveCount() +
                              this->myMixedHalos[h]->getDeadCount();
      this->numberOfHaloParticles += newAliveParticles;

      // Add this halo to valid halos on this processor for
      // subsequent halo properties analysis
      this->halos.push_back(this->haloStart[id]);
      this->haloCount.push_back(newAliveParticles);

      // Output trick - since the status of this particle was marked MIXED
      // when it was added to the mixed CosmoHalo vector, and now it has
      // been declared VALID, change it to ALIVE even if it was dead before
      vector<ID_T>* particles = this->myMixedHalos[h]->getParticles();
      vector<ID_T>::iterator iter2;
      for (iter2 = particles->begin(); iter2 != particles->end(); ++iter2)
        this->status[(*iter2)] = ALIVE;
    } else {
      // Halo will be kept by some other processor and is marked INVALID
      // May be in multiple neighbor zones, but all the same processor neighbor

      this->numberOfDeadHalos++;
      this->numberOfMixedHalos--;
      this->myMixedHalos[h]->setValid(INVALID);
    }

  }

  // If only one processor is running there are no halos to merge
  // IS THIS CORRECT? What about halos on (periodic) boundaries? --Adrian
  if (this->numProc == 1)
    for (unsigned int h = 0; h < this->myMixedHalos.size(); h++)
      this->myMixedHalos[h]->setValid(INVALID);



  // Collect totals for result checking

  int totalNumberOfMixed;
#ifdef USE_SERIAL_COSMO
  totalNumberOfMixed = numberOfMixed;
#else
  MPI_Allreduce((void*) &this->numberOfMixedHalos, (void*) &totalNumberOfMixed,
                1, MPI_INT, MPI_SUM, Partition::getComm());
#endif

  int totalAliveHalos;
#ifdef USE_SERIAL_COSMO
  totalAliveHalos = this->numberOfAliveHalos;
#else
  MPI_Allreduce((void*) &this->numberOfAliveHalos, (void*) &totalAliveHalos,
                1, MPI_INT, MPI_SUM, Partition::getComm());
#endif

  int64_t totalAliveHaloParticles;
  int64_t numberOfHaloParticles64 = this->numberOfHaloParticles;
#ifdef USE_SERIAL_COSMO
  totalAliveHaloParticles = numberOfHaloParticles64;
#else
  MPI_Allreduce((void*) &numberOfHaloParticles64,
                (void*) &totalAliveHaloParticles,
                1, MPI_LONG_LONG, MPI_SUM, Partition::getComm());
#endif

  if (this->myProc == MASTER) {
    cout << endl;
    cout << "Total halos found:    " << totalAliveHalos << endl;
    cout << "Total mixed halos:    " << totalNumberOfMixed << endl;
    cout << "Total halo particles: " << totalAliveHaloParticles << endl;
  }

}



/////////////////////////////////////////////////////////////////////////
//
// Write the output of the halo finder in the form of the input .cosmo file
//
// Encoded mixed halo VALID or INVALID into the status array such that
// ALIVE particles that are part of an INVALID mixed array will not write
// but DEAD particles that are part of a VALID mixed array will be written
//
// In order to make the output consistent with the serial output where the
// lowest tagged particle in a halo owns the halo, work must be done to
// identify the lowest tag.  This is because as particles are read onto
// this processor using the round robin read of every particle, those
// particles are no longer in tag order.  When the serial halo finder is
// called it has to use the index of the particle on this processor which
// is no longer the tag.
//
//      p    haloTag     tag    haloSize
//      0          0     523           3
//      1          0     522           0
//      2          0     266           0
//
// In the above example the halo will be credited to 523 instead of 266
// because the index of 523 is 0 and the index of 266 is 2.  So we must
// make a pass to map the indexes.
//
/////////////////////////////////////////////////////////////////////////

static ptrdiff_t drand48elmt(ptrdiff_t i) {
  return ptrdiff_t(drand48()*i);
}

void CosmoHaloFinderP::writeTaggedParticles(int hmin, float ss, int minPerHalo,
                                            bool writeAllTags,
                                            bool writeAll, bool clearTag)
{
  // Map the index of the particle on this process to the index of the
  // particle with the lowest tag value so that the written output refers
  // to the lowest tag as being the owner of the halo
  int* mapIndex = new int[this->particleCount];
  for (int p = 0; p < this->particleCount; p++)
    mapIndex[p] = p;

  // If the tag for the first particle of this halo is bigger than the tag
  // for this particle, change the map to identify this particle as the lowest
  for (int p = 0; p < this->particleCount; p++) {
    if (this->tag[mapIndex[this->haloTag[p]]] > this->tag[p])
      mapIndex[this->haloTag[p]] = p;
  }

  if (writeAllTags) {
    vector<ID_T> ssTag, ssParticleHaloTag;

    size_t NumAlive = 0;
    for (int p = 0; p < this->particleCount; p++) {
      if (this->status[p] != ALIVE)
        continue;

      if (this->haloSize[this->haloTag[p]] < this->pmin)
        continue;

      ++NumAlive;
    }

    size_t reserveSize = NumAlive;
    ssTag.reserve(reserveSize);
    ssParticleHaloTag.reserve(reserveSize);

    for (int p = 0; p < this->particleCount; p++) {
      if (this->status[p] != ALIVE)
        continue;

      if (this->haloSize[this->haloTag[p]] < this->pmin)
        continue;

      ssTag.push_back(this->tag[p]);
      ssParticleHaloTag.push_back(this->tag[this->haloTag[p]]);
    }

    GenericIO GIO(Partition::getComm(), outFile + ".haloparticletags");
    GIO.setNumElems(ssTag.size());
    GIO.addVariable("id", ssTag);
    GIO.addVariable("fof_halo_tag", ssParticleHaloTag);
    GIO.write();
  }

  unsigned CoordFlagsX = GenericIO::VarIsPhysCoordX |
                         GenericIO::VarMaybePhysGhost;
  unsigned CoordFlagsY = GenericIO::VarIsPhysCoordY |
                         GenericIO::VarMaybePhysGhost;
  unsigned CoordFlagsZ = GenericIO::VarIsPhysCoordZ |
                         GenericIO::VarMaybePhysGhost;

  if (hmin > 0) {
    vector<ID_T> ssTag, ssParticleHaloTag;
    vector<POSVEL_T> ssX, ssY, ssZ, ssVX, ssVY, ssVZ;

    size_t NumAlive = 0;
    for (int p = 0; p < this->particleCount; p++) {
      if (this->status[p] != ALIVE)
        continue;

      if (this->haloSize[this->haloTag[p]] < this->pmin)
        continue;

      if (this->haloSize[this->haloTag[p]] < hmin)
        continue;

      ++NumAlive;
    }

    size_t reserveSize = NumAlive;
    ssTag.reserve(reserveSize);
    ssParticleHaloTag.reserve(reserveSize);
    ssX.reserve(reserveSize);
    ssY.reserve(reserveSize);
    ssZ.reserve(reserveSize);
    ssVX.reserve(reserveSize);
    ssVY.reserve(reserveSize);
    ssVZ.reserve(reserveSize);

    for (int p = 0; p < this->particleCount; p++) {
      if (this->status[p] != ALIVE)
        continue;

      if (this->haloSize[this->haloTag[p]] < this->pmin)
        continue;

      if (this->haloSize[this->haloTag[p]] < hmin)
        continue;

      ssTag.push_back(this->tag[p]);
      ssParticleHaloTag.push_back(this->tag[this->haloTag[p]]);
      ssX.push_back(this->xx[p]);
      ssY.push_back(this->yy[p]);
      ssZ.push_back(this->zz[p]);
      ssVX.push_back(this->vx[p]);
      ssVY.push_back(this->vy[p]);
      ssVZ.push_back(this->vz[p]);
    }

    // Write the tagged particle file
    GenericIO GIO(Partition::getComm(), outFile + ".bighaloparticles");
    GIO.setNumElems(ssTag.size());
    GIO.setPhysOrigin(0.0);
    GIO.setPhysScale(this->boxSize);
    GIO.addVariable("x", ssX, CoordFlagsX);
    GIO.addVariable("y", ssY, CoordFlagsY);
    GIO.addVariable("z", ssZ, CoordFlagsZ);
    GIO.addVariable("vx", ssVX);
    GIO.addVariable("vy", ssVY);
    GIO.addVariable("vz", ssVZ);
    GIO.addVariable("id", ssTag);
    GIO.addVariable("fof_halo_tag", ssParticleHaloTag);
    GIO.write();
  }

  if (writeAll) {
    vector<ID_T> ssTag, ssParticleHaloTag;
    vector<POSVEL_T> ssX, ssY, ssZ, ssVX, ssVY, ssVZ;

    size_t NumAlive = 0;
    for (int p = 0; p < this->particleCount; p++) {
      if (this->status[p] != ALIVE)
        continue;

      ++NumAlive;
    }

    size_t reserveSize = NumAlive;
    ssTag.reserve(reserveSize);
    ssParticleHaloTag.reserve(reserveSize);
    ssX.reserve(reserveSize);
    ssY.reserve(reserveSize);
    ssZ.reserve(reserveSize);
    ssVX.reserve(reserveSize);
    ssVY.reserve(reserveSize);
    ssVZ.reserve(reserveSize);

    for (int p = 0; p < this->particleCount; p++) {
      if (this->status[p] != ALIVE)
        continue;

      ssTag.push_back(this->tag[p]);
      ssParticleHaloTag.push_back(
        (this->haloSize[this->haloTag[p]] < this->pmin)
        ? -1: this->tag[this->haloTag[p]]);
      ssX.push_back(this->xx[p]);
      ssY.push_back(this->yy[p]);
      ssZ.push_back(this->zz[p]);
      ssVX.push_back(this->vx[p]);
      ssVY.push_back(this->vy[p]);
      ssVZ.push_back(this->vz[p]);
    }

    // Write the tagged particle file
    GenericIO GIO(Partition::getComm(), outFile + ".allparticles");
    GIO.setNumElems(ssTag.size());
    GIO.setPhysOrigin(0.0);
    GIO.setPhysScale(this->boxSize);
    GIO.addVariable("x", ssX, CoordFlagsX);
    GIO.addVariable("y", ssY, CoordFlagsY);
    GIO.addVariable("z", ssZ, CoordFlagsZ);
    GIO.addVariable("vx", ssVX);
    GIO.addVariable("vy", ssVY);
    GIO.addVariable("vz", ssVZ);
    GIO.addVariable("id", ssTag);
    GIO.addVariable("fof_halo_tag", ssParticleHaloTag);
    GIO.write();
  }

  if (ss != 0.0) {
    vector<ID_T> ssTag, ssParticleHaloTag;
    vector<POSVEL_T> ssX, ssY, ssZ, ssVX, ssVY, ssVZ;

    size_t NumAlive = 0;
    for (int p = 0; p < this->particleCount; p++) {
      if (this->status[p] != ALIVE)
        continue;

      if (this->haloSize[this->haloTag[p]] < this->pmin)
        continue;

      ++NumAlive;
    }

    size_t reserveSize = (size_t) (ss*NumAlive);
    ssTag.reserve(reserveSize);
    ssParticleHaloTag.reserve(reserveSize);
    ssX.reserve(reserveSize);
    ssY.reserve(reserveSize);
    ssZ.reserve(reserveSize);
    ssVX.reserve(reserveSize);
    ssVY.reserve(reserveSize);
    ssVZ.reserve(reserveSize);

    vector<int> haloIndices;
    for (size_t h = 0; h < this->halos.size(); ++h) {
      haloIndices.clear();
      haloIndices.reserve(this->haloCount[h]);

      int p = this->halos[h];
      for (int s = 0; s < this->haloCount[h]; ++s) {
        if (this->status[p] != ALIVE)
          continue;

        haloIndices.push_back(p);
        p = this->haloList[p];
      }

      // randomly sort the array.
      std::random_shuffle(haloIndices.begin(), haloIndices.end(), drand48elmt);
      int NumOutput = std::max((int) (ss*this->haloCount[h]), minPerHalo);
      for (int i = 0; i < NumOutput && i < (int) haloIndices.size(); ++i) {
        p = haloIndices[i];
        ssTag.push_back(this->tag[p]);
        ssParticleHaloTag.push_back(this->tag[this->haloTag[p]]);
        ssX.push_back(this->xx[p]);
        ssY.push_back(this->yy[p]);
        ssZ.push_back(this->zz[p]);
        ssVX.push_back(this->vx[p]);
        ssVY.push_back(this->vy[p]);
        ssVZ.push_back(this->vz[p]);
      }
    }

    // NOTE: These particles are in-order by halo, and some users depend on this!

    // Write the tagged particle file
    GenericIO GIO(Partition::getComm(), outFile + ".haloparticles");
    GIO.setNumElems(ssTag.size());
    GIO.setPhysOrigin(0.0);
    GIO.setPhysScale(this->boxSize);
    GIO.addVariable("x", ssX, CoordFlagsX);
    GIO.addVariable("y", ssY, CoordFlagsY);
    GIO.addVariable("z", ssZ, CoordFlagsZ);
    GIO.addVariable("vx", ssVX);
    GIO.addVariable("vy", ssVY);
    GIO.addVariable("vz", ssVZ);
    GIO.addVariable("id", ssTag);
    GIO.addVariable("fof_halo_tag", ssParticleHaloTag);
    GIO.write();
  }
  delete [] mapIndex;

  // Clear the data stored in serial halo finder
  if (clearTag) {
    //clearHaloTag();
    clearHaloSize();
  }
}

} // END namespace cosmotk
