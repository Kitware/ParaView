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
  // Add a scale to the set.
  // The id has to be unique among the set.
  // Object and method parameters, if any, will be used to set the command.
  // A help string will be used, if any, to set the baloon help. 
  // Return 1 on success, 0 otherwise.
  int AddScale(int id, 
               vtkKWObject *object = 0, 
               const char *method_and_arg_string = 0,
               const char *balloonhelp_string = 0);

  // Description:
  // Get a scale from the set, given its unique id.
  // It is advised not to temper with the scale var name or value :)
  // Return a pointer to the scale, or NULL on error.
  vtkKWScale* GetScale(int id);
  int HasScale(int id);

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

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWScaleSet(const vtkKWScaleSet&); // Not implemented
  void operator=(const vtkKWScaleSet&); // Not implemented
};

#endif

