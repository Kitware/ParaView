/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*keep this declaration near the top of this file -RPM*/
static const char *FileHeader = "\n\
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n\
 * Copyright by the Board of Trustees of the University of Illinois.         *\n\
 * All rights reserved.                                                      *\n\
 *                                                                           *\n\
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *\n\
 * terms governing use, modification, and redistribution, is contained in    *\n\
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *\n\
 * of the source code distribution tree; Copyright.html can be found at the  *\n\
 * root level of an installed copy of the electronic HDF5 document set and   *\n\
 * is linked from the top-level documents page.  It can also be found at     *\n\
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *\n\
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *\n\
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *";
/*
 *
 * Created:	H5detect.c
 *		10 Aug 1997
 *		Robb Matzke
 *
 * Purpose:	This code was borrowed heavily from the `detect.c'
 *		program in the AIO distribution from Lawrence
 *		Livermore National Laboratory.
 *
 *		Detects machine byte order and floating point
 *		format and generates a C source file (native.c)
 *		to describe those paramters.
 *
 * Assumptions: We have an ANSI compiler.  We're on a Unix like
 *		system or configure has detected those Unix
 *		features which aren't available.  We're not
 *		running on a Vax or other machine with mixed
 *		endianess.
 *		
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#undef NDEBUG
#include "H5private.h"
#include "H5Tpublic.h"
#include "H5Rpublic.h"

#define MAXDETECT 64
/*
 * This structure holds information about a type that
 * was detected.
 */
typedef struct detected_t {
    const char		*varname;
    int			size;		/*total byte size		*/
    int			precision;	/*meaningful bits		*/
    int			offset;		/*bit offset to meaningful bits	*/
    int			perm[32];	/*byte order			*/
    int			sign;		/*location of sign bit		*/
    int			mpos, msize, imp;/*information about mantissa	*/
    int			epos, esize;	/*information about exponent	*/
    unsigned long	bias;		/*exponent bias for floating pt.*/
    size_t		align;		/*required byte alignment	*/
    size_t		comp_align;	/*alignment for structure       */
} detected_t;

/* This structure holds structure alignment for pointers, hvl_t, hobj_ref_t, 
 * hdset_reg_ref_t */
typedef struct malign_t {
    const char          *name;      
    size_t              comp_align;         /*alignment for structure   */
} malign_t;
   
static void print_results(int nd, detected_t *d, int na, malign_t *m);
static void iprint(detected_t *);
static int byte_cmp(int, void *, void *);
static int bit_cmp(int, int *, void *, void *);
static void fix_order(int, int, int, int *, const char **);
static int imp_bit(int, int *, void *, void *);
static unsigned long find_bias(int, int, int *, void *);
static void precision (detected_t*);
static void print_header(void);
static size_t align_g[] = {1, 2, 4, 8, 16};
static jmp_buf jbuf_g;


/*-------------------------------------------------------------------------
 * Function:	precision
 *
 * Purpose:	Determine the precision and offset.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		Thursday, June 18, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
precision (detected_t *d)
{
    int		n;
    
    if (0==d->msize) {
	/*
	 * An integer.	The permutation can have negative values at the
	 * beginning or end which represent padding of bytes.  We must adjust
	 * the precision and offset accordingly.
	 */
	if (d->perm[0] < 0) {
	    /*
	     * Lower addresses are padded.
	     */
	    for (n=0; n<d->size && d->perm[n]<0; n++) /*void*/;
	    d->precision = 8*(d->size-n);
	    d->offset = 0;
	} else if (d->perm[d->size - 1] < 0) {
	    /*
	     * Higher addresses are padded.
	     */
	    for (n=0; n<d->size && d->perm[d->size-(n+1)]; n++) /*void*/;
	    d->precision = 8*(d->size-n);
	    d->offset = 8*n;
	} else {
	    /*
	     * No padding.
	     */
	    d->precision = 8*d->size;
	    d->offset = 0;
	}
    } else {
	/* A floating point */
	d->offset = MIN3 (d->mpos, d->epos, d->sign);
	d->precision = d->msize + d->esize + 1;
    }
}



/*-------------------------------------------------------------------------
 * Function:	DETECT_I
 *
 * Purpose:	This macro takes a type like `int' and a base name like
 *		`nati' and detects the byte order.  The VAR is used to
 *		construct the names of the C variables defined.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 12 1996
 *
 * Modifications:
 *
 *	Robb Matzke, 4 Nov 1996
 *	The INFO.perm now contains `-1' for bytes that aren't used and
 *	are always zero.  This happens on the Cray for `short' where
 *	sizeof(short) is 8, but only the low-order 4 bytes are ever used.
 *
 *	Robb Matzke, 4 Nov 1996
 *	Added a `padding' field to indicate how many zero bytes appear to
 *	the left (N) or right (-N) of the value.
 *
 *	Robb Matzke, 5 Nov 1996
 *	Removed HFILE and CFILE arguments.
 *
 *-------------------------------------------------------------------------
 */
#define DETECT_I(TYPE,VAR,INFO) {					      \
   TYPE _v;								      \
   int _i, _j;								      \
   unsigned char *_x;							      \
   memset (&INFO, 0, sizeof(INFO));					      \
   INFO.varname = #VAR;							      \
   INFO.size = sizeof(TYPE);						      \
   for (_i=sizeof(TYPE),_v=0; _i>0; --_i) _v = (_v<<8) + _i;		      \
   for (_i=0,_x=(unsigned char *)&_v; _i<(signed)sizeof(TYPE); _i++) {	      \
      _j = (*_x++)-1;							      \
      assert (_j<(signed)sizeof(TYPE));					      \
      INFO.perm[_i] = _j;						      \
   }									      \
   INFO.sign = ('U'!=*(#VAR));						      \
   precision (&(INFO));							      \
   ALIGNMENT(TYPE, INFO);						      \
   if(!strcmp(INFO.varname, "SCHAR")  || !strcmp(INFO.varname, "SHORT") ||    \
      !strcmp(INFO.varname, "INT")   || !strcmp(INFO.varname, "LONG")  ||     \
      !strcmp(INFO.varname, "LLONG")) {                                       \
      COMP_ALIGNMENT(TYPE,INFO.comp_align);                                   \
   }                                                                          \
}

/*-------------------------------------------------------------------------
 * Function:	DETECT_F
 *
 * Purpose:	This macro takes a floating point type like `double' and
 *		a base name like `natd' and detects byte order, mantissa
 *		location, exponent location, sign bit location, presence or
 *		absence of implicit mantissa bit, and exponent bias and
 *		initializes a detected_t structure with those properties.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 12 1996
 *
 * Modifications:
 *
 *	Robb Matzke, 14 Aug 1996
 *	The byte order detection has been changed because on the Cray
 *	the last pass causes a rounding to occur that causes the least
 *	significant mantissa byte to change unexpectedly.
 *
 *	Robb Matzke, 5 Nov 1996
 *	Removed HFILE and CFILE arguments.
 *-------------------------------------------------------------------------
 */
#define DETECT_F(TYPE,VAR,INFO) {					      \
   TYPE _v1, _v2, _v3;							      \
   int _i, _j, _first=(-1), _last=(-1);					      \
   char *_mesg;								      \
									      \
   memset (&INFO, 0, sizeof(INFO));					      \
   INFO.varname = #VAR;							      \
   INFO.size = sizeof(TYPE);						      \
									      \
   /* Completely initialize temporary variables, in case the bits used in */  \
   /* the type take less space than the number of bits used to store the type */  \
   memset(&_v3,0,sizeof(TYPE));                                               \
   memset(&_v2,0,sizeof(TYPE));                                               \
   memset(&_v1,0,sizeof(TYPE));                                               \
									      \
   /* Byte Order */							      \
   for (_i=0,_v1=0.0,_v2=1.0; _i<(signed)sizeof(TYPE); _i++) {		      \
      _v3 = _v1; _v1 += _v2; _v2 /= 256.0;				      \
      if ((_j=byte_cmp(sizeof(TYPE), &_v3, &_v1))>=0) {			      \
	 if (0==_i || INFO.perm[_i-1]!=_j) {				      \
	    INFO.perm[_i] = _j;						      \
	    _last = _i;							      \
	    if (_first<0) _first = _i;					      \
	 }								      \
      }									      \
   }									      \
   fix_order (sizeof(TYPE), _first, _last, INFO.perm, (const char**)&_mesg);  \
									      \
   /* Implicit mantissa bit */						      \
   _v1 = 0.5;								      \
   _v2 = 1.0;								      \
   INFO.imp = imp_bit (sizeof(TYPE), INFO.perm, &_v1, &_v2);		      \
									      \
   /* Sign bit */							      \
   _v1 = 1.0;								      \
   _v2 = -1.0;								      \
   INFO.sign = bit_cmp (sizeof(TYPE), INFO.perm, &_v1, &_v2);		      \
									      \
   /* Mantissa */							      \
   INFO.mpos = 0;							      \
									      \
   _v1 = 1.0;								      \
   _v2 = 1.5;								      \
   INFO.msize = bit_cmp (sizeof(TYPE), INFO.perm, &_v1, &_v2);		      \
   INFO.msize += 1 + (INFO.imp?0:1) - INFO.mpos;			      \
									      \
   /* Exponent */							      \
   INFO.epos = INFO.mpos + INFO.msize;					      \
									      \
   INFO.esize = INFO.sign - INFO.epos;					      \
									      \
   _v1 = 1.0;								      \
   INFO.bias = find_bias (INFO.epos, INFO.esize, INFO.perm, &_v1);	      \
   precision (&(INFO));							      \
   ALIGNMENT(TYPE, INFO);						      \
   if(!strcmp(INFO.varname, "FLOAT") || !strcmp(INFO.varname, "DOUBLE") ||    \
      !strcmp(INFO.varname, "LDOUBLE")) {                                     \
      COMP_ALIGNMENT(TYPE,INFO.comp_align);                                   \
   }                                                                          \
}


/*-------------------------------------------------------------------------
 * Function:	DETECT_M
 *
 * Purpose:	This macro takes only miscellaneous structures or pointer
 *              (pointer, hvl_t, hobj_ref_t, hdset_reg_ref_t).  It  
 *		constructs the names and decides the alignment in structure.
 *
 * Return:	void
 *
 * Programmer:	Raymond Lu
 *		slu@ncsa.uiuc.edu
 *		Dec 9, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#define DETECT_M(TYPE,VAR,INFO) {					      \
   INFO.name = #VAR;							      \
   COMP_ALIGNMENT(TYPE, INFO.comp_align);				      \
}

/* Detect alignment for C structure */
#define COMP_ALIGNMENT(TYPE,COMP_ALIGN) {			              \
    struct {                                                                  \
        char    c;                                                            \
        TYPE    x;                                                            \
    } s;                                                                      \
                                                                              \
    COMP_ALIGN = (size_t)((char*)(&(s.x)) - (char*)(&s));                     \
}

#if defined(H5_HAVE_LONGJMP) && defined(H5_HAVE_SIGNAL)
#define ALIGNMENT(TYPE,INFO) {						      \
    char		*volatile _buf=NULL;				      \
    volatile TYPE	_val=1;						      \
    volatile TYPE	_val2;						      \
    volatile size_t	_ano=0;						      \
    void		(*_handler)(int) = signal(SIGBUS, sigbus_handler);    \
    void		(*_handler2)(int) = signal(SIGSEGV, sigsegv_handler);	\
									      \
    _buf = malloc(sizeof(TYPE)+align_g[NELMTS(align_g)-1]);		      \
    if (setjmp(jbuf_g)) _ano++;						      \
    if (_ano<NELMTS(align_g)) {						      \
	*((TYPE*)(_buf+align_g[_ano])) = _val; /*possible SIGBUS or SEGSEGV*/	\
	_val2 = *((TYPE*)(_buf+align_g[_ano]));	/*possible SIGBUS or SEGSEGV*/	\
	/* Cray Check: This section helps detect alignment on Cray's */	      \
        /*              vector machines (like the SV1) which mask off */      \
	/*              pointer values when pointing to non-word aligned */   \
	/*              locations with pointers that are supposed to be */    \
	/*              word aligned. -QAK */                                 \
	memset(_buf, 0xff, sizeof(TYPE)+align_g[NELMTS(align_g)-1]);	      \
	if(INFO.perm[0]) /* Big-Endian */				      \
	    memcpy(_buf+align_g[_ano]+(INFO.size-((INFO.offset+INFO.precision)/8)),((char *)&_val)+(INFO.size-((INFO.offset+INFO.precision)/8)),(size_t)(INFO.precision/8)); \
	else /* Little-Endian */					      \
	    memcpy(_buf+align_g[_ano]+(INFO.offset/8),((char *)&_val)+(INFO.offset/8),(size_t)(INFO.precision/8)); \
	_val2 = *((TYPE*)(_buf+align_g[_ano]));				      \
	if(_val!=_val2)							      \
	    longjmp(jbuf_g, 1);						      \
	/* End Cray Check */						      \
	(INFO.align)=align_g[_ano];					      \
    } else {								      \
	(INFO.align)=0;							      \
	fprintf(stderr, "unable to calculate alignment for %s\n", #TYPE);     \
    }									      \
    free(_buf);								      \
    signal(SIGBUS, _handler); /*restore original handler*/		      \
    signal(SIGSEGV, _handler2); /*restore original handler*/		      \
}
#else
#define ALIGNMENT(TYPE,INFO) (INFO.align)=0
#endif

#if 0
#if defined(H5_HAVE_FORK) && defined(H5_HAVE_WAITPID)
#define ALIGNMENT(TYPE,INFO) {						      \
    char	*_buf;							      \
    TYPE	_val=0;							      \
    size_t	_ano;							      \
    pid_t	_child;							      \
    int		_status;						      \
									      \
    srand((unsigned int)_val); /*suppress "set but unused" warning*/	      \
    for (_ano=0; _ano<NELMTS(align_g); _ano++) {			      \
	fflush(stdout);							      \
	fflush(stderr);							      \
	if (0==(_child=fork())) {					      \
	    _buf = malloc(sizeof(TYPE)+align_g[NELMTS(align_g)-1]);	      \
	    *((TYPE*)(_buf+align_g[_ano])) = _val;			      \
	    _val = *((TYPE*)(_buf+align_g[_ano]));			      \
	    free(_buf);							      \
	    exit(0);							      \
	} else if (_child<0) {						      \
	    perror("fork");						      \
	    exit(1);							      \
	}								      \
	if (waitpid(_child, &_status, 0)<0) {				      \
	    perror("waitpid");						      \
	    exit(1);							      \
	}								      \
	if (WIFEXITED(_status) && 0==WEXITSTATUS(_status)) {		      \
	    INFO.align=align_g[_ano];					      \
	    break;							      \
	}								      \
	if (WIFSIGNALED(_status) && SIGBUS==WTERMSIG(_status)) {	      \
	    continue;							      \
	}								      \
	_ano=NELMTS(align_g);						      \
	break;								      \
    }									      \
    if (_ano>=NELMTS(align_g)) {					      \
	INFO.align=0;							      \
	fprintf(stderr, "unable to calculate alignment for %s\n", #TYPE);     \
    }									      \
}
#else
#define ALIGNMENT(TYPE,INFO) (INFO.align)=0
#endif
#endif


/*-------------------------------------------------------------------------
 * Function:	sigsegv_handler
 *
 * Purpose:	Handler for SIGSEGV. We use signal() instead of sigaction()
 *		because it's more portable to non-Posix systems. Although
 *		it's not nearly as nice to work with, it does the job for
 *		this simple stuff.
 *
 * Return:	Returns via longjmp to jbuf_g.
 *
 * Programmer:	Robb Matzke
 *		Thursday, March 18, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
sigsegv_handler(int UNUSED signo)
{
    signal(SIGSEGV, sigsegv_handler);
    longjmp(jbuf_g, 1);
}
    

/*-------------------------------------------------------------------------
 * Function:	sigbus_handler
 *
 * Purpose:	Handler for SIGBUS. We use signal() instead of sigaction()
 *		because it's more portable to non-Posix systems. Although
 *		it's not nearly as nice to work with, it does the job for
 *		this simple stuff.
 *
 * Return:	Returns via longjmp to jbuf_g.
 *
 * Programmer:	Robb Matzke
 *		Thursday, March 18, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
sigbus_handler(int UNUSED signo)
{
    signal(SIGBUS, sigbus_handler);
    longjmp(jbuf_g, 1);
}
    

/*-------------------------------------------------------------------------
 * Function:	print_results
 *
 * Purpose:	Prints information about the detected data types.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 14, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
print_results(int nd, detected_t *d, int na, malign_t *misc_align)
{

    int		i, j;

    /* Include files */
    printf("\
#define H5T_PACKAGE /*suppress error about including H5Tpkg.h*/\n\
#define PABLO_MASK	H5Tinit_mask\n\
\n\
#include \"H5private.h\"\n\
#include \"H5Iprivate.h\"\n\
#include \"H5Eprivate.h\"\n\
#include \"H5FLprivate.h\"\n\
#include \"H5MMprivate.h\"\n\
#include \"H5Tpkg.h\"\n\
\n\
static int interface_initialize_g = 0;\n\
#define INTERFACE_INIT NULL\n\
\n\
/* Declare external the free list for H5T_t's */\n\
H5FL_EXTERN(H5T_t);\n\
\n\
\n");

    /* The interface termination function */
    printf("\n\
int\n\
H5TN_term_interface(void)\n\
{\n\
    interface_initialize_g = 0;\n\
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TN_term_interface);\n\
    \n\
    FUNC_LEAVE_NOAPI(0);\n\
}\n");

    /* The interface initialization function */
    printf("\n\
herr_t\n\
H5TN_init_interface(void)\n\
{\n\
    H5T_t	*dt = NULL;\n\
    herr_t	ret_value = SUCCEED;\n\
\n\
    FUNC_ENTER_NOAPI(H5TN_init_interface, FAIL);\n");

    for (i = 0; i < nd; i++) {

	/* Print a comment to describe this section of definitions. */
	printf("\n   /*\n");
	iprint(d+i);
	printf("    */\n");

	/* The part common to fixed and floating types */
	printf("\
    if (NULL==(dt = H5FL_CALLOC (H5T_t)))\n\
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,\"memory allocation failed\");\n\
    dt->state = H5T_STATE_IMMUTABLE;\n\
    dt->ent.header = HADDR_UNDEF;\n\
    dt->type = H5T_%s;\n\
    dt->size = %d;\n\
    dt->u.atomic.order = H5T_ORDER_%s;\n\
    dt->u.atomic.offset = %d;\n\
    dt->u.atomic.prec = %d;\n\
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;\n\
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;\n",
	       d[i].msize ? "FLOAT" : "INTEGER",/*class			*/
	       d[i].size,			/*size			*/
	       d[i].perm[0] ? "BE" : "LE",	/*byte order		*/
	       d[i].offset,			/*offset		*/
	       d[i].precision);			/*precision		*/

	if (0 == d[i].msize) {
	    /* The part unique to fixed point types */
	    printf("\
    dt->u.atomic.u.i.sign = H5T_SGN_%s;\n",
		   d[i].sign ? "2" : "NONE");
	} else {
	    /* The part unique to floating point types */
	    printf("\
    dt->u.atomic.u.f.sign = %d;\n\
    dt->u.atomic.u.f.epos = %d;\n\
    dt->u.atomic.u.f.esize = %d;\n\
    dt->u.atomic.u.f.ebias = 0x%08lx;\n\
    dt->u.atomic.u.f.mpos = %d;\n\
    dt->u.atomic.u.f.msize = %d;\n\
    dt->u.atomic.u.f.norm = H5T_NORM_%s;\n\
    dt->u.atomic.u.f.pad = H5T_PAD_ZERO;\n",
		   d[i].sign,	/*sign location */
		   d[i].epos,	/*exponent loc	*/
		   d[i].esize,	/*exponent size */
		   (unsigned long)(d[i].bias),	 /*exponent bias */
		   d[i].mpos,	/*mantissa loc	*/
		   d[i].msize,	/*mantissa size */
		   d[i].imp ? "IMPLIED" : "NONE");	/*normalization */
	}

	/* Atomize the type */
	printf("\
    if ((H5T_NATIVE_%s_g = H5I_register (H5I_DATATYPE, dt))<0)\n\
        HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,\"can't initialize type system (atom registration failure\");\n",
	       d[i].varname);
	printf("    H5T_NATIVE_%s_ALIGN_g = %lu;\n",
	       d[i].varname, (unsigned long)(d[i].align));

        /* Variables for alignment of compound datatype */
        if(!strcmp(d[i].varname, "SCHAR")  || !strcmp(d[i].varname, "SHORT") || 
            !strcmp(d[i].varname, "INT")   || !strcmp(d[i].varname, "LONG")  || 
            !strcmp(d[i].varname, "LLONG") || !strcmp(d[i].varname, "FLOAT") || 
            !strcmp(d[i].varname, "DOUBLE") || !strcmp(d[i].varname, "LDOUBLE")) { 
            printf("    H5T_NATIVE_%s_COMP_ALIGN_g = %lu;\n",
                    d[i].varname, (unsigned long)(d[i].comp_align));
        }
    }

    /* Structure alignment for pointers, hvl_t, hobj_ref_t, hdset_reg_ref_t */
    printf("\n    /* Structure alignment for pointers, hvl_t, hobj_ref_t, hdset_reg_ref_t */\n");
    for(j=0; j<na; j++)
        printf("    H5T_%s_COMP_ALIGN_g = %lu;\n", misc_align[j].name, (unsigned long)(misc_align[j].comp_align));
        
    printf("\
\n\
done:\n\
    if(ret_value<0) {\n\
        if(dt!=NULL)\n\
            H5FL_FREE(H5T_t,dt);\n\
    }\n\
\n\
    FUNC_LEAVE_NOAPI(ret_value);\n}\n");
}


/*-------------------------------------------------------------------------
 * Function:	iprint
 *
 * Purpose:	Prints information about the fields of a floating point
 *		format.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 13, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
iprint(detected_t *d)
{
    int		i, j, k, pass;

    for (pass=(d->size-1)/4; pass>=0; --pass) {
	/*
	 * Print the byte ordering above the bit fields.
	 */
	printf("    * ");
	for (i=MIN(pass*4+3,d->size-1); i>=pass*4; --i) {
	    printf ("%4d", d->perm[i]);
	    if (i>pass*4) fputs ("     ", stdout);
	}

	/*
	 * Print the bit fields
	 */
	printf("\n    * ");
	for (i=MIN(pass*4+3,d->size-1),
	     k=MIN(pass*32+31,8*d->size-1);
	     i>=pass*4; --i) {
	    for (j=7; j>=0; --j) {
		if (k==d->sign && d->msize) {
		    putchar('S');
		} else if (k>=d->epos && k<d->epos+d->esize) {
		    putchar('E');
		} else if (k>=d->mpos && k<d->mpos+d->msize) {
		    putchar('M');
		} else if (d->msize) {
		    putchar('?');   /*unknown floating point bit */
		} else if (d->sign) {
		    putchar('I');
		} else {
		    putchar('U');
		}
		--k;
	    }
	    if (i>pass*4) putchar(' ');
	}
	putchar('\n');
    }

    /*
     * Is there an implicit bit in the mantissa.
     */
    if (d->msize) {
	printf("    * Implicit bit? %s\n", d->imp ? "yes" : "no");
    }

    /*
     * Alignment
     */
    if (0==d->align) {
	printf("    * Alignment: NOT CALCULATED\n");
    } else if (1==d->align) {
	printf("    * Alignment: none\n");
    } else {
	printf("    * Alignment: %lu\n", (unsigned long)(d->align));
    }
}


/*-------------------------------------------------------------------------
 * Function:	byte_cmp
 *
 * Purpose:	Compares two chunks of memory A and B and returns the
 *		byte index into those arrays of the first byte that
 *		differs between A and B.
 *
 * Return:	Success:	Index of differing byte.
 *
 *		Failure:	-1 if all bytes are the same.
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 12, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
byte_cmp(int n, void *_a, void *_b)
{
    register int	i;
    unsigned char	*a = (unsigned char *) _a;
    unsigned char	*b = (unsigned char *) _b;

    for (i = 0; i < n; i++) if (a[i] != b[i]) return i;
    return -1;
}


/*-------------------------------------------------------------------------
 * Function:	bit_cmp
 *
 * Purpose:	Compares two bit vectors and returns the index for the
 *		first bit that differs between the two vectors.	 The
 *		size of the vector is NBYTES.  PERM is a mapping from
 *		actual order to little endian.
 *
 * Return:	Success:	Index of first differing bit.
 *
 *		Failure:	-1
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 13, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
bit_cmp(int nbytes, int *perm, void *_a, void *_b)
{
    int			i, j;
    unsigned char	*a = (unsigned char *) _a;
    unsigned char	*b = (unsigned char *) _b;
    unsigned char	aa, bb;

    for (i = 0; i < nbytes; i++) {
	assert(perm[i] < nbytes);
	if ((aa = a[perm[i]]) != (bb = b[perm[i]])) {
	    for (j = 0; j < 8; j++, aa >>= 1, bb >>= 1) {
		if ((aa & 1) != (bb & 1)) return i * 8 + j;
	    }
	    assert("INTERNAL ERROR" && 0);
	    abort();
	}
    }
    return -1;
}


/*-------------------------------------------------------------------------
 * Function:	fix_order
 *
 * Purpose:	Given an array PERM with elements FIRST through LAST
 *		initialized with zero origin byte numbers, this function
 *		creates a permutation vector that maps the actual order
 *		of a floating point number to little-endian.
 *
 *		This function assumes that the mantissa byte ordering
 *		implies the total ordering.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 13, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
fix_order(int n, int first, int last, int *perm, const char **mesg)
{
    int		i;

    if (first + 1 < last) {
	/*
	 * We have at least three points to consider.
	 */
	if (perm[last] < perm[last - 1] && perm[last - 1] < perm[last - 2]) {
	    /*
	     * Little endian.
	     */
	    if (mesg) *mesg = "Little-endian";
	    for (i = 0; i < n; i++) perm[i] = i;

	} else if (perm[last] > perm[last-1] && perm[last-1] > perm[last-2]) {
	    /*
	     * Big endian.
	     */
	    if (mesg) *mesg = "Big-endian";
	    for (i = 0; i < n; i++) perm[i] = (n - 1) - i;

	} else {
	    /*
	     * Bi-endian machines like VAX.
	     */
	    assert(0 == n / 2);
	    if (mesg) *mesg = "VAX";
	    for (i = 0; i < n; i += 2) {
		perm[i] = (n - 2) - i;
		perm[i + 1] = (n - 1) - i;
	    }
	}
    } else {
	fprintf(stderr,
	     "Failed to detect byte order of %d-byte floating point.\n", n);
	exit(1);
    }
}


/*-------------------------------------------------------------------------
 * Function:	imp_bit
 *
 * Purpose:	Looks for an implicit bit in the mantissa.  The value
 *		of _A should be 1.0 and the value of _B should be 0.5.
 *		Some floating-point formats discard the most significant
 *		bit of the mantissa after normalizing since it will always
 *		be a one (except for 0.0).  If this is true for the native
 *		floating point values stored in _A and _B then the function
 *		returns non-zero.
 *
 *		This function assumes that the exponent occupies higher
 *		order bits than the mantissa and that the most significant
 *		bit of the mantissa is next to the least signficant bit
 *		of the exponent.
 *		
 *
 * Return:	Success:	Non-zero if the most significant bit
 *				of the mantissa is discarded (ie, the
 *				mantissa has an implicit `one' as the
 *				most significant bit).	Otherwise,
 *				returns zero.
 *
 *		Failure:	exit(1)
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 13, 1996
 *
 * Modifications:
 *
 *	Robb Matzke, 6 Nov 1996
 *	Fixed a bug that occurs with non-implicit architectures.
 *
 *-------------------------------------------------------------------------
 */
static int
imp_bit(int n, int *perm, void *_a, void *_b)
{
    unsigned char	*a = (unsigned char *) _a;
    unsigned char	*b = (unsigned char *) _b;
    int			changed, major, minor;
    int			msmb;	/*most significant mantissa bit */

    /*
     * Look for the least significant bit that has changed between
     * A and B.	 This is the least significant bit of the exponent.
     */
    changed = bit_cmp(n, perm, a, b);
    assert(changed >= 0);

    /*
     * The bit to the right (less significant) of the changed bit should
     * be the most significant bit of the mantissa.  If it is non-zero
     * then the format does not remove the leading `1' of the mantissa.
     */
    msmb = changed - 1;
    major = msmb / 8;
    minor = msmb % 8;

    return (a[perm[major]] >> minor) & 0x01 ? 0 : 1;
}


/*-------------------------------------------------------------------------
 * Function:	find_bias
 *
 * Purpose:	Determines the bias of the exponent.  This function should
 *		be called with _A having a value of `1'.
 *
 * Return:	Success:	The exponent bias.
 *
 *		Failure:	
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 13, 1996
 *
 * Modifications:
 *
 *	Robb Matzke, 6 Nov 1996
 *	Fixed a bug with non-implicit architectures returning the
 *	wrong exponent bias.
 *
 *-------------------------------------------------------------------------
 */
static unsigned long
find_bias(int epos, int esize, int *perm, void *_a)
{
    unsigned char	*a = (unsigned char *) _a;
    unsigned char	mask;
    unsigned long	b, shift = 0, nbits, bias = 0;

    while (esize > 0) {
	nbits = MIN(esize, (8 - epos % 8));
	mask = (1 << nbits) - 1;
	b = (a[perm[epos / 8]] >> (epos % 8)) & mask;
	bias |= b << shift;

	shift += nbits;
	esize -= nbits;
	epos += nbits;
    }
    return bias;
}


/*-------------------------------------------------------------------------
 * Function:	print_header
 *
 * Purpose:	Prints the C file header for the generated file.
 *
 * Return:	void
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Mar 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
print_header(void)
{

    time_t		now = time(NULL);
    struct tm		*tm = localtime(&now);
    char		real_name[30];
    char		host_name[256];
    int			i;
    const char		*s;
#ifdef H5_HAVE_GETPWUID
    struct passwd	*pwd = NULL;
#else
    int			pwd = 1;
#endif
    static const char	*month_name[] =
    {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char	*purpose = "\
This machine-generated source code contains\n\
information about the various integer and\n\
floating point numeric formats found on this\n\
architecture.  The parameters below should be\n\
checked carefully and errors reported to the\n\
HDF5 maintainer.\n\
\n\
Each of the numeric formats listed below are\n\
printed from most significant bit to least\n\
significant bit even though the actual bytes\n\
might be stored in a different order in\n\
memory.	 The integers above each binary byte\n\
indicate the relative order of the bytes in\n\
memory; little-endian machines have\n\
decreasing numbers while big-endian machines\n\
have increasing numbers.\n\
\n\
The fields of the numbers are printed as\n\
letters with `S' for the mantissa sign bit,\n\
`M' for the mantissa magnitude, and `E' for\n\
the exponent.  The exponent has an associated\n\
bias which can be subtracted to find the\n\
true exponent.	The radix point is assumed\n\
to be before the first `M' bit.	 Any bit\n\
of a floating-point value not falling into one\n\
of these categories is printed as a question\n\
mark.  Bits of integer types are printed as\n\
`I' for 2's complement and `U' for magnitude.\n\
\n\
If the most significant bit of the normalized\n\
mantissa (always a `1' except for `0.0') is\n\
not stored then an `implicit=yes' appears\n\
under the field description.  In thie case,\n\
the radix point is still assumed to be\n\
before the first `M' but after the implicit\n\
bit.\n";

    /*
     * The real name is the first item from the passwd gecos field.
     */
#ifdef H5_HAVE_GETPWUID
    {
	size_t n;
	char *comma;
	if ((pwd = getpwuid(getuid()))) {
	    if ((comma = strchr(pwd->pw_gecos, ','))) {
		n = MIN(sizeof(real_name)-1, (unsigned)(comma-pwd->pw_gecos));
		strncpy(real_name, pwd->pw_gecos, n);
		real_name[n] = '\0';
	    } else {
		strncpy(real_name, pwd->pw_gecos, sizeof(real_name));
		real_name[sizeof(real_name) - 1] = '\0';
	    }
	} else {
	    real_name[0] = '\0';
	}
    }
#else
    real_name[0] = '\0';
#endif

    /*
     * The FQDM of this host or the empty string.
     */
#ifdef H5_HAVE_GETHOSTNAME
#ifdef WIN32 
/* windows DLL cannot recognize gethostname, so turn off on windows for the time being!
    KY, 2003-1-14 */
    host_name[0] = '\0';
#else
    if (gethostname(host_name, sizeof(host_name)) < 0) {
	host_name[0] = '\0';
    }
#endif
#else
    host_name[0] = '\0';
#endif
    
    /*
     * The file header: warning, copyright notice, build information.
     */
    printf("/* Generated automatically by H5detect -- do not edit */\n\n\n");
    puts(FileHeader);		/*the copyright notice--see top of this file */

    printf(" *\n * Created:\t\t%s %2d, %4d\n",
	   month_name[tm->tm_mon], tm->tm_mday, 1900 + tm->tm_year);
    if (pwd || real_name[0] || host_name[0]) {
	printf(" *\t\t\t");
	if (real_name[0]) printf("%s <", real_name);
#ifdef H5_HAVE_GETPWUID
	if (pwd) fputs(pwd->pw_name, stdout);
#endif
	if (host_name[0]) printf("@%s", host_name);
	if (real_name[0]) printf(">");
	putchar('\n');
    }
    printf(" *\n * Purpose:\t\t");
    for (s = purpose; *s; s++) {
	putchar(*s);
	if ('\n' == *s && s[1]) printf(" *\t\t\t");
    }

    printf(" *\n * Modifications:\n *\n");
    printf(" *\tDO NOT MAKE MODIFICATIONS TO THIS FILE!\n");
    printf(" *\tIt was generated by code in `H5detect.c'.\n");

    printf(" *\n *");
    for (i = 0; i < 73; i++) putchar('-');
    printf("\n */\n\n");

}


/*-------------------------------------------------------------------------
 * Function:	main
 *
 * Purpose:	Main entry point.
 *
 * Return:	Success:	exit(0)
 *
 *		Failure:	exit(1)
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 12, 1996
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
main(void)
{
    detected_t		d[MAXDETECT];
    malign_t            m[MAXDETECT];
    volatile int	nd = 0, na = 0;
    
#if defined(H5_HAVE_SETSYSINFO) && defined(SSI_NVPAIRS)
#if defined(UAC_NOPRINT) && defined(UAC_SIGBUS)
    /*
     * Make sure unaligned access generates SIGBUS and doesn't print warning
     * messages so that we can detect alignment constraints on the DEC Alpha.
     */
    int			nvpairs[2];
    nvpairs[0] = SSIN_UACPROC;
    nvpairs[1] = UAC_NOPRINT | UAC_SIGBUS;
    if (setsysinfo(SSI_NVPAIRS, nvpairs, 1, 0, 0)<0) {
	fprintf(stderr, "H5detect: unable to turn off UAC handling: %s\n",
		strerror(errno));
    }
#endif
#endif
    
    print_header();

    /* C89 integer types */
    DETECT_I(signed char,	  SCHAR,        d[nd]); nd++;
    DETECT_I(unsigned char,	  UCHAR,        d[nd]); nd++;
    DETECT_I(short,		  SHORT,        d[nd]); nd++; 
    DETECT_I(unsigned short,	  USHORT,       d[nd]); nd++;
    DETECT_I(int,		  INT,	        d[nd]); nd++;
    DETECT_I(unsigned int,	  UINT,	        d[nd]); nd++;
    DETECT_I(long,		  LONG,	        d[nd]); nd++;
    DETECT_I(unsigned long,	  ULONG,        d[nd]); nd++;

    /*
     * C9x integer types.
     */
#if H5_SIZEOF_INT8_T>0
    DETECT_I(int8_t, 		  INT8,         d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT8_T>0
    DETECT_I(uint8_t, 		  UINT8,        d[nd]); nd++;
#endif
#if H5_SIZEOF_INT_LEAST8_T>0
    DETECT_I(int_least8_t, 	  INT_LEAST8,   d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT_LEAST8_T>0
    DETECT_I(uint_least8_t, 	  UINT_LEAST8,  d[nd]); nd++;
#endif
#if H5_SIZEOF_INT_FAST8_T>0
    DETECT_I(int_fast8_t, 	  INT_FAST8,    d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT_FAST8_T>0
    DETECT_I(uint_fast8_t, 	  UINT_FAST8,   d[nd]); nd++;
#endif
#if H5_SIZEOF_INT16_T>0
    DETECT_I(int16_t, 		  INT16,        d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT16_T>0
    DETECT_I(uint16_t, 		  UINT16,       d[nd]); nd++;
#endif
#if H5_SIZEOF_INT_LEAST16_T>0
    DETECT_I(int_least16_t, 	  INT_LEAST16,  d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT_LEAST16_T>0
    DETECT_I(uint_least16_t, 	  UINT_LEAST16, d[nd]); nd++;
#endif
#if H5_SIZEOF_INT_FAST16_T>0
    DETECT_I(int_fast16_t, 	  INT_FAST16,   d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT_FAST16_T>0
    DETECT_I(uint_fast16_t, 	  UINT_FAST16,  d[nd]); nd++;
#endif
#if H5_SIZEOF_INT32_T>0
    DETECT_I(int32_t, 		  INT32,        d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT32_T>0
    DETECT_I(uint32_t, 		  UINT32,       d[nd]); nd++;
#endif
#if H5_SIZEOF_INT_LEAST32_T>0
    DETECT_I(int_least32_t, 	  INT_LEAST32,  d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT_LEAST32_T>0
    DETECT_I(uint_least32_t, 	  UINT_LEAST32, d[nd]); nd++;
#endif
#if H5_SIZEOF_INT_FAST32_T>0
    DETECT_I(int_fast32_t, 	  INT_FAST32,   d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT_FAST32_T>0
    DETECT_I(uint_fast32_t, 	  UINT_FAST32,  d[nd]); nd++;
#endif
#if H5_SIZEOF_INT64_T>0
    DETECT_I(int64_t, 		  INT64,        d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT64_T>0
    DETECT_I(uint64_t, 		  UINT64,       d[nd]); nd++;
#endif
#if H5_SIZEOF_INT_LEAST64_T>0
    DETECT_I(int_least64_t, 	  INT_LEAST64,  d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT_LEAST64_T>0
    DETECT_I(uint_least64_t, 	  UINT_LEAST64, d[nd]); nd++;
#endif
#if H5_SIZEOF_INT_FAST64_T>0
    DETECT_I(int_fast64_t, 	  INT_FAST64,   d[nd]); nd++;
#endif
#if H5_SIZEOF_UINT_FAST64_T>0
    DETECT_I(uint_fast64_t, 	  UINT_FAST64,  d[nd]); nd++;
#endif
    
#if H5_SIZEOF_LONG_LONG>0
    DETECT_I(long_long,		  LLONG,        d[nd]); nd++;
    DETECT_I(unsigned long_long,  ULLONG,       d[nd]); nd++;
#else
    /*
     * This architecture doesn't support an integer type larger than `long'
     * so we'll just make H5T_NATIVE_LLONG the same as H5T_NATIVE_LONG since
     * `long long' is probably equivalent to `long' here anyway.
     */
    DETECT_I(long,		  LLONG,        d[nd]); nd++;
    DETECT_I(unsigned long,	  ULLONG,       d[nd]); nd++;
#endif

    DETECT_F(float,		  FLOAT,        d[nd]); nd++;
    DETECT_F(double,		  DOUBLE,       d[nd]); nd++;

#if H5_SIZEOF_DOUBLE == H5_SIZEOF_LONG_DOUBLE
    /*
     * If sizeof(double)==sizeof(long double) then assume that `long double'
     * isn't supported and use `double' instead.  This suppresses warnings on
     * some systems and `long double' is probably the same as `double' here
     * anyway.
     */
    DETECT_F(double,		  LDOUBLE,      d[nd]); nd++;
#else
    DETECT_F(long double,	  LDOUBLE,      d[nd]); nd++;
#endif

    /* Detect structure alignment for pointers, hvl_t, hobj_ref_t, hdset_reg_ref_t */
    DETECT_M(void *,              POINTER,      m[na]); na++;
    DETECT_M(hvl_t,               HVL,          m[na]); na++;
    DETECT_M(hobj_ref_t,          HOBJREF,      m[na]); na++;
    DETECT_M(hdset_reg_ref_t,     HDSETREGREF,  m[na]); na++;
    
    print_results (nd, d, na, m);
    
    return 0;
}
