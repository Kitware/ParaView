#ifndef H_SMatrix_H
#define H_SMatrix_H

#include <math.h>
#include <stdio.h>
#define TOL_SMatrix 0.3e-15

class SMatrix {

public:
  int nz;    // number of nonzeros
  int Msize; // number of rows
  int Nsize; // number of columns
  int space_available;
  double *AA;
  int *IA,*JA;

  void allocate_SMatrix(int nz, int size_M, int size_N);
  void allocate_SMatrix_CR(int nonzero, int size_M, int size_N);
  void allocate_SMatrix_cont(int nz, int size_M, int size_N);

  void deallocate_SMatrix();
  void deallocate_SMatrix_cont();

  int get_nnz_SMatrix(int r, int c, double **M);

  int add_point_value_SMatrix(int Index, int Jndex, double value);

  void create_SMatrix(int r, int c, double **M);
  void create_SMatrix_CR(int r, int c, double **M);

  void transform(int type, int *nz_o, int *IA_o, int *JA_o, double *AA_o);

  void dump_SMatrix(FILE *pFile);

  void extract_diag_CR(double *diag, int shift);

  void dotSAV_CR(double *u, double *ans);

  //void create_SMatrixT(int r_in, int c_in, double **Min);
  //void dotSABTR(int r, double **B, double **ANS);

};


#endif
