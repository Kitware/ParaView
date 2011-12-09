#ifndef POD_INIT_H
#define POD_INIT_H


void  Set_Pod_Accelerator(Domain *Omega);
void Tet_Jfwd_bndr(Element *E, double *invm);

void force_C_zero(Element_List *E, Bsystem *Usys);


#endif
