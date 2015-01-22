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

// .NAME SUBFIND - calculate subhalos within individual FOF halos
//
// SUBFIND takes data from CosmoHaloFinderP about individual halos
// and data from all particles and calculates subhalos.  The output is the
// same form as that from CosmoHaloFinderP so that FOFHaloProperties can
// also be run on subhalos.
//

#ifndef SUBFIND_h
#define SUBFIND_h

#include "Definition.h"
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>


using namespace cosmotk;

struct particle_data
{
  int GroupPartIndex;           /*!< the corresponding particle */
  POSVEL_T u;                   /*!< internal energy of particle */
};

struct group_particle_data
{
  int  PartIndex;  /* mathing particle index */
  int  SubLen;     /* the length of the subhalo this particle belongs to */
  int  Level;
  POSVEL_T Potential; /* potential energy */
  POSVEL_T BindingEnergy;
  POSVEL_T Density;
  POSVEL_T Hsml;
  ID_T     ID;
};

struct NODE
{
  POSVEL_T len;         // sidelength of treenode
  POSVEL_T center[3];   // geometrical center of node
  union
  {
    int suns[8];        // temporary pointers to daughter nodes
    struct
    {
      POSVEL_T s[3];       // center of mass of node
      POSVEL_T mass;       // mass of node
      int cost;         // counts the number of interactions for node
      int sibling;      // next node in the walk if current node can be used
      int nextnode;     // next node in the walk if current node must be opened
      int father;       // parent node of each node (or -1 if we have root)
    }
    d;
  }
  u;
};

struct dens_dat
{
  int   Index;
  POSVEL_T Density;
};

struct cand_dat
{
  int head;
  int len;
};

struct energy_dat
{
  POSVEL_T energy;
  int ind;
};

struct r2data
{
  POSVEL_T r2;
  int   index;
};

int compare_dens(const void* a, const void* b);
int compare_grp_particles(const void *a, const void *b);
int compare_energy(const void *a, const void *b);
int ngb_compare_key(const void *a, const void *b);

using std::string;
using std::vector;

/////////////////////////////////////////////////////////////////////////
//
// SUBFIND takes the FOF halos and particle information, builds
// a Barnes Hut octree from the particles of each halo.  The smoothing
// length and local density for each particle has been previously calculated
// using all particles on this processor, not just those in the FOF halo.
//
/////////////////////////////////////////////////////////////////////////

class SUBFIND {
public:
  SUBFIND();
  ~SUBFIND();

  void setParameters(
        POSVEL_T particleMass,
        POSVEL_T gravityConst,
        int numSPH,
        int numClose);

  void setParticles(
        POSVEL_T* xLoc,
        POSVEL_T* yLoc,
        POSVEL_T* zLoc,
        POSVEL_T* xVel,
        POSVEL_T* yVel,
        POSVEL_T* zVel,
        POSVEL_T* mass,
        ID_T* ID,
        ID_T N
        );

  void setParticles(
        vector<POSVEL_T>* xLoc,
        vector<POSVEL_T>* yLoc,
        vector<POSVEL_T>* zLoc,
        vector<POSVEL_T>* xVel,
        vector<POSVEL_T>* yVel,
        vector<POSVEL_T>* zVel,
        vector<POSVEL_T>* mass,
        vector<ID_T>* id);

  void setHalos(
        int numberOfFOFHalos,
        int* fofHaloCount,
        int* fofHaloStart,
        int* fofHaloList);

  void run();
  void determine_densities();
  void process_group(int gr);
  void unbind(int lev, int head, int len);

  size_t   tree_treeallocate(int maxnodes, int NumPart);
  void     tree_treefree();
  int      tree_treebuild(int lev, int start, int len);
  void     tree_update_node_recursive(int no, int sib, int father);
  POSVEL_T tree_treeevaluate_potential(int target);

  POSVEL_T ngb_treefind(POSVEL_T x, POSVEL_T y, POSVEL_T z,
                        int desngb, POSVEL_T hguess);
  int      ngb_treefind_variable(POSVEL_T x, POSVEL_T y, POSVEL_T z,
                                 POSVEL_T hguess);

  void write_GrP_information();
  void write_subhalo_catalog();
  void write_subhalo_particles();

private:
  int  myProc;
  int  numProc;

  POSVEL_T* xx;                       // X location for all particles
  POSVEL_T* yy;                       // Y location for all particles
  POSVEL_T* zz;                       // Z location for all particles
  POSVEL_T* vx;                       // X velocity for all particles
  POSVEL_T* vy;                       // Y velocity for all particles
  POSVEL_T* vz;                       // Z velocity for all particles
  POSVEL_T* mass;                     // mass for all particles
  ID_T* tag;                          // Id for all particles

  particle_data* P;                   // All particles on this processor
  int NumPart;                        // Number of all particles

  group_particle_data* GrP;           // All FOF particles on this processor
  int  Ngroups;                       // Number of FOF halos
  int  NumFOFPart;                    // Number of FOF particles
  int *GroupLen;                      // Length of each FOF halo
  int *GroupOffset;                   // Offset to first particle in FOF halo

  NODE* Nodes_base;                   // BHTree nodes
  NODE* Nodes;                        // access the nodes which is shifted
                                      // such that Nodes[All.MaxPart]
                                      // gives the first allocated node

  r2data* R2list;

  // Used in linking subhalos within FOF halos
  int *Head,*Next,*Tail,*Len;

  int  DesLinkNgb;
  int  DesDensNgb;

  int last;
  int *Nextnode;
  int *Father;

  POSVEL_T PartMass;
  POSVEL_T Hubble;
  POSVEL_T G;
  POSVEL_T Softening;
  POSVEL_T ErrTolTheta;
  POSVEL_T Omega0;
  POSVEL_T OmegaLambda;

  POSVEL_T UnitLength_in_cm;
  POSVEL_T UnitMass_in_g;
  POSVEL_T UnitVelocity_in_cm_per_s;
  POSVEL_T UnitTime_in_s;

  POSVEL_T minLoc[DIMENSION];
  POSVEL_T maxLoc[DIMENSION];
  POSVEL_T BoxSize;
  POSVEL_T boxhalf, boxsize;
  POSVEL_T Time;

  int    MaxNodes;
};

#endif
