/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkKWWindowLevelPresetSelector - a window level preset selector.
// .SECTION Description
// This class is a widget that can be used to store and apply window/level
// presets. 
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWPresetSelector

#ifndef __vtkKWWindowLevelPresetSelector_h
#define __vtkKWWindowLevelPresetSelector_h

#include "vtkKWPresetSelector.h"

class KWWIDGETS_EXPORT vtkKWWindowLevelPresetSelector : public vtkKWPresetSelector
{
public:
  static vtkKWWindowLevelPresetSelector* New();
  vtkTypeRevisionMacro(vtkKWWindowLevelPresetSelector, vtkKWPresetSelector);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the window/level values for a given preset.
  // Return 1 on success, 0 otherwise
  virtual double GetPresetWindow(int id);
  virtual int SetPresetWindow(int id, double window);
  virtual double GetPresetLevel(int id);
  virtual int SetPresetLevel(int id, double level);

  // Description:
  // Query if a the pool has a given window/level preset
  virtual int HasPresetWithGroupWithWindowLevel(
    const char *group, double window, double level);

  // Description:
  // Callback invoked when user starts editing a specific preset field
  // located at cell ('row', 'col') with contents 'text'.
  // This method returns the value that is to become the initial 
  // contents of the temporary embedded widget used for editing.
  // Most of the time, this is the same as 'text'.
  // The next step (validation) is handled by PresetCellEditEndCallback
  virtual const char* PresetCellEditEndCallback(
    int row, int col, const char *text);

  // Description:
  // Callback invoked when the user successfully updated the preset field
  // located at ('row', 'col') with the new contents 'text', as a result
  // of editing the corresponding cell interactively.
  virtual void PresetCellUpdatedCallback(int row, int col, const char *text);

  // Description:
  // Some constants
  //BTX
  static const char *WindowColumnName;
  static const char *LevelColumnName;
  //ETX

protected:
  vtkKWWindowLevelPresetSelector() {};
  ~vtkKWWindowLevelPresetSelector() {};

  // Description:
  // Create the columns.
  // Subclasses should override this method to add their own columns and
  // display their own preset fields.
  virtual void CreateColumns();

  // Description:
  // Update the row in the list for a given preset
  // Subclass should override this method to display their own fields.
  // Return 1 on success, 0 if the row was not (or can not be) updated.
  // Subclasses should call the parent's UpdatePresetRow, and abort
  // if the result is not 1.
  virtual int UpdatePresetRow(int id);

  // Description:
  // Convenience methods to get the index of a given column
  virtual int GetWindowColumnIndex();
  virtual int GetLevelColumnIndex();

private:

  vtkKWWindowLevelPresetSelector(const vtkKWWindowLevelPresetSelector&); // Not implemented
  void operator=(const vtkKWWindowLevelPresetSelector&); // Not implemented
};

#endif
