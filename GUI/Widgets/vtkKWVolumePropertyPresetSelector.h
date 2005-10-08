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

class KWWIDGETS_EXPORT vtkKWVolumePropertyPresetSelector : public vtkKWPresetSelector
{
public:
  static vtkKWVolumePropertyPresetSelector* New();
  vtkTypeRevisionMacro(vtkKWVolumePropertyPresetSelector, vtkKWPresetSelector);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the volume property associated to the preset in the pool.
  // Note that the volume property object passed as parameter is not
  // stored or Register()'ed, only a copy is stored (and updated each
  // time this method is called later on).
  // Return 1 on success, 0 on error
  virtual int SetPresetVolumeProperty(int id, vtkVolumeProperty *prop);
  virtual vtkVolumeProperty* GetPresetVolumeProperty(int id);

protected:
  vtkKWVolumePropertyPresetSelector() {};
  ~vtkKWVolumePropertyPresetSelector();

  // Description:
  // Deallocate a preset.
  // Subclasses should override this method to release the memory allocated
  // by their own preset fields  (do not forget to call the superclass
  // first).
  virtual void DeAllocatePreset(int id);

private:

  vtkKWVolumePropertyPresetSelector(const vtkKWVolumePropertyPresetSelector&); // Not implemented
  void operator=(const vtkKWVolumePropertyPresetSelector&); // Not implemented
};

#endif
