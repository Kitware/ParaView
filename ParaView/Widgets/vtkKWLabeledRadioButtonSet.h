/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkKWLabeledRadioButtonSet - a radiobutton set with a label
// .SECTION Description
// This class creates a radiobutton set with a label on top of it; both are
// contained in a frame
// .SECTION See Also
// vtkKWRadioButtonSet

#ifndef __vtkKWLabeledRadioButtonSet_h
#define __vtkKWLabeledRadioButtonSet_h

#include "vtkKWLabeledWidget.h"

class vtkKWRadioButtonSet;

class VTK_EXPORT vtkKWLabeledRadioButtonSet : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledRadioButtonSet* New();
  vtkTypeRevisionMacro(vtkKWLabeledRadioButtonSet, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal object
  vtkGetObjectMacro(RadioButtonSet, vtkKWRadioButtonSet);

  // Description:
  // Set the widget packing order to be horizontal (default is vertical).
  virtual void SetPackHorizontally(int);
  vtkBooleanMacro(PackHorizontally, int);
  vtkGetMacro(PackHorizontally, int);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

protected:
  vtkKWLabeledRadioButtonSet();
  ~vtkKWLabeledRadioButtonSet();

  vtkKWRadioButtonSet *RadioButtonSet;

  int PackHorizontally;

  // Pack or repack the widget.

  virtual void Pack();

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWLabeledRadioButtonSet(const vtkKWLabeledRadioButtonSet&); // Not implemented
  void operator=(const vtkKWLabeledRadioButtonSet&); // Not implemented
};


#endif

