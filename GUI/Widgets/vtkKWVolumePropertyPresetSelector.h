/*=========================================================================

  Module:    vtkKWVolumePropertyPresetSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWVolumePropertyPresetSelector - a volume property preset selector.
// .SECTION Description
// This class is a widget that can be used to store and apply volume property
// presets. 
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWVolumePropertyPresetSelector_h
#define __vtkKWVolumePropertyPresetSelector_h

#include "vtkKWPresetSelector.h"

class vtkVolumeProperty;

class KWWidgets_EXPORT vtkKWVolumePropertyPresetSelector : public vtkKWPresetSelector
{
public:
  static vtkKWVolumePropertyPresetSelector* New();
  vtkTypeRevisionMacro(vtkKWVolumePropertyPresetSelector, vtkKWPresetSelector);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the volume property associated to the preset in the pool.
  // Note that the volume property object passed as parameter is neither
  // stored nor Register()'ed, only a copy is stored (and updated each
  // time this method is called later on).
  // Return 1 on success, 0 on error
  virtual int SetPresetVolumeProperty(int id, vtkVolumeProperty *prop);
  virtual vtkVolumeProperty* GetPresetVolumeProperty(int id);

  // Description:
  // Set/Get the modality for a given preset.
  // This is just a convenience field, that can be useful if the presets
  // are used on medical data.
  // The modality field is not displayed as a column by default, but this
  // can be changed using the SetModalityColumnVisibility() method.
  // This column can not be edited.
  // Return 1 on success, 0 otherwise
  virtual int SetPresetModality(int id, const char *modality);
  virtual const char* GetPresetModality(int id);

  // Description:
  // Set/Get the visibility of the modality column. Hidden by default.
  // No effect if called before Create().
  virtual void SetModalityColumnVisibility(int);
  virtual int GetModalityColumnVisibility();
  vtkBooleanMacro(ModalityColumnVisibility, int);

  // Description:
  // Some constants
  //BTX
  static const char *ModalityColumnName;
  //ETX

protected:
  vtkKWVolumePropertyPresetSelector() {};
  ~vtkKWVolumePropertyPresetSelector() {};

  // Description:
  // Create the columns.
  // Subclasses should override this method to add their own columns and
  // display their own preset fields (do not forget to call the superclass
  // first).
  virtual void CreateColumns();

  // Description:
  // Update the preset row, i.e. add a row for that preset if it is not
  // displayed already, hide it if it does not match GroupFilter, and
  // update the table columns with the corresponding preset fields.
  // Subclass should override this method to display their own fields.
  // Return 1 on success, 0 if the row was not (or can not be) updated.
  // Subclasses should call the parent's UpdatePresetRow, and abort
  // if the result is not 1.
  virtual int UpdatePresetRow(int id);

  // Description:
  // Deep copy contents of volume property 'source' into 'target'
  virtual void DeepCopyVolumeProperty(
    vtkVolumeProperty *target, vtkVolumeProperty *source);

  // Description:
  // Get the index of a given column.
  virtual int GetModalityColumnIndex();

private:

  vtkKWVolumePropertyPresetSelector(const vtkKWVolumePropertyPresetSelector&); // Not implemented
  void operator=(const vtkKWVolumePropertyPresetSelector&); // Not implemented
};

#endif
