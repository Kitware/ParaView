#ifndef H_GMATRIX
#define H_GMATRIX

#include <iostream>
#include "BlkMat.h"
#include <stdio.h>
using namespace NekBlkMat;

namespace MatSolve {

  /// Enumerator list for different types of matrix type supported by LAPACK
  enum MatrixForm{
    Symmetric,                    ///< Symmetric matrix
    Symmetric_Positive,           ///< Symmetric positive definite matrix
    Symmetric_Positive_Banded,    ///< Symmetric positive definite banded matrix
    General_Banded,               ///< General banded matrix
    General_Full                  ///< General full
  };

  /**
     Matrix class for holding global matrix for inversion
  */
  class Gmat{
    friend class SCmat;
  private:
    int _factored;         ///< Flag to identify if matrix is factored
    int *_ipiv;            ///< Pivoting array
    int  _lda;             ///< leading diagonal of matrix
    int _bwidth;           ///< Bandwdith for positive banded matrix \n
                           ///< Upper sug diagonals plus one for general
                           ///< banded matrix
    int _ldiag;            ///< Low sub diagonals for general banded matrix
    double* _Am;           ///< A matrix


  public:
    MatrixForm mat_form;   ///< enum list of type of matrix form

    Gmat(){
      _lda = 0;  _bwidth = 0; _ldiag = 0;
      _factored = 0;
      _Am =  NULL;}      ///< Default contructor
    ~Gmat();               ///< Default Destructor

    void Spectrum(double *er, double *ei, double *evecs);

    /// declare matrix memory
    void SetMemA();

    /// factorise matrix depending upon definition stored in mat_form
    void Factor();
    /// evaulate \f$u \leftarrow A^{-1} u\f$ for \a nrhs solves
    void Solve(double *u, int nrhs);

    // Private data access and setting
    /// return private data for _lda
    int get_lda     () { return _lda;}

    /// set private data: lda
    void set_lda    (int val) {_lda = val;}

    /// return private data for lda
    int get_bwidth    () { return _bwidth;}

    /// set private data: bwidth
    void set_bwidth (int val) {_bwidth = val;}

    /// return private data for _ldiag
    int get_ldiag    () { return _ldiag;}

    /// set private data: bwidth
    void set_ldiag (int val) {_ldiag = val;}

    void Assemble(BlkMat& Mat, const int *map, const double* signchg);
    void Assemble(BlkMat& Mat, const int *map, const double* signchg,
      const int nsolve);

    void   EigenValues(const char file[]);
    double MaxEigenValue();
    double L2ConditionNo();
    int    NullSpaceDim (double);

    void   plotmatrix();
    void   WriteMatrix(FILE *fp);
  };

} // end of namespace

#endif
