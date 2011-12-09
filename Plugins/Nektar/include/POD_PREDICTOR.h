#ifndef POD_PREDICTOR_H
#define POD_PREDICTOR_H


class POD_PREDICTOR{

public:
    int POD_ORDER;    /* Number of Snapshots */
    int Npoints;    /* Number of points    */
    int POD_MAX_MODE4RECONSTRUCTION;  /* Max mode number used to reconstruct the field */

    double **POD_DATA;  /* field values dimension: [POD_ORDER][Npoints]*/
    double *jacobian_weights;  /* pointwise values of jacobian multiplied by intergtation weights */

    double **CORR_MATRIX;    /* correlation matrix */
    double **TEMP_MODES;        /* POD temporal modes */
    double **SPAT_MODES;    /* POD spatial  modes */
    double *EIG_VALUES;         /* eigenvalues of correlation matrix */

    double *extarpolation_coefficients;

    FILE  *EIG_DATA_FILE;
    char  EIG_DATA_FILE_name[128];

   /**************************/

    double **inv_mass;          /* inverse of local mass matrix;  */
                                /* dimension [Nelements][(# of boundary modes)(# of modes)] */

   /**************************/


    void allocate_data(int Np, int Order);  /* initialization */
    void compute_extarpolation_coefficients(); /* assume we extrapolate one time step only */

    void shift_POD_DATA();      /* advance the fields one time step */
    void shift_CORR_MATRIX();   /* advance the correlation matrix one time step */
    void push_field(double *field);
    void update_CORR_MATRIX();  /* compute first row and column of the correlation matrix */
    void update_CORR_MATRIX_temp();
    void compute_TEMP_MODES();  /* compute temporal modes */
    void compute_SPAT_MODES();  /* compute spatial modes */
    void reconstruct_field();
    void POD_PREDICTOR::reconstruct_field_test_accuracy();
    void extrapolate_next_step();  /* extrapolate field one time step forward */
    void extrapolate_next_step_noPOD();
    void extrapolate_next_step_POD_filter(int FILTER_ORDER);
    void dump_field();
    void dump_extrap_field();
    void dump_CORR_MAT();
    void dump_SPAT_MODES();
    void dump_TEMP_MODES();
    void save_eig_data();
};
#endif
