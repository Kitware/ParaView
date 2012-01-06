#include <TransF77.h>

extern "C"
{
#
#if defined  (_CRAY)  || defined (__blrts__)  || defined (__bg__)   /* Crays  BlueGene*/
#
  // C++ needs prototypes


 // void FGCURV  (const int&, const int&, double * , double& ,
//    double *, int &);
  void FGSURF  (const int&, const int&, const int&, double *, double &,
    double &, double *, int&);

  static void fgcurv(const int& ider, const int & nu, double *r,
                     double& un, double *rp, int& ier)


  // void settolg (double *);
  void SETTOLG (double &);


  void LOCUV   (const int&, double *, double *, const int&,
    const int&, double *);
  void FINDMIN (const int& np, double *xc, double *zw,
    const int& nu, const int& nv, double *r);

  static void fgcurv(const int& ider, const int & nu, double *r,
         double& un, double *rp, int& ier) {
    FGCURV(ider,nu,r,un,rp,ier);
  }

  static void fgsurf(const int& ider,const int& nu,const int& nv,
         double *r, double& un, double& vn,
         double *rp, int& ier){
    FGSURF (ider,nu,nv,r,un,vn,rp,ier);
  }
/*
  static void settolg(double& tol){
    SETTOLG (tol);
  }
*/

  static void  locuv( const int& np, double *xg, double *xl,
          const int& nu, const int& nv, double *r ){
    LOCUV (np,xg,xl,nu,nv,r);
  }

#
# else
#
  // C++ needs prototypes

  void F77NAME(fgcurv) (const int&, const int&, double * , double& ,
      double *, int &);
  void F77NAME(fgsurf) (const int&, const int&, const int&, double *, double &,
      double &, double *, int&);
  void F77NAME(settolg) (double &);
  void F77NAME(locuv)   (const int&, double *, double *, const int&,
       const int&, double *);
  void F77NAME(findmin) (const int& np, double *xc, double *zw,
       const int& nu, const int& nv, double *r);


#if !(defined  (__blrts__) || defined (__bg__))

  static void fgcurv(const int& ider, const int & nu, double *r,
         double& un, double *rp, int& ier) {
    F77NAME(fgcurv) (ider,nu,r,un,rp,ier);
  }

  static void fgsurf(const int& ider,const int& nu,const int& nv,
         double *r, double& un, double& vn,
         double *rp, int& ier){
    F77NAME(fgsurf) (ider,nu,nv,r,un,vn,rp,ier);
  }

  static void settolg(double& tol){
    F77NAME(settolg) (tol);
  }

  static void  locuv( const int& np, double *xg, double *xl,
          const int& nu, const int& nv, double *r ){
    F77NAME(locuv) (np,xg,xl,nu,nv,r);
  }
#endif

#endif

}
