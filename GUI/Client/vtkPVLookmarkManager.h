/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLookmarkManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/


// .NAME vtkPVLookmarkManager - Interface panel to interact with lookmarks.
// .SECTION Description
// This class manages interactions with lookmarks.
// 
// The menu bar contains the following menus:
// File Menu    -> Lookmark File manipulations
// Edit Menu    -> Performs operations on lookmarks and folders
// Help         -> Brings up help dialog boxes
// 
// .SECTION See Also
// vtkPVLookmark, vtkKWLookmark, vtkKWLookmarkFolder, vtkXMLLookmarkElement


#ifndef __vtkPVLookmarkManager_h
#define __vtkPVLookmarkManager_h

#include "vtkPVTracedWidget.h"

class vtkKWLookmarkFolder;
class vtkKWLookmark;
class vtkPVLookmark;
class vtkXMLDataElement;
class vtkKWIcon;
class vtkKWFrame;
class vtkKWPushButton;
class vtkPVApplication;
class vtkPVSource;
class vtkPVRenderView;
class vtkRenderWindow;
class vtkKWMenu;
class vtkKWMessageDialog;
class vtkKWText;

//BTX
template<class DataType> class vtkVector;
//ETX

class VTK_EXPORT vtkPVLookmarkManager : public vtkPVTracedWidget
{
public:
  static vtkPVLookmarkManager* New();
  vtkTypeRevisionMacro(vtkPVLookmarkManager, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the interface objects.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Close/Display the window
  void Display();
  void CloseCallback();

  // Description:
  // Set the window to which the dialog will be slave.
  // If set, this dialog will always be on top of the master
  // window and will minimize with it (assuming that the windowing
  // system supports this)
  void SetMasterWindow(vtkKWWindow* win);

  // Callbacks to interface widgets

  // Description:
  // Called from File --> Import --> Replace/Append
  // Takes the user specified .lmk file and parses it to populate the panel with its contents
  void ImportLookmarksCallback();

  // Description:
  // Invoked from either the push button in the panel or via Edit --> Create Lookmark.
  // Adds a vtkKWLookmark widget to the bottom of the lookmark manager with a default
  // name of the form "Lookmark#". Saves the state of the visible sources in the SourceList
  // and their inputs. The window is moved behind the render window to save the image, but is
  // then moved back in front.
  void CreateLookmarkCallback();

  // Description: 
  // This should be moved into vtkPVLookmark
  // It is called when the user clicks on a lookmark's thumbnail
  // If "Lock to Dataset" is ON, it tries to find/open the lookmark's dataset and apply the pipeline
  // to it. Otherwise, the lookmark is applied to the currently selected source's dataset, maintaining
  // the current camera view and timestep. 
  // The reader is initialized using the attributes in the lookmark's state script. The rest of the
  // script is then executed.
  void ViewLookmarkCallback(vtkIdType i);

  // Description:
  // This saves the current state of the lookmark manager, hierarchy and all to the user specified .lmk file
  void SaveLookmarksCallback();

  // Description: 
  // This is called when File --> Export Folder is selected
  // Only one folder can be selected (except for folders nested in the selected folder)
  void SaveFolderCallback();

  // Description: 
  // This updates the lookmark's thumbnail, statescript, and dataset, while maintaining its name,
  // comments, and location in the lookmark manager
  void UpdateLookmarkCallback();

  // Description:
  // This removes the selected lookmarks and/or folders from the lookmark manager
  void RemoveCallback();

  // Description:
  // Add an empty folder to the bottom of the lookmark manager named "New Folder"
  // You can then drag items into it
  void NewFolderCallback();

  // Description:
  // Check/Uncheck the boxes of all lookmarks and folders
  void AllOnOffCallback(int flag);

  // Description:
  // This uses the backup file to repopulate the lookmark manager, bringing it up to the point just before the last operation
  void UndoCallback();

  // Description:
  // Only one lookmark/folder can be selected/checked when this is called. It will display a text widget in which to type the new name.
  void RenameLookmarkCallback();
  void RenameFolderCallback();

  // Description:
  // Callbacks for the help menu
  void DisplayUsersTutorial();
  void DisplayQuickStartGuide();

  // Description:
  // All lookmark items (lookmarks and folders) in the manager are assigned these end and perform drag and drop commands
  void DragAndDropEndCommand(int x, int y, vtkKWWidget *widget, vtkKWWidget *anchor, vtkKWWidget *target);
  void DragAndDropPerformCommand(int x, int y, vtkKWWidget *widget, vtkKWWidget *anchor);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:

  vtkPVLookmarkManager();
  ~vtkPVLookmarkManager();

  // convenience methods
  vtkGetObjectMacro(PVApplication,vtkPVApplication);
  vtkPVRenderView* GetPVRenderView(); 

  // Help menu helper functions
  virtual void ConfigureQuickStartGuide();
  virtual void ConfigureUsersTutorial();

  // The following methods should be moved to vtkPVLookmark:

  // called when lookmark's thumbnail is pressed and "Lock to dataset" is OFF
  void ViewLookmarkWithCurrentDataset(vtkPVLookmark *lmk);
  void SetLookmarkIconCommand(vtkKWLookmark *lmk, vtkIdType index);
  void UnsetLookmarkIconCommand(vtkKWLookmark *lmk);
  // Description:
  // helper functions for ViewLookmarkCalllback
  void TurnFiltersOff();
  vtkPVSource *SearchForDefaultDatasetInSourceList(char *datasetName);
  // Description:
  // This is a big function because I'm triying to do it all in one pass
  // parses the reader portion of the state file and uses it to initialize the reader module (both parameter and display settings)
  // the rest of the script is then executed, adding created filters to the lmk's collection as we go, and saving the visibility of 
  // the filters so that we can go back and set reset them at the end
  void ParseAndExecuteStateScript(vtkPVSource *reader, char *state, vtkPVLookmark *lmk, int useDatasetFlag);
  // Description: 
  // helper functions when parsing
  int GetArrayStatus(char *name, char *line);
  int GetIntegerScalarWidgetValue(char *line);
  double GetDoubleScalarWidgetValue(char *line);
  void GetDoubleVectorWidgetValue(char *line,double *x,double *y, double *z);
  char *GetReaderTclName(char *line);
  char *GetFieldName(char *line);
  char *GetFieldNameAndValue(char *line, int *val);
  char *GetStringEntryValue(char *line);
  char *GetStringValue(char *line);
  char *GetVectorEntryValue(char *line);
  // Description:
  // An added or updated lookmark widgets uses the return value of this to setup its thumbnail
  vtkKWIcon *GetIconOfRenderWindow(vtkRenderWindow *window);
  // Description:
  // performs a base64 encoding on the raw image data of the kwicon
  char *GetEncodedImageData(vtkKWIcon *lmkIcon);
  // Description:
  // Called from Add and Update
  // writes out the current session state and stores in lmk
  void StoreStateScript(vtkPVLookmark *lmk);

  // The following are drag-and-drop functions

  // Description:
  // Sets up the drag and drop targets for each lookmark and folder in manager. Often called after a change to the lookmark manager (such as Add, Remove, etc.)
  void ResetDragAndDropTargetsAndCallbacks();
  // Description:
  // Perform the actual D&D given a widget and its target location.
  // It will call AddDragAndDropEntry() and pack the widget to its new location
  virtual int DragAndDropWidget( vtkKWWidget *widget, vtkKWWidget *destAfterWidget);
  // Description:
  // When a lookmark or folder is deleted need to remove it as a target for all other widgets
  void RemoveItemAsDragAndDropTarget(vtkKWWidget *removedTarget);
  // Description
  // Helper functions for drag and drop capability
  void MoveCheckedChildren(vtkKWWidget *recursiveWidget, vtkKWWidget *frameToPackLmkItemInto);
  void DestroyUnusedLmkWidgets(vtkKWWidget *widget);
  int GetNumberOfChildLmkItems(vtkKWWidget *prnt);
  // Description:
  // These methods reassign the stored location values for all child lmk widgets or 
  // containers that are siblings in the lookmark manager hierarchy
  // depending on whether we are inserting or removing a lmk item
  // used in conjunction with Move and Remove callbacks
  void DecrementHigherSiblingLmkItemLocationIndices(vtkKWWidget *parent, int location);
  void IncrementHigherSiblingLmkItemLocationIndices(vtkKWWidget *parent, int location);
  // Description:
  // Just what the name implies - pack the child lmk items at a certain level in the hierarchy using their location values
  void PackChildrenBasedOnLocation(vtkKWWidget *parent);


  // helper function for removing checked lookmarks and folders
  void RemoveCheckedChildren(vtkKWWidget *parent, int forceRemoveFlag);

  // Description:
  // Recursively visit each xml element of the lookmark file, creating, packing, and storing vtkKWLookmark, vtkPVLookmark, and vtkKWLookmarkFolders as appropriate
  void ImportLookmarksInternal(int locationOfLmkItemAmongSiblings, vtkXMLDataElement *recursiveXmlElement, vtkKWWidget *parentWidget);

  // Description:
  // Convenience method for creating a Load/Save dialog box and returning the filename chosen by the user
  // 0 for load, 1 for save
  char* PromptForLookmarkFile(int save_flag);

  // Description:
  // Takes a vtkKWLookmarkElement and uses its attributes to initialize and return a new vtkPVLookmark
  vtkPVLookmark* GetPVLookmark(vtkXMLDataElement *elem);

  // Description:
  // Necessary to encode/decode newlines in the comments text and image data before/after being written/read to/from a lookmark file since they get lost in the call to WriteObject()
  // uses a '~' to encode
  void EncodeNewlines(char *str);
  void DecodeNewlines(char *str);

  // Description:
  // called before and after certain callbacks that launch dialog boxes because when the user presses OK or Cancel 
  // the lookmark mgr panel is also getting the mouse down event (which causes unexpected behavior)
  void SetButtonFrameState(int state);

  // Description: 
  // takes a filename, writes out an empty lookmark file, parses to get at the root element, 
  // recursively calls CreateNestedXMLElement and prints the root and all its elements to the file
  void SaveLookmarksInternal(char *path);
  void SaveLookmarksInternal(ostream *os);
  void CreateNestedXMLElements(vtkKWWidget *wid, vtkXMLDataElement *parentElement);

  // Description: 
  // takes a filename and folder widget, writes out an empty lookmark file, parses to get at the root element, 
  // recursively calls CreateNestedXMLElement and prints the folder and all its elements to the file
  void SaveFolderInternal(char *path, vtkKWLookmarkFolder *folder);

  // Convenience method called from CreateLookmark to assign a default name to the new lookmark that does not match any of the ones currently in the lookmark manager
  // of the form: "LookmarkN" where 'N' is an integer between 0 and numberOfLookmarks
  char* GetUnusedLookmarkName();

  // Description:
  // Updates the lookmark at lmkIndex location in array
  void UpdateLookmarkInternal(vtkIdType lmkIndex);

  // Description:
  // Check to see if lmkItem is a descendant of container
  int IsWidgetInsideFolder(vtkKWWidget *container,vtkKWWidget *lmkItem);

  // Description:
  // Before any action that changes the state of the lookmark manager (Add,Update,etc), this method is called to write out a lookmark file 
  // in the current directory of the current state of the lookmark manager. The file name is "LookmarkManager.lmk" by default until a lookmark file is imported or one is saved out
  void Checkpoint();

  // Description:
  // Set the title of the window
  vtkSetStringMacro(Title);

private:

vtkPVLookmarkManager(const vtkPVLookmarkManager&); // Not implemented
void operator=(const vtkPVLookmarkManager&); // Not implemented

//BTX
  vtkVector<vtkPVLookmark*> *PVLookmarks;
  vtkVector<vtkKWLookmark*> *KWLookmarks;
  vtkVector<vtkKWLookmarkFolder*> *LmkFolderWidgets;
//ETX
  
  vtkKWFrame *LmkPanelFrame;
  vtkKWFrame *LmkScrollFrame;
  vtkKWFrame *SeparatorFrame;

  vtkKWFrame *TopDragAndDropTarget;
  vtkKWFrame *BottomDragAndDropTarget;

  vtkKWMenu *Menu;
  vtkKWMenu *MenuFile;
  vtkKWMenu *MenuEdit;
  vtkKWMenu *MenuImport;
  vtkKWMenu *MenuHelp;

  vtkKWPushButton *CreateLmkButton;

  vtkKWMessageDialog *QuickStartGuideDialog;
  vtkKWText *QuickStartGuideTxt;
  vtkKWMessageDialog *UsersTutorialDialog;
  vtkKWText *UsersTutorialTxt;

  vtkPVApplication *PVApplication;

  char *Title;
  vtkKWWindow* MasterWindow;

};

#endif
