/*=========================================================================

  Module:    vtkKWTkconInit.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWTkconInit.h"

#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWResourceUtilities.h"

#include "vtkTk.h"

#include "Utilities/tkcon/vtkKWTkconTclLibrary.h"
 
//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTkconInit );
vtkCxxRevisionMacro(vtkKWTkconInit, "1.1");

int vtkKWTkconInit::Initialized = 0;

//----------------------------------------------------------------------------
void vtkKWTkconInit::Initialize(Tcl_Interp* interp)
{
  if (vtkKWTkconInit::Initialized)
    {
    return;
    }

  if (!interp)
    {
    vtkGenericWarningMacro(
      "An interpreter is needed to initialize the tkcon library.");
    return;
    }

  vtkKWTkconInit::Initialized = 1;

  // Evaluate the library

  unsigned char *buffer = 
    new unsigned char [file_tkcon_tcl_length];

  unsigned int i;
  unsigned char *cur_pos = buffer;
  for (i = 0; i < file_tkcon_tcl_nb_sections; i++)
    {
    size_t len = strlen((const char*)file_tkcon_tcl_sections[i]);
    memcpy(cur_pos, file_tkcon_tcl_sections[i], len);
    cur_pos += len;
    }

  vtkKWTkconInit::Execute(interp, 
                      buffer, 
                      file_tkcon_tcl_length,
                      file_tkcon_tcl_decoded_length);
  
  delete [] buffer;
}

//----------------------------------------------------------------------------
void vtkKWTkconInit::Execute(Tcl_Interp* interp, 
                            const unsigned char *buffer, 
                            unsigned long length,
                            unsigned long decoded_length)
{
  // Is the data encoded (zlib and/or base64) ?

  unsigned char *decoded_buffer = NULL;
  if (length && length != decoded_length)
    {
    if (!vtkKWResourceUtilities::DecodeBuffer(
          buffer, length, &decoded_buffer, decoded_length))
      {
      vtkGenericWarningMacro(<<"Error while decoding library");
      return;
      }
    buffer = decoded_buffer;
    length = decoded_length;
    }

  if (buffer && 
      Tcl_EvalEx(interp, (const char*)buffer, length, TCL_EVAL_GLOBAL)!=TCL_OK)
    {
    vtkGenericWarningMacro(
      << " Failed to initialize. Error:" << Tcl_GetStringResult(interp));
    }

  if (decoded_buffer)
    {
    delete [] decoded_buffer;
    }
}

//----------------------------------------------------------------------------
void vtkKWTkconInit::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


