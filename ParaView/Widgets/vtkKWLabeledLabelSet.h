/*=========================================================================

  Module:    vtkKWLabeledLabelSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledLabelSet - a "set of labeled labels" widget
// .SECTION Description
// A simple widget representing a set of labeled labels (vtkKWLabeledLabel).
// Nothing fancy here, just a way to pack related or unrelated labels
// Labeled labels are packed in the order they were added.

#ifndef __vtkKWLabeledLabelSet_h
#define __vtkKWLabeledLabelSet_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWLabeledLabel;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWLabeledLabelSet : public vtkKWWidget
{
public:
  static vtkKWLabeledLabelSet* New();
  vtkTypeRevisionMacro(vtkKWLabeledLabelSet,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget (a frame holding all the labeled labels).
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Add a labeled label to the set.
  // The id has to be unique among the set.
  // Text can be provided to set both labels in the labeled label.
  // A help string will be used, if any, to set the baloon help. 
  // Return 1 on success, 0 otherwise.
  int AddLabeledLabel(int id, 
                      const char *text = 0, 
                      const char *text2 = 0, 
                      const char *balloonhelp_string = 0);

  // Description:
  // Get a labeled label from the set, given its unique id.
  // Return a pointer to the labeled label, or NULL on error.
  vtkKWLabeledLabel* GetLabeledLabel(int id);
  int HasLabeledLabel(int id);

  // Description:
  // Convenience method to set the first and second label of a labeled label.
  void SetLabel(int id, const char*);
  void SetLabel2(int id, const char*);

  // Description:
  // Convenience method to hide/show a labeled label
  void HideLabeledLabel(int id);
  void ShowLabeledLabel(int id);
  void SetLabeledLabelVisibility(int id, int flag);
  int GetNumberOfVisibleLabeledLabels();

  // Description:
  // Synchronize the width of the first label of the labeled labels. 
  // The maximum size is found and assigned to each label. 
  void SynchroniseLabelsMaximumWidth();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWLabeledLabelSet();
  ~vtkKWLabeledLabelSet();

  //BTX

  // A labeled label slot associates a labeled label to a unique Id
  // No, I don't want to use a map between those two, for the following reasons:
  // a), we might need more information in the future, b) a map 
  // Register/Unregister pointers if they are pointers to VTK objects.
 
  class LabeledLabelSlot
  {
  public:
    int Id;
    vtkKWLabeledLabel *LabeledLabel;
  };

  typedef vtkLinkedList<LabeledLabelSlot*> LabeledLabelsContainer;
  typedef vtkLinkedListIterator<LabeledLabelSlot*> LabeledLabelsContainerIterator;
  LabeledLabelsContainer *LabeledLabels;

  // Helper methods

  LabeledLabelSlot* GetLabeledLabelSlot(int id);

  //ETX

  void Pack();

private:
  vtkKWLabeledLabelSet(const vtkKWLabeledLabelSet&); // Not implemented
  void operator=(const vtkKWLabeledLabelSet&); // Not implemented
};

#endif

