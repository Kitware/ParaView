/*=========================================================================

  Module:    vtkKWScaleSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWScaleSet - a "set of scales" widget
// .SECTION Description
// A simple widget representing a set of scales. Scales
// can be created, removed or queried based on unique ID provided by the user
// (ids are not handled by the class since it is likely that they will be 
// defined as enum's or #define by the user for easier retrieval, instead
// of having ivar's that would store the id's returned by the class).
// Scales are packed (gridded) in the order they were added.

#ifndef __vtkKWScaleSet_h
#define __vtkKWScaleSet_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWScale;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWScaleSet : public vtkKWWidget
{
public:
  static vtkKWScaleSet* New();
  vtkTypeRevisionMacro(vtkKWScaleSet,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget (a frame holding all the scales).
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Add a scale (or popup scale) to the set.
  // The id has to be unique among the set.
  // Object and method parameters, if any, will be used to set the command.
  // A help string will be used, if any, to set the baloon help. 
  // Return 1 on success, 0 otherwise.
  int AddScale(int id, 
               vtkKWObject *object = 0, 
               const char *method_and_arg_string = 0,
               const char *balloonhelp_string = 0);
  int AddPopupScale(int id, 
                    vtkKWObject *object = 0, 
                    const char *method_and_arg_string = 0,
                    const char *balloonhelp_string = 0);

  // Description:
  // Get a scale from the set, given its unique id.
  // It is advised not to temper with the scale var name or value :)
  // Return a pointer to the scale, or NULL on error.
  vtkKWScale* GetScale(int id);
  int HasScale(int id);
  int GetNumberOfScales();

  // Description:
  // Convenience method to hide/show a scale
  void HideScale(int id);
  void ShowScale(int id);
  void SetScaleVisibility(int id, int flag);
  int GetNumberOfVisibleScales();

  // Description:
  // Remove all scales
  void DeleteAllScales();

  // Description:
  // Set the widget packing order to be horizontal (default is vertical).
  // This means that given the insertion order of the scale in the set,
  // the scales will be packed in the horizontal direction.
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
  // Set the scales padding.
  virtual void SetPadding(int x, int y);

  // Description:
  // Set the scales border width.
  virtual void SetBorderWidth(int bd);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Synchronize the width of the first label of the labeled scales. 
  // The maximum size is found and assigned to each label. 
  virtual void SynchroniseLabelsMaximumWidth();

protected:
  vtkKWScaleSet();
  ~vtkKWScaleSet();

  int PackHorizontally;
  int MaximumNumberOfWidgetInPackingDirection;
  int PadX;
  int PadY;

  //BTX

  // A scale slot associates a scale to a unique Id
  // No, I don't want to use a map between those two, for the following 
  // reasons:
  // a), we might need more information in the future, b) a map 
  // Register/Unregister pointers if they are pointers to VTK objects.
 
  class ScaleSlot
  {
  public:
    int Id;
    vtkKWScale *Scale;
  };

  typedef vtkLinkedList<ScaleSlot*> ScalesContainer;
  typedef vtkLinkedListIterator<ScaleSlot*> ScalesContainerIterator;
  ScalesContainer *Scales;

  // Helper methods

  ScaleSlot* GetScaleSlot(int id);

  //ETX

  void Pack();
  int AddScaleInternal(int id, 
                       int popup_mode,
                       vtkKWObject *object, 
                       const char *method_and_arg_string, 
                       const char *balloonhelp_string);

private:
  vtkKWScaleSet(const vtkKWScaleSet&); // Not implemented
  void operator=(const vtkKWScaleSet&); // Not implemented
};

#endif

