/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGlyph3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkPVGlyph3D - A class to handle the UI for vtkGlyph3D
// .SECTION Description


#ifndef __vtkPVGlyph3D_h
#define __vtkPVGlyph3D_h

#include "vtkPVSource.h"
#include "vtkKWLabel.h"
#include "vtkPVInputMenu.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabeledEntry.h"
#include "vtkGlyph3D.h"

class VTK_EXPORT vtkPVGlyph3D : public vtkPVSource
{
public:
  static vtkPVGlyph3D* New();
  vtkTypeMacro(vtkPVGlyph3D, vtkPVSource);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Tcl callback for the scale and vector mode option menus
  void ChangeScaleMode();
  void ChangeVectorMode();

  // Description:
  // Fill in the source menu
  void UpdateSourceMenu();
  
  // Description:
  // Tcl callback for the source menu
  void ChangeSource();
  
  // Description:
  // Set/Get the tcl name of the current glyph source
  vtkSetStringMacro(GlyphSourceTclName);
  vtkGetStringMacro(GlyphSourceTclName);
  
  // Description:
  // Set/Get the current glyph scale mode
  vtkSetStringMacro(GlyphScaleMode);
  vtkGetStringMacro(GlyphScaleMode);

  // Description:
  // Set/Get the current glyph scale mode
  vtkSetStringMacro(GlyphVectorMode);
  vtkGetStringMacro(GlyphVectorMode);

  // Description:
  // Save this source to a file.
  void SaveInTclScript(ofstream *file);
  
protected:
  vtkPVGlyph3D();
  ~vtkPVGlyph3D();
  vtkPVGlyph3D(const vtkPVGlyph3D&) {};
  void operator=(const vtkPVGlyph3D&) {};

  char *GlyphSourceTclName;
  char *GlyphScaleMode;
  char *GlyphVectorMode;
  
  vtkKWWidget *GlyphSourceFrame;
  vtkKWLabel *GlyphSourceLabel;
  vtkPVInputMenu *GlyphSourceMenu;
  vtkKWWidget *ScaleModeFrame;
  vtkKWLabel *ScaleModeLabel;
  vtkKWOptionMenu *ScaleModeMenu;
  vtkKWWidget *VectorModeFrame;
  vtkKWLabel *VectorModeLabel;
  vtkKWOptionMenu *VectorModeMenu;
  vtkKWCheckButton *OrientCheck;
  vtkKWCheckButton *ScaleCheck;
  vtkKWLabeledEntry *ScaleEntry;
};

#endif
