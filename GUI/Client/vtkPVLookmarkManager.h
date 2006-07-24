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

#include "vtkKWTopLevel.h"

class vtkKWLookmarkFolder;
class vtkPVLookmark;
class vtkXMLDataElement;
class vtkKWFrame;
class vtkKWFrameWithScrollbar;
class vtkKWPushButton;
class vtkPVApplication;
class vtkPVRenderView;
class vtkKWMenu;
class vtkKWMessageDialog;
class vtkKWTextWithScrollbars;
class vtkPVTraceHelper;
class vtkPVWindow;
class vtkPVReaderModule;
class vtkTransform;
class vtkPVPickBoxWidget;

//BTX
template<class DataType> class vtkVector;
//ETX

class VTK_EXPORT vtkPVLookmarkManager : public vtkKWTopLevel
{
public:
  static vtkPVLookmarkManager* New();
  vtkTypeRevisionMacro(vtkPVLookmarkManager, vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Display the toplevel.
  virtual void Display();

  // Description:
  // Close the toplevel.
  virtual void Withdraw();

  // Description:
  // Get the trace helper framework.
  vtkGetObjectMacro(TraceHelper, vtkPVTraceHelper);

  // Description
  // Returns the first lookmark found with the given name
  vtkPVLookmark* GetPVLookmark(char *name);

  // Callbacks to interface widgets

  // Description:
  // Called from File --> Import --> Replace/Append
  // Takes the user specified .lmk file and parses it to populate the panel with its contents
  void ImportLookmarkFileCallback();
  void ImportBoundingBoxFileCallback();
  void ImportLookmarkFile(const char *path, int appendFlag);
  void ImportBoundingBoxFile(vtkPVReaderModule *reader, vtkPVLookmark *macro, char *boundingBoxFileName);
  // For tracing:
  void ImportBoundingBoxFile(char *dataFilePath, char *macroName, char *boundingBoxFileName);

  // Description:
  // Create either a Lookmark or a Lookmark Macro.
  // Adds a vtkKWLookmark widget to the lookmark manager with a default
  // name of the form "Lookmark#". Saves the state of the visible sources in the SourceList
  // and their inputs.
  void CreateLookmarkCallback(int macroFlag);
  vtkPVLookmark* CreateLookmark(char *name, int macroFlag);
  vtkPVLookmark* CreateLookmark(char *name, vtkKWWidget *parent, int location, int macroFlag);

  // Description:
  // This saves the current state of the lookmark manager, hierarchy and all to the user specified .lmk file
  void SaveAllCallback();
  void SaveAll(const char *path);

  // Description: 
  // This is called when File --> Export Folder is selected
  // Only one folder can be selected (except for folders nested in the selected folder)
  void ExportFolderCallback();

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
  void CreateFolderCallback();
  vtkKWLookmarkFolder* CreateFolder(const char *name, int macroFlag);

  // Description:
  // Check/Uncheck the boxes of all lookmarks and folders
  void SetStateOfAllLookmarkItems(int state);

  // Description:
  // This uses the backup file to repopulate the lookmark manager, bringing it up to the point just before the last operation
  void UndoCallback();
  void RedoCallback();

  // Description:
  // Only one lookmark/folder can be selected/checked when this is called. It will display a text widget in which to type the new name.
  void RenameLookmarkCallback();
  void RenameFolderCallback();

  // Description:
  // Access to the specialized vtkKWLookmarkFolder that holds the lookmark macros
  vtkKWLookmarkFolder *GetMacrosFolder();

  // Description:
  // When the manager is created, this looks for a file called #LookmarkMacros# or .LookmarkMacros in the users home directory
  // If found, this lookmark file is used to populate the Edit --> Add Macro Example menu. The idea is to have a way to 
  // distribute pre-defined macros with paraview that users might find useful for the type of data they work with.
  void ImportMacroExamplesCallback();

  // Description: 
  // Callback when an entry is selected in the Edit menu. Adds the selection to the "Macros" folder and to the Lookmark toolbar if enabled
  void AddMacroExampleCallback(int index);

  // Description:
  // Callback added to all lookmark and folder checkboxes. This does some bookkeeping like making sure that if an item nested in a selected folder
  // is deselected, the folder is also deselected
  void SelectItemCallback(char *name, int state);

  // Description:
  // Callbacks for the help menu
  void DisplayUsersTutorial();
  void DisplayQuickStartGuide();

  // Description:
  // Called when a lookmark item (lookmark or folder) is being dragged. If the mouse is over the first frame in the 
  // lookmark manager before the first lookmark item (so that user can drag to the top of the lookmark manager)
  void DragAndDropPerformCommand(int x, int y, vtkKWWidget *widget, vtkKWWidget *anchor);

  // Description:
  // All lookmark items (lookmarks and folders) in the manager are assigned this end command when the left button is release after it
  // has been dragging a lookmark item
  void DragAndDropEndCommand(int x, int y, vtkKWWidget *widget, vtkKWWidget *anchor, vtkKWWidget *target);

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

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  // convenience methods
  vtkPVApplication* GetPVApplication();
  vtkPVRenderView* GetPVRenderView(); 
  vtkPVWindow* GetPVWindow(); 

  // Description:
  // Helper function to construct path to file in user's home directory
  const char* GetPathToFileInHomeDirectory(const char* filename);

  // Help menu helper functions
  virtual void ConfigureQuickStartGuide();
  virtual void ConfigureUsersTutorial();

  // Description:
  // Sets up the drag and drop targets for each lookmark and folder in manager. Often called after a change to the lookmark manager (such as Add, Remove, etc.)
  void ResetDragAndDropTargetSetAndCallbacks();

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
  void DestroyUnusedFoldersFromWidget(vtkKWWidget *widget);
  int GetNumberOfChildLmkItems(vtkKWWidget *prnt);

  // Description:
  // These methods reassign the stored location values for all child lmk widgets or 
  // containers that are siblings in the lookmark manager hierarchy
  // depending on whether we are inserting or removing a lmk item
  // used in conjunction with Move and Remove callbacks
  void DecrementHigherSiblingLmkItemLocationIndices(vtkKWWidget *parent, int location);
  void IncrementHigherSiblingLmkItemLocationIndices(vtkKWWidget *parent, int location);

  // Description:
  // Just what the name implies - pack the lookmark items (lookmarks or folders) that are children of parent using their location values
  void PackChildrenBasedOnLocation(vtkKWWidget *parent);

  // Description:
  // helper function for removing checked lookmarks and folders that are children of parent
  void RemoveCheckedChildren(vtkKWWidget *parent, int forceRemoveFlag);

  // Description:
  // Recursively visit each xml element of the lookmark file, creating, packing, and storing vtkPVLookmarks and vtkKWLookmarkFolders as appropriate
  void ImportLookmarkFileInternal(int locationOfLmkItemAmongSiblings, vtkXMLDataElement *recursiveXmlElement, vtkKWWidget *parentWidget);
  void ImportBoundingBoxFileInternal(int locationOfLmkItemAmongSiblings, vtkXMLDataElement *recursiveXmlElement, vtkKWLookmarkFolder *parentWidget,vtkPVPickBoxWidget *box);

  // Description:
  // Recursively visit each xml element of the lookmark file, creatings entries in the "Add Macro Examples" menu
  void ImportMacroExamplesInternal(int locationOfLmkItemAmongSiblings, vtkXMLDataElement *recursiveXmlElement, vtkKWMenu *parentMenu);


  // Description:
  // Convenience method for creating a Load/Save dialog box and returning the filename chosen by the user
  // 0 for load, 1 for save
  char* PromptForFile(char *extension, int save_flag);

  // Description:
  // Takes a vtkKWLookmarkElement and uses its attributes to initialize and return a new vtkPVLookmark
  vtkPVLookmark* GetPVLookmark(vtkXMLDataElement *elem);

  // Description:
  // Necessary to encode/decode newlines in the comments text and image data before/after being written/read 
  // to/from a lookmark file since they get lost in the call to WriteObject() uses a '~' to encode
  void EncodeNewlines(char *str);
  void DecodeNewlines(char *str);

  void GetTransform(vtkTransform*,double bounds[6]);

  // Description:
  // called before and after certain callbacks that launch dialog boxes because when the user presses OK or Cancel 
  // the lookmark mgr panel is also getting the mouse down event (which causes unexpected behavior)
  void SetButtonFrameState(int state);

  // Description: 
  // prints the root and all its elements to the file
  void CreateNestedXMLElements(vtkKWWidget *wid, vtkXMLDataElement *parentElement);

  // Description: 
  // takes a filename and folder widget, writes out an empty lookmark file, parses to get at the root element, 
  // recursively calls CreateNestedXMLElement and prints the folder and all its elements to the file
  void SaveFolderInternal(char *path, vtkKWLookmarkFolder *folder);

  // Convenience method called from CreateLookmark to assign a default name to the new lookmark that 
  // does not match any of the ones currently in the lookmark manager
  // of the form: "LookmarkN" where 'N' is an integer between 0 and numberOfLookmarks
  char* GetUnusedLookmarkName();

  // Description:
  // Check to see if lookmark item is a descendant of container
  int IsWidgetInsideFolder(vtkKWWidget *widget, vtkKWWidget *folder);

  // Description:
  // Before any action that changes the state of the lookmark manager (Add,Update,etc), this 
  // method is called to write out a lookmark file in the current directory of the current state 
  // of the lookmark manager. The file name is "LookmarkManager.lmk" by default until a lookmark file is imported or one is saved out
  void Checkpoint();

  // Description:
  // Undo and redo share same underlying function
  void UndoRedoInternal();

private:

vtkPVLookmarkManager(const vtkPVLookmarkManager&); // Not implemented
void operator=(const vtkPVLookmarkManager&); // Not implemented

//BTX
  vtkVector<vtkPVLookmark*> *Lookmarks;
  vtkVector<vtkPVLookmark*> *MacroExamples;
  vtkVector<vtkKWLookmarkFolder*> *Folders;
//ETX
  
  vtkKWFrame              *WindowFrame;
  vtkKWFrameWithScrollbar *ScrollFrame;
  vtkKWFrame              *SeparatorFrame;

  vtkKWFrame *TopDragAndDropTarget;

  vtkKWMenu *MenuFile;
  vtkKWMenu *MenuEdit;
  vtkKWMenu *MenuImport;
  vtkKWMenu *MenuImportLmkFile;
  vtkKWMenu *MenuImportBoundingBoxFile;
  vtkKWMenu *MenuHelp;
  vtkKWMenu *MenuExamples;

  vtkKWPushButton *CreateLookmarkButton;

  vtkKWMessageDialog *QuickStartGuideDialog;
  vtkKWTextWithScrollbars *QuickStartGuideTxt;
  vtkKWMessageDialog *UsersTutorialDialog;
  vtkKWTextWithScrollbars *UsersTutorialTxt;

  vtkPVTraceHelper* TraceHelper;
};

#endif
