#ifndef H_DG_HELM
#define H_DG_HELM

#include <hotel.h>
#include "GMatrix.h"
#include "SClevel.h"

namespace MatSolve{

enum Btype  {
  Orthogonal,
  Modified
};

class DGMatrix {
 public:
   DGMatrix(){};                      // default constructor
   DGMatrix(Btype val){btype = val;}; // default constructor

  ~DGMatrix(){                        // Destructor
    delete Ainv;
    delete[] gmap[0];
    delete[] gmap;
  };

  void GenMat(Element_List *U, Element_List *Uf, Bndry *Ubc, Bsystem *Ubsys,
        double lambda);
  void Solve   (Element_List *U, Element_List *Uf, Bndry *Ubc, Bsystem *Ubsys);
  void SolveSC (double *f);

  void HelmHoltz(Element_List *U, Element_List *Uf);
  void Project_Helm(BlkMat& Helm, SClevel& SClev, const Element_List& U);


 private:
  Btype btype;
  double DGLambda;
  int **gmap;    // mapping array of solved elements;
  int nsolve;
  int nbsolve;

  double fluxfac; // scheme token fluxfac = 0 is Bassi Rebay
                  // fluxfac = 0.5 is LDG;
  SClevel *SClev;
  Gmat    *Ainv;
  void set_gmap(const Element_List *U, const Bndry *Ubc, const Bsystem *Ubsys);
};
} // end of namespace
#endif
