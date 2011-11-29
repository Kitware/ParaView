/********************************gs.h**************************************


*********************************gs.h*************************************/

#ifndef _gs_h
#define _gs_h

typedef struct gather_scatter_id *gs_ADT;

gs_ADT gs_init(int *elms, int nel, int level);
void   gs_gop(gs_ADT gs_handle, double *vals, char *operation);
void   gs_gop(gs_ADT gs_handle, double *vals, char operation[]);
void   gs_print_template(gs_ADT gs_handle, int who);
void   gs_free(gs_ADT gs_handle);
#endif
