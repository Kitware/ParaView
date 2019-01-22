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

#include <stddef.h>

#include <sys/types.h>


#include <dirent.h>

#include "Partition.h"
#include "FOFDistribute.h"
#include "GenericIOMPIReader.h"
#include "SimpleTimings.h"

#include <cstring>
using namespace std;

namespace cosmotk {
/////////////////////////////////////////////////////////////////////////
//
// Halo data space is partitioned for the number of processors
// which currently is a factor of two but is easily extended.  Halos
// are read in from files where each processor reads one file into a buffer,
// extracts the halos which really belong on the processor (ALIVE) and
// those in a buffer region around the edge (DEAD).  The buffer is then
// passed round robin to every other processor so that all halos are
// examined by all processors.  All dead halos are tagged with the
// neighbor zone (26 neighbors in 3D) so that later halos can be associated
// with zones.
//
/////////////////////////////////////////////////////////////////////////

FOFDistribute::FOFDistribute()
{
  // Get the number of processors running this problem and rank
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();

  // Get the number of processors in each dimension
  Partition::getDecompSize(this->layoutSize);

  // Get my position within the Cartesian topology
  Partition::getMyPosition(this->layoutPos);

  // Get neighbors of this processor including the wraparound
  Partition::getNeighbors(this->neighbor);

  this->numberOfAliveHalos = 0;
}

FOFDistribute::~FOFDistribute()
{
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for halo distribution
//
/////////////////////////////////////////////////////////////////////////

void FOFDistribute::setParameters(
                        const string& baseName,
                        POSVEL_T rL)
{
  // Base file name which will have processor id appended for actual files
  this->baseFile = baseName;

  // Physical total space and amount of physical space to use for dead halos
  this->boxSize = rL;

  if (this->myProc == MASTER) {
    cout << endl << "------------------------------------" << endl;
    cout << "boxSize:  " << this->boxSize << endl;
  }

}

/////////////////////////////////////////////////////////////////////////
//
// Set box sizes for determining if a halo is in the alive or dead
// region of this processor.  Data space is a DIMENSION torus.
//
/////////////////////////////////////////////////////////////////////////

void FOFDistribute::initialize()
{
#ifdef DEBUG
  if (this->myProc == MASTER)
    cout << "Decomposition: [" << this->layoutSize[0] << ":"
         << this->layoutSize[1] << ":" << this->layoutSize[2] << "]" << endl;
#endif

  // Set subextents on halo locations for this processor
  POSVEL_T boxStep[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    boxStep[dim] = this->boxSize / this->layoutSize[dim];

    // Alive halos
    this->minAlive[dim] = this->layoutPos[dim] * boxStep[dim];
    this->maxAlive[dim] = this->minAlive[dim] + boxStep[dim];
    if (this->maxAlive[dim] > this->boxSize)
      this->maxAlive[dim] = this->boxSize;
  }
}


void FOFDistribute::setHalos(vector<POSVEL_T>* xLoc,
                             vector<POSVEL_T>* yLoc,
                             vector<POSVEL_T>* zLoc,
                             vector<POSVEL_T>* xVel,
                             vector<POSVEL_T>* yVel,
                             vector<POSVEL_T>* zVel,
                             vector<POSVEL_T>* mass,
                             vector<POSVEL_T>* xCenter,
                             vector<POSVEL_T>* yCenter,
                             vector<POSVEL_T>* zCenter,
                             vector<POSVEL_T>* vDisp,
                             vector<ID_T>* id,
                             vector<int>* cnt)
{
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;
  this->vx = xVel;
  this->vy = yVel;
  this->vz = zVel;
  this->ms = mass;
  this->cx = xCenter;
  this->cy = yCenter;
  this->cz = zCenter;
  this->vdisp = vDisp;
  this->tag = id;
  this->count = cnt;
}

void FOFDistribute::readHalosGIO(int reserveQ, bool useAlltoallv,
                                 bool redistCenter)
{
  gio::GenericIOMPIReader GIO;
  GIO.SetCommunicator(Partition::getComm());
  GIO.SetFileName(this->baseFile);
  GIO.OpenAndReadHeader();

  MPI_Datatype CosmoHaloType;

  {
    MPI_Datatype type[3] = { MPI_FLOAT, MPI_LONG, MPI_UB };
    int blocklen[3] = { 11, 2, 1 };
    MPI_Aint disp[3] = { offsetof(CosmoHalo, floatData),
                         offsetof(CosmoHalo, intData),
                         sizeof(CosmoHalo) };
    MPI_Type_struct(3, blocklen, disp, type, &CosmoHaloType);
    MPI_Type_commit(&CosmoHaloType);
  }

  std::vector<int> pCounts(numProc), pRecvCounts(numProc),
                   pDisp(numProc),   pRecvDisp(numProc);
  std::vector< std::vector<CosmoHalo> > pByProc(numProc);
  std::vector<CosmoHalo> pBuffer, pRecvBuffer;

  this->haloCount = GIO.GetNumberOfElements();
  this->xx->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->yy->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->zz->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->vx->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->vy->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->vz->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->ms->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->cx->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->cy->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->cz->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->vdisp->resize(haloCount + gio::CRCSize/sizeof(POSVEL_T));
  this->tag->resize(haloCount + gio::CRCSize/sizeof(ID_T));
  this->count->resize(haloCount + gio::CRCSize/sizeof(int));

  GIO.AddVariable("fof_halo_mean_x", *this->xx, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_mean_y", *this->yy, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_mean_z", *this->zz, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_mean_vx", *this->vx, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_mean_vy", *this->vy, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_mean_vz", *this->vz, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_mass", *this->ms, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_center_x", *this->cx, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_center_y", *this->cy, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_center_z", *this->cz, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_tag", *this->tag, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.AddVariable("fof_halo_count", *this->count, gio::GenericIOBase::ValueHasExtraSpace);
  GIO.ReadData();

  this->xx->resize(haloCount);
  this->yy->resize(haloCount);
  this->zz->resize(haloCount);
  this->vx->resize(haloCount);
  this->vy->resize(haloCount);
  this->vz->resize(haloCount);
  this->ms->resize(haloCount);
  this->cx->resize(haloCount);
  this->cy->resize(haloCount);
  this->cz->resize(haloCount);
  this->vdisp->resize(haloCount);
  this->tag->resize(haloCount);
  this->count->resize(haloCount);

  // Find a home for every halo...
  for (int i = 0; i < haloCount; ++i) {
    if ((*this->xx)[i] >= this->boxSize)
      (*this->xx)[i] -= this->boxSize;
    if ((*this->yy)[i] >= this->boxSize)
      (*this->yy)[i] -= this->boxSize;
    if ((*this->zz)[i] >= this->boxSize)
      (*this->zz)[i] -= this->boxSize;

    if ((*this->xx)[i] < 0)
      (*this->xx)[i] += this->boxSize;
    if ((*this->yy)[i] < 0)
      (*this->yy)[i] += this->boxSize;
    if ((*this->zz)[i] < 0)
      (*this->zz)[i] += this->boxSize;

    if ((*this->cx)[i] >= this->boxSize)
      (*this->cx)[i] -= this->boxSize;
    if ((*this->cy)[i] >= this->boxSize)
      (*this->cy)[i] -= this->boxSize;
    if ((*this->cz)[i] >= this->boxSize)
      (*this->cz)[i] -= this->boxSize;

    if ((*this->cx)[i] < 0)
      (*this->cx)[i] += this->boxSize;
    if ((*this->cy)[i] < 0)
      (*this->cy)[i] += this->boxSize;
    if ((*this->cz)[i] < 0)
      (*this->cz)[i] += this->boxSize;

    // Figure out to which rank this halo belongs
    POSVEL_T xCoord = redistCenter ? (*this->cx)[i] : (*this->xx)[i];
    POSVEL_T yCoord = redistCenter ? (*this->cy)[i] : (*this->yy)[i];
    POSVEL_T zCoord = redistCenter ? (*this->cz)[i] : (*this->zz)[i];
    float sizeX = this->boxSize / this->layoutSize[0];
    float sizeY = this->boxSize / this->layoutSize[1];
    float sizeZ = this->boxSize / this->layoutSize[2];
    int coords[3] = { (int) (xCoord/sizeX),
                      (int) (yCoord/sizeY),
                      (int) (zCoord/sizeZ)
                    };

    int home;
    MPI_Cart_rank(Partition::getComm(), coords, &home);

    CosmoHalo halo = { { (*this->xx)[i],
                         (*this->yy)[i],
                         (*this->zz)[i],
                         (*this->vx)[i],
                         (*this->vy)[i],
                         (*this->vz)[i],
                         (*this->ms)[i],
                         (*this->cx)[i],
                         (*this->cy)[i],
                         (*this->cz)[i],
                         (*this->vdisp)[i]
                       }, {
                         (*this->tag)[i],
                         (*this->count)[i]
                       }
                     };

    pByProc[home].push_back(halo);
  }

  this->xx->resize(0);
  this->yy->resize(0);
  this->zz->resize(0);
  this->vx->resize(0);
  this->vy->resize(0);
  this->vz->resize(0);
  this->ms->resize(0);
  this->cx->resize(0);
  this->cy->resize(0);
  this->cz->resize(0);
  this->vdisp->resize(0);
  this->tag->resize(0);
  this->count->resize(0);

  if (reserveQ) {
    this->xx->reserve(haloCount);
    this->yy->reserve(haloCount);
    this->zz->reserve(haloCount);
    this->vx->reserve(haloCount);
    this->vy->reserve(haloCount);
    this->vz->reserve(haloCount);
    this->ms->reserve(haloCount);
    this->cx->reserve(haloCount);
    this->cy->reserve(haloCount);
    this->cz->reserve(haloCount);
    this->vdisp->reserve(haloCount);
    this->tag->reserve(haloCount);
    this->count->reserve(haloCount);
  }

  SimpleTimings::TimerRef t_ala2a = SimpleTimings::getTimer("ala2a");
  SimpleTimings::startTimer(t_ala2a);

  this->haloCount = 0;

  // Record all sizes into a single buffer and send this to all ranks
  long totalToSend = 0;
  for (int i = 0; i < numProc; ++i) {
    int sz = (int) pByProc[i].size();
    pCounts[i] = sz;
    totalToSend += sz;
  }

  MPI_Alltoall(&pCounts[0], 1, MPI_INT,
               &pRecvCounts[0], 1, MPI_INT,
               Partition::getComm());
 // pRecvCounts now holds the number of halos that this rank should
  // get from every other rank
  long totalToRecv = 0;
  for (int i = 0; i < numProc; ++i) {
    totalToRecv += pRecvCounts[i];
  }

  // Allocate and pack the buffer with all halos to send
  pBuffer.reserve(totalToSend);
  for (int i = 0; i < numProc; ++i)
  for (int j = 0; j < (int) pByProc[i].size(); ++j) {
    pBuffer.push_back(pByProc[i][j]);
  }

  // Calculate displacements
  pDisp[0] = pRecvDisp[0] = 0;
  for (int i = 1; i < numProc; ++i) {
    pDisp[i] = pDisp[i-1] + pCounts[i-1];
    pRecvDisp[i] = pRecvDisp[i-1] + pRecvCounts[i-1];
  }

  // Send all halos to their new homes
  pRecvBuffer.resize(totalToRecv);
  if (useAlltoallv) {
    MPI_Alltoallv(&pBuffer[0], &pCounts[0], &pDisp[0], CosmoHaloType,
                  &pRecvBuffer[0], &pRecvCounts[0], &pRecvDisp[0], CosmoHaloType,
                  Partition::getComm());
  } else {
    vector<MPI_Request> requests;

    for (int i = 0; i < numProc; ++i) {
      if (pRecvCounts[i] == 0)
        continue;

      requests.resize(requests.size()+1);
      MPI_Irecv(&pRecvBuffer[pRecvDisp[i]], pRecvCounts[i], CosmoHaloType, i, 0,
                Partition::getComm(), &requests[requests.size()-1]);
    }

    MPI_Barrier(Partition::getComm());

    for (int i = 0; i < numProc; ++i) {
      if (pCounts[i] == 0)
        continue;

      requests.resize(requests.size()+1);
      MPI_Isend(&pBuffer[pDisp[i]], pCounts[i], CosmoHaloType, i, 0,
                Partition::getComm(), &requests[requests.size()-1]);
    }

    vector<MPI_Status> status(requests.size());
    MPI_Waitall((int) requests.size(), &requests[0], &status[0]);
  }

  // We now have all of our halos, put them in our local arrays
  for (long i = 0; i < totalToRecv; ++i) {
    POSVEL_T loc[DIMENSION], vel[DIMENSION], mass, cent[DIMENSION], vd;
    ID_T id, cnt;

    int j = 0;
    for (int dim = 0; dim < DIMENSION; dim++) {
      loc[dim] = pRecvBuffer[i].floatData[j++];
    }
    for (int dim = 0; dim < DIMENSION; dim++) {
      vel[dim] = pRecvBuffer[i].floatData[j++];
    }
    mass = pRecvBuffer[i].floatData[j++];
    for (int dim = 0; dim < DIMENSION; dim++) {
      cent[dim] = pRecvBuffer[i].floatData[j++];
    }
    vd = pRecvBuffer[i].floatData[j++];

    id  = pRecvBuffer[i].intData[0];
    cnt = pRecvBuffer[i].intData[1];

    POSVEL_T rpx = redistCenter ? cent[0] : loc[0];
    POSVEL_T rpy = redistCenter ? cent[1] : loc[1];
    POSVEL_T rpz = redistCenter ? cent[2] : loc[2];

    // Is the halo ALIVE on this processor
    if ((rpx >= minAlive[0] && rpx < maxAlive[0]) &&
        (rpy >= minAlive[1] && rpy < maxAlive[1]) &&
        (rpz >= minAlive[2] && rpz < maxAlive[2])) {

      this->xx->push_back(loc[0]);
      this->yy->push_back(loc[1]);
      this->zz->push_back(loc[2]);
      this->vx->push_back(vel[0]);
      this->vy->push_back(vel[1]);
      this->vz->push_back(vel[2]);
      this->ms->push_back(mass);
      this->cx->push_back(cent[0]);
      this->cy->push_back(cent[1]);
      this->cz->push_back(cent[2]);
      this->vdisp->push_back(vd);
      this->tag->push_back(id);
      this->count->push_back(cnt);

      this->numberOfAliveHalos++;
      this->haloCount++;
    }
  }

  // Count the halos across processors
  long totalAliveHalos = 0;
  MPI_Allreduce((void*) &this->numberOfAliveHalos,
                (void*) &totalAliveHalos,
                1, MPI_LONG, MPI_SUM, Partition::getComm());

#ifdef DEBUG
  cout << "Rank " << setw(3) << this->myProc
       << " #alive = " << this->numberOfAliveHalos << endl;
#endif

  MPI_Type_free(&CosmoHaloType);

  if (this->myProc == MASTER) {
    cout << "ReadHalosGIO TotalAliveHalos "
         << totalAliveHalos << endl;
  }

  SimpleTimings::stopTimerStats(t_ala2a);
}

} // END namespace cosmotk
