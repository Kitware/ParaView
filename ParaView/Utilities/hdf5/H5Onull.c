/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5Onull.c
 *                      Aug  6 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             The null message.
 *
 * Modifications:       
 *
 *-------------------------------------------------------------------------
 */
#include "H5private.h"
#include "H5Oprivate.h"

#define PABLO_MASK      H5O_null_mask

/* This message derives from H5O */
const H5O_class_t H5O_NULL[1] = {{
    H5O_NULL_ID,            /*message id number             */
    "null",                 /*message name for debugging    */
    0,                      /*native message size           */
    NULL,                   /*no decode method              */
    NULL,                   /*no encode method              */
    NULL,                   /*no copy method                */
    NULL,                   /*no size method                */
    NULL,                   /*no reset method               */
    NULL,                   /*no free method                */
    NULL,                   /*no get share method           */
    NULL,                   /*no set share method           */
    NULL,                   /*no debug method               */
}};
