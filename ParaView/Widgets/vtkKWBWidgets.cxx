/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWBWidgets.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWBWidgets.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkbwidgets.h"

#include <tk.h>
 
//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWBWidgets );

int vtkKWBWidgetsCommand(ClientData cd, Tcl_Interp *interp,
			    int argc, char *argv[]);

#define minus_width 9
#define minus_height 9
static unsigned char minus_bits[] = {
 0xff,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x7d,0x01,0x01,0x01,0x01,0x01,0x01,
 0x01,0xff,0x01};

#define plus_width 9
#define plus_height 9
static unsigned char plus_bits[] = {
 0xff,0x01,0x01,0x01,0x11,0x01,0x11,0x01,0x7d,0x01,0x11,0x01,0x11,0x01,0x01,
 0x01,0xff,0x01};

//----------------------------------------------------------------------------
vtkKWBWidgets::vtkKWBWidgets()
{
  this->CommandFunction = vtkKWBWidgetsCommand;
}

//----------------------------------------------------------------------------
vtkKWBWidgets::~vtkKWBWidgets()
{
}

//----------------------------------------------------------------------------
int vtkKWBWidgets::CreatePhoto(Tcl_Interp* interp, char *name, 
				unsigned char *data, int width, int height)
{
  Tk_PhotoHandle photo;
  Tk_PhotoImageBlock block;

  ostrstream command;
  command << "image create photo " << name << " -height "
	  << height << " -width " << width << ends;
  if (Tcl_GlobalEval(interp, command.str()) != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to create image. Error:" 
    << interp->result);
    command.rdbuf()->freeze(0);     
    return VTK_ERROR;
    }
  command.rdbuf()->freeze(0);     
  
  block.width = width;
  block.height = height;
  block.pixelSize = 3;
  block.pitch = block.width*block.pixelSize;
  block.offset[0] = 0;
  block.offset[1] = 1;
  block.offset[2] = 2;
  block.pixelPtr = data;

  photo = Tk_FindPhoto(interp, name);
  if (!photo)
    {
    vtkGenericWarningMacro("error looking up color ramp image");
    return VTK_ERROR;
    }  
  Tk_PhotoPutBlock(photo, &block, 0, 0, block.width, block.height);
  return VTK_OK;

}

//----------------------------------------------------------------------------
void vtkKWBWidgets::Initialize(Tcl_Interp* interp)
{
  if (!interp)
    {
    vtkGenericWarningMacro("An interpreter is needed to initialize bwidgets.");
    return;
    }

  if ( CreatePhoto(interp, "bwminus", minus_bits,  minus_width, minus_height) 
       != VTK_OK )
    {
    return;
    }

  if ( CreatePhoto(interp, "bwplus", plus_bits,  plus_width, plus_height) 
       != VTK_OK )
    {
    return;
    }

  char* script = new char[strlen(bwidgets)+1];
  strcpy(script, bwidgets);
  if (Tcl_GlobalEval(interp, script) != TCL_OK)
    {
    vtkGenericWarningMacro(<< "BWidgets failed to initialize. Error:" 
    << interp->result);
    }
  delete[] script;
  
}

//----------------------------------------------------------------------------
void vtkKWBWidgets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
