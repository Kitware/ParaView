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

#include "vtkObject.h"

class vtkKWApplication;
class vtkPVSource;
class vtkPVSourceCollection;

class VTK_EXPORT vtkPVLookmark : public vtkObject
{
public:

  static vtkPVLookmark* New();
  vtkTypeRevisionMacro(vtkPVLookmark,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The name of the lookmark. Always the same as the one displayed in the lookmark widget.
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // This is only allocated and written to right before a lookmark is about to be written to a lookmark file.
  // The newlines contained herein are encoded to '~' before writing because they are lost when the ->SetObject(vtkPVLookmark) method is called
  vtkGetStringMacro(Comments);
  vtkSetStringMacro(Comments);
  
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
  // The full path to the lookmark's 'default dataset'. This is originally just set to the dataset from which the lookmark was created but can later be set to 
  // a different one by turning 'Use default dataset' option OFF in the loookmark manager and setting the 'use as default dataset' option in the dialog box
  vtkGetStringMacro(Dataset);
  vtkSetStringMacro(Dataset);

  // Description:
  // When a lookmark is recreated/viewed and the stored state script is parsed, each filter that is created gets stored in this object's vtkPVSourceCOllection
  void AddPVSource(vtkPVSource *src);

  // Description:
  // Each time before a lookmark is recreated this method is called which *attempts* to delete the vtkPVSources stored in this lookmakr's collection. 
  // Of course if one of these filters have been set as input to another one it cannot and will not be deleted. This helps in cleaning up the Source window.
  int DeletePVSources();

protected:

  vtkPVLookmark();
  ~vtkPVLookmark();
  
  char* Name;
  char* Comments;
  char* StateScript;
  char* ImageData;
  char* Dataset;
  float* CenterOfRotation;
  vtkPVSourceCollection* Sources;

private:
  vtkPVLookmark(const vtkPVLookmark&); // Not implemented
  void operator=(const vtkPVLookmark&); // Not implemented
};

#endif

