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
// comments, state script, and image data. I separated it from the interface (vtkKWLookmark). Although it should probably be a subclass of vtkLookmark
// instead of vtkObject to a) be consistent with the naming convention in paraview and b) they both share several attributes. 
// Each object of this class
// keeps a collection of the vtkPVSources used to make up the view this lookmark represents. Encapsulating loookmark data
// in one class such as this also helps when writing to the xml lookmark file since we can make use of the vtkXMLLookmarkWriter->SetObject()
// method.
//
// .SECTION See Also
// vtkObject vtkPVLookmarkManager

#ifndef __vtkPVLookmark_h
#define __vtkPVLookmark_h

#include "vtkKWLookmark.h"

class vtkKWApplication;
class vtkPVSource;
class vtkPVSourceCollection;
class vtkRenderWindow;
class vtkPVApplication;
class vtkPVRenderView;
class vtkPVLookmarkManager;

class VTK_EXPORT vtkPVLookmark : public vtkKWLookmark
{
public:

  static vtkPVLookmark* New();
  vtkTypeRevisionMacro(vtkPVLookmark,vtkKWLookmark);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description: 
  // It is called when the user clicks on a lookmark's thumbnail
  // If "Lock to Dataset" is ON, it tries to find/open the lookmark's dataset and apply the pipeline
  // to it. Otherwise, the lookmark is applied to the currently selected source's dataset, maintaining
  // the current camera view and timestep. 
  // The reader is initialized using the attributes in the lookmark's state script. The rest of the
  // script is then executed.
  void View();

  // Description:
  // Updates the lookmark's icon and state while maintaining any existing name, comments, etc.
  void Update();

  // Description:
  // Converts the image in the render window to a vtkKWIcon stored with the lookmark
  void CreateIconFromMainView();

  // Description:
  // Given an encoded string of raw image data, set up the lookmark's icon
  void CreateIconFromImageData();

  // Description:
  // Called from Add and Update. 
  // Turns vtkPVWindow->SaveVisibleSourcesOnlyFlag ON before the call to SaveState and OFF afterwards.
  // This ensures that only those sources that are visible, and all of their inputs leading up to the reader, are saved.
  // Writes out the current session state and stores in lmk.
  void StoreStateScript();

  //Description:
  // This is simply a 'dump' of the current session state script at the time the lookmark is created. However, it should only contain the state information
  // for the vtkPVSources that 'contribute' to the view, meaning visible 'leaf' node filters and any visible or nonvisible sources between it and the reader.
  vtkGetStringMacro(StateScript);
  vtkSetStringMacro(StateScript);
 
  // Description:
  // This is the raw base64 encoded image data that makes up the thumbnail
  vtkGetStringMacro(ImageData);
  vtkSetStringMacro(ImageData);

  // Description:
  // This is the focal point of the lookmark's view at creation time
  vtkGetVector3Macro(CenterOfRotation,float); 
  vtkSetVector3Macro(CenterOfRotation,float); 

  // Description:
  // The value represents this lookmark widget's packing location among sibling lmk widgets and lmk containers.
  // Used for moving widget.
  vtkGetMacro(Location,int);
  vtkSetMacro(Location,int);

  // Description:
  // A hack to make sure the scrollbar in the lookmark manager is enabled after import operations
  // need to find a better way.
  void EnableScrollBar();

protected:

  vtkPVLookmark();
  ~vtkPVLookmark();

  // convenience methods
  vtkPVApplication *GetPVApplication();
  vtkPVRenderView* GetPVRenderView(); 
  vtkPVLookmarkManager* GetPVLookmarkManager(); 

  // Description:
  // When a lookmark is recreated/viewed and the stored state script is parsed, each filter that is created gets stored in this object's vtkPVSourceCOllection
  void AddPVSource(vtkPVSource *src);

  // Description:
  // Each time before a lookmark is recreated this method is called which *attempts* to delete the vtkPVSources stored in this lookmakr's collection. 
  // Of course if one of these filters have been set as input to another one it cannot and will not be deleted. This helps in cleaning up the Source window.
  int DeletePVSources();

  // called when lookmark's thumbnail is pressed and "Lock to dataset" is OFF
  void ViewLookmarkWithCurrentDataset();

  // Description:
  // An added or updated lookmark widgets uses the return value of this to setup its thumbnail
  vtkKWIcon *GetIconOfRenderWindow(vtkRenderWindow *window);

  // Description:
  // performs a base64 encoding on the raw image data of the kwicon
  char *GetEncodedImageData(vtkKWIcon *lmkIcon);

  // Description:
  // Assigns the vtkKWIcon to the lookmark's vtkKWLabel
  void SetLookmarkImage(vtkKWIcon *icon);

  void SetLookmarkIconCommand();
  void UnsetLookmarkIconCommand();

  // Description:
  // helper functions for ViewLookmarkCalllback
  void TurnFiltersOff();
  vtkPVSource *SearchForDefaultDatasetInSourceList();
  // Description:
  // This is a big function because I'm triying to do it all in one pass
  // parses the reader portion of the state file and uses it to initialize the reader module (both parameter and display settings)
  // the rest of the script is then executed, adding created filters to the lmk's collection as we go, and saving the visibility of 
  // the filters so that we can go back and set reset them at the end
  void ParseAndExecuteStateScript(vtkPVSource *reader, char *state, int useDatasetFlag);
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

  char* StateScript;
  char* ImageData;
  float* CenterOfRotation;
  vtkPVSourceCollection* Sources;
  int Location;

private:
  vtkPVLookmark(const vtkPVLookmark&); // Not implemented
  void operator=(const vtkPVLookmark&); // Not implemented
};

#endif

