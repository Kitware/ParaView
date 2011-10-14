
#include "SMatrix.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
//#include "veclib.h"


void sort2D(int n, int *original_V, int *sorted_V, double *original_A, double *sorted_A);
int check_doubling(int n , int *V );

void SMatrix::allocate_SMatrix(int nonzero, int size_M, int size_N){
  nz = nonzero;
  Msize = size_M;
  Nsize = size_N;
  space_available = nz;
  AA = new double[nz];
  JA = new int[nz];
  IA = new int[nz];

  register int i;
  /* initialize */
  memset(AA,'\0',nz*sizeof(double));
  for (i = 0; i < nz; i++){
    JA[i] = -1;
  }
  for (i = 0; i < nz; i++){
    IA[i] = -1;
  }
}

void SMatrix::allocate_SMatrix_cont(int nonzero, int size_M, int size_N){
  nz = nonzero;
  Msize = size_M;
  Nsize = size_N;
  space_available = nz;
  AA = new double[nz];

  IA = new int[nz*2];
  JA = IA+nz;

  register int i;
  /* initialize */
  memset(AA,'\0',nz*sizeof(double));
  for (i = 0; i < nz*2; i++)
    IA[i] = -1;
}



void SMatrix::allocate_SMatrix_CR(int nonzero, int size_M, int size_N){
  nz = nonzero;
  Msize = size_M; // number of rows
  Nsize = size_N; // number of columns
  space_available = nz;
  AA = new double[nz];
  JA = new int[nz];
  IA = new int[size_M+1];

  register int i;
  /* initialize */
  memset(AA,'\0',nz*sizeof(double));
  for (i = 0; i < nz; i++)
    JA[i] = -1;

  for (i = 0; i <= Msize; i++)
    IA[i] = -1;

}

void SMatrix::deallocate_SMatrix(){
  delete[] AA;
  delete[] JA;
  delete[] IA;
}

void SMatrix::deallocate_SMatrix_cont(){
  delete[] AA;
  delete[] IA;
}

int SMatrix::get_nnz_SMatrix(int r, int c, double **M){

  register int ans = 0, i,j;
  for (i=0;i<r;i++){
    for (j=0;j<c;j++){
      if (fabs(M[i][j]) > TOL_SMatrix){
  ans++;
      }
    }
  }
  return ans;
}



void SMatrix::create_SMatrix(int r, int c, double **M){

  int index,i,j;

  index = 0;

  for (i=0;i<r;i++){
    for (j=0;j<c;j++){
      if (fabs(M[i][j]) > TOL_SMatrix){
  AA[index] = M[i][j];
  IA[index] = i;
  JA[index] = j;
  index++;
      }
    }
  }
}

void SMatrix::create_SMatrix_CR(int r, int c, double **M){

  int index,index2,indexAA,FLAG,i,j;

  index = 0;
  index2 = 0;
  indexAA=0;
  FLAG = 0;

  for (i=0;i<=r;i++)
    IA[i] = 0;

  for (i=0;i<r;i++){
    for (j=0;j<c;j++){
      if (fabs(M[i][j]) > TOL_SMatrix){
  AA[index] = M[i][j];
  JA[index] = j;
  if (FLAG == 0){
    IA[index2] = indexAA;
    index2++;
    FLAG = 1;
  }
  index++;
  indexAA++;
      }
    }
    if (FLAG ==0){
      IA[index2] = indexAA;
      index2++;
    }
    FLAG = 0;
  }

  IA[r] = nz;


}


int SMatrix::add_point_value_SMatrix(int Index, int Jndex, double value){

  int i,FLAG = 0;

  for (i=0;i<nz;i++){
    if (IA[i] == Index && JA[i] == Jndex){
      AA[i] += value;
      FLAG = 1;
      break;
    }
  }
  /* if coordinates of point where found - add the value and exit*/


  if (FLAG == 1){
    return 0;
  }

  /* if coordinates where not found - check if there is a space for
     new value and append the point */

  FLAG = 0;
  for (i = 0; i < space_available; i++){
    if (IA[i] == -1 && JA[i] == -1){
      AA[i] = value;
      IA[i] = Index;
      JA[i] = Jndex;
      FLAG = 1;
      break;
    }
  }

  /* if the was a space for a new point - exit*/
  if (FLAG == 1){
    //printf(" point inserted  \n");
    return 1;
  }


  /* if there was no space for a new point - increase the size of arrays */

  double *AA_temp;
  int *JA_temp,*IA_temp;

  AA_temp = new double[space_available+1];
  JA_temp = new int[space_available+1];
  IA_temp = new int[space_available+1];

  space_available += 1;

  memcpy (AA_temp,AA, nz*sizeof(double));
  memcpy (JA_temp,JA, nz*sizeof(int));
  memcpy (IA_temp,IA, nz*sizeof(int));

  if (nz >= space_available )
    printf(" add_point_value_SMatrix ERROR: nz >= space_available : %d >= %d ",nz,space_available);

  AA_temp[nz] = value;
  IA_temp[nz] = Index;
  JA_temp[nz] = Jndex;


  for (i = nz+1; i < space_available; i++){
    AA_temp[i] = 0.0;
    IA_temp[i] = -1;
    JA_temp[i] = -1;
  }

  delete[] AA;
  delete[] IA;
  delete[] JA;

  AA = AA_temp;
  IA = IA_temp;
  JA = JA_temp;

  /* since new point was appended increase the # of non-zero entries by one */
  nz++;

  return 2;
}


void SMatrix::transform(int type, int *nz_o, int *IA_o, int *JA_o, double *AA_o){

  /* TYPE  -  TRANSFORM                       */
  /*  0       condense -> condence row format */
  /*  1       condence row format  -> comdense */

  /* meanwhile only type 0 is iimplemented */


  int *save_JA;
  double *temp_AA;
  int row,i,test,counter,global_counter = 0;

  //temp_JA = new int[Msize];
  temp_AA = new double[Msize];
  save_JA = new int[Msize];

  IA_o[0] = 0;

  switch(type){
  case 0:

    for (row = 0; row < Msize; row++){

      counter = 0;
      for (i = 0; i < nz; i++){
        if (IA[i] == row){
          JA_o[counter+global_counter] = JA[i];
          temp_AA[counter] = AA[i];
          counter++;
        }
      }
      memcpy(save_JA,&JA_o[global_counter],counter*sizeof(int));

      /* sort temp_JA , result -> JA_sorted */
      std::sort<int*>(&JA_o[global_counter], (&JA_o[global_counter])+counter);

      test = check_doubling(counter,&JA_o[global_counter]);

      sort2D(counter,save_JA,&JA_o[global_counter],temp_AA,&AA_o[global_counter]);

      global_counter += counter;
      IA_o[row+1] = global_counter;

    }
  }
  nz_o[0] = global_counter;

  //delete[] temp_JA;
  delete[] temp_AA;
  delete[] save_JA;
}


void SMatrix::dump_SMatrix(FILE *pFile){
  int i;
  for (i = 0; i < nz; i++){
      if (IA[i] != -1 && JA[i] != -1)
        fprintf(pFile,"%d %d %2.16f \n",IA[i],JA[i],AA[i]);
  }
}


void sort2D(int n, int *original_V, int *sorted_V, double *original_A, double *sorted_A){

  /* sort arra original_A  with respect to mapping of original_V onto sorted_V  */
  int i,j;

  for (i = 0; i < n; i++){
    for (j = 0; j < n; j++){
      if (sorted_V[i] == original_V[j]){
        sorted_A[i] = original_A[j];
        break;
      }
    }
  }

}

int check_doubling(int n , int *V ){

  register int i;
  int FLAG = 0;
  for (i = 1; i < n; i++){
    if (V[i] == V[i-1]){
      printf(" have found doubling at V[%d] = V[%d] = %d \n", i-1,i,V[i]);
      FLAG = 1;
    }
  }
  return FLAG;
}

void SMatrix::extract_diag_CR(double *diag, int shift){
  int  i,j,J1,J2;
  /* exstract diagonal terms from matrix */

  for (i=0; i < Msize; i++){
    J1 = IA[i];
    J2 = IA[i+1];
    diag[i] = 0.0;
    for (j = 0; j < (J2-J1); j++){
      if (JA[j+J1] == (i+shift)){
        diag[i] = AA[j+J1];
        break;
      }
    }
  }
}

void SMatrix::dotSAV_CR(double *u, double *ans){
  /* Sparse Matrix dot Vector u = ans */
  int i,j,J1,J2;
  register double sum;

  for (i=0; i < Msize; i++){

    J1 = IA[i];
    J2 = IA[i+1];

    sum = 0.0;
    for (j=0; j < (J2-J1); j++)
      sum += AA[j+J1]*u[JA[j+J1]];

    ans[i] = sum;

  }
}
