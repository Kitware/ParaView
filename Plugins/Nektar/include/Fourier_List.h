
class Fourier_List: public Element_List {
 public:
  Fourier_List();
  Fourier_List(Element **hea, int n);

  Element *operator()(int i);
  Element *operator[](int i);

  void Cat_mem();
  void Mem_shift(double *, double *);

  Element_List *gen_aux_field(char ty);
  void Terror(char *string);
  void Trans(Element_List *EL, Nek_Trans_Type ntt);
  void Iprod(Element_List *EL);
  void Grad(Element_List *AL, Element_List *BL, Element_List *CL, char Trip);
  void Grad_d(double *, double *, double *, char Trip);

  // CE107
  void Grad_h(double *, double *, double *, double *, char Trip);

  void Grad_z(Element_List *EL);
  void Grad_z_d(double *);
  void HelmHoltz(Metric *lambda);
  void Set_field(char *string);
  void zerofield();
  void FFT(Element_List *EL, Nek_Trans_Type ntt);

  // CE107
  void FFT32(Element_List *EL, Nek_Trans_Type ntt);

  void Set_state(char type);
};
