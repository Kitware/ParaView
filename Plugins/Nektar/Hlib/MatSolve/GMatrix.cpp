#include <cassert>
#include <math.h>
#include "Lapack.h"
#include "Vmath.h"
#include "BlkMat.h"
#include "GMatrix.h"
#include <stdio.h>

using namespace std;

namespace MatSolve {

  /// \brief Default destructor releasing A and ipiv if defined
  Gmat::~Gmat(){
    if(_ipiv) delete[] _ipiv;
    if(_Am)    delete[] _Am;
    _Am = NULL;
  }

  /** \brief factorise matrix depending upon definition stored in mat_form.

  Options for mat_from are:

  - mat_form = Symmetric-Positive implies Cholesky factorization using
  lapack::dpptrf.

  - mat_form = Symmetric implies Factorisation using Bunch-Kaufman
  pivoting using lapack::dsptrf.

  - mat_form = Symmetric-Positive-Banded implies lower diagonal banded
  cholesky factorisation using lapack::dpbtrf.

  - mat_form = General-Banded implies factoring using lapack::dgbtrf.

  - mat_form = General-Full implies factoring using lapack::dgetrf.

  */
  void Gmat::Factor(){
    int info;

    if(_factored) return;

    switch(mat_form){
    case Symmetric:
      _ipiv = new int[_lda];
      lapack::dsptrf('L',_lda,_Am,_ipiv,info);
      assert(info==0);
      break;
    case Symmetric_Positive:
      lapack::dpptrf('L', _lda, _Am, info);
      assert(info==0);
      break;
    case Symmetric_Positive_Banded:
      lapack::dpbtrf('L',_lda,_bwidth-1,_Am,_bwidth,info);
      assert(info==0);
      break;
    case General_Banded:
      _ipiv = new int[_lda];
      lapack::dgbtrf(_lda,_lda,_ldiag,_bwidth-1,_Am,2*_ldiag+_bwidth,_ipiv,info);
      assert(info==0);
      break;
    case General_Full:
      _ipiv = new int[_lda];
      lapack::dgetrf(_lda,_lda,_Am,_lda,_ipiv,info);
      assert(info==0);
      break;
    }

    _factored = 1;

  }

  /**  \brief  evaulate \f$u \leftarrow _Am^{-1} u\f$ for \a nrhs solves

  Options for mat_from are:

    - mat_form = Symmetric-Positive implies Cholesky back solve using
    lapack::dpptrs.

    - mat_form = Symmetric implies Back solve from  using lapack::dsptrs.

    - mat_form = Symmetric-Positive-Banded implies lower diagonal banded
    cholesky backsole using  lapack::dpbtrs.

    - mat_form = General-Banded implies LU back solve using lapack::dgbtrs.

    - mat_form = General-Full implies LU back solve using lapack::dgetrs.

   */

  void Gmat::Solve(double *u, int nrhs){
    int info;

    if(!_factored) Factor();

    switch(mat_form){
    case Symmetric:
      lapack::dsptrs('L',_lda,nrhs,_Am,_ipiv,u,_lda,info);
      assert(info==0);
      break;
    case Symmetric_Positive:
      lapack::dpptrs('L', _lda,nrhs,_Am,u,_lda,info);
      assert(info==0);
      break;
    case Symmetric_Positive_Banded:
      lapack::dpbtrs('L',_lda,_bwidth-1,nrhs,_Am,_bwidth,u,_lda,info);
      assert(info==0);
      break;
    case General_Banded:
      lapack::dgbtrs('N',_lda,_ldiag,_bwidth-1,nrhs,_Am,
         2*_ldiag+_bwidth,_ipiv,u,_lda,info);
      assert(info==0);
    break;
    case General_Full:
      lapack::dgetrs('N',_lda,nrhs,_Am,_lda,_ipiv,u,_lda,info);
      assert(info==0);
    break;
    }
  }

  // declare the memory of _Am depending upon its definition
  void Gmat::SetMemA(){

    if(_Am) return;

    assert((_lda != 0)&&(_lda > 0));

    switch(mat_form){
    case Symmetric:     case Symmetric_Positive:
      _Am = new double [_lda*(_lda+1)/2];
      vmath::zero(_lda*(_lda+1)/2,_Am,1);
      break;
    case Symmetric_Positive_Banded:
      assert(_bwidth > 0);
      _Am = new double [_lda*_bwidth];
      vmath::zero(_lda*_bwidth,_Am,1);
      break;
    case General_Banded:
      assert((_bwidth > 0)&&(_ldiag > 0));
      _Am = new double [_lda*2*(_ldiag+_bwidth)];
      vmath::zero(_lda*2*(_ldiag+_bwidth),_Am,1);
      break;
    case General_Full:
      _Am = new double [_lda*_lda];
      vmath::zero(_lda*_lda,_Am,1);
      break;
    }

  }


  //-------------------------------------------------------------

  /** \brief Assemble global matrix from local values in Amat
      using the mapping array  map[i] applying signchange if it exists

      - Inputs:  A, map, signchg
     - Output:

     Assemble the matrix (this) from \e A using the mapping array
  \e map[i]. Note that if \e map[i] is zero then the entry is not projected.

  The type of assembly  which is applied depends upon G.mat_form

  - G.mat_form = Symmetric-Positive or Symmetric then matrix is
  assembled according to a lower symmetric packed format.

  - G.mat_form = Symmetric-Positive-Banded then matrix is assembled
    according to a lower symmetric banded packed format.

  - G.mat_form = General-Banded then matrix is assembled according to
  a general banded format.

  - G.mat_form = General-Full then matrix is assembled according to a
  column major format

    - Note input is assumed to be in row major format
  */

  void Gmat::Assemble(BlkMat& Mat, const int *map, const double* signchg){
    int i,j,idi,idj,p,q,n;
    int nrows = Mat.get_mrowblk();
    int ncols = Mat.get_mcolblk();
    int *offset;
    double *Asub,sign;

    offset = Mat.get_offset();

    // make sure memory is allocated
    if(!_Am) SetMemA();

    switch(mat_form){
    case Symmetric_Positive: case Symmetric:
      for(p = 0; p < nrows; ++p)
  for(q = 0; q < ncols; ++q)
    if(Asub = Mat.get_mat(p,q,n,n))
      for(j = 0; j < n; ++j)
        if((idj=map[offset[q]+j])+1)
    for(i = 0; i < n; ++i)
      if((idi=map[offset[p]+i])+1)
        if(idj <= idi){
          sign = (signchg)?
      signchg[offset[q]+j]*signchg[offset[p]+i] : 1;
          _Am[idi + idj*(2*_lda -idj- 1)/2] += sign*Asub[i*n+j];
        }
      break;
    case Symmetric_Positive_Banded:
      for(p = 0; p < nrows; ++p)
  for(q = 0; q < ncols; ++q)
    if(Asub = Mat.get_mat(p,q,n,n))
      for(j  = 0; j < n; ++j)
        if((idj=map[offset[q]+j])+1)
    for(i = 0; i < n; ++i)
      if((idi=map[offset[p]+i])+1)
        if(idj <= idi){
          sign = (signchg)?
      signchg[offset[q]+j]*signchg[offset[p]+i] : 1;
          _Am[_bwidth*idj +(idi-idj)] += sign*Asub[i*n+j];
        }
      break;
    case General_Banded:
      for(p = 0; p < nrows; ++p)
  for(q = 0; q < ncols; ++q)
    if(Asub = Mat.get_mat(p,q,n,n))
      for(j = 0; j < n; ++j)
        if((idj=map[offset[q]+j])+1)
    for(i = 0; i < n; ++i)
      if((idi=map[offset[p]+i])+1){
        sign = (signchg)?
          signchg[offset[q]+j]*signchg[offset[p]+i] : 1;
        _Am[idj*(2*_ldiag+_bwidth)+_ldiag+_bwidth-1+(idi-idj)]
          += sign*Asub[i*n+j];
      }
      break;
    case General_Full:
      for(p = 0; p < nrows; ++p)
  for(q = 0; q < ncols; ++q)
    if(Asub = Mat.get_mat(p,q,n,n))
      for(j = 0; j < n; ++j)
        if((idj = map[offset[q]+j])+1)
    for(i = 0; i < n; ++i)
      if((idi = map[offset[p]+i])+1){
        sign = (signchg)?
          signchg[offset[q]+j]*signchg[offset[p]+i] : 1;
        _Am[idj*_lda+idi] += sign*Asub[i*n+j];
      }
      break;
    }
  }

  // same as assemble above but using nsolve as cuttoff criteria
  void Gmat::Assemble(BlkMat& Mat, const int *map, const double* signchg,
          int nsolve){
    int i,j,idi,idj,p,q,n;
    int nrows = Mat.get_mrowblk();
    int ncols = Mat.get_mcolblk();
    int *offset;
    double *Asub,sign;

    offset = Mat.get_offset();

    // make sure memory is allocated
    if(!_Am) SetMemA();

    switch(mat_form){
    case Symmetric_Positive: case Symmetric:
      for(p = 0; p < nrows; ++p)
  for(q = 0; q < ncols; ++q)
    if(Asub = Mat.get_mat(p,q,n,n))
      for(j = 0; j < n; ++j)
        if((idj=map[offset[q]+j])<nsolve)
    for(i = 0; i < n; ++i)
      if((idi=map[offset[p]+i])<nsolve)
        if(idj <= idi){
          sign = (signchg)?
      signchg[offset[q]+j]*signchg[offset[p]+i] : 1;
          _Am[idi + idj*(2*_lda -idj- 1)/2] += sign*Asub[i*n+j];
        }
      break;
    case Symmetric_Positive_Banded:
      for(p = 0; p < nrows; ++p)
  for(q = 0; q < ncols; ++q)
    if(Asub = Mat.get_mat(p,q,n,n))
      for(j  = 0; j < n; ++j)
        if((idj=map[offset[q]+j])<nsolve)
    for(i = 0; i < n; ++i)
      if((idi=map[offset[p]+i])<nsolve)
        if(idj <= idi){
          sign = (signchg)?
      signchg[offset[q]+j]*signchg[offset[p]+i] : 1;
          _Am[_bwidth*idj +(idi-idj)] += sign*Asub[i*n+j];
        }
      break;
    case General_Banded:
      for(p = 0; p < nrows; ++p)
  for(q = 0; q < ncols; ++q)
    if(Asub = Mat.get_mat(p,q,n,n))
      for(j = 0; j < n; ++j)
        if((idj=map[offset[q]+j])<nsolve)
    for(i = 0; i < n; ++i)
      if((idi=map[offset[p]+i])<nsolve){
        sign = (signchg)?
          signchg[offset[q]+j]*signchg[offset[p]+i] : 1;
        _Am[idj*(2*_ldiag+_bwidth)+_ldiag+_bwidth-1+(idi-idj)]
          += sign*Asub[i*n+j];
      }
      break;
    case General_Full:
      for(p = 0; p < nrows; ++p)
  for(q = 0; q < ncols; ++q)
    if(Asub = Mat.get_mat(p,q,n,n))
      for(j = 0; j < n; ++j)
        if((idj = map[offset[q]+j])<nsolve)
    for(i = 0; i < n; ++i)
      if((idi = map[offset[p]+i])<nsolve){
        sign = (signchg)?
          signchg[offset[q]+j]*signchg[offset[p]+i] : 1;
        _Am[idj*_lda+idi] += sign*Asub[i*n+j];
      }
      break;
    }
  }

  double Gmat::L2ConditionNo(){
    double *er = new double [_lda];
    double *ei = new double [_lda];
    double max,min;

    Spectrum(er,ei,(double *)NULL);

    vmath::vmul (_lda,er,1,er,1,er,1);
    vmath::vmul (_lda,ei,1,ei,1,ei,1);
    vmath::vadd (_lda,er,1,ei,1,er,1);

    max = sqrt(er[vmath::imax(_lda,er,1)]);
    min = sqrt(er[vmath::imin(_lda,er,1)]);

    if(min < 1e-11){ // if min < 1e-11 find second smallest ev
      fprintf(stderr,"Min ev < 1e-11 using second ev\n");
      er[vmath::imin(_lda,er,1)] += max;
      min = sqrt(er[vmath::imin(_lda,er,1)]);
    }

    delete[] er;
    delete[] ei;
    return max/min;
  }


  double Gmat::MaxEigenValue(){
    double *er = new double[_lda];
    double *ei = new double[_lda];
    double max;

    Spectrum(er,ei,(double *)NULL);

    vmath::vmul(_lda,er,1,er,1,er,1);
    vmath::vmul(_lda,ei,1,ei,1,ei,1);
    vmath::vadd(_lda,er,1,ei,1,er,1);

    max = sqrt(er[vmath::imax(_lda,er,1)]);
    delete[] er;
    delete[] ei;
    return max;
  }


  int  Gmat::NullSpaceDim(const double tol){
    double *er = new double [_lda];
    double *ei = new double [_lda];
    int i,ndim;

    Spectrum(er,ei,(double *)NULL);

    vmath::vmul (_lda,er,1,er,1,er,1);
    vmath::vmul (_lda,ei,1,ei,1,ei,1);
    vmath::vadd (_lda,er,1,ei,1,er,1);
    vmath::vsqrt(_lda,er,1,er,1);

    ndim = 0;
    for(i = 0; i < _lda; ++i)
      if(er[i] < tol)
  ++ndim;

    delete[] er;
    delete[] ei;

    return ndim;
  }

  void Gmat::EigenValues(const char file[]){
    int i;
    double *er = new double [_lda];
    double *ei = new double [_lda];
    FILE *fp;

    fp = fopen(file,"w");

    Spectrum(er,ei,(double *)NULL);

    fprintf(fp,"# Real Imag Magnitude\n");
    for(i = 0; i < _lda; ++i)
      fprintf(fp,"%lg %lg %lg \n",er[i],ei[i],sqrt(er[i]*er[i]+ei[i]*ei[i]));

    fclose(fp);
    delete[] er;
    delete[] ei;
  }

  void Gmat::Spectrum(double *er, double *ei, double *evecs){
    double dum;
    int info;

    switch(mat_form){
    case Symmetric_Positive: case Symmetric:
      {
  double *work  = new double [3*_lda];
  vmath::zero(_lda,ei,1);
  lapack::dspev('N','L',_lda,_Am,er,&dum,1,work,info);
  assert(info==0);
  delete[] work;
  break;
      }
    case Symmetric_Positive_Banded:
      {
  double *work  = new double [3*_lda];
  vmath::zero(_lda,ei,1);
  lapack::dsbev('N','L',_lda,_bwidth-1,_Am,_bwidth,er,&dum,1,work,info);
  assert(info==0);
  delete[] work;
  break;
      }
    case General_Banded:
      NekError::error(fatal,"GMatrix::Spectrum","Eigenvalue evaluation "
          "for genaral baneded matrix needs coding");
      break;
    case General_Full:
      {
  double *work  = new double [4*_lda];
  if(evecs)
    lapack::dgeev('N','V',_lda,_Am,_lda,er,ei,&dum,1,
      evecs,_lda,work,4*_lda,info);
  else{
    double dum1;
    lapack::dgeev('N','N',_lda,_Am,_lda,er,ei,&dum,1,
      &dum1,1,work,4*_lda,info);
  }
  assert(info==0);
  delete[] work;
  break;
      }
    }
  }

  void Gmat::plotmatrix(){
    int i,j;

    switch(mat_form){
    case Symmetric_Positive: case Symmetric:
      NekError::error(warning,"Gmat:showmatrix","Not set up to show matrix");
      break;
    case Symmetric_Positive_Banded:
      NekError::error(warning,"Gmat:showmatrix","Not set up to show matrix");
      break;
    case General_Banded:
      NekError::error(warning,"Gmat:showmatrix","Not set up to show matrix");
      break;
    case General_Full:
      {
  fprintf(stdout,"\n");
  for(i = 0; i < _lda; ++i){
    for(j = 0; j < _lda; ++j)
      if(fabs(_Am[i*_lda + j]) > 1e-12)
        if(_Am[i*_lda+j] > 0)
    fprintf(stdout,"+ ");
        else
    fprintf(stdout,"* ");
      else
        fprintf(stdout,"- ");
    fprintf(stdout,"\n");
  }
  fprintf(stdout,"\n");
  break;
      }
    }
  }

  void Gmat::WriteMatrix(FILE *fp){
    int i,j;

    switch(mat_form){
    case Symmetric_Positive: case Symmetric:
      NekError::error(warning,"Gmat:WriteMatrix","Not set up to show matrix");
      break;
    case Symmetric_Positive_Banded:
      NekError::error(warning,"Gmat:WriteMatrix","Not set up to show matrix");
      break;
    case General_Banded:
      NekError::error(warning,"Gmat:WriteMatrix","Not set up to show matrix");
      break;
    case General_Full:
      {
  fprintf(fp,"%d x %d \n",_lda,_lda);
  for(i = 0; i < _lda; ++i){
    for(j = 0; j < _lda; ++j)
      fprintf(fp,"%lf  ",_Am[i*_lda+j]);
    fprintf(fp,"\n");
  }
  break;
      }
    }
  }

}
