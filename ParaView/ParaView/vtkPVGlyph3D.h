/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyph3D.h
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
// .NAME vtkPVGlyph3D - A class to handle the UI for vtkGlyph3D
// .SECTION Description


#ifndef __vtkPVGlyph3D_h
#define __vtkPVGlyph3D_h

#include "vtkPVSource.h"
#include "vtkKWLabel.h"
#include "vtkPVInputMenu.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVVectorEntry.h"
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
  
  // Description:
  // For scripting. Reset does not work for scale mode.
  vtkGetObjectMacro(ScaleModeMenu, vtkKWOptionMenu);

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
  vtkPVLabeledToggle *OrientCheck;
  vtkPVLabeledToggle *ScaleCheck;
  vtkPVVectorEntry *ScaleEntry;
};

#endif
