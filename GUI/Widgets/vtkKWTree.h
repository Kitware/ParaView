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
// .NAME vtkKWTree - tree widget
// .SECTION Description
// A simple tree widget

#ifndef __vtkKWTree_h
#define __vtkKWTree_h

#include "vtkKWCoreWidget.h"

class KWWIDGETS_EXPORT vtkKWTree : public vtkKWCoreWidget
{
public:
  static vtkKWTree* New();
  vtkTypeRevisionMacro(vtkKWTree,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the selection to node
  virtual void SetSelectionToNode(const char *node);

  // Description:
  // Clear the selection
  virtual void ClearSelection();

  // Description:
  // Return the selection as a list of space separated selected nodes
  virtual const char* GetSelection();

  // Description:
  // Return if a node is selected
  virtual int HasSelection();

  // Description:
  // Add a new node identified by 'node' at the end of the children list of 
  // 'parent'. If parent is NULL, or an emptry string or 'root', insert at the
  // root of the tree automatically.
  // Provides its text (i.e. the label displayed at the node 
  // position), an optional user-data field to associate with that node,
  // its open and selectable status.
  // On a Pentium M 1.8 GHz, a Debug build could fill about 5000 nodes/s.
  virtual void AddNode(const char *parent,
                       const char *node,
                       const char *text = NULL,
                       const char *data = NULL,
                       int is_open = 0,
                       int is_selectable = 1);

  // Description:
  // Arrange the tree to see a given node
  virtual void SeeNode(const char *node);

  // Description:
  // Open/close a node.
  virtual void OpenNode(const char *node);
  virtual void CloseNode(const char *node);
  virtual int IsNodeOpen(const char *node);

  // Description:
  // Open/close the first node of the tree.
  virtual void OpenFirstNode();
  virtual void CloseFirstNode();

  // Description:
  // Open/close a tree, i.e. a node and all its children.
  virtual void OpenTree(const char *node);
  virtual void CloseTree(const char *node);

  // Description:
  // Query if given node exists in the tree
  virtual int HasNode(const char *node);

  // Description:
  // Delete all nodes
  virtual void DeleteAllNodes();

  // Description:
  // Get node's children as a space separated list of nodes
  virtual const char* GetNodeChildren(const char *node);

  // Description:
  // Get node's parent
  virtual const char* GetNodeParent(const char *node);

  // Description:
  // Set/Get the parameters
  virtual const char* GetNodeUserData(const char *node);
  virtual void SetNodeUserData(const char *node, const char *data);
  virtual const char* GetNodeText(const char *node);
  virtual void SetNodeText(const char *node, const char *text);
  virtual int GetNodeSelectableFlag(const char *node);
  virtual void SetNodeSelectableFlag(const char *node, int flag);
  virtual const char* GetNodeFont(const char *node);
  virtual void SetNodeFont(const char *node, const char *font);
  virtual void SetNodeFontWeightToBold(const char *node);
  virtual void SetNodeFontWeightToNormal(const char *node);
  virtual void SetNodeFontSlantToItalic(const char *node);
  virtual void SetNodeFontSlantToRoman(const char *node);

  // Description:
  // Set/Get the width/height.
  virtual void SetWidth(int);
  virtual int GetWidth();
  virtual void SetHeight(int);
  virtual int GetHeight();

  // Description:
  // Specifies wether or not the tree should be redrawn when entering idle. 
  // Set it to false if you call update while modifying the tree
  vtkBooleanMacro(RedrawOnIdle, int);
  virtual void SetRedrawOnIdle(int);
  virtual int GetRedrawOnIdle();

  // Description:
  // If true, the selection box will be drawn across the entire tree from
  // left-to-right instead of just around the item text.
  vtkBooleanMacro(SelectionFill, int);
  virtual void SetSelectionFill(int);
  virtual int GetSelectionFill();

  // Description:
  // Set/Get the selection foreground and background color
  virtual void GetSelectionBackgroundColor(double *r, double *g, double *b);
  virtual double* GetSelectionBackgroundColor();
  virtual void SetSelectionBackgroundColor(double r, double g, double b);
  virtual void SetSelectionBackgroundColor(double rgb[3])
    { this->SetSelectionBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void GetSelectionForegroundColor(double *r, double *g, double *b);
  virtual double* GetSelectionForegroundColor();
  virtual void SetSelectionForegroundColor(double r, double g, double b);
  virtual void SetSelectionForegroundColor(double rgb[3])
    { this->SetSelectionForegroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set the method to be called when the user opens a node. The path of the
  // opened node is appended to the command.
  virtual void SetOpenCommand(vtkKWObject *obj, const char *method);

  // Description:
  // Set the method to be called when the user closes a node. The path of the
  // closed node is appended to the command.
  virtual void SetCloseCommand(vtkKWObject *obj, const char *method);

  // Description:
  // Associates a object/method to execute whenever the event sequence given 
  // by 'event' occurs on the label of a node. The node idenfier on which
  // the event occurs is appended to the command.
  virtual void SetBindText(
    const char *event, vtkKWObject *obj, const char *method);

  // Description:
  // Convenience method to set the callback for single click and double
  // click on a node. This, in turn, just calls SetBindText.
  virtual void SetDoubleClickOnNodeCommand(
    vtkKWObject *obj, const char *method);
  virtual void SetSingleClickOnNodeCommand(
    vtkKWObject *obj, const char *method);
 
  // Description:
  // Set the callback to invoke when the selection changes.
  virtual void SetSelectionChangedCommand(
    vtkKWObject *obj, const char *method);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWTree();
  ~vtkKWTree() {};

private:
  vtkKWTree(const vtkKWTree&); // Not implemented
  void operator=(const vtkKWTree&); // Not implemented
};

#endif
