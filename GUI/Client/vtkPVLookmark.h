/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLookmark.h

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

// .NAME vtkPVLookmark - An interface widget for a container of lookmarks in the Lookmark Manager
// .SECTION Description
// This class represents a lookmark object and stores several attributes such as name, default dataset,
// comments, state script, and image data. I separated it from the interface (vtkKWLookmark). Encapsulating loookmark data
// in one class such as this also helps when writing to the xml lookmark file since we can make use of the vtkXMLLookmarkWriter->SetObject()
// method.
//
// .SECTION See Also
// vtkObject vtkPVLookmarkManager, vtkKWLookmark

#ifndef __vtkPVLookmark_h
#define __vtkPVLookmark_h

#include "vtkKWLookmark.h"

class vtkPVSource;
class vtkPVSourceCollection;
class vtkRenderWindow;
class vtkPVApplication;
class vtkPVRenderView;
class vtkPVLookmarkManager;
class vtkPVTraceHelper;
class vtkKWPushButton;
class vtkPVLookmarkObserver;
class vtkKWIcon;
class vtkPVWindow;

class VTK_EXPORT vtkPVLookmark : public vtkKWLookmark
{
public:

  static vtkPVLookmark* New();
  vtkTypeRevisionMacro(vtkPVLookmark,vtkKWLookmark);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the trace helper framework.
  vtkGetObjectMacro(TraceHelper, vtkPVTraceHelper);

  // Description: 
  // This will execute the lookmark. It is called when the user clicks on a lookmark's thumbnail.
  // It tries to find/open the lookmark's dataset(s) and/or source(s) and apply the pipeline
  // to them. The reader is initialized using the attributes in the lookmark's state script. The rest of the
  // script is then executed.
  void PreView();
  void ViewCallback();
  void View();

  // Description: 
  // This is called when the user clicks on a macro's thumbnail. 
  // It uses the stored script to initialize the currently selected dataset, maintaining
  // the current camera view and timestep. 
  void PreViewMacro();
  void ViewMacroCallback();
  void ViewMacro();

  void ReleaseEvent();

  // Description:
  // Initialize this pvlookmark using the ivars of the the one being passed
  void Clone(vtkPVLookmark*& lmk);

  // Description:
  // Updates the lookmark's icon and state while maintaining any existing name, comments, etc.
  void Update();

  // Description:
  // Converts the image in the render window to a vtkKWIcon stored with the lookmark - called when lookmark is first being created or updated
  void CreateIconFromMainView();

  // Description: 
  // Use ivars to update widgets
  void UpdateWidgetValues();

  // Description:
  // Called when lookmark is being created or updated. 
  // Turns vtkPVWindow->SaveVisibleSourcesOnlyFlag ON before the call to SaveState and OFF afterwards.
  // This ensures that only those sources that are visible, and all of their inputs leading up to the reader, are saved.
  // Writes out the current session state and stores in lmk.
  // Also initializes the comments field with filter names if first being created
  void StoreStateScript();

  //Description:
  // This is simply a 'dump' of the current session state script at the time the lookmark is created. However, it should only contain the state information
  // for the vtkPVSources that 'contribute' to the view, meaning visible 'leaf' node filters and any visible or nonvisible sources between it and the reader.
  vtkGetStringMacro(StateScript);
  vtkSetStringMacro(StateScript);

  // Description:
  // Store the paraview application version with lookmark to support backwards compatability in the future
  vtkGetStringMacro(Version);
  vtkSetStringMacro(Version);
 
  // Description:
  // This is the raw base64 encoded image data that makes up the thumbnail
  vtkGetStringMacro(ImageData);
  vtkSetStringMacro(ImageData);

  // Description:
  // This is the focal point of the lookmark's view at creation time
  vtkGetVector3Macro(CenterOfRotation,float); 
  vtkSetVector3Macro(CenterOfRotation,float); 

  // Description:
  // The value represents this lookmark widget's packed location among sibling lmk widgets and lmk folder.
  // Used for moving widget.
  vtkGetMacro(Location,int);
  vtkSetMacro(Location,int);

  // Description:
  // A hack to make sure the scrollbar in the lookmark manager is enabled after import operations
  // need to find a better way.
  void EnableScrollBar();

  // Description:
  // Called from vtkPVSource when being deleted to remove itself from this lookmark's collection of pvsources
  void RemovePVSource(vtkPVSource *src);

  // Description:
  // Adding a copy of this lookmark's thumbnail to the window's Lookmark Toolbar if this is a macro
  vtkGetObjectMacro(ToolbarButton,vtkKWPushButton);
  void AddLookmarkToolbarButton(vtkKWIcon *icon);

  // Description:
  // Finds which readers or sources should be stored with this lookmark when created and initializes the dataset ivar
  void InitializeDataset();

  // Description:
//BTX
  vtkSetVectorMacro(Bounds,double,6);
  vtkGetVectorMacro(Bounds,double,6);
//ETX

  // Description:
  // Was this lookmark generated using a bounding box?
  vtkGetMacro(BoundingBoxFlag,int);
  vtkSetMacro(BoundingBoxFlag,int);

protected:

  vtkPVLookmark();
  ~vtkPVLookmark();
  
  // Description:
  // Since there could be entire pipelines in the selection window that are invisible and thus be ignored when creating this lookmark
  // this is a check to see if atleast one of a reader's or source's filters are visible and thus contribute to the lookmark view
  int IsSourceOrOutputsVisible(vtkPVSource *src,int visibilityFlag);

  // convenience methods
  vtkPVApplication *GetPVApplication();
  vtkPVRenderView* GetPVRenderView(); 
  vtkPVWindow* GetPVWindow(); 
  vtkPVLookmarkManager* GetPVLookmarkManager(); 

  virtual void ExecuteEvent(vtkObject* , unsigned long event, void* calldata);
//BTX
  vtkPVLookmarkObserver* Observer;
  friend class vtkPVLookmarkObserver;
//ETX

  // Description:
  // When a lookmark is recreated/viewed and the stored state script is parsed, each filter that is created gets stored in this object's vtkPVSourceCOllection
  void AddPVSource(vtkPVSource *src);

  // Description:
  // Each time before a lookmark is recreated this method is called which *attempts* to delete the vtkPVSources stored in this lookmakr's collection. 
  // Of course if one of these filters have been set as input to another one it cannot and will not be deleted. This helps in cleaning up the Source window.
  int DeletePVSources();

  // Description:
  // An added or updated lookmark widget uses the return value of this to setup its thumbnail
  vtkKWIcon *GetIconOfRenderWindow(vtkRenderWindow *window);

  // Description:
  // performs a base64 encoding on the raw image data of the kwicon
  char *GetEncodedImageData(vtkKWIcon *lmkIcon);

  // Description: 
  // This is used to make sure the user cannot click on a lookmark at certain times
  void SetLookmarkIconCommand();
  void UnsetLookmarkIconCommand();

  // Description:
  // helper functions when viewing this lookmark
  void TurnFiltersOff();
  void TurnScalarBarsOff();

  // Description:
  // This is a big function because I'm triying to do it all in one pass
  // parses the reader portion of the state file and uses it to initialize the reader module (both parameter and display settings)
  // the rest of the script is then executed, adding created filters to the lmk's collection as we go, and saving the visibility of 
  // the filters so that we can go back and set reset them at the end
  void ParseAndExecuteStateScript(char *state, int macroFlag);
  vtkPVSource *GetReaderForLookmark(vtkPVSourceCollection *col,char *module, char *pathname, int &newDatasetFlag, int &updateLookmarkFlag);
  vtkPVSource *GetReaderForMacro(vtkPVSourceCollection *col, char *pathname);
  vtkPVSource *GetSourceForLookmark(vtkPVSourceCollection *col,char *sourcename);
  vtkPVSource *GetSourceForMacro(vtkPVSourceCollection *col,char *sourcename);

  char *Version;
  char* StateScript;
  char* ImageData;
  float* CenterOfRotation;
  vtkPVSourceCollection* Sources;
  int Location;
  unsigned long ErrorEventTag;
  int ReleaseEventFlag;

  vtkPVTraceHelper* TraceHelper;

  vtkKWPushButton *ToolbarButton;

  double Bounds[6];
  int BoundingBoxFlag;

private:
  vtkPVLookmark(const vtkPVLookmark&); // Not implemented
  void operator=(const vtkPVLookmark&); // Not implemented
};

#endif

