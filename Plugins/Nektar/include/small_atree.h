#ifndef SMALL_VESSEL_H
#define SMALL_VESSEL_H


#define Lrr_small_atree  50.0
#define Rmin_small_atree (0.02)
#define k1_small_atree   (2.0E+7)
#define k2_small_atree   (-22.53)
#define k3_small_atree   (8.65E+5)
#define ni_small_atree   0.03525
#define omega_small_atree (2.0*M_PI/0.699148936170213)
#define density_small_atree 1.0


class my_dcmplx{

 public:
    double real,imag;

    void set_val(double re, double im);
    void my_dcmplx_axpy(double a, my_dcmplx x, my_dcmplx y);
    void my_dcmplx_axpy(double a, my_dcmplx x, double b, double y);
    void my_dcmplx_dot(my_dcmplx x, my_dcmplx y);
    void my_dcmplx_inv_dot(my_dcmplx x, my_dcmplx y);
    void my_dcmplx_sin(my_dcmplx x);
    void my_dcmplx_cos(my_dcmplx x);
    void my_dcmplx_sqrt(my_dcmplx x);
    void my_dcmplx_conj(my_dcmplx x);

};



class SMALL_VESSEL{

 public:
  double  radius;    /* radius of vessel        */
  double  Length;    /* length of vessel        */
  my_dcmplx* Zo;       /* Impedance at the inlet  */
  my_dcmplx* Zl;       /* Impedance at the outlet */
  char type;         /* type = T if this is a terminal vessel
                   = C if this is connecting vessel */
  double Compliance; /* Complines */
  double Ws;         /* Womersley number */
  my_dcmplx* Fj;

  SMALL_VESSEL* child_1;
  SMALL_VESSEL* child_2;

  SMALL_VESSEL(double ro);

  void create_small_vessel(double ro);

  my_dcmplx get_ZL(int mode);
  my_dcmplx get_Z0(int mode);

};


#endif
