#include "nektar.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <veclib.h>
#include "Quad.h"
#include "Tri.h"
//#include "Nodal_Quad.h"
#include "Tet.h"
#include "Hex.h"
#include "Prism.h"
#include "Pyr.h"



void C_Free::allocate_memory(){
  int itmp;

  vl = 0.0; /* default values */
  wl = 0.0;
  vg = 0.0; /* default values */
  wg = 0.0;

  /* allocate memory for coordinates */
  /*****************************************/
  coordX = dmatrix(0,nvc-1,0,nwc-1);
  coordY = dmatrix(0,nvc-1,0,nwc-1);
  coordZ = dmatrix(0,nvc-1,0,nwc-1);
  /*****************************************/

  /* allocate memory for first derivatives */
  dx_dv = dmatrix(0,nvc-1,0,nwc-1);
  dy_dv = dmatrix(0,nvc-1,0,nwc-1);
  dz_dv = dmatrix(0,nvc-1,0,nwc-1);
  dx_dw = dmatrix(0,nvc-1,0,nwc-1);
  dy_dw = dmatrix(0,nvc-1,0,nwc-1);
  dz_dw = dmatrix(0,nvc-1,0,nwc-1);
  /*****************************************/

  /* allocate memory for second derivatives */
  /*****************************************/
  dx_dvdw = dmatrix(0,nvc-1,0,nwc-1);
  dy_dvdw = dmatrix(0,nvc-1,0,nwc-1);
  dz_dvdw = dmatrix(0,nvc-1,0,nwc-1);

  /*****************************************/

  /* C - standard interpolating  matrix  CT = transpose(C) */
  /*****************************************/
  C  = dmatrix(0,3,0,3);
  CT = dmatrix(0,3,0,3);

  C[0][0] =  1.0; C[0][1] =  0.0; C[0][2] =  0.0; C[0][3] =  0.0;
  C[1][0] =  0.0; C[1][1] =  0.0; C[1][2] =  1.0; C[1][3] =  0.0;
  C[2][0] = -3.0; C[2][1] =  3.0; C[2][2] = -2.0; C[2][3] = -1.0;
  C[3][0] =  2.0; C[3][1] = -2.0; C[3][2] =  1.0; C[3][3] =  1.0;

  for (int jtmp = 0; jtmp < 4; jtmp++){
    for (itmp = 0; itmp < 4; itmp++)
      CT[itmp][jtmp] = C[jtmp][itmp];
  }

  /*****************************************/
  V = dvector(0,3);
  W = dvector(0,3);

  for (itmp = 0; itmp < 4; itmp++)
    V[itmp] = W[itmp] = 0.0;

  M = dmatrix(0,3,0,3);

  // index set to the cell number that does not exists - do not modify that !!!
  Icell = nvc;
  Jcell = nwc;

  CMCTx = dmatrix(0,3,0,3);
  CMCTy = dmatrix(0,3,0,3);
  CMCTz = dmatrix(0,3,0,3);

}


void C_Free::load_from_grdFile(int index_DB, FILE *pFile){

  /* read and does not save databas with index < index_DB */
  /* read and save databas with index = index_DB */



  int i,j,k,itmp,NDB;
  double x,y,z;
  int *nv,*nw;
  char *p;
  char string[BUFSIZ];

  rewind(pFile);

  fscanf(pFile,"%d \n",&NDB);
  nv = new int[NDB];
  nw = new int[NDB];

  for (itmp = 0; itmp < NDB; itmp++){
    p = fgets (string, BUFSIZ, pFile);
    sscanf(p,"%d %d %d \n",&i,&j,&k);
    nw[itmp] = i;
    nv[itmp] = j;
  }


  for (itmp = 0; itmp < index_DB; itmp++){
    for (i = 0; i < nv[itmp]; i++){
      for (j = 0; j < nw[itmp]; j++)
        fscanf(pFile,"%lf ",&x);
    }
    for (i = 0; i < nv[itmp]; i++){
      for (j = 0; j < nw[itmp]; j++)
        fscanf(pFile,"%lf ",&y);
    }
    for (i = 0; i < nv[itmp]; i++){
      for (j = 0; j < nw[itmp]; j++)
        fscanf(pFile,"%lf ",&z);
    }
  }

  /*read and save*/


  for (i = 0; i < nv[index_DB]; i++){
    for (j = 0; j < nw[index_DB]; j++){
        fscanf(pFile,"%lf ",&x);
        coordX[i][j] = x;
    }
  }
  for (i = 0; i < nv[index_DB]; i++){
    for (j = 0; j < nw[index_DB]; j++){
        fscanf(pFile,"%lf ",&y);
        coordY[i][j] = y;
    }
  }
  for (i = 0; i < nv[index_DB]; i++){
    for (j = 0; j < nw[index_DB]; j++){
        fscanf(pFile,"%lf ",&z);
        coordZ[i][j] = z;
    }
  }

  rewind(pFile);


  delete[] nv;
  delete[] nw;

}

void C_Free::set_1der(){

  int i,j;
  double delta = 1.0, inv_delta, inv_2delta, inv_12delta;
  inv_delta = 1.0/delta;
  inv_2delta = 1.0/(2.0*delta);
  inv_12delta = 1.0/(12.0*delta);

  for (i = 0; i < nvc; i++){

    j = 0;
    switch(nwc){
    case 2:   //first order derivatives
      //default:
      dx_dv[i][j] = (-coordX[i][0]+coordX[i][1])*inv_delta;
      dy_dv[i][j] = (-coordY[i][0]+coordY[i][1])*inv_delta;
      dz_dv[i][j] = (-coordZ[i][0]+coordZ[i][1])*inv_delta;
      break;
      //case 3: //second order derivatives
      //case 4:
    default:
      if (Vperiodic == 0){
        dx_dv[i][j] = (-3.0*coordX[i][0]+4.0*coordX[i][1]-coordX[i][2])*inv_2delta;
        dy_dv[i][j] = (-3.0*coordY[i][0]+4.0*coordY[i][1]-coordY[i][2])*inv_2delta;
        dz_dv[i][j] = (-3.0*coordZ[i][0]+4.0*coordZ[i][1]-coordZ[i][2])*inv_2delta;
      }
      else{
        //dx_dv[i][j] = (-coordX[i][j+2]+8.0*coordX[i][j+1]-8.0*coordX[i][nwc-2]+coordX[i][nwc-3])*inv_12delta;
        //dy_dv[i][j] = (-coordY[i][j+2]+8.0*coordY[i][j+1]-8.0*coordY[i][nwc-2]+coordY[i][nwc-3])*inv_12delta;
        //dz_dv[i][j] = (-coordZ[i][j+2]+8.0*coordZ[i][j+1]-8.0*coordZ[i][nwc-2]+coordZ[i][nwc-3])*inv_12delta;

        dx_dv[i][j] = (coordX[i][1]-coordX[i][nwc-2])*inv_2delta;
        dy_dv[i][j] = (coordY[i][1]-coordY[i][nwc-2])*inv_2delta;
        dz_dv[i][j] = (coordZ[i][1]-coordZ[i][nwc-2])*inv_2delta;
      }
      break;
      // default: // 4th order
      // dx_dv[i][j] = (-25.0*coordX[i][0]+48.0*coordX[i][1]-36.0*coordX[i][2]+16.0*coordX[i][3]-3.0*coordX[i][4])*inv_12delta;
      //dy_dv[i][j] = (-25.0*coordY[i][0]+48.0*coordY[i][1]-36.0*coordY[i][2]+16.0*coordY[i][3]-3.0*coordY[i][4])*inv_12delta;
      //dz_dv[i][j] = (-25.0*coordZ[i][0]+48.0*coordZ[i][1]-36.0*coordZ[i][2]+16.0*coordZ[i][3]-3.0*coordZ[i][4])*inv_12delta;
    }


    j = 1;
    switch(nwc){
    case 2: //first order derivatives
       dx_dv[i][j] = (coordX[i][1]-coordX[i][0])*inv_delta;
       dy_dv[i][j] = (coordY[i][1]-coordY[i][0])*inv_delta;
       dz_dv[i][j] = (coordZ[i][1]-coordZ[i][0])*inv_delta;
       break;
    case 3: //second order derivatives
    case 4:
    default:
      if (Vperiodic == 0){
         dx_dv[i][j] = (coordX[i][2]-coordX[i][0])*inv_2delta;
         dy_dv[i][j] = (coordY[i][2]-coordY[i][0])*inv_2delta;
         dz_dv[i][j] = (coordZ[i][2]-coordZ[i][0])*inv_2delta;
      }
      else{
     dx_dv[i][j] = (-coordX[i][j+2]+8.0*coordX[i][j+1]-8.0*coordX[i][j-1]+coordX[i][nwc-2])*inv_12delta;
     dy_dv[i][j] = (-coordY[i][j+2]+8.0*coordY[i][j+1]-8.0*coordY[i][j-1]+coordY[i][nwc-2])*inv_12delta;
     dz_dv[i][j] = (-coordZ[i][j+2]+8.0*coordZ[i][j+1]-8.0*coordZ[i][j-1]+coordZ[i][nwc-2])*inv_12delta;
      }
       break;
       //default: // 4th order
       //dx_dv[i][j] = (-18.0*coordX[i][j-1]-5.0*coordX[i][j]+18.0*coordX[i][j+1]+9.0*coordX[i][j+2]-4.0*coordX[i][j+3])/(42.0*delta);
       //dy_dv[i][j] = (-18.0*coordY[i][j-1]-5.0*coordY[i][j]+18.0*coordY[i][j+1]+9.0*coordY[i][j+2]-4.0*coordY[i][j+3])/(42.0*delta);
       //dz_dv[i][j] = (-18.0*coordZ[i][j-1]-5.0*coordZ[i][j]+18.0*coordZ[i][j+1]+9.0*coordZ[i][j+2]-4.0*coordZ[i][j+3])/(42.0*delta);
    }

    // 4th order   central difference 1st derivatives
    for (j = 2; j < (nwc-2); j++)
     dx_dv[i][j] = (-coordX[i][j+2]+8.0*coordX[i][j+1]-8.0*coordX[i][j-1]+coordX[i][j-2])*inv_12delta;

    for (j = 2; j < (nwc-2); j++)
     dy_dv[i][j] = (-coordY[i][j+2]+8.0*coordY[i][j+1]-8.0*coordY[i][j-1]+coordY[i][j-2])*inv_12delta;

    for (j = 2; j < (nwc-2); j++)
     dz_dv[i][j] = (-coordZ[i][j+2]+8.0*coordZ[i][j+1]-8.0*coordZ[i][j-1]+coordZ[i][j-2])*inv_12delta;


    j = nwc-2;
    switch(nwc){
    case 2:
      break;
    case 3: //second order derivatives
        dx_dv[i][j] = (coordX[i][j+1]-coordX[i][j-1])*inv_2delta;
        dy_dv[i][j] = (coordY[i][j+1]-coordY[i][j-1])*inv_2delta;
        dz_dv[i][j] = (coordZ[i][j+1]-coordZ[i][j-1])*inv_2delta;
  break;
    case 4:
    default:
      if (Vperiodic == 0){
       dx_dv[i][j] = (18.0*coordX[i][j+1]+5.0*coordX[i][j]-18.0*coordX[i][j-1]-9.0*coordX[i][j-2]+4.0*coordX[i][j-3])/(42.0*delta);
       dy_dv[i][j] = (18.0*coordY[i][j+1]+5.0*coordY[i][j]-18.0*coordY[i][j-1]-9.0*coordY[i][j-2]+4.0*coordY[i][j-3])/(42.0*delta);
       dz_dv[i][j] = (18.0*coordZ[i][j+1]+5.0*coordZ[i][j]-18.0*coordZ[i][j-1]-9.0*coordZ[i][j-2]+4.0*coordZ[i][j-3])/(42.0*delta);
      }
      else{
  dx_dv[i][j] = (-coordX[i][1]+8.0*coordX[i][j+1]-8.0*coordX[i][j-1]+coordX[i][j-2])*inv_12delta;
  dy_dv[i][j] = (-coordY[i][1]+8.0*coordY[i][j+1]-8.0*coordY[i][j-1]+coordY[i][j-2])*inv_12delta;
  dz_dv[i][j] = (-coordZ[i][1]+8.0*coordZ[i][j+1]-8.0*coordZ[i][j-1]+coordZ[i][j-2])*inv_12delta;
      }

       break;
       //default: //4th order
       //dx_dv[i][j] = (18.0*coordX[i][j+1]+5.0*coordX[i][j]-18.0*coordX[i][j-1]-9.0*coordX[i][j-2]+4.0*coordX[i][j-3])/(42.0*delta);
       //dy_dv[i][j] = (18.0*coordY[i][j+1]+5.0*coordY[i][j]-18.0*coordY[i][j-1]-9.0*coordY[i][j-2]+4.0*coordY[i][j-3])/(42.0*delta);
       //dz_dv[i][j] = (18.0*coordZ[i][j+1]+5.0*coordZ[i][j]-18.0*coordZ[i][j-1]-9.0*coordZ[i][j-2]+4.0*coordZ[i][j-3])/(42.0*delta);
    }

    j = nwc-1;
    switch(nwc){
    case 2: //first order derivatives
      //default:
       dx_dv[i][j] = (coordX[i][j]-coordX[i][j-1])*inv_delta;
       dy_dv[i][j] = (coordY[i][j]-coordY[i][j-1])*inv_delta;
       dz_dv[i][j] = (coordZ[i][j]-coordZ[i][j-1])*inv_delta;
       break;
       //case 3: //second order derivatives
       //case 4:
    default:
      if (Vperiodic == 0){
        dx_dv[i][j] = (3.0*coordX[i][j]-4.0*coordX[i][j-1]+coordX[i][j-2])*inv_2delta;
        dy_dv[i][j] = (3.0*coordY[i][j]-4.0*coordY[i][j-1]+coordY[i][j-2])*inv_2delta;
        dz_dv[i][j] = (3.0*coordZ[i][j]-4.0*coordZ[i][j-1]+coordZ[i][j-2])*inv_2delta;
      }
      else{
  //dx_dv[i][j] = (-coordX[i][2]+8.0*coordX[i][1]-8.0*coordX[i][j-1]+coordX[i][j-2])*inv_12delta;
        // dy_dv[i][j] = (-coordY[i][2]+8.0*coordY[i][1]-8.0*coordY[i][j-1]+coordY[i][j-2])*inv_12delta;
        // dz_dv[i][j] = (-coordZ[i][2]+8.0*coordZ[i][1]-8.0*coordZ[i][j-1]+coordZ[i][j-2])*inv_12delta;

   dx_dv[i][j] = (coordX[i][1]-coordX[i][j-1])*inv_2delta;
   dy_dv[i][j] = (coordY[i][1]-coordY[i][j-1])*inv_2delta;
   dz_dv[i][j] = (coordZ[i][1]-coordZ[i][j-1])*inv_2delta;

      }
       //break;
      //default: //4th order
      //dx_dv[i][j] = (25.0*coordX[i][j]-48.0*coordX[i][j-1]+36.0*coordX[i][j-2]-16.0*coordX[i][j-3]+3.0*coordX[i][j-4])*inv_12delta;
      //dy_dv[i][j] = (25.0*coordY[i][j]-48.0*coordY[i][j-1]+36.0*coordY[i][j-2]-16.0*coordY[i][j-3]+3.0*coordY[i][j-4])*inv_12delta;
      //dz_dv[i][j] = (25.0*coordZ[i][j]-48.0*coordZ[i][j-1]+36.0*coordZ[i][j-2]-16.0*coordZ[i][j-3]+3.0*coordZ[i][j-4])*inv_12delta;
    }

  }
    for (j = 0; j < nwc; j++){

    i = 0;
    switch(nvc){
    case 2:
      dx_dw[i][j] = (coordX[1][j]-coordX[0][j])*inv_delta;
      dy_dw[i][j] = (coordY[1][j]-coordY[0][j])*inv_delta;
      dz_dw[i][j] = (coordZ[1][j]-coordZ[0][j])*inv_delta;
      break;
    case 3:
    case 4:
    default:
      dx_dw[i][j] = (-3.0*coordX[0][j]+4.0*coordX[1][j]-coordX[2][j])*inv_2delta;
      dy_dw[i][j] = (-3.0*coordY[0][j]+4.0*coordY[1][j]-coordY[2][j])*inv_2delta;
      dz_dw[i][j] = (-3.0*coordZ[0][j]+4.0*coordZ[1][j]-coordZ[2][j])*inv_2delta;
      break;
      /*
    default:
      dx_dw[i][j] = (-25.0*coordX[0][j]+48.0*coordX[1][j]-36.0*coordX[2][j]+16.0*coordX[3][j]-3.0*coordX[4][j])*inv_12delta;
      dy_dw[i][j] = (-25.0*coordY[0][j]+48.0*coordY[1][j]-36.0*coordY[2][j]+16.0*coordY[3][j]-3.0*coordY[4][j])*inv_12delta;
      dz_dw[i][j] = (-25.0*coordZ[0][j]+48.0*coordZ[1][j]-36.0*coordZ[2][j]+16.0*coordZ[3][j]-3.0*coordZ[4][j])*inv_12delta;
      */
    }
    i = 1;
    switch(nvc){
    case 2:
      dx_dw[i][j] = (coordX[1][j]-coordX[0][j])*inv_delta;
      dy_dw[i][j] = (coordY[1][j]-coordY[0][j])*inv_delta;
      dz_dw[i][j] = (coordZ[1][j]-coordZ[0][j])*inv_delta;
      break;
    case 3:
    case 4:
    default:
      dx_dw[i][j] = (coordX[2][j]-coordX[0][j])*inv_2delta;
      dy_dw[i][j] = (coordY[2][j]-coordY[0][j])*inv_2delta;
      dz_dw[i][j] = (coordZ[2][j]-coordZ[0][j])*inv_2delta;
      break;
      //default:
      //dx_dw[i][j] = (-18.0*coordX[i-1][j]-5.0*coordX[i][j]+18.0*coordX[i+1][j]+9.0*coordX[i+2][j]-4.0*coordX[i+3][j])/(42.0*delta);
      //dy_dw[i][j] = (-18.0*coordY[i-1][j]-5.0*coordY[i][j]+18.0*coordY[i+1][j]+9.0*coordY[i+2][j]-4.0*coordY[i+3][j])/(42.0*delta);
      //dz_dw[i][j] = (-18.0*coordZ[i-1][j]-5.0*coordZ[i][j]+18.0*coordZ[i+1][j]+9.0*coordZ[i+2][j]-4.0*coordZ[i+3][j])/(42.0*delta);
    }

     for (i = 2; i < (nvc-2); i++)
      dx_dw[i][j] = (-coordX[i+2][j]+8.0*coordX[i+1][j]-8.0*coordX[i-1][j]+coordX[i-2][j])*inv_12delta;

    for (i = 2; i < (nvc-2); i++)
      dy_dw[i][j] = (-coordY[i+2][j]+8.0*coordY[i+1][j]-8.0*coordY[i-1][j]+coordY[i-2][j])*inv_12delta;

    for (i = 2; i < (nvc-2); i++)
      dz_dw[i][j] = (-coordZ[i+2][j]+8.0*coordZ[i+1][j]-8.0*coordZ[i-1][j]+coordZ[i-2][j])*inv_12delta;


    i = nvc-1;
    switch(nvc){
    case 2:
      break;
    case 3:
    case 4:
    default:
      //dx_dw[i][j] = (coordX[i][j]-coordX[i-1][j])*inv_delta;
      //dy_dw[i][j] = (coordY[i][j]-coordY[i-1][j])*inv_delta;
      //dz_dw[i][j] = (coordZ[i][j]-coordZ[i-1][j])*inv_delta;

      dx_dw[i][j] = (3.0*coordX[i][j]-4.0*coordX[i-1][j]+coordX[i-2][j])*inv_2delta;
      dy_dw[i][j] = (3.0*coordY[i][j]-4.0*coordY[i-1][j]+coordY[i-2][j])*inv_2delta;
      dz_dw[i][j] = (3.0*coordZ[i][j]-4.0*coordZ[i-1][j]+coordZ[i-2][j])*inv_2delta;
      break;
      /*
    default:
      dx_dw[i][j] = (25.0*coordX[i][j]-48.0*coordX[i-1][j]+36.0*coordX[i-2][j]-16.0*coordX[i-3][j]+3.0*coordX[i-4][j])*inv_12delta;
      dy_dw[i][j] = (25.0*coordY[i][j]-48.0*coordY[i-1][j]+36.0*coordY[i-2][j]-16.0*coordY[i-3][j]+3.0*coordY[i-4][j])*inv_12delta;
      dz_dw[i][j] = (25.0*coordZ[i][j]-48.0*coordZ[i-1][j]+36.0*coordZ[i-2][j]-16.0*coordZ[i-3][j]+3.0*coordZ[i-4][j])*inv_12delta;
    */
    }


    i = nvc-2;
    switch(nvc){
    case 2:
      break;
    case 3:
    case 4:
    default:
      //dx_dw[i][j] = (coordX[2][j]-coordX[0][j])*inv_2delta;
      //dy_dw[i][j] = (coordY[2][j]-coordY[0][j])*inv_2delta;
      //dz_dw[i][j] = (coordZ[2][j]-coordZ[0][j])*inv_2delta;
      //break;
      //default:
      dx_dw[i][j] = (18.0*coordX[i+1][j]+5.0*coordX[i][j]-18.0*coordX[i-1][j]-9.0*coordX[i-2][j]+4.0*coordX[i-3][j])/(42.0*delta);
      dy_dw[i][j] = (18.0*coordY[i+1][j]+5.0*coordY[i][j]-18.0*coordY[i-1][j]-9.0*coordY[i-2][j]+4.0*coordY[i-3][j])/(42.0*delta);
      dz_dw[i][j] = (18.0*coordZ[i+1][j]+5.0*coordZ[i][j]-18.0*coordZ[i-1][j]-9.0*coordZ[i-2][j]+4.0*coordZ[i-3][j])/(42.0*delta);
    }

  }
}


void C_Free::set_2der(){

  int i,j;
  double delta = 1.0, inv_2delta = 0.5, inv_12delta = 1.0/12.0;

  // compute d^r / [dvdw]   r = x,y,z

  i = 0;
  for (j = 0; j < nwc; j++){
    //dx_dvdw[i][j] = (-25.0*dx_dv[0][j]+48.0*dx_dv[1][j]-36.0*dx_dv[2][j]+16.0*dx_dv[3][j]-3.0*dx_dv[4][j])*inv_12delta;
    //dy_dvdw[i][j] = (-25.0*dy_dv[0][j]+48.0*dy_dv[1][j]-36.0*dy_dv[2][j]+16.0*dy_dv[3][j]-3.0*dy_dv[4][j])*inv_12delta;
    //dz_dvdw[i][j] = (-25.0*dz_dv[0][j]+48.0*dz_dv[1][j]-36.0*dz_dv[2][j]+16.0*dz_dv[3][j]-3.0*dz_dv[4][j])*inv_12delta;

    dx_dvdw[i][j] = (-3.0*dx_dv[0][j]+4.0*dx_dv[1][j]-dx_dv[2][j])*inv_2delta;
    dy_dvdw[i][j] = (-3.0*dy_dv[0][j]+4.0*dy_dv[1][j]-dy_dv[2][j])*inv_2delta;
    dz_dvdw[i][j] = (-3.0*dz_dv[0][j]+4.0*dz_dv[1][j]-dz_dv[2][j])*inv_2delta;
  }

    i = 1;
    for (j = 0; j < nwc; j++){
      dx_dvdw[i][j] = (dx_dv[i+1][j]-dx_dv[i-1][j])*inv_2delta;
      // dx_dvdw[i][j] = (-18.0*dx_dv[i-1][j]-5.0*dx_dv[i][j]+18.0*dx_dv[i+1][j]+9.0*dx_dv[i+2][j]-4.0*dx_dv[i+3][j])/(42.0*delta);
    }

    for (j = 0; j < nwc; j++){
      // dy_dvdw[i][j] = (-18.0*dy_dv[i-1][j]-5.0*dy_dv[i][j]+18.0*dy_dv[i+1][j]+9.0*dy_dv[i+2][j]-4.0*dy_dv[i+3][j])/(42.0*delta);
      dy_dvdw[i][j] = (dy_dv[i+1][j]-dy_dv[i-1][j])*inv_2delta;
    }

    for (j = 0; j < nwc; j++){
      //dz_dvdw[i][j] = (-18.0*dz_dv[i-1][j]-5.0*dz_dv[i][j]+18.0*dz_dv[i+1][j]+9.0*dz_dv[i+2][j]-4.0*dz_dv[i+3][j])/(42.0*delta);
      dz_dvdw[i][j] = (dz_dv[i+1][j]-dz_dv[i-1][j])*inv_2delta;
    }

   for (i = 2; i < (nvc-2); i++){

     for (j = 0; j < nwc; j++)
       dx_dvdw[i][j] = (-dx_dv[i+2][j]+8.0*dx_dv[i+1][j]-8.0*dx_dv[i-1][j]+dx_dv[i-2][j])*inv_12delta;

     for (j = 0; j < nwc; j++)
       dy_dvdw[i][j] = (-dy_dv[i+2][j]+8.0*dy_dv[i+1][j]-8.0*dy_dv[i-1][j]+dy_dv[i-2][j])*inv_12delta;

     for (j = 0; j < nwc; j++)
       dz_dvdw[i][j] = (-dz_dv[i+2][j]+8.0*dz_dv[i+1][j]-8.0*dz_dv[i-1][j]+dz_dv[i-2][j])*inv_12delta;

   }


   i = nvc-2;
   for (j = 0; j < nwc; j++){
     dx_dvdw[i][j] = -(-18.0*dx_dv[i+1][j]-5.0*dx_dv[i][j]+18.0*dx_dv[i-1][j]+9.0*dx_dv[i-2][j]-4.0*dx_dv[i-3][j])/(42.0*delta);
     //dx_dvdw[i][j] = (dx_dv[i+1][j]-dx_dv[i-1][j])*inv_2delta;
   }

   for (j = 0; j < nwc; j++){
     dy_dvdw[i][j] = -(-18.0*dy_dv[i+1][j]-5.0*dy_dv[i][j]+18.0*dy_dv[i-1][j]+9.0*dy_dv[i-2][j]-4.0*dy_dv[i-3][j])/(42.0*delta);
     //dy_dvdw[i][j] = (dy_dv[i+1][j]-dy_dv[i-1][j])*inv_2delta;
   }

   for (j = 0; j < nwc; j++){
     dz_dvdw[i][j] = -(-18.0*dz_dv[i+1][j]-5.0*dz_dv[i][j]+18.0*dz_dv[i-1][j]+9.0*dz_dv[i-2][j]-4.0*dz_dv[i-3][j])/(42.0*delta);
     //dz_dvdw[i][j] = (dz_dv[i+1][j]-dz_dv[i-1][j])*inv_2delta;
   }


  i = nvc-1;
  for (j = 0; j < (nwc-0); j++){
    //dx_dvdw[i][j] = (25.0*dx_dv[i][j]-48.0*dx_dv[i-1][j]+36.0*dx_dv[i-2][j]-16.0*dx_dv[i-3][j]+3.0*dx_dv[i-4][j])*inv_12delta;
    //dy_dvdw[i][j] = (25.0*dy_dv[i][j]-48.0*dy_dv[i-1][j]+36.0*dy_dv[i-2][j]-16.0*dy_dv[i-3][j]+3.0*dy_dv[i-4][j])*inv_12delta;
    //dz_dvdw[i][j] = (25.0*dz_dv[i][j]-48.0*dz_dv[i-1][j]+36.0*dz_dv[i-2][j]-16.0*dz_dv[i-3][j]+3.0*dz_dv[i-4][j])*inv_12delta;

    dx_dvdw[i][j] = (3.0*dx_dv[i][j]-4.0*dx_dv[i-1][j]+dx_dv[i-2][j])*inv_2delta;
    dy_dvdw[i][j] = (3.0*dy_dv[i][j]-4.0*dy_dv[i-1][j]+dy_dv[i-2][j])*inv_2delta;
    dz_dvdw[i][j] = (3.0*dz_dv[i][j]-4.0*dz_dv[i-1][j]+dz_dv[i-2][j])*inv_2delta;
  }
}
