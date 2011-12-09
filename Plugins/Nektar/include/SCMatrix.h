#ifndef H_SCMATRIX
#define H_SCMATRIX

#include "GMatrix.h"

#include "hstruct.h" // only required for fill() to be compatible with Nektar

namespace MatSolve {

enum SCStatus {
  Unset,          ///< initialised matrix but not set
  Filled,         ///< Matrix is filled with components
  Condensed       ///< Matrix is statically condensed
};

/**
    \brief Matrix class for statically condensing and inverting matrices.

    The class should be filled with the matrix system:
    \f[    \left [
    \begin{array}{cc} A & B\\
                      D & C
     \end{array} \right ]. \f]

    The functions attached to the class help to evaluate the following
    statically condensed solution proceedure:

    - Starting with a matrix problem where \f$u_c\f$ represents the
    coupled degrees of freedom and \f$u_d\f$ represents the decoupled
    degrees of freedom,i.e
    \f[
    \left [
    \begin{array}{cc} A & B\\
                      D & C
     \end{array} \right ]
    \left [
    \begin{array}{c} u_c\\
                     u_d
     \end{array} \right ]
     =
    \left [
    \begin{array}{c} f_c\\
                     f_d
     \end{array} \right ]\f]
 This can be algrebraically manipulated into the form:
    \f[
    \left [
    \begin{array}{cc} A-BC^{-1}D & 0\\
                      D & C
     \end{array} \right ]
    \left [
    \begin{array}{c} u_c\\
                     u_d
     \end{array} \right ]
     =
    \left [
    \begin{array}{c} f_c - BC^{-1} f_d \\
                     f_d
     \end{array} \right ]\f]

    - Which can then be solved independently for each row in two steps,i.e.
    \f[
      \begin{array}{ccl}
      u_c &=& [A-BC^{-1}D]^{-1} (f_c - BC^{-1} f_d) \\
      u_d &=& C^{-1} f_d - C^{-1}D u_c
      \end{array} \f]

In the spectral/\e hp method each condensed matrix can come from the
\f$k^{th}\f$ elemental contribution. The decoupled matrices \f$ C_k\f$
belong to the interior degrees of freedom. However the coupled
matrices \f$[A-BC^{-1}D]_k\f$ need to be assembled into a global
matrix or kept locally and iteratively solved.

The static condensation routine can also be applied in a multi-level approach.

The matrices are of dimension
  - \a A is [n x n],
  - \a B is [n x m]
  - \a C is [m x m]

Which are  stored in  \e column major form, whilst the matrix
 - \a D is [m x n]
 is stored in \e row major form so that \f$D^T = C\f$ if the matrix is symmetric
\n
*/
 class SCmat{
   friend class SClevel;
 private:
  int  symmetric; ///< Flag to identify if matrix is symmetric. If
      ///< symmetric = 1 \f$ D^T = C\f$;
  int  n;         ///< Dimension of square matrix \a A
  int  m;         ///< Dimension of square matrix \a C
  int  npm;       ///< Size of n plus m;

  double *A; ///< if state = Filled: A \n
             ///< if state = Condensed -> \f$  A-BC^{-1}D   \f$ \n
  double *B; ///< if state = Filled: B\n
             ///< if state = Condensed = 1:\f$ BC^{-1}\f$
  double *C; ///< if state = Filled: C\n
             ///< if state = Condensed: \f$ C^{-1}\f$
  double *D; ///< if state = Filled: D\n
             ///< if state = Condensed = 1: \f$ C^{-1}D\f$\n
             ///< (note D is stored in row major form so if matrix is
             ///< \a symmetric (=1) then \f$D^T\f$ = B)

 public:
  SCStatus state;  ///< Flag to check whether matrix is condensed\n
                  ///<  -- Unset    : only initialised\n
                  ///<  -- Filled   : standard form\n
                  ///<  -- Condensed: condensed \n
  SCmat(){
    state     = Unset;
    symmetric = 0;
    A = NULL;    B = NULL;
    C = NULL;    D = NULL;
  } ///<  Default constructer

    ///  Constructor for a closed packed matrix
  SCmat(const char trans, const int nc, const int nd,
  const int symm, const double *set_A);

  /// Constructor for a general packed matrix
  SCmat(const char trans, const int nc, const int nd,
  const int symm, const double *set_A, const int *Amap);

  ~SCmat();  ///<  Destructor

  // Private data access routines
  /// return private data for n
  int get_n() {return n;}
  /// return private data for npm
  int get_npm() {return npm;}
  /// return private data for symmetric
  int get_symm() {return symmetric;}

  /// setup memory based on nc and nd and the symmetric flag;
  void SetMem(const int nc, const int nd, const int symmetric);

  /// condense matrix system
  void Condense();

  /// Assemble global matrix from local values in A
  void Assemble(Gmat *mat, const int *map, const int store);

  /// Assemble from locally condensed matrix  A  to the new matrix
  /// newmat based on the mapping map[i];
  void Assemble(SCmat& newmat, const int *map, const int offset);

  /// Calculate \a fout \f$ = fin - BC^{-1} fd\f$ where fd[0] = fin[n]
  void CondenseRHS(const double *fin, double *fout);

  /// Calculate \a ud \f$= C^{-1}\f$ \a fd - \f$ C^{-1} D\f$ \a uc;
  void SolveUd(double *fd);

  /// Enforce a signchange on the entries of A depending on the value of sc
  void Signchange_A(const double *sc);

  // delete A matrix once it is not required
  void DeleteA();

  //---Nektar Related functions
  /// Fill SCmat from LocMat matrix system given in Mat
  void Fill(const LocMat& Mat, const int symm);


  /// Setup local mapping from nektar rslv data
  void Setup_LocMap(const int *map1, const  int *map2, const int nmap2,
        int *newmap, const int noffset, const int cstart,
        const int coffset, const int nsolve);

};

} // end of namespace
#endif
