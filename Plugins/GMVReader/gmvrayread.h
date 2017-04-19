#ifndef _GMVRAYREADH_
#define _GMVRAYREADH_

#ifndef RDATA_INIT
#define EXTERN extern
#else
#define EXTERN /**/
#endif

/*  Keyword types.  */
#define RAYS        1
#define RAYIDS      2
#define RAYEND     51

#define NRAYVARS 20

struct gmvray
         {
          int    npts;
          double *x;
          double *y;
          double *z;
          double *field[NRAYVARS];
         };

EXTERN struct gmvray_data_type
         {
          int     nrays;    /*  Number of rays in the file.  */
          int     nvars;    /*  Number of ray variable fields.  */
          char    *varnames;  /*  Ray variable names.  */
          short   vartype[NRAYVARS];   /*  Variable types.  */
                                       /*  0 = nodes, 1 = line segments.  */
          int     *rayids;
          struct gmvray *gmvrays;
         } 
     gmvray_data;

/*  C, C++ prototypes.  */

int gmvrayread_open(char *filnam);

void gmvrayread_close(void);

void gmvrayread_data(void);

#endif
