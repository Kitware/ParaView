/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabeledEntry.h
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
// .NAME vtkKWLabeledEntry - an entry with a label
// .SECTION Description
// The LabeledEntry creates an entry with a label in front of it; both are
// contained in a frame


#ifndef __vtkKWLabeledEntry_h
#define __vtkKWLabeledEntry_h

#include "vtkKWLabel.h"
#include "vtkKWEntry.h"

class vtkKWApplication;

class VTK_EXPORT vtkKWLabeledEntry : public vtkKWWidget
{
public:
  static vtkKWLabeledEntry* New();
  vtkTypeMacro(vtkKWLabeledEntry, vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);

  // Description:
  // Set the label for the frame.
  void SetLabel(const char *);
  
  // Description:
  // get the internal entry
  vtkKWEntry *GetEntry() { return this->Entry; }
  
  // Description:
  // Set/Get the value of the entry in a few different formats.
  // In the SetValue method with float, the second argument is the
  // number of decimal places to display.
  void SetValue(const char *);
  void SetValue(int a);
  void SetValue(float f,int size);
  char *GetValue();
  int GetValueAsInt();
  float GetValueAsFloat();
  
protected:
  vtkKWLabeledEntry();
  ~vtkKWLabeledEntry();
  vtkKWLabeledEntry(const vtkKWLabeledEntry&) {};
  void operator=(const vtkKWLabeledEntry&) {};

  vtkKWLabel *Label;
  vtkKWEntry *Entry;
};


#endif
