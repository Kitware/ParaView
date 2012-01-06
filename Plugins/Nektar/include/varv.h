/* ------------------------------------------------------------------------- *
 * The following functions calculate the Large Eddy Simulation additions     *
 * for the Smagorinsky modelling.                                            *
 *                                                                           *
 * RCS Information                                                           *
 * ---------------
 * $Author:
 * $Date:
 * $Source:
 * $Revision:
 * ------------------------------------------------------------------------- */

/*   This file contains the structures necessary for the les.c  */

typedef struct varv_variables {
double *NLx ;
double *NLy ;
double *NLz ;
} varv_variables ;


varv_variables *CsCalc(Domain *omega);
