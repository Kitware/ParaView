#ifndef NEKWORK
#include <smart_ptr.hpp>

extern nektar::scoped_c_ptr<double> Tri_wk;

extern double *Quad_wk;
extern double *Quad_Jbwd_wk;
extern double *Quad_Iprod_wk;
extern double *Quad_Grad_wk;
extern double *Quad_HelmHoltz_wk;

extern double *Tet_wk;
extern double *Tet_form_diprod_wk;
extern double *Tet_Jbwd_wk;
extern double *Tet_Iprod_wk;
extern double *Tet_Grad_wk;
extern double *Tet_HelmHoltz_wk;

extern double *Pyr_wk;
extern double *Pyr_form_diprod_wk;
extern double *Pyr_Jbwd_wk;
extern double *Pyr_Iprod_wk;
extern double *Pyr_Grad_wk;
extern double *Pyr_HelmHoltz_wk;
extern double *Pyr_InterpToGaussFace_wk;

extern double *Prism_wk;
extern double *Prism_form_diprod_wk;
extern double *Prism_Jbwd_wk;
extern double *Prism_Iprod_wk;
extern double *Prism_Grad_wk;
extern double *Prism_HelmHoltz_wk;
extern double *Prism_InterpToGaussFace_wk;

extern double *Hex_wk;
extern double *Hex_form_diprod_wk;
extern double *Hex_Jbwd_wk;
extern double *Hex_Iprod_wk;
extern double *Hex_Grad_wk;
extern double *Hex_HelmHoltz_wk;
extern double *Hex_InterpToGaussFace_wk;

#endif
