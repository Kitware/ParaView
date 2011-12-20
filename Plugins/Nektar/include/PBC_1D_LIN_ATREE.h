
#ifndef PBC_1D_LIN_ATREE_H
#define PBC_1D_LIN_ATREE_H


class PBC1DLINATREE {

  public:

  int Nout;               /* number of outlets                                    */
  int Ninl;               /* number of inlets                                     */
  int Nimpedance_modes;   /* number of impedance modes                            */
  int Nsteps_per_cycle;   /* number of time steps per cycle                       */
  int time_step_in_cycle; /* index of time step in currend cycle                  */
  int Imp_index_start;    /* index of element of global array corresponding
                             to the first element in local partition              */
  int Imp_length_local;   /* number of elements of global array to be stored
                             in local partition                                   */
  int parallel_convolution; /* = 1 for parallel convolution, =0 otherwise         */

  double Tperiod;         /* length of cycle - time period                        */
  double **FR_history;    /* Flow-Rate history; dimension[Nout][Nsteps_per_cycle] */
  double **A_cos_sin;     /* Fourier coef. of flow-rate history                   */
  double **impedance;     /* impedance if phys. space;
                                               dimension[Nout][Nsteps_per_cycle]  */
  double *Pressure;       /* values of pressure computed by convolution
                                                   MPIdimension[Nout]                */
  double *Area;           /* area of all outlet and inlet boundaries              */

  char *standard_labels[26]; /* standatd labels for inlets and outlets            */

  int *Nfaces_per_outlet;    /* number of faces at each outlet - local variable   */
  int **ID_faces_per_outlet; /* local indices of faces at each outlet             */

  int *Nfaces_per_inlet;    /* number of faces at each outlet - local variable   */
  int **ID_faces_per_inlet; /* local indices of faces at each outlet             */


  my_dcmplx **impedance_modes; /* values of impedance in fourier space
                                  for each outlet                                 */

  double *R1,*C1,*flowrate_RCR_old;

//  MPI_Comm  *Communicator_inlet,           *Communicator_outlet,
//            *Communicator_inlet_plus_ROOT, *Communicator_outlet_plus_ROOT;

  int *Nnodes_inlet, *Nnodes_outlet;


  int create(Bndry *Ubc);
  void set_geofac(Bndry *Ubc, Bndry *Vbc, Bndry *Wbc);
  void setRC_BC(char *name);
  void resetRC_BC(char *name);
  void read_flowratehistory(char *name);
  void read_flowratehistory_new(char *name);
  void save_flowratehistory(char *name);
  void save_flowratehistory_new(char *name);

  double get_Pval(Bndry *Ubc);
  void update_time_step_in_cycle();
  void update_FR_history(double *flowrate);
  void compute_pressure();
  void compute_pressure_steady(double *flowrate);
  void compute_pressure_RC_nonsteady(double *flowrate);
};

#endif
