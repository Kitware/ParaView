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
#include <assert.h>

#include "Partition.h"
#include "HaloCenterFinder.h"

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;

namespace cosmotk {
/////////////////////////////////////////////////////////////////////////
//
// HaloCenterFinder takes all particles in a halo and calculations the
// most bound particle (MBP) or most connected particle (MCP) using
// an N2/2 algorithm on small halos and ChainingMesh algorithms on
// large halos.
//
/////////////////////////////////////////////////////////////////////////

HaloCenterFinder::HaloCenterFinder()
{
  // Get the number of processors and rank of this processor
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();
}

HaloCenterFinder::~HaloCenterFinder()
{
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for the halo center finder
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::setParameters(
                        POSVEL_T pDist,
                        POSVEL_T rSmooth_,
                        POSVEL_T distConvertFactor,
                        float rL_,
                        int np_,
                        float omega_matter_,
                        float omega_cb_,
                        float hubble_,
                        float redshift_)
{
  // Halo finder parameters
  this->bb = pDist;
  this->rSmooth = rSmooth_;
  this->distFactor = distConvertFactor;
  this->rL = rL_;
  this->np = np_;
  this->omega_matter = omega_matter_;
  this->omega_cb = omega_cb_;
  this->hubble = hubble_;
  this->redshift = redshift_;
}

/////////////////////////////////////////////////////////////////////////
//
// Set the particle vectors that have already been read and which
// contain only the alive particles for this processor
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::setParticles(
                        long haloCount,
                        POSVEL_T* xLoc,
                        POSVEL_T* yLoc,
                        POSVEL_T* zLoc,
                        POSVEL_T* massHalo,
                        ID_T* id)
{
  this->particleCount = haloCount;
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;
  this->mass = massHalo;
  this->tag = id;
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

int HaloCenterFinder::mostConnectedParticleN2()
{
  // Arrange in an upper triangular grid of friend counts
  // friendCount will hold number of friends that a particle has
  //
  int* friendCount = new int[this->particleCount];
  for (int i = 0; i < this->particleCount; i++)
    friendCount[i] = 0;

  // Iterate on all particles in halo adding to count if friends of each other
  // Iterate in upper triangular fashion
  for (int p = 0; p < this->particleCount; p++) {

    // Get halo particle after the current one
    for (int q = p+1; q < this->particleCount; q++) {

      // Calculate distance betweent the two
      POSVEL_T xdist = fabs(this->xx[p] - this->xx[q]);
      POSVEL_T ydist = fabs(this->yy[p] - this->yy[q]);
      POSVEL_T zdist = fabs(this->zz[p] - this->zz[q]);

      if ((xdist < this->bb) && (ydist < this->bb) && (zdist < this->bb)) {
        POSVEL_T dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
        if (dist < this->bb) {
          friendCount[p]++;
          friendCount[q]++;
        }
      }
    }
  }

  // Particle with the most friends
  int maxFriends = 0;
  int result = 0;

  for (int i = 0; i < this->particleCount; i++) {
    if (friendCount[i] > maxFriends) {
      maxFriends = friendCount[i];
      result = i;
    }
  }

  delete [] friendCount;
  return result;
}

/////////////////////////////////////////////////////////////////////
//
//This method projects the particle coordinates to the x,y,z dimensions and takes a one dimensional histogram. Whereever the peak is, the code then takes all of the particles near that peak, and peformes a two dimensional
//histogram to more accurately find the center.
//
////////////////////////////////////////////////////////////////////
int HaloCenterFinder::mostConnectedParticleHist()
{
/*
  for(int i = 0; i < this->particleCount; i++){
     this->xx[i]=i%50;
  }
*/
  POSVEL_T x_min=this->xx[0];
  POSVEL_T x_max=this->xx[0];
  POSVEL_T y_min=this->yy[0];
  POSVEL_T y_max=this->yy[0];
  POSVEL_T z_min=this->zz[0];
  POSVEL_T z_max=this->zz[0];
  POSVEL_T del_x=0;
  POSVEL_T del_y=0;
  POSVEL_T del_z=0;

//////// Use approximate halo radius to figure out size of bins for the histograms (radius formula from Zarija Lukic et al. "The Halo Mass Function: High-Redshift Evoluton and Univesality" (2008))
  int num_part = this->np;

  // Is this the box side length in physical units (Mpc/h)?
  // If so it needs to be a float, not an int
  // I think particle info is in physical units not grid units here
  POSVEL_T box_size = this->rL;

//
//  int me=8;
//

  POSVEL_T omega_m = this->omega_matter; //matter content of universe
  POSVEL_T z_val = this->redshift;

  // assume flat universe
  POSVEL_T omega_lambda = 1.0-omega_m;

  POSVEL_T omega_z=omega_m*(pow((1+z_val),3.0))/(omega_m*pow((1+z_val),3.0)+omega_lambda);

  POSVEL_T delta=200;

  //double mass_particle=2.775*(pow(10,11))*omega_m*pow((box_size/num_part),3.0);//YYY use when have access to box_size and num_particles in box

  // omega_cb (CDM+baryons) for particle mass calculation
  // DO I NEED TO DIVIDE BY h TO GET CORRECT UNITS?
  // I DIVIDE BY h IN Basedata, BUT THAT MAY NOT EVER BE USED
  POSVEL_T mass_particle=2.775e11*this->omega_cb*pow(box_size/num_part,3.0);

  // omega_matter for background evolution
  POSVEL_T radius=9.51e-5*pow((omega_z/omega_m),(1.0/3.0))*pow(((1.0/delta)*mass_particle*this->particleCount),(1.0/3.0));

  //if(this->myProc==me)printf("\nRADIUS=%.5f,NP=%d,omega_z=%.5f,mass_particle=%.5f,boxsize=%d",radius,num_part,omega_z,mass_particle,box_size);
  //if(this->myProc==me)printf("\n\nRADIUS=%.5f,NP=%d,omega_z=%.5f,mass_particle=%.5f,boxsize=%d,np_halo=%d",radius,num_part,omega_z,mass_particle,box_size,this->particleCount);

//////////////////////////////////////////


  //Find the maximun and minimum values for the coordinates used for the 1-d histograms.
  for(int i = 0; i < this->particleCount; i++){
     //if(this->myProc==me)printf("\ni_long=%d, val=%.5f",i,this->xx[i]);
     if(this->xx[i] < x_min)x_min=this->xx[i];
     if(this->yy[i] < y_min)y_min=this->yy[i];
     if(this->zz[i] < z_min)z_min=this->zz[i];
     if(this->xx[i] > x_max)x_max=this->xx[i];
     if(this->yy[i] > y_max)y_max=this->yy[i];
     if(this->zz[i] > z_max)z_max=this->zz[i];
  }

  POSVEL_T perc_1d=0.06;
  POSVEL_T temp_val=((x_max-x_min)/(perc_1d*radius));
  int bin_size=(int) temp_val;
  if (bin_size < 5)bin_size=5;//make sure always have some bins, (5 was just a small number, but frankly arbitrary).
  //if(this->myProc==me)printf("\nBINS==%d",bin_size);
  del_x=(x_max-x_min)/bin_size;
  del_y=(y_max-y_min)/bin_size;
  del_z=(z_max-z_min)/bin_size;
  //assert(del_x != 0 && del_y != 0 && del_z != 0);
  int* x_hist = new int[bin_size];
  int* y_hist = new int[bin_size];
  int* z_hist = new int[bin_size];
  for(int i=0; i < bin_size; i++){
     x_hist[i]=0;
     y_hist[i]=0;
     z_hist[i]=0;
  }
  int max_bin_x=0;
  int max_bin_y=0;
  int max_bin_z=0;
  int x_indx=0;
  int y_indx=0;
  int z_indx=0;

  //Perform 1d histogram, and find peaks for x,y,z.
  for(int i = 0; i < this->particleCount; i++){
     int my_index=(this->xx[i]==x_max) ? bin_size-1: (int) ((this->xx[i]-x_min)/del_x);
     if(my_index < 0 || my_index > bin_size-1){
        printf("\nIncorrect X.  x=%.3e, x_max=%.3e, x_min=%.3e, del_x=%.3e, my_indx=%d",this->xx[i],x_max,x_min,del_x,my_index);
        printf("\nWarning, Halo Center Finding Algorithm Error for ID=%" ID_T_FMT ". Dont panic, using MBP algorithm for this Halo.\n",this->tag[i]);
        return -1;
      }
     x_hist[my_index]++;
     if(max_bin_x < x_hist[my_index]){
       max_bin_x=x_hist[my_index];
       x_indx=my_index;
     }

     my_index=(this->yy[i]==y_max) ? bin_size-1: (int) ((this->yy[i]-y_min)/del_y);
     if(my_index < 0 || my_index > bin_size-1){
        printf("\nIncorrect Y. y=%.3e, y_max=%.3e, y_min=%.3e, del_y=%.3e, my_indx=%d",this->yy[i],y_max,y_min,del_y,my_index);
        printf("\nWarning, Halo Center Finding Algorithm Error for ID=%" ID_T_FMT ". Dont panic, using MBP algorithm for this Halo.\n",this->tag[i]);
        return -1;
      }
     y_hist[my_index]++;
     if(max_bin_y < y_hist[my_index]){
       max_bin_y=y_hist[my_index];
       y_indx=my_index;
     }

     my_index=(this->zz[i]==z_max) ? bin_size-1: (int) ((this->zz[i]-z_min)/del_z);
     if(my_index < 0 || my_index > bin_size-1){
        printf("\nIncorrect Z. z=%.3e, z_max=%.3e, z_min=%.3e, del_z=%.3e, my_indx=%d",this->zz[i],z_max,z_min,del_z,my_index);
        printf("\nWarning, Halo Center Finding Algorithm Error for ID=%" ID_T_FMT ". Dont panic, using MBP algorithm for this Halo.\n",this->tag[i]);
        return -1;
      }
     z_hist[my_index]++;
     if(max_bin_z < z_hist[my_index]){
       max_bin_z=z_hist[my_index];
       z_indx=my_index;
     }

  }
  //for(int i=0;i<bin_size;i++){
      //if(this->myProc==me)printf("\ni=%d, val=%d",i,x_hist[i]);
  //}
  delete [] x_hist;
  delete [] y_hist;
  delete [] z_hist;

  POSVEL_T center[3];//coordinate of center
  center[0]=x_min+del_x*x_indx+del_x/2.0;//use midpoint of bin
  center[1]=y_min+del_y*y_indx+del_y/2.0;
  center[2]=z_min+del_z*z_indx+del_z/2.0;
  //if(this->myProc == me)printf("\n FIRST HISTO:My numpart=%d, x_min=%.5e, x_max=%.5e, del_x=%.5e,bin_size=%d,center(%.5e,%.5e,%.5e)",this->particleCount,x_min,x_max,del_x,bin_size,center[0],center[1],center[2]);
  //now reduce the dataset around the found center for the 2-d histograms
  POSVEL_T range=0.4*radius;
  if(range < 0.3)range=0.3;//make sure search range is not too small.
  POSVEL_T* x_short= new POSVEL_T [this->particleCount];
  POSVEL_T* y_short= new POSVEL_T [this->particleCount];
  POSVEL_T* z_short= new POSVEL_T [this->particleCount];
  x_min=center[0]-range;//set x,y,z, max and min to be a cube around the estimated center
  y_min=center[1]-range;
  z_min=center[2]-range;
  x_max=center[0]+range;
  y_max=center[1]+range;
  z_max=center[2]+range;
  int count=0;
  for(int i = 0; i < this->particleCount; i++){//throw out ALL other particles not in the cube around the center ( YYY in future could do 1 pass first to determine size and save memory)
     if((this->xx[i] > x_min) && (this->xx[i] < x_max)
     && (this->yy[i] > y_min) && (this->yy[i] < y_max)
     && (this->zz[i] > z_min) && (this->zz[i] < z_max)){
        x_short[count]=this->xx[i];
        y_short[count]=this->yy[i];
        z_short[count]=this->zz[i];
        count++;

     }
  }
  x_min=x_short[0];
  y_min=y_short[0];
  z_min=z_short[0];
  x_max=x_short[0];
  y_max=y_short[0];
  z_max=z_short[0];
  //int len=x_short.size();
  for(int i=0; i< count; i++){
     if(x_short[i] < x_min)x_min=x_short[i];
     if(y_short[i] < y_min)y_min=y_short[i];
     if(z_short[i] < z_min)z_min=z_short[i];
     if(x_short[i] > x_max)x_max=x_short[i];
     if(y_short[i] > y_max)y_max=y_short[i];
     if(z_short[i] > z_max)z_max=z_short[i];

  }

  //perform the 2d histograms to
  POSVEL_T perc_2d=0.01;
  temp_val=((x_max-x_min)/(perc_2d*radius));
  if(((y_max-y_min)/(perc_2d*radius)) > temp_val)temp_val=((y_max-y_min)/(perc_2d*radius));
  if(((z_max-z_min)/(perc_2d*radius)) > temp_val)temp_val=((z_max-z_min)/(perc_2d*radius));
  int bin_size_2d=(int) temp_val;
  if (bin_size_2d < 5)bin_size_2d=5;//make sure always have some bins, (5 was just a small number, but frankly arbitrary).

  //if(this->myProc==me)printf("\nBINS2D==%d",bin_size);
  int dims=bin_size_2d*bin_size_2d;
  int* x_y_hist = new int [dims];
  int* y_z_hist = new int [dims];
  int* x_z_hist = new int [dims];

  for(int i=0;i<bin_size_2d*bin_size_2d; i++){
     x_y_hist[i]=0;
     x_z_hist[i]=0;
     y_z_hist[i]=0;
  }

  //int* x_z_hist = new int [bin_size_2d][bin_size_2d]; Currently no need for third 2d histogram (if performed, so far we have found that the y and z values will match up)

  int max_xy=0;
  int max_yz=0;
  int max_xz=0;
  x_indx=0;
  y_indx=0;
  z_indx=0;
  int x_indx2=0;
  int y_indx2=0;
  int z_indx2=0;


  //POSVEL_T del_2d= 2*range/bin_size_2d;
  del_x=(x_max-x_min)/bin_size_2d;
  del_y=(y_max-y_min)/bin_size_2d;
  del_z=(z_max-z_min)/bin_size_2d;
  for(int i = 0; i < count; i++ ){
    //if(this->myProc==me)printf("\nI=%d,count=%d",i,count);
    int index_x= (x_short[i]==x_max) ? bin_size_2d-1 : (int) ((x_short[i]-x_min)/del_x);
    int index_y= (y_short[i]==y_max) ? bin_size_2d-1 : (int) ((y_short[i]-y_min)/del_y);
    int index_z= (z_short[i]==z_max) ? bin_size_2d-1 : (int) ((z_short[i]-z_min)/del_z);

    if(index_x<0 || index_x > bin_size_2d-1){
      printf("\nIncorrect 2dX:%d,%d,%d,%.5e,%.5e,%.5e,%.5f",this->myProc,index_x,bin_size_2d,x_short[i],x_min,del_x,x_max);
      printf("\nWarning, Halo Center Finding Algorithm Error for ID=%" ID_T_FMT ". Dont panic, using MBP algorithm for this Halo.\n",this->tag[i]);
      return -1;
    }
    if(index_y<0 || index_y > bin_size_2d-1){
      printf("\nIncorrect 2dY:%d,%d,%d,%.5e,%.5e,%.5e,%.5f",this->myProc,index_y,bin_size_2d,y_short[i],y_min,del_y,y_max);
      printf("\nWarning, Halo Center Finding Algorithm Error for ID=%" ID_T_FMT ". Dont panic, using MBP algorithm for this Halo.\n",this->tag[i]);
      return -1;
    }
    if(index_z<0 || index_z > bin_size_2d-1){
      printf("\nIncorrect 2dZ:%d,%d,%d,%.5e,%.5e,%.5e,%.5f",this->myProc,index_z,bin_size_2d,z_short[i],z_min,del_z,z_max);
      printf("\nWarning, Halo Center Finding Algorithm Error for ID=%" ID_T_FMT ". Dont panic, using MBP algorithm for this Halo.\n",this->tag[i]);
      return -1;
    }
    int hash=bin_size_2d*index_x+index_y;
    x_y_hist[hash]++;
    if(max_xy < x_y_hist[hash]){
      max_xy=x_y_hist[hash];
      x_indx=index_x;
      y_indx=index_y;
    }
    hash=bin_size_2d*index_y+index_z;
    y_z_hist[hash]++;
    if(max_yz < y_z_hist[hash]){
      max_yz=y_z_hist[hash];
      y_indx2=index_y;
      z_indx=index_z;
    }
    hash=bin_size_2d*index_z+index_x;
    x_z_hist[hash]++;
    if(max_xz < x_z_hist[hash]){
      max_xz=y_z_hist[hash];
      x_indx2=index_x;
      z_indx2=index_z;
    }

  }
  //if(this->myProc == me)printf("\nx1,x2=%d,%d,y1,y2=%d,%d,z1,z2=%d,%d",x_indx,x_indx2,y_indx,y_indx2,z_indx,z_indx2);

  center[0]=x_min+del_x*x_indx+del_x/2.0;//use midpoint of bin
  center[1]=y_min+del_y*y_indx+del_y/2.0;
  center[2]=z_min+del_z*z_indx+del_z/2.0;

  int temp[3];
  temp[0]=x_min+del_x*x_indx2+del_x/2.0;//use midpoint of bin
  temp[1]=y_min+del_y*y_indx2+del_y/2.0;
  temp[2]=z_min+del_z*z_indx2+del_z/2.0;
  POSVEL_T err=sqrt(pow((temp[0]-center[0]),2) + pow((temp[1]-center[1]),2) + pow((temp[2]-center[2]),2));
  /* silence compiler warning */
  static_cast<void>(err);
  //if(this->myProc == me)printf("error= %.5e, radius=%.5e",err,radius);
  //YYY Could have some metric for error capture here (meaning that one could have some sort of error control, based on how big the value of "error" is).
  //if(err > radius)return -1;

  //if(this->myProc == me)printf("\n SECOND HISTO:My numpart=%d, x_min=%.5e, x_max=%.5e, del_x=%.5e,bin_size=%d,center(%.5e,%.5e,%.5e)",this->particleCount,x_min,x_max,del_x,bin_size_2d,center[0],center[1],center[2]);
 //Now just need to find closest particle to this center coordinate:

  POSVEL_T dist=sqrt(pow((this->xx[0]-center[0]),2) + pow((this->yy[0]-center[1]),2) + pow((this->zz[0]-center[2]),2));
  int halo_center=0;
  for(int i = 0; i < this->particleCount; i++){
     POSVEL_T tmp=sqrt(pow((this->xx[i]-center[0]),2) + pow((this->yy[i]-center[1]),2) + pow((this->zz[i]-center[2]),2));
     if(tmp < dist){
       dist=tmp;
       halo_center=i;
     }
  }
  if(!(halo_center >= 0 && halo_center < this->particleCount))fprintf(stderr,"MYINDEX=%d, this->particlecount=%ld \n",halo_center, this->particleCount);
  fflush(stderr);
  assert(halo_center >= 0 && halo_center < this->particleCount);

  delete [] x_y_hist;
  delete [] x_z_hist;
  delete [] y_z_hist;
  delete [] x_short;
  delete [] y_short;
  delete [] z_short;
  //if(this->myProc == 0)printf("\n Particle_center(%.5e,%.5e,%.5e)",this->xx[halo_center],yy[halo_center],zz[halo_center]);
  return halo_center;
}


/////////////////////////////////////////////////////////////////////////
//
// Most connected particle using a chaining mesh of particles in one FOF halo
// Build chaining mesh with a grid size such that all friends will be in
// adjacent mesh grids.
//
/////////////////////////////////////////////////////////////////////////

int HaloCenterFinder::mostConnectedParticleChainMesh()
{
  int bp, bi, bj, bk;
  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T xdist, ydist, zdist, dist;

  // Build the chaining mesh
  int chainFactor = MCP_CHAIN_FACTOR;
  POSVEL_T chainSize = (this->bb * this->distFactor) / chainFactor;
  ChainingMesh* haloChain = buildChainingMesh(chainSize);

  // Save the number of friends for each particle in the halo
  int* friendCount = new int[this->particleCount];
  for (int i = 0; i < this->particleCount; i++)
    friendCount[i] = 0;

  // Get chaining mesh information
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();

  // Calculate the friend count within each bucket using upper triangular loop
  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        bp = buckets[bi][bj][bk];
        while (bp != -1) {

          wp = bucketList[bp];
          while (wp != -1) {
            xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
            ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
            zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
            dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
            if (dist != 0.0 && dist < this->bb) {
              friendCount[bp]++;
              friendCount[wp]++;
            }
            wp = bucketList[wp];
          }
          bp = bucketList[bp];
        }
      }
    }
  }

  // Walk every bucket in the chaining mesh, processing all particles in bucket
  // against all neighbor bucket particles one time, storing the friend
  // count in two places, using the sliding window trick
  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        // Set the walking window around this bucket
        first[0] = bi - chainFactor; last[0] = bi + chainFactor;
        first[1] = bj - chainFactor; last[1] = bj + chainFactor;
        first[2] = bk - chainFactor; last[2] = bk + chainFactor;

        for (int dim = 0; dim < DIMENSION; dim++) {
          if (first[dim] < 0)
            first[dim] = 0;
          if (last[dim] >= meshSize[dim])
            last[dim] = meshSize[dim] - 1;
        }

        // First particle in the bucket being processed
        bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // For the current particle in the current bucket count friends
          // going to all neighbor buckets in the chaining mesh.
          // With the sliding window we calculate the distance between
          // two particles and can fill in both, but when the second particle's
          // bucket is reached we can't calculate and add in again
          // So we must be aware of which buckets have not already been
          // compared to this bucket and calculate only for planes and rows
          // that have not already been processed

          // Do entire trailing plane of buckets that has not been processed
          for (wi = bi + 1; wi <= last[0]; wi++) {
            for (wj = first[1]; wj <= last[1]; wj++) {
              for (wk = first[2]; wk <= last[2]; wk++) {
                wp = buckets[wi][wj][wk];
                while (wp != -1) {
                  xdist = (POSVEL_T) fabs(this->xx[bp] - this->xx[wp]);
                  ydist = (POSVEL_T) fabs(this->yy[bp] - this->yy[wp]);
                  zdist = (POSVEL_T) fabs(this->zz[bp] - this->zz[wp]);
                  dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                  if (dist != 0.0 && dist < this->bb) {
                    friendCount[bp]++;
                    friendCount[wp]++;
                  }
                  wp = bucketList[wp];
                }
              }
            }
          }

          // Do entire trailing row that has not been processed in this plane
          wi = bi;
          for (wj = bj + 1; wj <= last[1]; wj++) {
            for (wk = first[2]; wk <= last[2]; wk++) {
              wp = buckets[wi][wj][wk];
              while (wp != -1) {
                xdist = (POSVEL_T) fabs(this->xx[bp] - this->xx[wp]);
                ydist = (POSVEL_T) fabs(this->yy[bp] - this->yy[wp]);
                zdist = (POSVEL_T) fabs(this->zz[bp] - this->zz[wp]);
                dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
                if (dist != 0.0 && dist < this->bb) {
                  friendCount[bp]++;
                  friendCount[wp]++;
                }
                wp = bucketList[wp];
              }
            }
          }

          // Do bucket trailing buckets in this row
          wi = bi;
          wj = bj;
          for (wk = bk+1; wk <= last[2]; wk++) {
            wp = buckets[wi][wj][wk];
            while (wp != -1) {
              xdist = (POSVEL_T) fabs(this->xx[bp] - this->xx[wp]);
              ydist = (POSVEL_T) fabs(this->yy[bp] - this->yy[wp]);
              zdist = (POSVEL_T) fabs(this->zz[bp] - this->zz[wp]);
              dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist));
              if (dist != 0.0 && dist < this->bb) {
                friendCount[bp]++;
                friendCount[wp]++;
              }
              wp = bucketList[wp];
            }
          }
          bp = bucketList[bp];
        }
      }
    }
  }
  // Particle with the most friends
  int maxFriends = 0;
  int result = 0;

  for (int i = 0; i < this->particleCount; i++) {
    if (friendCount[i] > maxFriends) {
      maxFriends = friendCount[i];
      result = i;
    }
  }

  delete [] friendCount;
  delete haloChain;

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

int HaloCenterFinder::mostBoundParticleN2(POTENTIAL_T* minPotential)
{
  POSVEL_T rsm2 = this->rSmooth*this->rSmooth;

  // Arrange in an upper triangular grid to save computation
  POTENTIAL_T* lpot = new POTENTIAL_T[this->particleCount];
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < this->particleCount; i++)
    lpot[i] = 0.0;

  // First particle in halo to calculate minimum potential on
  for (int p = 0; p < this->particleCount; p++) {

    // Next particle in halo in minimum potential loop
    POTENTIAL_T lpotp = lpot[p];
#ifdef _OPENMP
#pragma omp parallel for reduction(+:lpotp)
#endif
    for (int q = p+1; q < this->particleCount; q++) {

      POSVEL_T xdist = (POSVEL_T)(this->xx[p] - this->xx[q]);
      POSVEL_T ydist = (POSVEL_T)(this->yy[p] - this->yy[q]);
      POSVEL_T zdist = (POSVEL_T)(this->zz[p] - this->zz[q]);

      POSVEL_T dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist) + rsm2);

      if (dist != 0.0) {
        lpotp   = (POTENTIAL_T)(lpotp   - (this->mass[q] / dist));
        lpot[q] = (POTENTIAL_T)(lpot[q] - (this->mass[p] / dist));
      }
    }

    lpot[p] = lpotp;
  }

  *minPotential = MAX_FLOAT;
  int result = 0;
  for (int i = 0; i < this->particleCount; i++) {
    if (lpot[i] < *minPotential) {
      *minPotential = lpot[i];
      result = i;
    }
  }
  delete [] lpot;

  return result;
}

/////////////////////////////////////////////////////////////////////////
//
// Most bound particle using a chaining mesh of particles in one FOF halo.
// and a combination of actual particle-to-particle values and estimation
// values based on number of particles in a bucket and the distance to the
// nearest corner.
//
// For the center area of a halo calculate the actual values for 26 neigbors.
// For the perimeter area of a halo use a bounding box of those neighbors
// to make up the actual portion and a estimate to other particles in the
// neighbors.  This is to keep a particle from being too close to the
// closest corner and giving a skewed answer.
//
// The refinement in the center buckets will be called level 1 because all
// buckets to a distance of 1 are calculated fully.  The refinement of the
// perimeter buckets will be called level 0 because only the center bucket
// is calculated fully.
//
// Note that in refining, level 0 must be brought up to level 1, and then
// refinement to more buckets becomes the same.
//
/////////////////////////////////////////////////////////////////////////

int HaloCenterFinder::mostBoundParticleAStar(POTENTIAL_T* minimumPotential)
{
  // Chaining mesh size is a factor of the interparticle halo distance
  POSVEL_T chainSize = this->bb * this->distFactor;

  // Boundary around edges of a bucket for calculating estimate
  POSVEL_T boundaryFactor = 10.0f * this->distFactor;
  POSVEL_T boundarySize = chainSize / boundaryFactor;

  // Actual values calculated for 26 neighbors in the center of a halo
  // Factor to decide what distance this is out from the center
  int eachSideFactor = 7;

  // Create the chaining mesh for this halo
  ChainingMesh* haloChain = buildChainingMesh(chainSize);

  // Get chaining mesh information
  int* meshSize = haloChain->getMeshSize();

  // Bucket ID allows finding the bucket every particle is in
  int* bucketID = new int[this->particleCount];

  // Refinement level for a particle indicate how many buckets out have actual
  // values calculate rather than estimates
  int* refineLevel = new int[this->particleCount];

  // Minimum potential made up of actual part and estimated part
  POSVEL_T* estimate = new POSVEL_T[this->particleCount];
  for (int i = 0; i < this->particleCount; i++)
    estimate[i] = 0.0;

  // Calculate better guesses (refinement level 1) around the center of halo
  // Use estimates with boundary around neighbors of perimeter
  int* minActual = new int[DIMENSION];
  int* maxActual = new int[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    int eachSide = meshSize[dim] / eachSideFactor;
    int middle = meshSize[dim] / 2;
    minActual[dim] = middle - eachSide;
    maxActual[dim] = middle + eachSide;
  }


  //////////////////////////////////////////////////////////////////////////
  //
  // Calculate actual for particles within individual bucket
  //
  aStarThisBucketPart(haloChain, bucketID, estimate);

  //////////////////////////////////////////////////////////////////////////
  //
  // Calculate actual values for immediate 26 neighbors for buckets in
  // the center of the halo (refinement level = 1)
  //
  aStarActualNeighborPart(haloChain, minActual, maxActual,
                          refineLevel, estimate);

  //////////////////////////////////////////////////////////////////////////
  //
  // Calculate estimated values for immediate 26 neighbors for buckets on
  // the edges of the halo (refinement level = 0)
  //
  aStarEstimatedNeighborPart(haloChain, minActual, maxActual,
                             refineLevel, estimate, boundarySize);

  //////////////////////////////////////////////////////////////////////////
  //
  // All buckets beyond the 27 nearest get an estimate based on count in
  // the bucket and the distance to the nearest point
  //
  aStarEstimatedPart(haloChain, estimate);

  //////////////////////////////////////////////////////////////////////////
  //
  // Iterative phase to refine individual particles
  //
  POSVEL_T minPotential = estimate[0];
  int minParticleCur = 0;
  int winDelta = 1;

  // Find the current minimum potential particle after actual and estimates
  for (int i = 0; i < this->particleCount; i++) {
    if (estimate[i] < minPotential) {
      minPotential = estimate[i];
      minParticleCur = i;
    }
  }
  POSVEL_T minPotentialLast = minPotential;
  int minParticleLast = -1;

  // Decode the bucket from the ID
  int id = bucketID[minParticleCur];
  int bk = id % meshSize[2];
  id = id - bk;
  int bj = (id % (meshSize[2] * meshSize[1])) / meshSize[2];
  id = id - (bj * meshSize[2]);
  int bi = id / (meshSize[2] * meshSize[1]);

  // Calculate the maximum winDelta for this bucket
  int maxDelta = max(max(
                     max(meshSize[0] - bi, bi), max(meshSize[1] - bj, bj)),
                     max(meshSize[2] - bk, bk));

  // Terminate when a particle is the minimum twice in a row AND
  // it has been calculated precisely without estimates over the entire halo
  int pass = 1;
  while (winDelta <= maxDelta) {
    while (minParticleLast != minParticleCur) {

      // Refine the value for all particles in the same bucket as the minimum
      // Alter the minimum in the reference
      // Return the particle index that is the new minimum of that bucket
      while (winDelta > refineLevel[minParticleCur] &&
             estimate[minParticleCur] <= minPotentialLast) {
        pass++;
        refineLevel[minParticleCur]++;

        // Going from level 0 to level 1 is special because the 27 neighbors
        // are part actual and part estimated.  After that all refinements are
        // replacing an estimate with an actual
        if (refineLevel[minParticleCur] == 1) {
          refineAStarLevel_1(haloChain, bi, bj, bk, minActual, maxActual,
                             minParticleCur, estimate,
                             boundarySize);
        } else {
          refineAStarLevel_N(haloChain, bi, bj, bk,
                             minParticleCur, estimate,
                             refineLevel[minParticleCur]);
        }
      }
      if (winDelta <= refineLevel[minParticleCur]) {
        minPotentialLast = estimate[minParticleCur];
        minParticleLast = minParticleCur;
      }

      // Find the current minimum particle
      minPotential = minPotentialLast;
      for (int i = 0; i < this->particleCount; i++) {
        if (estimate[i] <= minPotential) {
          minPotential = estimate[i];
          minParticleCur = i;
        }
      }

      // Decode the bucket from the ID
      id = bucketID[minParticleCur];
      bk = id % meshSize[2];
      id = id - bk;
      bj = (id % (meshSize[2] * meshSize[1])) / meshSize[2];
      id = id - (bj * meshSize[2]);
      bi = id / (meshSize[2] * meshSize[1]);

      // Calculate the maximum winDelta for this bucket
      maxDelta = max(max(
                     max(meshSize[0] - bi, bi), max(meshSize[1] - bj, bj)),
                     max(meshSize[2] - bk, bk));
    }
    pass++;
    winDelta++;
    minParticleLast = 0;
  }
  int result = minParticleCur;
  *minimumPotential = estimate[minParticleCur];

  delete [] estimate;
  delete [] bucketID;
  delete [] refineLevel;
  delete [] minActual;
  delete [] maxActual;
  delete haloChain;

  return result;
}

/////////////////////////////////////////////////////////////////////////
//
// Within a bucket calculate the actual values between all particles
// Set the bucket ID so that the associated bucket can be located quickly
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::aStarThisBucketPart(
                        ChainingMesh* haloChain,
                        int* bucketID,
                        POSVEL_T* estimate)
{
  POSVEL_T rsm2 = this->rSmooth*this->rSmooth;

  POSVEL_T xdist, ydist, zdist, dist;
  int bp, bp2, bi, bj, bk;

  // Get chaining mesh information
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();

  // Calculate actual values for all particles in the same bucket
  // All pairs are calculated one time and stored twice
  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // Remember the bucket that every particle is in
          bucketID[bp] = (bi * meshSize[1] * meshSize[2]) +
                         (bj * meshSize[2]) + bk;

          bp2 = bucketList[bp];
          while (bp2 != -1) {
            xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[bp2]);
            ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[bp2]);
            zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[bp2]);
            dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist) + rsm2);
            if (dist != 0.0) {
              estimate[bp] -= (this->mass[bp2] / dist);
              estimate[bp2] -= (this->mass[bp] / dist);
            }
            bp2 = bucketList[bp2];
          }
          bp = bucketList[bp];
        }
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////
//
// Calculate the actual values to particles in 26 immediate neighbors
// only for buckets in the center of the halo, indicated by min/maxActual.
// Do this with a sliding window so that an N^2/2 algorithm is done where
// calculations are stored in both particles at same time.  Set refineLevel
// to 1 indicating buckets to a distance of one from the particle were
// calculated completely.
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::aStarActualNeighborPart(
                        ChainingMesh* haloChain,
                        int* minActual,
                        int* maxActual,
                        int* refineLevel,
                        POSVEL_T* estimate)
{
  POSVEL_T rsm2 = this->rSmooth*this->rSmooth;

  // Walking window extents and size
  int bp, bi, bj, bk;
  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T xdist, ydist, zdist, dist;

  // Get chaining mesh information
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();

  // Process the perimeter buckets which contribute to the actual values
  // but which will get estimate values for their own particles
  for (bi = minActual[0] - 1; bi <= maxActual[0] + 1; bi++) {
    for (bj = minActual[1] - 1; bj <= maxActual[1] + 1; bj++) {
      for (bk = minActual[2] - 1; bk <= maxActual[2] + 1; bk++) {

        // Only do the perimeter buckets
        if ((bucketCount[bi][bj][bk] > 0) &&
            ((bi < minActual[0] || bi > maxActual[0]) ||
             (bj < minActual[1] || bj > maxActual[1]) ||
             (bk < minActual[2] || bk > maxActual[2]))) {

          // Set a window around this bucket for calculating actual potentials
          first[0] = bi - 1;    last[0] = bi + 1;
          first[1] = bj - 1;    last[1] = bj + 1;
          first[2] = bk - 1;    last[2] = bk + 1;
          for (int dim = 0; dim < DIMENSION; dim++) {
            if (first[dim] < minActual[dim])
              first[dim] = minActual[dim];
            if (last[dim] > maxActual[dim])
              last[dim] = maxActual[dim];
          }

          bp = buckets[bi][bj][bk];
          while (bp != -1) {

            // Check each bucket in the window
            for (wi = first[0]; wi <= last[0]; wi++) {
              for (wj = first[1]; wj <= last[1]; wj++) {
                for (wk = first[2]; wk <= last[2]; wk++) {

                  // Only do the window bucket if it is in the actual region
                  if (bucketCount[wi][wj][wk] != 0 &&
                      wi >= minActual[0] && wi <= maxActual[0] &&
                      wj >= minActual[1] && wj <= maxActual[1] &&
                      wk >= minActual[2] && wk <= maxActual[2]) {

                    wp = buckets[wi][wj][wk];
                    while (wp != -1) {
                      xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
                      ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
                      zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
                      dist = sqrt((xdist*xdist)+(ydist*ydist)+(zdist*zdist)+rsm2);
                      if (dist != 0.0) {
                        estimate[bp] -= (this->mass[wp] / dist);
                        estimate[wp] -= (this->mass[bp] / dist);
                      }
                      wp = bucketList[wp];
                    }
                  }
                }
              }
            }
            bp = bucketList[bp];
          }
        }
      }
    }
  }

  // Process the buckets in the center
  for (bi = minActual[0]; bi <= maxActual[0]; bi++) {
    for (bj = minActual[1]; bj <= maxActual[1]; bj++) {
      for (bk = minActual[2]; bk <= maxActual[2]; bk++) {

        // Set a window around this bucket for calculating actual potentials
        first[0] = bi - 1;    last[0] = bi + 1;
        first[1] = bj - 1;    last[1] = bj + 1;
        first[2] = bk - 1;    last[2] = bk + 1;
        for (int dim = 0; dim < DIMENSION; dim++) {
          if (first[dim] < minActual[dim])
            first[dim] = minActual[dim];
          if (last[dim] > maxActual[dim])
            last[dim] = maxActual[dim];
        }

        bp = buckets[bi][bj][bk];
        while (bp != -1) {

          // For the current particle in the current bucket calculate
          // the actual part from the 27 surrounding buckets
          // With the sliding window we calculate the distance between
          // two particles and can fill in both, but when the second particle's
          // bucket is reached we can't calculate and add in again
          // So we must be aware of which buckets have not already been
          // compared to this bucket and calculate only for planes and rows
          // that have not already been processed
          refineLevel[bp] = 1;

          // Do entire trailing plane of buckets that has not been processed
          for (wi = bi + 1; wi <= last[0]; wi++) {
            for (wj = first[1]; wj <= last[1]; wj++) {
              for (wk = first[2]; wk <= last[2]; wk++) {

                wp = buckets[wi][wj][wk];
                while (wp != -1) {
                  xdist = fabs(this->xx[bp] - this->xx[wp]);
                  ydist = fabs(this->yy[bp] - this->yy[wp]);
                  zdist = fabs(this->zz[bp] - this->zz[wp]);
                  dist = sqrt((xdist*xdist)+(ydist*ydist)+(zdist*zdist)+rsm2);
                  if (dist != 0.0) {
                    estimate[bp] -= (this->mass[wp] / dist);
                    estimate[wp] -= (this->mass[bp] / dist);
                  }
                  wp = bucketList[wp];
                }
              }
            }
          }

          // Do entire trailing row that has not been processed in this plane
          wi = bi;
          for (wj = bj + 1; wj <= last[1]; wj++) {
            for (wk = first[2]; wk <= last[2]; wk++) {
              wp = buckets[wi][wj][wk];
              while (wp != -1) {
                xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
                ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
                zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
                dist = sqrt((xdist*xdist)+(ydist*ydist)+(zdist*zdist)+rsm2);
                if (dist != 0) {
                  estimate[bp] -= (this->mass[wp] / dist);
                  estimate[wp] -= (this->mass[bp] / dist);
                }
                wp = bucketList[wp];
              }
            }
          }

          // Do bucket for right hand neighbor
          wi = bi;
          wj = bj;
          for (wk = bk + 1; wk <= last[2]; wk++) {
            wp = buckets[wi][wj][wk];
            while (wp != -1) {
              xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
              ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
              zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
              dist = sqrt((xdist*xdist)+(ydist*ydist)+(zdist*zdist)+rsm2);
              if (dist != 0.0) {
                estimate[bp] -= (this->mass[wp] / dist);
                estimate[wp] -= (this->mass[bp] / dist);
              }
              wp = bucketList[wp];
            }
          }
          bp = bucketList[bp];
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate the estimated values to particles in 26 immediate neighbors
// Actual values are calculated within the boundary for safety and
// an estimation to the remaining points using the nearest point in the
// neighbor outside of the boundary
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::aStarEstimatedNeighborPart(
                        ChainingMesh* haloChain,
                        int* minActual,
                        int* maxActual,
                        int* refineLevel,
                        POSVEL_T* estimate,
                        POSVEL_T boundarySize)
{
  POSVEL_T rsm2 = this->rSmooth*this->rSmooth;

  // Walking window extents and size
  int bp, bi, bj, bk;
  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T minBound[DIMENSION], maxBound[DIMENSION];
  POSVEL_T xNear = 0.0;
  POSVEL_T yNear = 0.0;
  POSVEL_T zNear = 0.0;
  POSVEL_T xdist, ydist, zdist, dist;

  // Get chaining mesh information
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();
  POSVEL_T* minRange = haloChain->getMinRange();
  POSVEL_T chainSize = haloChain->getChainSize();

  // Calculate estimates for all buckets not in the center
  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        if ((bucketCount[bi][bj][bk] > 0) &&
            ((bi < minActual[0] || bi > maxActual[0]) ||
             (bj < minActual[1] || bj > maxActual[1]) ||
             (bk < minActual[2] || bk > maxActual[2]))) {

          // Set a window around this bucket for calculating estimates
          first[0] = bi - 1;    last[0] = bi + 1;
          first[1] = bj - 1;    last[1] = bj + 1;
          first[2] = bk - 1;    last[2] = bk + 1;

          // Calculate the bounding box around the current bucket
          minBound[0] = minRange[0] + (bi * chainSize) - boundarySize;
          maxBound[0] = minRange[0] + ((bi + 1) * chainSize) + boundarySize;
          minBound[1] = minRange[1] + (bj * chainSize) - boundarySize;
          maxBound[1] = minRange[1] + ((bj + 1) * chainSize) + boundarySize;
          minBound[2] = minRange[2] + (bk * chainSize) - boundarySize;
          maxBound[2] = minRange[2] + ((bk + 1) * chainSize) + boundarySize;

          for (int dim = 0; dim < DIMENSION; dim++) {
            if (first[dim] < 0) {
              first[dim] = 0;
              minBound[dim] = 0.0;
            }
            if (last[dim] >= meshSize[dim]) {
              last[dim] = meshSize[dim] - 1;
              maxBound[dim] = (meshSize[dim] - 1) * chainSize;
            }
          }

          // Calculate actual and estimated for every particle in this bucket
          bp = buckets[bi][bj][bk];
          while (bp != -1) {

            // Since it is not fully calculated refinement level is 0
            refineLevel[bp] = 0;

            // Process all neighbor buckets of this one
            for (wi = first[0]; wi <= last[0]; wi++) {
              for (wj = first[1]; wj <= last[1]; wj++) {
                for (wk = first[2]; wk <= last[2]; wk++) {

                  // If bucket has particles, and is not within the region which
                  // calculates actual neighbor values
                  if ((bucketCount[wi][wj][wk] > 0) &&
                      ((wi > maxActual[0] || wi < minActual[0]) ||
                       (wj > maxActual[1] || wj < minActual[1]) ||
                       (wk > maxActual[2] || wk < minActual[2])) &&
                      (wi != bi || wj != bj || wk != bk)) {

                    // What is the nearest point between buckets
                    if (wi < bi)  xNear = minBound[0];
                    if (wi == bi) xNear = (minBound[0] + maxBound[0]) / 2.0f;
                    if (wi > bi)  xNear = maxBound[0];
                    if (wj < bj)  yNear = minBound[1];
                    if (wj == bj) yNear = (minBound[1] + maxBound[1]) / 2.0f;
                    if (wj > bj)  yNear = maxBound[1];
                    if (wk < bk)  zNear = minBound[2];
                    if (wk == bk) zNear = (minBound[2] + maxBound[2]) / 2.0f;
                    if (wk > bk)  zNear = maxBound[2];

                    wp = buckets[wi][wj][wk];
                    int estimatedParticleCount = 0;
                    while (wp != -1) {
                      if (this->xx[wp] > minBound[0] &&
                          this->xx[wp] < maxBound[0] &&
                          this->yy[wp] > minBound[1] &&
                          this->yy[wp] < maxBound[1] &&
                          this->zz[wp] > minBound[2] &&
                          this->zz[wp] < maxBound[2]) {

                        // Is the window particle within the boundary condition
                        // Calculate actual potential
                        xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
                        ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
                        zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
                        dist = sqrt(xdist*xdist+ydist*ydist+zdist*zdist+rsm2);
                        if (dist != 0.0) {
                          estimate[bp] -= (this->mass[wp] / dist);
                        }
                      } else {
                        // Count to create estimated potential
                        estimatedParticleCount++;
                      }
                      wp = bucketList[wp];
                    }

                    // Find nearest corner or location to this bucket
                    // Calculate estimated value for the part of the bucket
                    xdist = (POSVEL_T)fabs(this->xx[bp] - xNear);
                    ydist = (POSVEL_T)fabs(this->yy[bp] - yNear);
                    zdist = (POSVEL_T)fabs(this->zz[bp] - zNear);
                    dist = sqrt((xdist*xdist)+(ydist*ydist)+(zdist*zdist)+rsm2);
                    if (dist != 0) {
                      estimate[bp] -=
                        ((this->mass[bp] / dist) * estimatedParticleCount);
                    }
                  }
                }
              }
            }
            bp = bucketList[bp];
          }
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Add in an estimation for all buckets outside of the immediate 27 neighbors
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::aStarEstimatedPart(
                        ChainingMesh* haloChain,
                        POSVEL_T* estimate)
{
  POSVEL_T rsm2 = this->rSmooth*this->rSmooth;

  // Walking window extents and size
  int bp, bi, bj, bk;
  int wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T xdist, ydist, zdist, dist;
  POSVEL_T xNear, yNear, zNear;

  // Get chaining mesh information
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();
  POSVEL_T chainSize = haloChain->getChainSize();
  POSVEL_T* minRange = haloChain->getMinRange();

  for (bi = 0; bi < meshSize[0]; bi++) {
    for (bj = 0; bj < meshSize[1]; bj++) {
      for (bk = 0; bk < meshSize[2]; bk++) {

        // Set a window around this bucket for calculating actual potentials
        first[0] = bi - 1;    last[0] = bi + 1;
        first[1] = bj - 1;    last[1] = bj + 1;
        first[2] = bk - 1;    last[2] = bk + 1;
        for (int dim = 0; dim < DIMENSION; dim++) {
          if (first[dim] < 0)
            first[dim] = 0;
          if (last[dim] >= meshSize[dim])
            last[dim] = meshSize[dim] - 1;
        }

        for (wi = 0; wi < meshSize[0]; wi++) {
          for (wj = 0; wj < meshSize[1]; wj++) {
            for (wk = 0; wk < meshSize[2]; wk++) {

              // Exclude the buckets for which actual values were calculated
              if ((wi < first[0] || wi > last[0] ||
                   wj < first[1] || wj > last[1] ||
                   wk < first[2] || wk > last[2]) &&
                  (bucketCount[wi][wj][wk] > 0)) {

                // Nearest corner of the compared bucket to this particle
                bp = buckets[bi][bj][bk];
                xNear = minRange[0] + (wi * chainSize);
                yNear = minRange[1] + (wj * chainSize);
                zNear = minRange[2] + (wk * chainSize);
                if (bp >= 0 && bp < this->particleCount)
                  {
                  if (this->xx[bp] > xNear)
                    xNear += chainSize;
                  if (this->yy[bp] > yNear)
                    yNear += chainSize;
                  if (this->zz[bp] > zNear)
                    zNear += chainSize;
                  }

                // Iterate on all particles in the bucket doing the estimate
                // to the near corner of the other buckets
                while (bp != -1) {
                  xdist = fabs(this->xx[bp] - xNear);
                  ydist = fabs(this->yy[bp] - yNear);
                  zdist = fabs(this->zz[bp] - zNear);
                  dist = sqrt((xdist*xdist)+(ydist*ydist)+(zdist*zdist)+rsm2);
                  if (dist != 0) {
                    estimate[bp] -=
                      ((this->mass[bp] / dist) * bucketCount[wi][wj][wk]);
                  }
                  bp = bucketList[bp];
                }
              }
            }
          }
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Refine the estimate for the particle in the halo with window delta
// given the buckets in the chaining mesh, relative locations of particles
// in this halo, the index of this halo, and the bucket it is in
// The newly refined estimate is updated.
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::refineAStarLevel_1(
                        ChainingMesh* haloChain,
                        int bi,
                        int bj,
                        int bk,
                        int* minActual,
                        int* maxActual,
                        int bp,
                        POSVEL_T* estimate,
                        POSVEL_T boundarySize)
{
  POSVEL_T rsm2 = this->rSmooth*this->rSmooth;

  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  POSVEL_T xdist, ydist, zdist, dist;
  POSVEL_T xNear = 0.0;
  POSVEL_T yNear = 0.0;
  POSVEL_T zNear = 0.0;
  POSVEL_T minBound[DIMENSION], maxBound[DIMENSION];

  // Get chaining mesh information
  POSVEL_T chainSize = haloChain->getChainSize();
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();
  POSVEL_T* minRange = haloChain->getMinRange();

  // Going out window delta in all directions
  // Subtract the estimate from the current value
  // Add the new values
  first[0] = bi - 1;   last[0] = bi + 1;
  first[1] = bj - 1;   last[1] = bj + 1;
  first[2] = bk - 1;   last[2] = bk + 1;

  // Calculate the bounding box around the current bucket
  minBound[0] = minRange[0] + (bi * chainSize) - boundarySize;
  maxBound[0] = minRange[0] + ((bi + 1) * chainSize) + boundarySize;
  minBound[1] = minRange[1] + (bj * chainSize) - boundarySize;
  maxBound[1] = minRange[1] + ((bj + 1) * chainSize) + boundarySize;
  minBound[2] = minRange[2] + (bk * chainSize) - boundarySize;
  maxBound[2] = minRange[2] + ((bk + 1) * chainSize) + boundarySize;

  for (int dim = 0; dim < DIMENSION; dim++) {
    if (first[dim] < 0) {
      first[dim] = 0;
      minBound[dim] = 0.0;
    }
    if (last[dim] >= meshSize[dim]) {
      last[dim] = meshSize[dim] - 1;
      maxBound[dim] = meshSize[dim] * chainSize;
    }
  }

  for (wi = first[0]; wi <= last[0]; wi++) {
    for (wj = first[1]; wj <= last[1]; wj++) {
      for (wk = first[2]; wk <= last[2]; wk++) {

        // If bucket has particles, and is not within the region which
        // calculates actual neighbor values (because if it is, it would
        // have already calculated actuals for this bucket) and if it is
        // not this bucket which already had the n^2 algorithm run
        if ((bucketCount[wi][wj][wk] > 0) &&
            ((wi > maxActual[0] || wi < minActual[0]) ||
             (wj > maxActual[1] || wj < minActual[1]) ||
             (wk > maxActual[2] || wk < minActual[2])) &&
            (wi != bi || wj != bj || wk != bk)) {


          // What is the nearest point between buckets
          if (wi < bi)  xNear = minBound[0];
          if (wi == bi) xNear = (minBound[0] + maxBound[0]) / 2.0;
          if (wi > bi)  xNear = maxBound[0];
          if (wj < bj)  yNear = minBound[1];
          if (wj == bj) yNear = (minBound[1] + maxBound[1]) / 2.0;
          if (wj > bj)  yNear = maxBound[1];
          if (wk < bk)  zNear = minBound[2];
          if (wk == bk) zNear = (minBound[2] + maxBound[2]) / 2.0;
          if (wk > bk)  zNear = maxBound[2];

          wp = buckets[wi][wj][wk];
          int estimatedParticleCount = 0;
          while (wp != -1) {

            // If inside the boundary around the bucket ignore because
            // actual potential was already calculated in initialPhase
            if (
              (this->xx[wp] <= minBound[0] || this->xx[wp] >= maxBound[0]) ||
              (this->yy[wp] <= minBound[1] || this->yy[wp] >= maxBound[1]) ||
              (this->zz[wp] <= minBound[2] || this->zz[wp] >= maxBound[2])) {

              // Count to create estimated potential which is added
              estimatedParticleCount++;

              // Calculate actual potential
              xdist = (POSVEL_T)fabs(this->xx[bp] - this->xx[wp]);
              ydist = (POSVEL_T)fabs(this->yy[bp] - this->yy[wp]);
              zdist = (POSVEL_T)fabs(this->zz[bp] - this->zz[wp]);
              dist = sqrt(xdist*xdist + ydist*ydist + zdist*zdist + rsm2);
              if (dist != 0.0) {
                estimate[bp] -= (this->mass[wp] / dist);
              }
            }
            wp = bucketList[wp];
          }

          // Find nearest corner or location to this bucket
          // Calculate estimated value for the part of the bucket
          xdist = (POSVEL_T)fabs(this->xx[bp] - xNear);
          ydist = (POSVEL_T)fabs(this->yy[bp] - yNear);
          zdist = (POSVEL_T)fabs(this->zz[bp] - zNear);
          dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist) + rsm2);
          if (dist != 0) {
            estimate[bp] += ((this->mass[bp] / dist) * estimatedParticleCount);
          }
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Refine the estimate for the particle in the halo with window delta
// given the buckets in the chaining mesh, relative locations of particles
// in this halo, the index of this halo, and the bucket it is in
// The newly refined estimate is updated.
//
/////////////////////////////////////////////////////////////////////////

void HaloCenterFinder::refineAStarLevel_N(
                        ChainingMesh* haloChain,
                        int bi,
                        int bj,
                        int bk,
                        int bp,
                        POSVEL_T* estimate,
                        int winDelta)
{
  POSVEL_T rsm2 = this->rSmooth*this->rSmooth;

  int wp, wi, wj, wk;
  int first[DIMENSION], last[DIMENSION];
  int oldDelta = winDelta - 1;
  POSVEL_T xdist, ydist, zdist, dist;
  POSVEL_T xNear, yNear, zNear;

  // Get chaining mesh information
  POSVEL_T chainSize = haloChain->getChainSize();
  int*** bucketCount = haloChain->getBucketCount();
  int*** buckets = haloChain->getBuckets();
  int* bucketList = haloChain->getBucketList();
  int* meshSize = haloChain->getMeshSize();
  POSVEL_T* minRange = haloChain->getMinRange();

  // Going out window delta in all directions
  // Subtract the estimate from the current value
  // Add the new values
  first[0] = bi - winDelta;   last[0] = bi + winDelta;
  first[1] = bj - winDelta;   last[1] = bj + winDelta;
  first[2] = bk - winDelta;   last[2] = bk + winDelta;
  for (int dim = 0; dim < DIMENSION; dim++) {
    if (first[dim] < 0)
      first[dim] = 0;
    if (last[dim] >= meshSize[dim])
      last[dim] = meshSize[dim] - 1;
  }

  // Walk the new delta window
  // Exclude buckets which already contributed actual values
  // For other buckets add the estimate and subtract the actual
  for (wi = first[0]; wi <= last[0]; wi++) {
    for (wj = first[1]; wj <= last[1]; wj++) {
      for (wk = first[2]; wk <= last[2]; wk++) {

        if ((wi < (bi - oldDelta) || wi > (bi + oldDelta) ||
             wj < (bj - oldDelta) || wj > (bj + oldDelta) ||
             wk < (bk - oldDelta) || wk > (bk + oldDelta)) &&
            (bucketCount[wi][wj][wk] > 0)) {

            // Nearest corner of the bucket to contribute new actuals
            xNear = minRange[0] + (wi * chainSize);
            yNear = minRange[1] + (wj * chainSize);
            zNear = minRange[2] + (wk * chainSize);
            if (this->xx[bp] > xNear) xNear += chainSize;
            if (this->yy[bp] > yNear) yNear += chainSize;
            if (this->zz[bp] > zNear) zNear += chainSize;

            // Distance of this particle to the corner gives estimate
            // which was subtracted in initialPhase and now is added back
            xdist = (POSVEL_T)fabs(this->xx[bp] - xNear);
            ydist = (POSVEL_T)fabs(this->yy[bp] - yNear);
            zdist = (POSVEL_T)fabs(this->zz[bp] - zNear);
            dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist) + rsm2);
            if (dist != 0) {
              estimate[bp] +=
                ((this->mass[bp] / dist) * bucketCount[wi][wj][wk]);
            }

            // Subtract actual values from the new bucket to this particle
            wp = buckets[wi][wj][wk];
            while (wp != -1) {
              xdist = fabs(this->xx[bp] - this->xx[wp]);
              ydist = fabs(this->yy[bp] - this->yy[wp]);
              zdist = fabs(this->zz[bp] - this->zz[wp]);
              dist = sqrt((xdist*xdist) + (ydist*ydist) + (zdist*zdist) + rsm2);
              if (dist != 0) {
                estimate[bp] -= (this->mass[wp] / dist);
              }
              wp = bucketList[wp];
            }
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Build a chaining mesh from the particles of a single halo
// Used to find most connected and most bound particles for halo center
// Space is allocated for locations of the halo and for a mapping of
// the index within a halo to the index of the particle within the processor
//
/////////////////////////////////////////////////////////////////////////

ChainingMesh* HaloCenterFinder::buildChainingMesh(POSVEL_T chainSize)
{
  // Find the bounding box of this halo
  POSVEL_T* minLoc = new POSVEL_T[DIMENSION];
  POSVEL_T* maxLoc = new POSVEL_T[DIMENSION];
  minLoc[0] = maxLoc[0] = this->xx[0];
  minLoc[1] = maxLoc[1] = this->yy[0];
  minLoc[2] = maxLoc[2] = this->zz[0];

  // Transfer the locations for this halo into separate vectors
  for (int p = 0; p < this->particleCount; p++) {

    if (minLoc[0] > this->xx[p]) minLoc[0] = this->xx[p];
    if (maxLoc[0] < this->xx[p]) maxLoc[0] = this->xx[p];
    if (minLoc[1] > this->yy[p]) minLoc[1] = this->yy[p];
    if (maxLoc[1] < this->yy[p]) maxLoc[1] = this->yy[p];
    if (minLoc[2] > this->zz[p]) minLoc[2] = this->zz[p];
    if (maxLoc[2] < this->zz[p]) maxLoc[2] = this->zz[p];
  }

  // Want chaining mesh greater than 2 in any dimension
  bool tooSmall = true;
  while (tooSmall == true) {
    tooSmall = false;
    for (int dim = 0; dim < DIMENSION; dim++) {
      if (((maxLoc[dim] - minLoc[dim]) / chainSize) < 3.0)
        tooSmall = true;
    }
    if (tooSmall == true) {
      chainSize /= 2.0;
    }
  }

  // Build the chaining mesh
  ChainingMesh* haloChain = new ChainingMesh(minLoc, maxLoc, chainSize,
                        this->particleCount,
                        this->xx, this->yy, this->zz);
  delete [] minLoc;
  delete [] maxLoc;

  return haloChain;
}

}
