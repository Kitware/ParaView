#ifndef H_STOKES_SLV
#define H_STOKES_SLV

#include <hotel.h>
#include "GMatrix.h"
#include "SClevel.h"

typedef struct dom Domain;

namespace MatSolve{


  class StokesMatrix {
  public:
    StokesMatrix(){};                      // default constructor

    ~StokesMatrix(){                       // Destructor
      delete Ainv;
    };

    void GenMat     (Element_List *U, Element_List *P, Bsystem *Ubsys,
         Bsystem *Pbsys, Metric *lambda, double beta);
    void GenMat     (Element_List *U, Element_List *P, Bsystem *Ubsys,
         Bsystem *Pbsys, Metric *lambda, double beta, bool RealMode);
    void Solve      (Domain *, int mode);
    void SolveReal  (Domain *, int mode);
    void SolveSC    (double *f);

    void ProjectStokesReal    (LocMat *helm, LocMatDiv *div,
             int eid, Bsystem *Ubsys);

    void Project    (LocMat *pse1, LocMat *pse2, LocMat *pse3,
         LocMat *pse4, LocMat *helm, LocMatDiv *div,
         int eid, Bsystem *Ubsys);

    void Project_Beta_Real(LocMat *helm, LocMatDiv *bet, int eid, Bsystem *Ubsys);

    void Project_Beta(LocMat *helm, LocMatDiv *bet, int eid, Bsystem *Ubsys);

    void Project_CT(LocMat *amat, LocMat *bmat, LocMat *helm, int psize,
        int eid, Bsystem *Ubsys);

    void Project_CT2(LocMat *Wxmat, LocMat *Wymat, LocMatDiv *bet,
         LocMat *helm, int eid, Bsystem *Ubsys);

    void Project_L1(LocMat *amat, LocMat *bmat, LocMat *cmat,
        LocMat *helm, LocMatDiv *bet,
        int eid, Bsystem *Ubsys);
    void PSE_Operator(Element_List *U,   Element_List *V,
          Element_List *W,   Element_List *Ui,
          Element_List *Vi,  Element_List *Wi,
          Element_List *Pin, Element_List *Pini,
          Metric *lambda,    double kinvis);


    void PSEForce(Domain *Omega);
    void L2_Operator(Element_List *U,   Element_List *V,
         Element_List *W,   Element_List *Ui,
         Element_List *Vi,  Element_List *Wi,
         Element_List *Pin, Element_List *Pini,
         Metric *lambda,    double kinvis);

    void L2_Operator(Domain *Omega);

    void Set_locmap_real (int *map,int nsolve, Element_List *U);
    void Set_locmap (int *map,int nsolve, Element_List *U);

    void Set_SignChange (Element_List *U, Bsystem *Ubsys, bool RealMode);
    void Stokes_Operator(Element_List *U, Element_List *V,
       Element_List *W, Element_List *P,
       Metric *lambda,  double kinvis);
    void Stokes3D_Operator(Element_List *U,   Element_List *V,
         Element_List *W,   Element_List *Ui,
         Element_List *Vi,  Element_List *Wi,
         Element_List *Pin, Element_List *Pini,
         Metric *lambda,    double kinvis, double real);
    void Stokes3D_Operator_Real(Element_List *U,   Element_List *V,
        Element_List *W,   Element_List *Pin,
        Metric *lambda,    double kinvis, double real);

  private:
    SClevel *SClev;
    Gmat    *Ainv;

  };
} // end of namespace
#endif
