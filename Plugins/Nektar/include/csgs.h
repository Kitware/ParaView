#ifndef csgs_h
#define csgs_h

#ifdef PARALLEL

// user headers
#include "mpi.h"

#ifdef __cplusplus
extern "C" {
#endif

struct csgs_impl;
typedef struct csgs_impl* CSGS;

CSGS CSGS_init(const int* elms, int num_elms, const int* part, int num_part,
        const int* dist, MPI_Comm comm);
void CSGS_free(CSGS csgs);

void CSGS_post_recv(CSGS csgs);

void CSGS_plus(CSGS csgs, double* val);
void CSGS_fabs_max(CSGS csgs, double* val);

int CSGS_rank(CSGS csgs);
int CSGS_size(CSGS csgs);
int CSGS_comm_size(CSGS csgs);

#ifdef __cplusplus
}
#endif


#endif

#endif
