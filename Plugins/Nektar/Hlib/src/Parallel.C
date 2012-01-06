/**************************************************************************/
//                                                                        //
//   Author:    Spencer Sherwin                                           //
//   Design:    T.Warburton && S.Sherwin                                  //
//                                                                        //
//   Date  :    19/1/99                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission of the author.                     //
//                                                                        //
//  This file contains a list of dummy functions for the parallel code    //
//  which are need to be defined for the serial version to execute        //
//  normally without the addition of any extra file. For the parallel     //
//  version to work the same functions are redefined in comm.C in the     //
//  source level code. To incorporate a new message passing language the  //
//  routine should be redefined in comm.C. The gs library should also     //
//  have precedence over libhybrid                                        //
/**************************************************************************/

#include <math.h>
#include <veclib.h>
#include <string.h>
#include "hotel.h"


/* dummy functions */
void init_comm (int *argc, char **argv[]){
  pllinfo[get_active_handle()].nprocs = 1;
  pllinfo[get_active_handle()].procid = 0;
}

#ifndef PARALLEL
int numnodes (void){
  return 1;
}

int mynode (void){
  return 0;
}
void gdmax (double *x, int n, double *work){
  error_msg(No message passing routines linked);
}

void gimax (int *x, int n, int *work){
  error_msg(No message passing routines linked);
}

void gdsum (double *x, int n, double *work){
  error_msg(No message passing routines linked);
}

void gisum (int *x, int n, int *work){
  error_msg(No message passing routines linked);
}

#endif

void BCreduce(double *bc, Bsystem *Ubsys){
  error_msg(No message passing routines linked);
}

void GatherBlockMatrices(Element *Mesh, Bsystem *B){
  error_msg(No message passing routines linked);
}

void Set_Comm_GatherBlockMatrices(Element *Mesh, Bsystem *B){
  error_msg(No message passing routines linked);
}

void parallel_gather(double *w, Bsystem *B){
  error_msg(No message passing routines linked);
}

void exit_comm(void){
  exit(0);

}

void default_partitioner(Element_List *EL, int *partition){
  register int i,j;
  int nel = EL->nel;
  int start, stop;
  int active_handle = get_active_handle();

  ROOTONLY
    fprintf(stderr,"Partitioner         : basic internal algorithm\n");

  /* at present just assign each element with skipped storage */

  stop  = ((pllinfo[active_handle].nprocs-pllinfo[active_handle].procid-1)+1)*nel/pllinfo[active_handle].nprocs;
  start = (pllinfo[active_handle].nprocs-pllinfo[active_handle].procid-1)*nel/pllinfo[active_handle].nprocs;

  pllinfo[active_handle].nloop = stop - start;
  pllinfo[active_handle].eloop = ivector(0,pllinfo[active_handle].nloop-1);

  for(i = 0; i < pllinfo[active_handle].nloop; ++i)
    pllinfo[active_handle].eloop[i] = start + i;

  for(i = 0; i < pllinfo[active_handle].nprocs; ++i)
    for(j = i*nel/pllinfo[active_handle].nprocs; j < (i+1)*nel/pllinfo[active_handle].nprocs; ++j)
      partition[j] = i;

}
