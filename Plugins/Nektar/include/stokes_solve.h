#ifndef H_STOKES_SLV
#define H_STOKES_SLV

#include <hotel.h>
#include "GMatrix.h"
#include "SClevel.h"
//#include "nektar.h"

typedef struct dom Domain;

namespace MatSolve{

  class StokesMatrix {
  public:
    StokesMatrix(){};                      // default constructor

    ~StokesMatrix(){                       // Destructor
      delete Ainv;
    };

    void GenMat     (Element_List *U, Element_List *P, Bsystem *Ubsys,
         Bsystem *Pbsys, Metric *lambda);
    void Solve      (Domain *);
    void SolveSC    (double *f);

    void Project    (LocMat *pse1, LocMat *pse2, LocMat *pse3,
         LocMat *pse4, LocMat *helm, LocMatDiv *div,
         int eid, Bsystem *Ubsys);


    void Set_locmap (int *map,int nsolve);
    void Set_SignChange (Element_List *U, Bsystem *Ubsys);
    void Stokes_Operator(Element_List *U, Element_List *V,
       Element_List *P,Metric *lambda, double kinvis);

  private:
    SClevel *SClev;
    Gmat    *Ainv;

  };
} // end of namespace
#endif
