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

#include "SUBFIND.h"
#include "Partition.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <math.h>

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#define  GRAVITY     6.672e-8
#define  SOLAR_MASS  1.989e33
#define  SOLAR_LUM   3.826e33
#define  RAD_CONST   7.565e-15
#define  AVOGADRO    6.0222e23
#define  BOLTZMANN   1.3806e-16
#define  GAS_CONST   8.31425e7
#define  C           2.9979e10
#define  PLANCK      6.6262e-27
#define  CM_PER_MPC  3.085678e24
#define  PROTONMASS  1.6726e-24
#define  HUBBLE      3.2407789e-18  /* in h/sec */
#define  SEC_PER_MEGAYEAR   3.155e13
#define  SEC_PER_YEAR       3.155e7

#define MAX_NGB_CHECK 2

//#define DEBUG

/////////////////////////////////////////////////////////////////////////
//
// Constructor for doing SUBFIND on all FOF halos
//
/////////////////////////////////////////////////////////////////////////

SUBFIND::SUBFIND()
{
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();
}

/////////////////////////////////////////////////////////////////////////
//
// Destructor
//
/////////////////////////////////////////////////////////////////////////

SUBFIND::~SUBFIND()
{
  delete [] P;
  delete [] GroupLen;
  delete [] GroupOffset;
  delete [] GrP;
}

/////////////////////////////////////////////////////////////////////////
//
// Set the parameter information for the physics
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::setParameters(
                        POSVEL_T particleMass,
                        POSVEL_T gravityConst,
                        int numSPH,
                        int numClose)
{
  this->PartMass = particleMass;
  this->G = gravityConst;
  this->DesLinkNgb = numClose;  // Number of neighbors for subhalo candidate
  this->DesDensNgb = numSPH;    // Number of neighbors for smoothing length

  ErrTolTheta = 0.6;
  Softening = 0.005;
  Omega0 = 0.3;
  OmegaLambda = 0.7;
  Time = 1.0;

  UnitLength_in_cm = 3.08568e+24;
  UnitMass_in_g = 1.989e+43;
  UnitVelocity_in_cm_per_s = 100000;
  UnitTime_in_s = UnitLength_in_cm / UnitVelocity_in_cm_per_s;
  Hubble = HUBBLE * UnitTime_in_s;
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle information for this processor
// All particle information is stored in P array for now
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::setParticles(
                  POSVEL_T* xLoc,
                  POSVEL_T* yLoc,
                  POSVEL_T* zLoc,
                  POSVEL_T* xVel,
                  POSVEL_T* yVel,
                  POSVEL_T* zVel,
                  POSVEL_T* mass,
                  ID_T* ID,
                  ID_T N
                  )
{
  // All particles on this processor
  this->NumPart = static_cast<int>( N );

  this->xx   = xLoc;
  this->yy   = yLoc;
  this->zz   = zLoc;
  this->vx   = xVel;
  this->vy   = yVel;
  this->vz   = zVel;
  this->mass = mass;
  this->tag  = ID;

  // Collect the domain range on this processor
    minLoc[0] = maxLoc[0] = this->xx[0];
    minLoc[1] = maxLoc[1] = this->yy[0];
    minLoc[2] = maxLoc[2] = this->zz[0];

    for (int i = 1; i < this->NumPart; i++) {
      if (minLoc[0] > this->xx[i]) minLoc[0] = this->xx[i];
      if (maxLoc[0] < this->xx[i]) maxLoc[0] = this->xx[i];
      if (minLoc[1] > this->yy[i]) minLoc[1] = this->yy[i];
      if (maxLoc[1] < this->yy[i]) maxLoc[1] = this->yy[i];
      if (minLoc[2] > this->zz[i]) minLoc[2] = this->zz[i];
      if (maxLoc[2] < this->zz[i]) maxLoc[2] = this->zz[i];
    }
    this->BoxSize = maxLoc[0] - minLoc[0];

    // Copy HACC information into the SUBFIND structure for now
    P = new particle_data[this->NumPart];

    for (int p = 0; p < this->NumPart; p++) {
      P[p].GroupPartIndex = -1;
      P[p].u = 0.0;
    }
#ifdef DEBUG
  cout << "Rank "      << myProc    << " build all particles " << NumPart
       << " minloc "   << minLoc[0] << "," << minLoc[1] << "," << minLoc[2]
       << "  maxloc "  << maxLoc[0] << "," << maxLoc[1] << "," << maxLoc[2]
       << "  boxsize " << BoxSize   << endl;
#endif
}

void SUBFIND::setParticles(
                        vector<POSVEL_T>* xLoc,
                        vector<POSVEL_T>* yLoc,
                        vector<POSVEL_T>* zLoc,
                        vector<POSVEL_T>* xVel,
                        vector<POSVEL_T>* yVel,
                        vector<POSVEL_T>* zVel,
                        vector<POSVEL_T>* mass,
                        vector<ID_T>* id)
{
  // All particles on this processor
  this->NumPart = xLoc->size();

  // Extract the contiguous data block from a vector pointer
  this->xx = &(*xLoc)[0];
  this->yy = &(*yLoc)[0];
  this->zz = &(*zLoc)[0];
  this->vx = &(*xVel)[0];
  this->vy = &(*yVel)[0];
  this->vz = &(*zVel)[0];
  this->mass = &(*mass)[0];
  this->tag = &(*id)[0];

  // Collect the domain range on this processor
  minLoc[0] = maxLoc[0] = this->xx[0];
  minLoc[1] = maxLoc[1] = this->yy[0];
  minLoc[2] = maxLoc[2] = this->zz[0];

  for (int i = 1; i < this->NumPart; i++) {
    if (minLoc[0] > this->xx[i]) minLoc[0] = this->xx[i];
    if (maxLoc[0] < this->xx[i]) maxLoc[0] = this->xx[i];
    if (minLoc[1] > this->yy[i]) minLoc[1] = this->yy[i];
    if (maxLoc[1] < this->yy[i]) maxLoc[1] = this->yy[i];
    if (minLoc[2] > this->zz[i]) minLoc[2] = this->zz[i];
    if (maxLoc[2] < this->zz[i]) maxLoc[2] = this->zz[i];
  }
  this->BoxSize = maxLoc[0] - minLoc[0];

  // Copy HACC information into the SUBFIND structure for now
  P = new particle_data[this->NumPart];

  for (int p = 0; p < this->NumPart; p++) {
    P[p].GroupPartIndex = -1;
    P[p].u = 0.0;
  }
#ifdef DEBUG
  cout << "Rank "      << myProc    << " build all particles " << NumPart
       << " minloc "   << minLoc[0] << "," << minLoc[1] << "," << minLoc[2]
       << "  maxloc "  << maxLoc[0] << "," << maxLoc[1] << "," << maxLoc[2]
       << "  boxsize " << BoxSize << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Set the FOF particle information for the entire processor
// Build the GrP array which is only FOF particles
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::setHalos(int numberOfFOFHalos,
                       int* fofHaloCount,
                       int* fofHaloStart,
                       int* fofHaloList)
{
  this->Ngroups = numberOfFOFHalos;
  this->GroupLen = new int[Ngroups];
  this->GroupOffset = new int[Ngroups];
#ifdef DEBUG
  cout << "Rank " << myProc << " Number of FOF halos " << Ngroups << endl;
#endif

  // Total FOF particles on this processor
  this->NumFOFPart = 0;
  int offset = 0;
  for (int h = 0; h < this->Ngroups; h++) {
    this->GroupOffset[h] = offset;
    this->GroupLen[h] = fofHaloCount[h];
    offset += fofHaloCount[h];
  }
  this->NumFOFPart = offset;

  // Information about particles which are also FOF particles
  GrP = new group_particle_data[this->NumFOFPart];

#ifdef DEBUG
  cout << "Rank " << myProc << " local FOF particles " << NumFOFPart << endl;
#endif

  // Walk every FOF halo building above arrays
  // FOF halos are threaded using indirect addressing to locate all particles
  int gpIndx = 0;
  for (int h = 0; h < this->Ngroups; h++) {
    int gp = fofHaloStart[h];
    while (gp != -1) {
      P[gp].GroupPartIndex = gpIndx;
      GrP[gpIndx].PartIndex = gp;
      GrP[gpIndx].ID = tag[gp];
      gpIndx++;
      gp = fofHaloList[gp];
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Run the SUBFIND algorithm on every FOF halo
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::run()
{
  tree_treeallocate(NumPart, NumPart);

  // Find smoothing length and density for only the FOF particles
  determine_densities();

  // Iterate over all FOF halos to find subhalos
  for(int gr = 0; gr < Ngroups; gr++)
    process_group(gr);

  write_GrP_information();
  write_subhalo_catalog();
  write_subhalo_particles();

  tree_treefree();
}

/////////////////////////////////////////////////////////////////////////
//
// Run the SUBFIND algorithm on one FOF halo
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::process_group(int gr)
{
  int i, k, ss, ngbs, ndiff, N, Offs, head = 0, head_attach, count_cand;
  int listofdifferent[2];
  int curr_len, curr_lev, count, prev;
  int target_group_index, target_part_index;
  int ngb_part_index, ngb_group_index;

  // Number of FOF particles in the halo
  N = this->GroupLen[gr];
  Offs = this->GroupOffset[gr];

#ifdef DEBUG
  cout << "Rank " << myProc << " process_group "
       << gr << " GroupLen " << N << "  offset " << Offs << endl;
#endif
  dens_dat* dens_list = new dens_dat[N];
  cand_dat* candidates = new cand_dat[N];

  // Store the density for each FOF particles
  for(i = 0; i < N; i++)
    {
      dens_list[i].Density = GrP[Offs + i].Density;
      dens_list[i].Index = Offs + i;
    }
  // sort according to density, largest density first
  qsort(dens_list, N, sizeof(dens_dat), compare_dens);

  Head = new int[N];
  Tail = new int[N];
  Len  = new int[N];
  Next = new int[N];

  // This appears to be a trick to allow GrP[] which has all halo particles
  // in it (so every halo is Offs offset into the array) and the Head, etc
  // arrays which start at 0 for every halo, to use same index for a particle
  Head -= Offs;
  Next -= Offs;
  Tail -= Offs;
  Len -= Offs;

  for(i = Offs; i < Offs + N; i++)
    Next[i] = i + 1;

  for(i = Offs; i < Offs + N; i++)
    {
      GrP[i].Level = 0;
      GrP[i].SubLen = 0;
    }

  // build tree for all particles of this FOF halo group
  tree_treebuild(0, Offs, N);

  for(i = Offs; i < Offs + N; i++)
    Head[i] = Next[i] = -1;

  // Iterate over all FOF particles in decreasing density order placing
  // in subgroups of near neighbors and creating subhalo candidates
  //
  for(i = 0, count_cand = 0; i < N; i++)
    {
      // Particle index into list of FOF particles
      target_group_index = dens_list[i].Index;

      // Particle index into list of all particles
      target_part_index = GrP[target_group_index].PartIndex;

      if(P[GrP[target_group_index].PartIndex].GroupPartIndex !=
         target_group_index)
        {
          printf("bummer %d %d\n",
                  P[GrP[target_group_index].PartIndex].GroupPartIndex,
                  target_group_index);
        }

      // Find the required number of nearest neighbors using tree
      ngb_treefind(this->xx[target_part_index],
                   this->yy[target_part_index],
                   this->zz[target_part_index],
                   DesLinkNgb,
                   GrP[target_group_index].Hsml);

      // note: returned neighbours are already sorted by distance
      for(k = 0, ndiff = 0, ngbs = 0;
          k < DesLinkNgb && ngbs < MAX_NGB_CHECK && ndiff < 2; k++)
        {
          ngb_part_index = R2list[k].index;

          // to exclude the particle itself
          if(ngb_part_index != target_part_index)
            {
              ngb_group_index = P[ngb_part_index].GroupPartIndex;

              // we only look at neighbours that are denser
              if(GrP[ngb_group_index].Density > GrP[target_group_index].Density)
                {
                  ngbs++;

                  // neighbor is attached to a group
                  if(Head[ngb_group_index] >= 0)
                    {
                      if(ndiff == 1)
                        if(listofdifferent[0] == Head[ngb_group_index])
                          continue;

                      // a new group has been found
                      listofdifferent[ndiff++] = Head[ngb_group_index];
                    }
                  else
                    {
                      printf("this may not occur.\n");
                    }
                }
            }
        }

      // treat the different possible cases
      switch (ndiff)
        {
        case 0:         /* this appears to be a lonely maximum -> new group */
          head = target_group_index;
          Head[target_group_index] = target_group_index;
          Tail[target_group_index] = target_group_index;
          Len[target_group_index] = 1;
          Next[target_group_index] = -1;
          break;

        case 1:         /* the particle is attached to exactly one group */
          head = listofdifferent[0];
          Head[target_group_index] = head;
          Next[Tail[head]] = target_group_index;
          Tail[head] = target_group_index;
          Len[head]++;
          Next[target_group_index] = -1;
          break;

        case 2:         /* the particle merges two groups together */
          head = listofdifferent[0];
          head_attach = listofdifferent[1];

          // other group is longer, swap them
          if(Len[head_attach] > Len[head])
            {
              head = listofdifferent[1];
              head_attach = listofdifferent[0];
            }

          // only in case the attached group is long enough
          // we bother to register is as a subhalo candidate
          if(Len[head_attach] >= DesLinkNgb)
            {
              candidates[count_cand].len = Len[head_attach];
              candidates[count_cand].head = Head[head_attach];
              count_cand++;
            }

          // now join the two groups
          Next[Tail[head]] = head_attach;
          Tail[head] = Tail[head_attach];
          Len[head] += Len[head_attach];

          ss = head_attach;
          do
            {
              Head[ss] = head;
            }
          while((ss = Next[ss]) >= 0);

          // finally, attach the particle
          Head[target_group_index] = head;
          Next[Tail[head]] = target_group_index;
          Tail[head] = target_group_index;
          Len[head]++;
          Next[target_group_index] = -1;
          break;

        default:
          printf("can't be! (a)\n");
          break;
        }
    }

  // add the full thing as a subhalo candidate
  for(i = 0, prev = -1; i < N; i++)
    {
      if(Head[Offs + i] == Offs + i)
        if(Next[Tail[Offs + i]] == -1)
          {
            if(prev < 0)
              head = Offs + i;
            if(prev >= 0)
              Next[prev] = Offs + i;

            prev = Tail[Offs + i];
          }
    }

  candidates[count_cand].len = N;
  candidates[count_cand].head = head;
  count_cand++;

#ifdef DEBUG
  cout << "Final candidate " << count_cand << "  len " << N << "  head "
       << head << endl;
#endif
  for(k = 0; k < count_cand; k++)
    unbind(k, candidates[k].head, candidates[k].len);

  delete [] (Len + Offs);
  delete [] (Tail + Offs);
  delete [] (Next + Offs);
  delete [] (Head + Offs);

  delete [] candidates;
  delete [] dens_list;

  // this will bring the particles of the halo into subhalo order
  qsort(&GrP[Offs], N, sizeof(group_particle_data),
        compare_grp_particles);

#ifdef DEBUG
  printf("\nGroupLen=%d  (gr=%d)\n", N, gr);
#endif

  for(i = Offs, curr_len = curr_lev = -1, count = 0; i < Offs + N; i++)
    {
      if(curr_len != GrP[i].SubLen || curr_lev != GrP[i].Level)
        {
          curr_len = GrP[i].SubLen;
          curr_lev = GrP[i].Level;

          count += GrP[i].SubLen;

#ifdef DEBUG
          if(GrP[i].SubLen > 0)
            printf("SubLen=%d  (lev=%d) \n", GrP[i].SubLen, GrP[i].Level);
          else
            printf("Fuzz=%d  (lev=%d)\n", N - count, GrP[i].Level);
#endif
        }
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Unbind particles from this subhalo
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::unbind(int lev, int head, int len)
{
  int i, j, p, num, part_index, minindex;
  int max_remove_per_step, unbound, dm_part;//, non_gas_part;
  POSVEL_T s[3], dx[3], v[3], dv[3];
  POSVEL_T sqa, H_of_a, pot, minpot = 0;
  //POSVEL_T boxsize, boxhalf;
  POSVEL_T TotMass;

  //boxsize = BoxSize;
  //boxhalf = 0.5 * BoxSize;

  sqa = sqrt(Time);

  H_of_a = Omega0 / (Time * Time * Time) +
           (1 - Omega0 - OmegaLambda) / (Time * Time) + OmegaLambda;
  H_of_a = Hubble * sqrt(H_of_a);

  energy_dat* subpart_egys = new energy_dat[len];

  for(i = 0, p = head; i < len; i++, p = Next[p])
    if(GrP[p].Level <= lev && GrP[p].SubLen == 0)
      GrP[p].Level = lev;

  do
    {
      // Build the tree for just this subhalo
      tree_treebuild(lev, head, len);

      // let's compute the potential energy
      for(i = 0, p = head, num = 0; i < len; i++, p = Next[p])
        {
          // this means it can still be bound to this subhalo-candidate
          if(GrP[p].Level <= lev && GrP[p].SubLen == 0)
            {
              part_index = GrP[p].PartIndex;

              pot = tree_treeevaluate_potential(part_index);

              // note: add self-energy
              GrP[p].Potential = pot + this->mass[part_index] / Softening;
              GrP[p].Potential *= G / Time;

              num++;
            }
        }

      s[0] = s[1] = s[2] = v[0] = v[1] = v[2] = 0;

      for(i = 0, p = head, num = 0, TotMass = 0.0; i < len; i++, p = Next[p])
        {
          // this means it can still be bound to this subhalo-candidate
          if(GrP[p].Level <= lev && GrP[p].SubLen == 0)
            {
              num++;
              part_index = GrP[p].PartIndex;
              v[0] += this->mass[part_index] * this->vx[part_index];
              v[1] += this->mass[part_index] * this->vy[part_index];
              v[2] += this->mass[part_index] * this->vz[part_index];
              TotMass += this->mass[part_index];
            }
        }

      if(num)
        for(j = 0; j < 3; j++)
          v[j] /= TotMass;

      for(i = 0, p = head, minindex = -1; i < len; i++, p = Next[p])
        {
          // this means it can still be bound to this subhalo-candidate
          if(GrP[p].Level <= lev && GrP[p].SubLen == 0)
            if(GrP[p].Potential < minpot || minindex == -1)
              {
                minpot = GrP[p].Potential;
                minindex = p;
              }
        }

      // position of minimum potential
      s[0] = this->xx[GrP[minindex].PartIndex];
      s[1] = this->yy[GrP[minindex].PartIndex];
      s[2] = this->zz[GrP[minindex].PartIndex];

      for(i = 0, p = head, num = 0; i < len; i++, p = Next[p])
        {
          // this means it can still be bound to this subhalo-candidate
          if(GrP[p].Level <= lev && GrP[p].SubLen == 0)
            {
              part_index = GrP[p].PartIndex;
              dv[0] = sqa * (this->vx[part_index] - v[0]);
              dx[0] = Time * (this->xx[part_index] - s[0]);
              dv[0] += H_of_a * dx[0];
              dv[1] = sqa * (this->vy[part_index] - v[1]);
              dx[1] = Time * (this->yy[part_index] - s[1]);
              dv[1] += H_of_a * dx[1];
              dv[2] = sqa * (this->vz[part_index] - v[2]);
              dx[2] = Time * (this->zz[part_index] - s[2]);
              dv[2] += H_of_a * dx[2];

              GrP[p].BindingEnergy = GrP[p].Potential +
                     0.5 * (dv[0] * dv[0] + dv[1] * dv[1] + dv[2] * dv[2]);
              GrP[p].BindingEnergy += P[part_index].u;
              subpart_egys[num].energy = GrP[p].BindingEnergy;
              subpart_egys[num].ind = p;

              num++;
            }
        }
      qsort(subpart_egys, num, sizeof(energy_dat), compare_energy);

      // now omit unbound particles,  but at most max_remove_per_step
      unbound = 0;
      max_remove_per_step = 0.25 * num;
      dm_part = 0;
      //non_gas_part = 0;

      for(i = 0; i < num; i++)
        {
          p = subpart_egys[i].ind;
#ifndef ONLY_CANDIDATES
          if(subpart_egys[i].energy > 0)
#else
          if(0 > 0)
#endif
            {
              // this will it unbing on this level
              GrP[p].Level++;
              unbound++;
              if(unbound >= max_remove_per_step)
                break;
            }
          else
            {
              dm_part++;
            }
        }

      num -= unbound;

      if(num < DesLinkNgb)
        break;
    }
  while(unbound > 0);

  delete [] subpart_egys;

  // we have something that is still bound!
  if(dm_part >= DesLinkNgb)
    {
      for(i = 0, p = head; i < len; i++, p = Next[p])
        if(GrP[p].Level <= lev && GrP[p].SubLen == 0)
          {
            GrP[p].SubLen = num;
            GrP[p].Level = lev;
          }
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle information for the entire processor
//
/////////////////////////////////////////////////////////////////////////

int SUBFIND::tree_treebuild(int lev, int start, int len)
{
  int i, j, subnode = 0, parent = -1, numnodes, count;
  int nfree, th, nn, flag, num = 0;
  POSVEL_T lenhalf;
  NODE *nfreep;

  Nodes = Nodes_base - NumPart;

  // select first node
  nfree = NumPart;
  nfreep = &Nodes[nfree];

  // create an empty  root node
  boxsize = maxLoc[0] - minLoc[0];
  boxhalf = 0.5 * BoxSize;

  for(j = 0; j < DIMENSION; j++)
    nfreep->center[j] = 0.5 * (maxLoc[j] + minLoc[j]);
  nfreep->len = BoxSize;

  for(i = 0; i < 8; i++)
    nfreep->u.suns[i] = -1;

  numnodes = 1;
  nfreep++;
  nfree++;

  // insert all particles
  count = 0;
  if(lev < 0)
    i = 0;
  else
    i = GrP[start].PartIndex;

  while(i < NumPart)
    {
      if(lev < 0)
        {
          flag = 1;
        }
      else
        {
          flag = 1;

          if(GrP[P[i].GroupPartIndex].Level > lev)
            flag = 0;

          if(GrP[P[i].GroupPartIndex].SubLen > 0)
            flag = 0;
        }

      if(flag)
        {
          num++;

          th = NumPart;

          while(1)
            {
              // we are dealing with an internal node
              if(th >= NumPart)
                {
                  subnode = 0;
                  if(this->xx[i] > Nodes[th].center[0])
                    subnode += 1;
                  if(this->yy[i] > Nodes[th].center[1])
                    subnode += 2;
                  if(this->zz[i] > Nodes[th].center[2])
                    subnode += 4;

                  nn = Nodes[th].u.suns[subnode];

                  // something is in the daughter slot already
                  if(nn >= 0)
                    {
                      // subnode can still be used in the next step of the walk
                      parent = th;
                      th = nn;
                    }
                  else
                    {
                      // here we have found an empty slot where we can
                      // attach the new particle as a leaf
                      Nodes[th].u.suns[subnode] = i;
                      break;    // done for this particle
                    }
                }
              else
                {
                  // we try to insert into a leaf with a single particle
                  // need to generate a new internal node at this point
                  Nodes[parent].u.suns[subnode] = nfree;

                  nfreep->len = 0.5 * Nodes[parent].len;
                  lenhalf = 0.25 * Nodes[parent].len;

                  if(subnode & 1)
                    nfreep->center[0] = Nodes[parent].center[0] + lenhalf;
                  else
                    nfreep->center[0] = Nodes[parent].center[0] - lenhalf;

                  if(subnode & 2)
                    nfreep->center[1] = Nodes[parent].center[1] + lenhalf;
                  else
                    nfreep->center[1] = Nodes[parent].center[1] - lenhalf;

                  if(subnode & 4)
                    nfreep->center[2] = Nodes[parent].center[2] + lenhalf;
                  else
                    nfreep->center[2] = Nodes[parent].center[2] - lenhalf;

                  nfreep->u.suns[0] = -1;
                  nfreep->u.suns[1] = -1;
                  nfreep->u.suns[2] = -1;
                  nfreep->u.suns[3] = -1;
                  nfreep->u.suns[4] = -1;
                  nfreep->u.suns[5] = -1;
                  nfreep->u.suns[6] = -1;
                  nfreep->u.suns[7] = -1;

                  subnode = 0;
                  if(this->xx[th] > nfreep->center[0])
                    subnode += 1;
                  if(this->yy[th] > nfreep->center[1])
                    subnode += 2;
                  if(this->zz[th] > nfreep->center[2])
                    subnode += 4;

                  if(nfreep->len < 1.0e-3 * Softening)
                    {
                      // seems like we're dealing with particles
                      // at identical locations. randomize
                      // subnode index-well below gravitational softening scale
                      subnode = (int) (8.0 * drand48());
                      if(subnode >= 8)
                        subnode = 7;
                    }

                  nfreep->u.suns[subnode] = th;

                  // resume trying to insert the new particle at the newly
                  // created internal node
                  th = nfree;
                  numnodes++;
                  nfree++;
                  nfreep++;

                  if((numnodes) >= MaxNodes)
                    {
                      printf("maximum number %d of tree-nodes reached.\n", MaxNodes);
                      printf("for particle %d  %g %g %g\n", i, xx[i], yy[i], zz[i]);
                    }
                }
            }
        }

      count++;

      if(lev < 0)
        i++;
      else
        {
          if(count >= len)
            break;

          i = GrP[Next[P[i].GroupPartIndex]].PartIndex;
        }
    }

  // now compute the multipole moments recursively
  last = -1;
  tree_update_node_recursive(NumPart, -1, -1);

  if(last >= NumPart)
    Nodes[last].u.d.nextnode = -1;
  else
    Nextnode[last] = -1;

  return numnodes;
}

/////////////////////////////////////////////////////////////////////////////
//
// this routine computes the multipole moments for a given internal node and
// all its subnodes using a recursive computation.  Note that the moments of
// the daughter nodes are already stored in single precision. For very large
// particle numbers, loss of precision may results for certain particle
// distributions
//
/////////////////////////////////////////////////////////////////////////////

void SUBFIND::tree_update_node_recursive(int no, int sib, int father)
{
  int j, jj, p, pp = 0, nextsib, suns[8];
  POSVEL_T mass;
  POSVEL_T s[3];

  if(no >= NumPart)
    {
      for(j = 0; j < 8; j++)
        // this "backup" is necessary because the nextnode entry will
        // overwrite one element (union!)
        suns[j] = Nodes[no].u.suns[j];
      if(last >= 0)
        {
          if(last >= NumPart)
            Nodes[last].u.d.nextnode = no;
          else
            Nextnode[last] = no;
        }

      last = no;

      mass = 0;
      s[0] = 0;
      s[1] = 0;
      s[2] = 0;

      for(j = 0; j < 8; j++)
        {
          if((p = suns[j]) >= 0)
            {
              // check if we have a sibling on the same level
              for(jj = j + 1; jj < 8; jj++)
                if((pp = suns[jj]) >= 0)
                  break;

              if(jj < 8)        // yes, we do
                nextsib = pp;
              else
                nextsib = sib;

              tree_update_node_recursive(p, nextsib, no);

              // an internal node or pseudo particle
              if(p >= NumPart)
                {
                  // we assume a fixed particle mass
                  mass += Nodes[p].u.d.mass;
                  s[0] += Nodes[p].u.d.mass * Nodes[p].u.d.s[0];
                  s[1] += Nodes[p].u.d.mass * Nodes[p].u.d.s[1];
                  s[2] += Nodes[p].u.d.mass * Nodes[p].u.d.s[2];
                }
              // a particle
              else
                {
                  mass += this->mass[p];
                  s[0] += this->mass[p] * this->xx[p];
                  s[1] += this->mass[p] * this->yy[p];
                  s[2] += this->mass[p] * this->zz[p];
                }
            }
        }

      if(mass>0)
        {
          s[0] /= mass;
          s[1] /= mass;
          s[2] /= mass;
        }
      else
        {
          s[0] = Nodes[no].center[0];
          s[1] = Nodes[no].center[1];
          s[2] = Nodes[no].center[2];
        }

      Nodes[no].u.d.s[0] = s[0];
      Nodes[no].u.d.s[1] = s[1];
      Nodes[no].u.d.s[2] = s[2];
      Nodes[no].u.d.mass = mass;

      Nodes[no].u.d.sibling = sib;
      Nodes[no].u.d.father = father;
    }
  // single particle or pseudo particle
  else
    {
      if(last >= 0)
        {
          if(last >= NumPart)
            Nodes[last].u.d.nextnode = no;
          else
            Nextnode[last] = no;
        }

      last = no;

      // only set it for single particles
      if(no < NumPart)
        Father[no] = father;
    }
}

POSVEL_T SUBFIND::tree_treeevaluate_potential(int target)
{
  NODE *nop = 0;
  int no;
  POSVEL_T r2, dx, dy, dz, mass, r, u, h, h_inv, wp;
  POSVEL_T pot, pos_x, pos_y, pos_z;

  pos_x = this->xx[target];
  pos_y = this->yy[target];
  pos_z = this->zz[target];

  h = 2.8 * Softening;
  h_inv = 1.0 / h;

  pot = 0;

  no = NumPart;

  while(no >= 0)
    {
      // single particle
      if(no < NumPart)
        {
          dx = this->xx[no] - pos_x;
          dy = this->yy[no] - pos_y;
          dz = this->zz[no] - pos_z;
          mass = this->mass[no];
        }
      else
        {
          nop = &Nodes[no];
          dx = nop->u.d.s[0] - pos_x;
          dy = nop->u.d.s[1] - pos_y;
          dz = nop->u.d.s[2] - pos_z;
          mass = nop->u.d.mass;
        }
      r2 = dx * dx + dy * dy + dz * dz;

      if(no < NumPart)
        {
          no = Nextnode[no];
        }
      // we have an internal node. Need to check opening criterion
      else
        {
          if(nop->len * nop->len > r2 * ErrTolTheta * ErrTolTheta)
            {
              // open cell
              no = nop->u.d.nextnode;
              continue;
            }
          // node can be used
          no = nop->u.d.sibling;
        }
      r = sqrt(r2);

      if(r >= h)
        pot -= mass / r;
      else
        {
          u = r * h_inv;

          if(u < 0.5)
            wp = -2.8 + u * u *
                 (5.333333333333 + u * u * (6.4 * u - 9.6));
          else
            wp =
              -3.2 + 0.066666666667 / u + u * u *
              (10.666666666667 + u * (-16.0 + u * (9.6 - 2.133333333333 * u)));

          pot += mass * h_inv * wp;
        }
    }

  return pot;
}


/////////////////////////////////////////////////////////////////////////
//
// Find the nearest neighbors to the particle position
//
/////////////////////////////////////////////////////////////////////////

POSVEL_T SUBFIND::ngb_treefind(POSVEL_T x, POSVEL_T y, POSVEL_T z,
                               int desngb, POSVEL_T hguess)
{
  int numngb;
  POSVEL_T part_dens;
  POSVEL_T h2max;

  if(hguess == 0)
    {
      part_dens = Omega0 * 3 * Hubble * Hubble / (8 * M_PI * G) / PartMass;
      hguess = pow(3 * desngb / (4 * M_PI) / part_dens, 1.0 / 3);
#ifdef DEBUG
      printf("part_dens=%g  hguess=%g\n", part_dens, hguess);
#endif
    }

  do
    {
      numngb = ngb_treefind_variable(x, y, z, hguess);

      if(numngb < desngb)
        {
          hguess *= 1.26;
          continue;
        }

      if(numngb >= desngb)
        {
          qsort(R2list, numngb, sizeof(r2data), ngb_compare_key);
          h2max = R2list[desngb - 1].r2;
          break;
        }

      hguess *= 1.26;
    }
  while(1);

  return sqrt(h2max);
}


/////////////////////////////////////////////////////////////////////////
//
// Find the nearest neighbors to the particle position
//
/////////////////////////////////////////////////////////////////////////

int SUBFIND::ngb_treefind_variable(POSVEL_T x, POSVEL_T y, POSVEL_T z,
                                   POSVEL_T hguess)
{
  int numngb, no, p;
  POSVEL_T dx, dy, dz, r2, h2;
  NODE *thisnode;

  h2 = hguess * hguess;

  numngb = 0;
  no = NumPart;

  while(no >= 0)
    {
      if(no < NumPart)          // single particle
        {
          p = no;
          no = Nextnode[no];

          dx = this->xx[p] - x;
          if(dx < -hguess)
            continue;
          if(dx > hguess)
            continue;

          dy = this->yy[p] - y;
          if(dy < -hguess)
            continue;
          if(dy > hguess)
            continue;

          dz = this->zz[p] - z;
          if(dz < -hguess)
            continue;
          if(dz > hguess)
            continue;

          r2 = dx * dx + dy * dy + dz * dz;

          if(r2 < h2)
            {
              R2list[numngb].r2 = r2;
              R2list[numngb].index = p;
              numngb++;
            }
        }
      else
        {
          thisnode = &Nodes[no];

          // in case the node can be discarded
          no = Nodes[no].u.d.sibling;
          if(((thisnode->center[0] - x) + 0.5 * thisnode->len) < -hguess)
            continue;
          if(((thisnode->center[0] - x) - 0.5 * thisnode->len) > hguess)
            continue;
          if(((thisnode->center[1] - y) + 0.5 * thisnode->len) < -hguess)
            continue;
          if(((thisnode->center[1] - y) - 0.5 * thisnode->len) > hguess)
            continue;
          if(((thisnode->center[2] - z) + 0.5 * thisnode->len) < -hguess)
            continue;
          if(((thisnode->center[2] - z) - 0.5 * thisnode->len) > hguess)
            continue;

          // ok, we need to open the node
          no = thisnode->u.d.nextnode;
        }
    }
  return numngb;
}

/////////////////////////////////////////////////////////////////////////////
//
// this function allocates memory used for storage of the tree
// and auxiliary arrays for tree-walk and link-lists.
// usually maxnodes = 0.7 * maxpart is sufficient
//
/////////////////////////////////////////////////////////////////////////////

size_t SUBFIND::tree_treeallocate(int maxnodes, int maxpart)
{
#ifdef DEBUG
  cout << "Tree Allocate maxnodes " << maxnodes
       << "  maxpart " << maxpart << endl;
#endif
  size_t bytes, allbytes = 0;

  MaxNodes = maxnodes;

  bytes = (MaxNodes + 1) * sizeof(NODE);
  Nodes_base = new NODE[MaxNodes + 1];
  allbytes += bytes;

  bytes = maxpart * sizeof(int);
  Nextnode = new int[maxpart];
  allbytes += bytes;

  bytes = maxpart * sizeof(int);
  Father = new int[maxpart];
  allbytes += bytes;

  bytes = maxpart * sizeof(r2data);
  R2list = new r2data[maxpart];
  allbytes += bytes;

  return allbytes;
}


/////////////////////////////////////////////////////////////////////////
//
// Set the particle information for the entire processor
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::tree_treefree(void)
{
  delete [] R2list;
  delete [] Father;
  delete [] Nextnode;
  delete [] Nodes_base;
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle information for the entire processor
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::determine_densities(void)
{
  int i, n, signal, p;
  POSVEL_T h, hinv3, wk, u, r, rho;

  // build tree for all particles
#ifdef DEBUG
  printf("Rank %d Construct full tree\n", myProc);
#endif
  tree_treebuild(-1, 0, 0);

#ifdef DEBUG
  printf("Rank %d Computing densities...\n", myProc);
#endif
  for(i = 0, signal = 0, h = 0; i < NumPart; i++)
    {
      if(i > (signal / 100.0) * NumPart)
        {
#ifdef DEBUG
          printf("x");
#endif
          signal++;
        }

      // then the particle is in a group
      if(P[i].GroupPartIndex >= 0)
        {
          h = ngb_treefind(this->xx[i], this->yy[i], this->zz[i],
                           DesDensNgb, h * 1.2);

          GrP[P[i].GroupPartIndex].Hsml = h;

          hinv3 = 1.0 / (h * h * h);

          for(n = 0, rho = 0; n < DesDensNgb; n++)
            {
              p = R2list[n].index;
              r = sqrt(R2list[n].r2);
              u = r / h;

              if(u < 0.5)
                wk = hinv3 * (2.546479089470 + 15.278874536822 * (u - 1) * u * u);
              else
                wk = hinv3 * 5.092958178941 * (1.0 - u) * (1.0 - u) * (1.0 - u);

              rho += this->mass[p] * wk;
            }

          GrP[P[i].GroupPartIndex].Density = rho;
        }
    }
#ifdef DEBUG
  printf("\nRank %d done.\n", myProc);
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Write the GrP array for final results of subhalo finding
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::write_GrP_information()
{
  // Write the information from the GrP array
#ifdef DEBUG
 cout << "Rank " << myProc << " write_GrP_information" << endl;
#endif

  ostringstream GrPname;
  GrPname << "sb256.GrP." << myProc;
  ofstream GrPstream(GrPname.str().c_str(), ios::out);
  for (int i = 0; i < NumFOFPart; i++) {
    GrPstream << i
              << " partIndx " << GrP[i].PartIndex
              << " sublen " << GrP[i].SubLen
              << " level " << GrP[i].Level
              << " pot " << GrP[i].Potential
              << " bind " << GrP[i].BindingEnergy
              << " dens " << GrP[i].Density
              << " hsml " << GrP[i].Hsml
              << " ID " << GrP[i].ID << endl;
  }
  GrPstream.close();

#ifdef DEBUG
  cout << "Rank " << myProc << " FINISH write_GrP_information" << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Write the GrP array for final results of subhalo finding
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::write_subhalo_catalog()
{
  // Write ascii and cosmo file of the subhalo catalog
#ifdef DEBUG
  cout << "Rank " << myProc << " write_subhalo_catalog" << endl;
#endif

  ostringstream aname, cname;
  if (this->numProc == 1) {
    aname << "sb256.subhalocatalog.ascii";
    cname << "sb256.subhalocatalog.cosmo";
  } else {
    aname << "sb256.subhalocatalog.ascii." << myProc;
    cname << "sb256.subhalocatalog.cosmo." << myProc;
  }
  ofstream aStream(aname.str().c_str(), ios::out);
  ofstream cStream(cname.str().c_str(), ios::out|ios::binary);

  char str[1024];
  float fBlock[COSMO_FLOAT];
  int iBlock[COSMO_INT];

  // Iterate over all FOF particles
  for (int p = 0; p < this->NumFOFPart; p++) {

#ifdef DEBUG
    cout << "Rank " << myProc << "subhalo catalog of p = " << p << endl;
#endif

    ID_T subhaloID = GrP[p].ID;
    int subhaloLen = GrP[p].SubLen;

    // Subhalo particle and not fuzz
    if (subhaloLen != 0) {

      POSVEL_T subhaloMass = subhaloLen * PartMass;

      // Find mean velocity and index of particle with minimum potential
      POSVEL_T xMeanVel = 0.0;
      POSVEL_T yMeanVel = 0.0;
      POSVEL_T zMeanVel = 0.0;
      POSVEL_T minPot = GrP[p].Potential;
      int minPotIndex = GrP[p].PartIndex;

      for (int v = 0; v < subhaloLen; v++) {
        xMeanVel += this->vx[GrP[p + v].PartIndex];
        yMeanVel += this->vy[GrP[p + v].PartIndex];
        zMeanVel += this->vz[GrP[p + v].PartIndex];

        if (GrP[p + v].Potential < minPot) {
          minPot = GrP[p + v].Potential;
          minPotIndex = GrP[p + v].PartIndex;
        }
      }
      xMeanVel /= subhaloLen;
      yMeanVel /= subhaloLen;
      zMeanVel /= subhaloLen;
      p += (subhaloLen - 1);

      // Write ascii
      sprintf(str, "%12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12ld\n",
        this->xx[minPotIndex],
        xMeanVel,
        this->yy[minPotIndex],
        yMeanVel,
        this->zz[minPotIndex],
        zMeanVel,
        subhaloMass,
        subhaloID);
        aStream << str;

      fBlock[0] = this->xx[minPotIndex];
      fBlock[1] = xMeanVel;
      fBlock[2] = this->yy[minPotIndex];
      fBlock[3] = yMeanVel;
      fBlock[4] = this->zz[minPotIndex];
      fBlock[5] = zMeanVel;
      fBlock[6] = subhaloMass;
      cStream.write(reinterpret_cast<char*>(fBlock),
                    COSMO_FLOAT * sizeof(POSVEL_T));

      iBlock[0] = subhaloID;
      cStream.write(reinterpret_cast<char*>(iBlock),
                    COSMO_INT * sizeof(ID_T));
    }
  }
  aStream.close();
  cStream.close();

#ifdef DEBUG
  cout << "Rank " << myProc << " FINISH write_subhalo_catalog" << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Write the GrP array for final results of subhalo finding
//
/////////////////////////////////////////////////////////////////////////

void SUBFIND::write_subhalo_particles()
{
  // Write cosmo file of all FOF particles with subhalos
#ifdef DEBUG
  cout << "Rank " << myProc << " write_subhalo_particles" << endl;
#endif

  ostringstream cname;
  if (this->numProc == 1) {
    cname << "sb256.subhaloparticles.cosmo";
  } else {
    cname << "sb256.subhaloparticles.cosmo." << myProc;
  }
  ofstream cStream(cname.str().c_str(), ios::out|ios::binary);

  float fBlock[COSMO_FLOAT];
  int iBlock[COSMO_INT];

  for (int p = 0; p < this->NumFOFPart; p++) {

#ifdef DEBUG
    cout << "Rank " << myProc << "subhalo particles of p = " << p << endl;
#endif

    ID_T subhaloID = GrP[p].ID;
    int subhaloLen = GrP[p].SubLen;

    if (subhaloLen != 0) {

      for (int v = 0; v < subhaloLen; v++) {
        int pIndex = GrP[p+v].PartIndex;
        fBlock[0] = this->xx[pIndex];
        fBlock[1] = this->vx[pIndex];
        fBlock[2] = this->yy[pIndex];
        fBlock[3] = this->vy[pIndex];
        fBlock[4] = this->zz[pIndex];
        fBlock[5] = this->vz[pIndex];
        fBlock[6] = this->mass[pIndex];
        cStream.write(reinterpret_cast<char*>(fBlock),
                      COSMO_FLOAT * sizeof(POSVEL_T));

        iBlock[0] = subhaloID;
        cStream.write(reinterpret_cast<char*>(iBlock),
                      COSMO_INT * sizeof(ID_T));
      }
      p += (subhaloLen - 1);
    } else {
      int pIndex = GrP[p].PartIndex;
      fBlock[0] = this->xx[pIndex];
      fBlock[1] = this->vx[pIndex];
      fBlock[2] = this->yy[pIndex];
      fBlock[3] = this->vy[pIndex];
      fBlock[4] = this->zz[pIndex];
      fBlock[5] = this->vz[pIndex];
      fBlock[6] = this->mass[pIndex];
      cStream.write(reinterpret_cast<char*>(fBlock),
                    COSMO_FLOAT * sizeof(POSVEL_T));

      iBlock[0] = -1;
      cStream.write(reinterpret_cast<char*>(iBlock),
                    COSMO_INT * sizeof(ID_T));
    }
  }
  cStream.close();

#ifdef DEBUG
  cout << "Rank " << myProc << " FINISH write_subhalo_particles" << endl;
#endif

}

/////////////////////////////////////////////////////////////////////////
//
// For sorting on decreasing density
//
/////////////////////////////////////////////////////////////////////////

int compare_dens(const void *a, const void *b)
{
  if(((dens_dat *) a)->Density >
     ((dens_dat *) b)->Density)
    return -1;

  if(((dens_dat *) a)->Density <
     ((dens_dat *) b)->Density)
    return +1;

  return 0;
}

/////////////////////////////////////////////////////////////////////////
//
// For sorting on decreasing energy
//
/////////////////////////////////////////////////////////////////////////

int compare_energy(const void *a, const void *b)
{
  if(((energy_dat *) a)->energy >
     ((energy_dat *) b)->energy)
    return -1;

  if(((energy_dat *) a)->energy <
     ((energy_dat *) b)->energy)
    return +1;

  return 0;
}

/////////////////////////////////////////////////////////////////////////
//
// For sorting FOF particles into subhalo clusters
// By subhalo length, level and then binding energy
//
/////////////////////////////////////////////////////////////////////////

int compare_grp_particles(const void *a, const void *b)
{
  if(((group_particle_data *) a)->SubLen >
     ((group_particle_data *) b)->SubLen)
    return -1;

  if(((group_particle_data *) a)->SubLen <
     ((group_particle_data *) b)->SubLen)
    return +1;

  if(((group_particle_data *) a)->Level >
     ((group_particle_data *) b)->Level)
    return -1;

  if(((group_particle_data *) a)->Level <
     ((group_particle_data *) b)->Level)
    return +1;

  if(((group_particle_data *) a)->BindingEnergy <
     ((group_particle_data *) b)->BindingEnergy)
    return -1;

  if(((group_particle_data *) a)->BindingEnergy >
     ((group_particle_data *) b)->BindingEnergy)
    return +1;

  return 0;
}

/////////////////////////////////////////////////////////////////////////
//
// For sorting near neighbors on distance
//
/////////////////////////////////////////////////////////////////////////

int ngb_compare_key(const void *a, const void *b)
{
  if(((r2data *) a)->r2 < (((r2data *) b)->r2))
    return -1;

  if(((r2data *) a)->r2 > (((r2data *) b)->r2))
    return +1;

  return 0;
}
