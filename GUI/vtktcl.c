/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtktcl.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h"
#include "vtkTcl.h"

extern int Vtkcommontcl_Init(Tcl_Interp *interp);
int Vtktcl_Init(Tcl_Interp *interp)
{
  /* init the core vtk stuff */
  if (Vtkcommontcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }

  return TCL_OK;
}

int Vtktcl_SafeInit(Tcl_Interp *interp)
{
  return Vtktcl_Init(interp);
}
