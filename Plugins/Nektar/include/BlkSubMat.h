#ifndef H_BLKSUBMAT
#define H_BLKSUBMAT


#include "Vmath.h"
#include "Lapack.h"
#include "NekError.h"

#include <stdlib.h>
#include <iostream>
#include <vector>

#include <assert.h>
using namespace std;

namespace NekBlkMat {

  enum MatStorage{
    RowMajor,  ///< Row major matrix storage
    ColMajor   ///< column major matrix storage
  };

  class BlkSubMat {
  public:
    /// default constructor  - no matrix
    BlkSubMat(const int id, const int rows, const int cols){
      _id   = id;
      _rows = rows;
      _cols = cols;
      _mat  = new double [_rows*_cols];
      vmath::zero(_rows*_cols,_mat,1);
    }

    /// default constructor - matrix
    BlkSubMat(const int id, const int rows, const int cols,
        const double *mat){
      _id   = id;
      _rows = rows;
      _cols = cols;
      _mat  = new double [_rows*_cols];
      vmath::vcopy(_rows*_cols,mat,1,_mat,1);
    }

    /// copy constructor
    BlkSubMat(const BlkSubMat& a){
      _id   = a._id;
      _rows = a._rows;
      _cols = a._cols;
      _mat  = new double [_rows*_cols];
      vmath::vcopy(_rows*_cols,a._mat,1,_mat,1);
    }

    /// default destructor
    ~BlkSubMat(){
      if(_rows*_cols)
  if(_mat) delete[] _mat;
    }

    void AddVal(const int id1, const int id2, const double val){
      _mat[id1*_cols+id2] += val;
    }

    void AddMat(const int rows, const int cols, const double *mat){

      if((_rows != rows) || (_cols != cols))
  NekError::error(warning,"BlVeck::AddMat",
      "Adding sub-matrices of different dimension");

      vmath::vadd(_rows*_cols,mat,1,_mat,1,_mat,1);
    }


    /// y = alpha*A*v + beta y
    /// formA,  provides the storage format of matrix A
    void geMxv(MatStorage formA, const double& alpha, const double *v,
         const double& beta, double *y){

      switch(formA){
      case RowMajor:
  blas::dgemv('T',_cols,_rows,alpha,_mat,_cols,v,1,beta,y,1);
  break;
      case ColMajor:
  blas::dgemv('N',_cols,_rows,alpha,_mat,_cols,v,1,beta,y,1);
  break;
      default:
  NekError::error(warning,"BlkMat::geMxv",
      "unknown storage format for matrix A");
      break;
      }

    }

    void Mxvpy(const double * v, double * y){
      blas::dgemv('T',_cols,_rows,1.0,_mat,_cols,v,1,1.0,y,1);
    }

    void Mtxvpy(const double * v, double * y){
      blas::dgemv('N',_cols,_rows,1.0,_mat,_cols,v,1,1.0,y,1);
    }


    void scal(const double alpha){
      blas::dscal(_rows*_cols,alpha,_mat,1);
    }

    void axpy(const double alpha, const BlkSubMat& A){
      blas::daxpy(A._rows*A._cols,alpha,A._mat,1,_mat,1);
    }

    /// alpha*A*B + beta (this)
    /// formA, formB provide the storage format of matrices A and B
    void geMxM(MatStorage formA, MatStorage formB, const double& alpha,
         const BlkSubMat& A, const BlkSubMat& B, const double& beta){

      switch(formB){
      case RowMajor:
  switch(formA){
  case RowMajor:
    blas::Cdgemm(A._rows,B._cols,A._cols,alpha,A._mat,
           A._cols,B._mat,B._cols,beta,_mat,_cols);
    break;
  case ColMajor:
    blas::dgemm('N','T',B._cols,A._cols,A._rows,alpha,B._mat,
          B._cols,A._mat,A._cols,beta,_mat,_cols);
    break;
  default:
    NekError::error(warning, "BlkMat::geMxM",
        "unknown storage format for matrix A");
    break;
  }
  break;
      case ColMajor:
  switch(formA){
  case RowMajor:
    blas::dgemm('T','N',B._rows,A._rows,A._cols,alpha,B._mat,
          B._cols,A._mat,A._cols,beta,_mat,_cols);
    break;
  case ColMajor:
    blas::dgemm('N','N',A._rows,B._cols,A._cols,alpha,A._mat,
          A._cols,B._mat,B._cols,beta,_mat,_cols);
    break;
  default:
    NekError::error(warning, "BlkMat::geMxM",
        "unknown storage format for matrix A");
    break;
  }
  break;
      default:
  NekError::error(warning, "BlkMat::geMxM",
      "unknown storage format for matrix B");
      break;
      }

    }

    // A * B + (this)
    void MxMpM(const BlkSubMat& a, const BlkSubMat& b){
      blas::Cdgemm(a._rows,b._cols,a._cols,1.0,a._mat,
       a._cols,b._mat,b._cols,1.0,_mat,_cols);
    }

    // A^T * B + (this)
    void MtxMpM(const BlkSubMat& a, const BlkSubMat& b){
      blas::dgemm('N','T',b._cols,a._cols,a._rows,1.0,b._mat,
      b._cols,a._mat,a._cols,1.0,_mat,_cols);
    }

    // A * B^T + (this)
    void MxMtpM(const BlkSubMat& a, const BlkSubMat& b){
      blas::dgemm('T','N',b._rows,a._rows,a._cols,1.0,b._mat,
      b._cols,a._mat,a._cols,1.0,_mat,_cols);
    }

    void PrintBlk(){
      for(int i = 0; i < _rows; ++i){
  for(int j = 0; j < _cols; ++j)
    cout << _mat[i*_cols+j] << " ";
  cout << endl;
      }
    }

    int     get_rows() const { return _rows; }
    int     get_cols() const { return _cols; }
    int     get_id()   const { return _id;   }
    double *get_mat() { return _mat; }
    double  get_val(const int i, const int j) { return _mat[i*_cols+j]; }

    // copy operator;
    BlkSubMat& operator = (const BlkSubMat& a){
      if(this != &a)
  vmath::vcopy(_rows*_cols,a._mat,1,_mat,1);
      return *this;
    }
    // (this) = (this) + a
    BlkSubMat& operator += (const BlkSubMat& a){
      vmath::vadd(_rows*_cols,a._mat,1,_mat,1,_mat,1);
      return *this;
    }
    // (this) = (this) - a
    BlkSubMat& operator -= (const BlkSubMat& a){
      vmath::vsub(_rows*_cols,_mat,1,a._mat,1,_mat,1);
      return *this;
    }

    void neg(){
      vmath::neg(_rows*_cols,_mat,1);
    }

    void zero(){
      vmath::zero(_rows*_cols,_mat,1);
    }

    void invert(void){
    int    *ipiv, info;
    double *Wk;

    if(_rows != _cols)
      NekError::error(fatal,"BlkSubMat::invert()","Matrix not square");

    ipiv = new int   [_rows];
    Wk   = new double[_rows*_cols];

    // invert C as a general matrix
#if defined (__blrts__) || defined (__bg__)
    dgetrf(_rows,_cols,_mat,_cols,ipiv,info);          assert(info==0);
    dgetri(_rows,_mat,_cols,ipiv,Wk,_rows*_cols,info); assert(info==0);
#else
    lapack::dgetrf(_rows,_cols,_mat,_cols,ipiv,info);          assert(info==0);
    lapack::dgetri(_rows,_mat,_cols,ipiv,Wk,_rows*_cols,info); assert(info==0);
#endif

    delete[] Wk;
    delete[] ipiv;
  }

    void rescale(const double *scale){
      if(_rows != _cols)
  NekError::error(fatal,"BlkSubMat::rescale","_rows not equal to _cols");

      for(int i = 0; i < _rows; ++i){
  // scale rows
  vmath::vmul(_cols,scale,1,_mat+i*_cols,1,_mat+i*_cols,1);
  // scale cols
  vmath::vmul(_rows,scale,1,_mat+i,_cols,_mat+i,_cols);
      }
    }

  private:
    int _id;       // id of this submatrix
    int _rows;     // number of rows in matrix;
    int _cols;     // number of columns in matrix;
    double *_mat;  // linear matrix stored in row major format

  };

}
#endif
