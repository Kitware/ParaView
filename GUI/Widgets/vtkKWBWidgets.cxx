/*=========================================================================

  Module:    vtkKWBWidgets.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWBWidgets.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"

#include "vtkTk.h"

#include "Utilities/vtkKWBWidgetsTclLibrary.h"
#include "Utilities/vtkKWComboboxTclLibrary.h"
 
//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWBWidgets );
vtkCxxRevisionMacro(vtkKWBWidgets, "1.21");

int vtkKWBWidgetsCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

#define minus_width 9
#define minus_height 9
static unsigned char minus_bits[] = {
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0
};

#define plus_width 9
#define plus_height 9
static unsigned char plus_bits[] = {
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  255,255,255,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0
};


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
void vtkKWBWidgets::Initialize(Tcl_Interp* interp)
{
  if (!interp)
    {
    vtkGenericWarningMacro("An interpreter is needed to initialize bwidgets.");
    return;
    }

  if (!vtkKWTkUtilities::UpdatePhoto(
        interp, "bwminus", minus_bits, minus_width, minus_height, 3) ||
      !vtkKWTkUtilities::UpdatePhoto(
        interp, "bwplus", plus_bits, plus_width, plus_height, 3))
    {
    return;
    }

  vtkKWBWidgets::Execute(interp, bwidgets1, "BWidgets1");
  vtkKWBWidgets::Execute(interp, bwidgets2, "BWidgets2");
  vtkKWBWidgets::Execute(interp, bwidgets3, "BWidgets3");
  vtkKWBWidgets::Execute(interp, bwidgets4, "BWidgets4");
  vtkKWBWidgets::Execute(interp, bwidgets5, "BWidgets5");
  vtkKWBWidgets::Execute(interp, bwidgets6, "BWidgets6");
  vtkKWBWidgets::Execute(interp, vtkcomboboxwidget1, "ComboBox1");
  vtkKWBWidgets::Execute(interp, vtkcomboboxwidget2, "ComboBox2");
  vtkKWBWidgets::Execute(interp, vtkcomboboxwidget3, "ComboBox2");
}

//----------------------------------------------------------------------------
void vtkKWBWidgets::Execute(Tcl_Interp* interp, const char* str, const char* module)
{
  const unsigned int maxlen = 32000;
  if ( strlen(str) > maxlen )
    {
    cout << "The size of tcl string for module " << module << " is " << strlen(str) 
      << " (higher than " << maxlen << "), so compilers that cannot "
      "handle such a large strings might not compile this." << endl;
    cout << "The line is: [";
    cout.write(str+maxlen, 100);
    cout << "]" << endl;
    }
  char* script = new char[strlen(str)+1];
  strcpy(script, str);
  if (Tcl_GlobalEval(interp, script) != TCL_OK)
    {
    vtkGenericWarningMacro(<< module << " failed to initialize. Error:" 
    << interp->result);
    }
  delete[] script;
}

//----------------------------------------------------------------------------
void vtkKWBWidgets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


