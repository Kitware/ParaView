/*
 * Copyright (C) 1997 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Quincey Koziol
 *              Monday, Apr 20
 *
 * Purpose:     This file contains declarations which are visible only within
 *              the H5A package.  Source files outside the H5A package should
 *              include H5Aprivate.h instead.
 */
#ifndef H5A_PACKAGE
#error "Do not include this file outside the H5A package!"
#endif

#ifndef _H5Apkg_H
#define _H5Apkg_H

/*
 * Define this to enable debugging.
 */
#ifdef NDEBUG
#  undef H5A_DEBUG
#endif

#include "H5Aprivate.h"
#include "H5HGprivate.h"
#include "H5Sprivate.h"

struct H5A_t {
    unsigned       initialized;/* Indicate whether the attribute has been modified */
    unsigned       ent_opened; /* Object header entry opened? */
    H5G_entry_t ent;        /* Object Header entry (for both datasets & groups) */
    char        *name;      /* Attribute's name */
    H5T_t       *dt;        /* Attribute's datatype */
    size_t      dt_size;    /* Size of datatype on disk */
    H5S_t       *ds;        /* Attribute's dataspace */
    size_t      ds_size;    /* Size of dataspace on disk */
    void        *data;      /* Attribute data (on a temporary basis) */
    size_t      data_size;  /* Size of data on disk */
    H5HG_t      sh_heap;    /*if defined, attribute is in global heap        */
    H5F_t       *sh_file;   /*file pointer if this is a shared attribute    */
};

/* Function prototypes for H5T package scope */

#endif
