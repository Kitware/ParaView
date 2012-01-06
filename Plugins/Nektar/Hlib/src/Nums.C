/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission of the author.                     //
//                                                                        //
/**************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <polylib.h>
#include "veclib.h"
#include "hotel.h"
#include "nekstruct.h"

#include <stdio.h>

#define RERR -100

static int Tri_vnum  [][2] = {{0,1},{1,2},{0,2}};
static int Quad_vnum  [][2] = {{0,1},{1,2},{3,2},{0,3}};
static int Tet_vnum [][3] = {{0,1,2},{0,1,3},{1,2,3},{0,2,3}};
static int Pyr_vnum [][4] = {{0,1,2,3},{0,1,4,RERR},{1,2,4,RERR},
             {3,2,4,RERR},{0,3,4,RERR}};
static int Prism_vnum [][4] = {{0,1,2,3},{0,1,4,RERR},{1,2,5,4},
             {3,2,5,RERR},{0,3,5,4}};
static int Hex_vnum [][4] = {{0,1,2,3},{0,1,5,4},{1,2,6,5},
           {3,2,6,7},{0,3,7,4},{4,5,6,7}};

int  Element::dim(){return 0;}

/*

Function name: Element::vnum

Function Purpose:
 Return local vertex id of a vertex on an edge (in 2d) or face (in 3d).

Argument 1: int i
Purpose:
 i is id of the edge (2d) or face (3d)

Argument 2: int j
Purpose:
 j is the local id on the edge or face of the vertex in question.

Function Notes:

*/

int Tri::vnum(int i, int j){
  return Tri_vnum[i][j];
}




int Quad::vnum(int i, int j){
  return Quad_vnum[i][j];
}




int Tet::vnum(int i, int j){
  return Tet_vnum[i][j];
}




int Pyr::vnum(int i, int j){
  return Pyr_vnum[i][j];
}




int Prism::vnum(int i, int j){
  return Prism_vnum[i][j];
}




int Hex::vnum(int i, int j){
  return Hex_vnum[i][j];
}




int  Element::vnum(int,int){ERR;return -1;}



static int Tet_ednum [][3]   = {{0,1,2},{0,4,3},{1,5,4},{2,5,3}};
static int Pyr_ednum [][4]   = {{0,1,2,3},{0,5,4,RERR},{1,6,5,RERR},
              {2,6,7,RERR},{3,7,4,RERR}};
static int Prism_ednum [][4] = {{0,1,2,3},{0,5,4,RERR},{1,6,8,5},
        {2,6,7,RERR},{3,7,8,4}};
static int Hex_ednum [][4]   = {{0,1,2,3},{0,5,8,4},{1,6,9,5},{2,6,10,7},
              {3,7,11,4},{8,9,10,11}};

/*

Function name: Element::ednum

Function Purpose:

Argument 1: int
Purpose:

Argument 2: int
Purpose:

Function Notes:

*/

int Tri::ednum(int , int ){
  return -1;
}




int Quad::ednum(int , int ){
  return -1;
}




int Tet::ednum(int i, int j){
  return Tet_ednum[i][j];
}




int Pyr::ednum(int i, int j){
  return Pyr_ednum[i][j];
}




int Prism::ednum(int i, int j){
  return Prism_ednum[i][j];
}




int Hex::ednum(int i, int j){
  return Hex_ednum[i][j];
}




int  Element::ednum(int,int){ERR;return -1;}

//make new function called ednum2, counter-clockwise orientation defined from inside the elements
static int Tet_ednum2 [][3]   = {{0,1,2},{0,3,4},{1,4,5},{2,5,3}};
static int Pyr_ednum2 [][4]   = {{0,1,2,3},{0,4,5,RERR},{1,5,6,RERR},
                                {2,6,7,RERR},{3,7,4,RERR}};
static int Prism_ednum2 [][4] = {{0,1,2,3},{0,4,5,RERR},{1,5,8,6},
                                {2,6,7,RERR},{3,7,8,4}};
static int Hex_ednum2 [][4]   = {{0,1,2,3},{0,4,8,5},{1,5,9,6},{2,6,10,7},
                                {3,7,11,4},{8,9,10,11}};


int Tri::ednum2(int , int ){
  return -1;
}

int Quad::ednum2(int , int ){
  return -1;
}

int Tet::ednum2(int i, int j){
  return Tet_ednum2[i][j];
}

int Pyr::ednum2(int i, int j){
  return Pyr_ednum2[i][j];
}

int Prism::ednum2(int i, int j){
  return Prism_ednum2[i][j];
}

int Hex::ednum2(int i, int j){
  return Hex_ednum2[i][j];
}

int  Element::ednum2(int,int){ERR;return -1;}







static int Tet_ednum1[][3] = {{0,2,1},{0,3,4},{1,4,5},{2,3,5}};

static int Pyr_ednum1[][4] = {{0,3,2,1},{0,4,5,RERR},{1,5,6,RERR},
            {2,7,6,RERR},{3,4,7,RERR}};

static int Prism_ednum1[][4] = {{0,3,2,1},{0,4,5,RERR},{1,5,8,6},
        {2,7,6,RERR},{3,4,8,7}};

static int Hex_ednum1[][4] = {{0,3,2,1},{0,4,8,5},{1,5,9,6},{2,7,10,6},
            {3,4,11,7},{8,11,10,9}};



/*

Function name: Element::ednum1

Function Purpose:

Argument 1: int
Purpose:

Argument 2: int
Purpose:

Function Notes:

*/

int Tri::ednum1(int , int ){
  return -1;
}




int Quad::ednum1(int , int ){
  return -1;
}




int Tet::ednum1(int i, int j){
  return Tet_ednum1[i][j];
}




int Pyr::ednum1(int i, int j){
  return Pyr_ednum1[i][j];
}




int Prism::ednum1(int i, int j){
  return Prism_ednum1[i][j];
}




int Hex::ednum1(int i, int j){
  return Hex_ednum1[i][j];
}




int  Element::ednum1(int,int){ERR;return -1;}


static int Tri_edvnum[][2] = {{0,1},{1,2},{0,2}};
static int Quad_edvnum[][2] = {{0,1},{1,2},{3,2},{0, 3}};
static int Tet_edvnum [][2] = {{0,1},{1,2},{0,2},{0,3},{1,3},{2,3}};
static int Pyr_edvnum[][2] = {{0,1}, {1,2}, {3,2}, {0,3},
            {0,5}, {1,5}, {2,5}, {3,5}};
static int Prism_edvnum[][2] = {{0,1}, {1,2}, {3,2}, {0,3},
            {0,4}, {1,4}, {2,5}, {3,5},{4,5}};

static int Hex_edvnum[][2] = {{0,1}, {1,2}, {3,2}, {0,3},
            {0,4}, {1,5}, {2,6}, {3,7},
            {4,5}, {5,6}, {7,6}, {4,7}};


/**

Function name: Element::edvnum

Function Purpose:

Argument 1: int edg
Purpose:  local id of edge. edg = 0,Nedges-1;

Argument 2: int v
Purpose: local id of vertex 'v' on edge. v=0,1;

Function Notes:

Returns the vertex id of the vertices along a given edge for the
specified edges type.

*/

int Tri::edvnum(int edg, int v){
  return Tri_edvnum[edg][v];
}




int Quad::edvnum(int edg, int v){
  return Quad_edvnum[edg][v];
}

int Tet::edvnum(int i, int j){
  return Tet_edvnum[i][j];
}

int Pyr::edvnum(int i, int j){
  return Pyr_edvnum[i][j];
}

int Prism::edvnum(int i, int j){
  return Prism_edvnum[i][j];
}

int Hex::edvnum(int i, int j){
  return Hex_edvnum[i][j];
}




int Element::edvnum(int,int){return -1;}


static int Tri_fnum  [][2] = {{0,1},{1,2},{2,0}};
static int Quad_fnum  [][2] = {{0,1},{1,2},{2,3},{3,0}};
static int Tet_fnum[][3]  = {{1,0,2},{0,1,3},{1,2,3},{2,0,3}};
static int Pyr_fnum[][4]  = {{0,3,2,1},{0,1,5,4},{1,2,6,5},
           {2,3,7,6},{0,4,7,3},{4,5,6,7}};
static int Prism_fnum[][4]  = {{1,0,3,2},{1,4,0,-1},{1,2,5,4},
             {2,3,5,-1},{0,4,5,3}};
static int Hex_fnum[][4]  = {{0,3,2,1},{0,1,5,4},{1,2,6,5},{2,3,7,6},
           {0,4,7,3},{4,5,6,7}};


/*

Function name: Element::fnum

Function Purpose:

Argument 1: int i
Purpose:

Argument 2: int j
Purpose:

Function Notes:

*/

int Tri::fnum(int i, int j){
  return Tri_fnum[i][j];
}




int Quad::fnum(int i, int j){
  return Quad_fnum[i][j];
}




int Tet::fnum(int i, int j){
  return Tet_fnum[i][j];
}




int Pyr::fnum(int i, int j){
  fprintf(stderr, "Pyr::fnum not to be used\n");
  return Pyr_fnum[i][j];
}




int Prism::fnum(int i, int j){
  //  fprintf(stderr, "Prism::fnum not to be used\n");
  return Prism_fnum[i][j];
}




int Hex::fnum(int i, int j){
  return Hex_fnum[i][j];
}




int  Element::fnum(int,int){ERR;return -1;}



static int Tri_fnum1 [][2] = {{1,0},{2,1},{0,2}};

static int Quad_fnum1 [][2] = {{1,0},{2,1},{3,2},{0,3}};

static int Tet_fnum1[][3] = {{0,1,2},{1,0,3},{2,1,3},{0,2,3}};

static int Pyr_fnum1[][4] = {{0,1,2,3},{0,4,5,1},{1,5,6,2},
           {2,6,7,5},{0,3,7,4},{4,7,6,5}};


static int Prism_fnum1[][4] = {{0,1,2,3},{0,4,5,1},{1,5,6,2},
             {2,6,7,5},{0,3,7,4},{4,7,6,5}};

static int Hex_fnum1[][4] = {{0,1,2,3},{0,4,5,1},{1,5,6,2},
           {2,6,7,5},{0,3,7,4},{4,7,6,5}};

/*

Function name: Element::fnum1

Function Purpose:

Argument 1: int i
Purpose:

Argument 2: int j
Purpose:

Function Notes:

*/

int Tri::fnum1(int i, int j){
  return Tri_fnum1[i][j];
}




int Quad::fnum1(int i, int j){
  return Quad_fnum1[i][j];
}




int Tet::fnum1(int i, int j){
  return Tet_fnum1[i][j];
}




int Pyr::fnum1(int i, int j){
  fprintf(stderr, "Pyr::fnum1 not to be used\n");
  return Pyr_fnum1[i][j];
}




int Prism::fnum1(int i, int j){
  fprintf(stderr, "Prism::fnum1 not to be used\n");
  return Prism_fnum1[i][j];
}




int Hex::fnum1(int i, int j){
  return Hex_fnum1[i][j];
}




int  Element::fnum1(int,int){ERR;return -1;}



int Tri_Nfverts[3] = {2,2,2};
int Quad_Nfverts[4] = {2,2,2,2};

int Tet_Nfverts[4] = {3,3,3,3};
int Prism_Nfverts[5] = {4,3,4,3,4};
int Hex_Nfverts[6] = {4,4,4,4,4,4};
int Pyr_Nfverts[5] = {4,3,3,3,3};


/*

Function name: Element::Nfverts

Function Purpose:

Argument 1: int fac
Purpose:

Function Notes:

*/

int Tri::Nfverts(int fac){
  return Tri_Nfverts[fac];
}




int Quad::Nfverts(int fac){
  return Quad_Nfverts[fac];
}




int Tet::Nfverts(int fac){
  return Tet_Nfverts[fac];
}




int Pyr::Nfverts(int fac){
  return Pyr_Nfverts[fac];
}




int Prism::Nfverts(int fac){
  return Prism_Nfverts[fac];
}




int Hex::Nfverts(int fac){
  return Hex_Nfverts[fac];
}




int  Element::Nfverts(int){return 0;}




/*

Function name: Element::Nfmodes

Function Purpose:

Function Notes:

*/

int Tri::Nfmodes(){
  return face[0].l*(face[0].l+1)/2;
}




int Quad::Nfmodes(){
  return face[0].l*face[0].l;
}




int Tet::Nfmodes(){
  return Nmodes-Nbmodes;
}




int Pyr::Nfmodes(){
  fprintf(stderr, "Pyr::Nfmodes\n");
  return -1;
}




int Prism::Nfmodes(){
  fprintf(stderr, "Prism::Nfmodes\n");
  return -1;
}




int Hex::Nfmodes(){
  fprintf(stderr, "Hex::Nfmodes\n");
  return -1;
}




int  Element::Nfmodes(){ERR;return 0;}




/*

Function name: Element::data_len

Function Purpose:

Argument 1: int *size
Purpose:

Function Notes:

*/

int Tri::data_len(int *size){
  int i,cnt = 0;

  cnt += Nverts;

  for(i=0;i<Nedges;++i){
    cnt += *size;
    ++size;
  }

  i = *size;
  cnt += i*(i+1)/2;
  return cnt;
}




int Quad::data_len(int *size){
  int i,cnt = 0;

  cnt += Nverts;

  for(i=0;i<Nedges;++i){
    cnt += *size;
    ++size;
  }

  i = *size;
  cnt += i*i;

  return cnt;
}




int Tet::data_len(int *size){
  int i,j,cnt = 0;

  cnt += Nverts;

  for(i=0;i<Nedges;++i){
    cnt += *size;
    ++size;
  }

  for(i=0;i<Nfaces;++i){
    j = *size;
    cnt += j*(j+1)/2;
    ++size;
  }

  j = *size;
  cnt += j*(j+1)*(j+2)/6;
  return cnt;
}




int Pyr::data_len(int *size){
  int i,j,cnt = 0;

  cnt += NPyr_verts;

  for(i=0;i<NPyr_edges;++i){
    cnt += *size;
    ++size;
  }

  for(i=0;i<NPyr_faces;++i){
    j = *size;
    cnt += (Nfverts(i) == 3) ? j*(j+1)/2 : j*j;
    ++size;
  }

  j = *size;
  cnt += j*(j+1)*(j+2)/6;
  return cnt;
}




int Prism::data_len(int *size){
  int i,j,cnt = 0;

  cnt += NPrism_verts;

  for(i=0;i<NPrism_edges;++i){
    cnt += *size;
    ++size;
  }

  for(i=0;i<NPrism_faces;++i){
    j = *size;
    cnt += (Nfverts(i) == 3) ? j*(j+1)/2 : j*j;
    ++size;
  }

  j = *size;
  cnt += (j-1)*j*j/2;

  return cnt;
}




int Hex::data_len(int *size){
  int i,j,cnt = 0;

  cnt += NHex_verts;

  for(i=0;i<NHex_edges;++i){
    cnt += *size;
    ++size;
  }

  for(i=0;i<NHex_faces;++i){
    j = *size;
    cnt += j*j;
    ++size;
  }

  j = *size;
  cnt += j*j*j;

  return cnt;
}




int  Element::data_len(int *){ERR; return -1;}
