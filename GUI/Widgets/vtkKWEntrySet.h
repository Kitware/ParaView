/*=========================================================================

  Module:    vtkKWEntrySet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWEntrySet - a "set of entries" widget
// .SECTION Description
// A simple widget representing a set of entries. Entries
// can be created, removed or queried based on unique ID provided by the user
// (ids are not handled by the class since it is likely that they will be 
// defined as enum's or #define by the user for easier retrieval, instead
// of having ivar's that would store the id's returned by the class).
// Entries are packed (gridded) in the order they were added.

#ifndef __vtkKWEntrySet_h
#define __vtkKWEntrySet_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWEntry;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWEntrySet : public vtkKWWidget
{
public:
  static vtkKWEntrySet* New();
  vtkTypeRevisionMacro(vtkKWEntrySet,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget (a frame holding all the entries).
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Add a entry to the set.
  // The id has to be unique among the set.
  // Object and method parameters, if any, will be used to set the command.
  // A help string will be used, if any, to set the baloon help. 
  // Return 1 on success, 0 otherwise.
  int AddEntry(int id, 
               vtkKWObject *object = 0, 
               const char *method_and_arg_string = 0,
               const char *balloonhelp_string = 0);

  // Description:
  // Get a entry from the set, given its unique id.
  // It is advised not to temper with the entry var name or value :)
  // Return a pointer to the entry, or NULL on error.
  vtkKWEntry* GetEntry(int id);
  int HasEntry(int id);

  // Description:
  // Convenience method to hide/show a entry
  void HideEntry(int id);
  void ShowEntry(int id);
  void SetEntryVisibility(int id, int flag);
  int GetNumberOfVisibleEntries();

  // Description:
  // Set the widget packing order to be horizontal (default is vertical).
  void SetPackHorizontally(int);
  vtkBooleanMacro(PackHorizontally, int);
  vtkGetMacro(PackHorizontally, int);

  // Description:
  // Set the maximum number of widgets that will be packed in the packing
  // direction (i.e. horizontally or vertically). Default is 0, meaning that
  // all widgets are packed along the same direction. If 3 (for example) and
  // direction is horizontal, you end up with 3 columns.
  void SetMaximumNumberOfWidgetInPackingDirection(int);
  vtkGetMacro(MaximumNumberOfWidgetInPackingDirection, int);

  // Description:
  // Set the widgets padding.
  virtual void SetPadding(int x, int y);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWEntrySet();
  ~vtkKWEntrySet();

  int PackHorizontally;
  int MaximumNumberOfWidgetInPackingDirection;
  int PadX;
  int PadY;

  //BTX

  // A entry slot associates a entry to a unique Id
  // I don't want to use a map between those two, for the following reasons:
  // a), we might need more information in the future, b) a map 
  // Register/Unregister pointers if they are pointers to VTK objects.
 
  class EntrySlot
  {
  public:
    int Id;
    vtkKWEntry *Entry;
  };

  typedef vtkLinkedList<EntrySlot*> EntriesContainer;
  typedef vtkLinkedListIterator<EntrySlot*> EntriesContainerIterator;
  EntriesContainer *Entries;

  // Helper methods

  EntrySlot* GetEntrySlot(int id);

  //ETX

  void Pack();

private:
  vtkKWEntrySet(const vtkKWEntrySet&); // Not implemented
  void operator=(const vtkKWEntrySet&); // Not implemented
};

#endif

