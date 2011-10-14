#ifndef PREDICTOR_H
#define PREDICTOR_H


class PREDICTOR{


public:
    int ORDER;                 /* Number of Snapshots */
    int Npoints;    /* Number of points    */


    double **DATA;          /* field values dimension: [ORDER][Npoints]*/
    double *extarpolation_coefficients;

    void allocate_data(int Np, int Order);  /* initialization */
    void compute_extarpolation_coefficients(); /* assume we extrapolate one time step only */
    void push_field(double *field);
    void shift_DATA();      /* advance the fields one time step */
    void extrapolate_next_step();  /* extrapolate field one time step forward */
    void force_C_zero(Element_List *U,  Bsystem *Bsys);
};
#endif
