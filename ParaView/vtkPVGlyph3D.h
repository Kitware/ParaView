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

#ifndef __vtkPVGlyph3D_h
#define __vtkPVGlyph3D_h

#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkGlyph3D.h"
#include "vtkPVSource.h"
#include "vtkPVComposite.h"

class VTK_EXPORT vtkPVGlyph3D : public vtkPVSource
{
public:
  static vtkPVGlyph3D* New();
  vtkTypeMacro(vtkPVGlyph3D, vtkPVSource);

  void Create(vtkKWApplication *app, char *args);

  vtkGetObjectMacro(Glyph, vtkGlyph3D);
  
  vtkGetObjectMacro(GlyphComposite, vtkPVComposite);
  void SetGlyphComposite(vtkPVComposite *comp);
  
  void ShowGlyphComposite();
  void ScaleFactorChanged();
  
  vtkPVData *GetDataWidget();

protected:
  vtkPVGlyph3D();
  ~vtkPVGlyph3D();
  vtkPVGlyph3D(const vtkPVGlyph3D&) {};
  void operator=(const vtkPVGlyph3D&) {};
  
  vtkKWWidget *GlyphCompositeButton;
  vtkKWLabel *ScaleFactorLabel;
  vtkKWEntry *ScaleFactorEntry;
  vtkKWWidget *Accept;

  vtkGlyph3D *Glyph;
  vtkPVComposite *GlyphComposite;
};

#endif
