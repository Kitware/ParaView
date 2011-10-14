#ifndef H_MATSOLVE
#define H_MATSOLVE

#include "Vmath.h"
#include "Blas.h"
#include "GMatrix.h"
#include "SClevel.h"

using namespace std;

namespace MatSolve{

  enum NekMatSType  {
    Direct,
    Iterative
  };

  class NekMat {
  private:
    SClevel *SClev;         ///< Statically condense matrix level data.
    Gmat    *Ainv;          ///< Local inverse of SC matrix for direct solver
    int     SClevels;       ///< Number of levels of static condensation.
    NekMatSType SlvType;    ///< Iterative of direct identifier

  public:
    //Constructors
    NekMat(){}                               ///< Defaultx Constructor
    ///< Coonstructor from Element_List
    ~NekMat();

  /// Setup Matrix system using nektar data
  void SetupSC(Element_List& U, Bsystem *B,  Bndry *Ubc, SolveType Stype);

  /// full solve call using nektar structures
  void Solve(Element_List& U, Element_List& Uf, Bndry& Ubc,
       Bsystem& Ubsys, SolveType Stype);

  /// set up rhs
  void SetRHS(Element_List& U, Element_List& Uf, Bndry& Ubc,
        Metric& lambda, SolveType Stype);

  /// solve Au=f where f is in local storage format
  void Solve(double *f);

  /// find the bandwidth
  int bandwidth(const int lev);

  void Pack_BndInt  (double *f,double *fs);
  void UnPack_BndInt(double *f,double *fs);
};

}
#endif
