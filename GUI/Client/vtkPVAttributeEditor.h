/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAttributeEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAttributeEditor - A special PVSource.
// .SECTION Description
// This class will set up defaults for thePVData.
// It will also create a special PointLabelDisplay.
// Both of these features should be specified in XML so we
// can get rid of this special case.


#ifndef __vtkPVAttributeEditor_h
#define __vtkPVAttributeEditor_h

#include "vtkPVSource.h"

class vtkSMPointLabelDisplay;
class vtkCollection;
class vtkKWFrame;
class vtkKWLabel;
class vtkDataSetAttributes;
class vtkPVFileEntry;
class vtkKWPushButton;
class vtkKWEntry;
class vtkCallbackCommand;
class vtkPVReaderModule;


class VTK_EXPORT vtkPVAttributeEditor : public vtkPVSource
{
public:
  static vtkPVAttributeEditor* New();
  vtkTypeRevisionMacro(vtkPVAttributeEditor, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  virtual void CreateProperties();

  void SetPVInput(const char* name, int idx, vtkPVSource *input);


  // Description:
  // Control the visibility of the pick display as well.
  virtual void SetVisibilityNoTrace(int val);

  void PickMethodObserver();

  // Description:
  // Called when the browse button is pressed.
  void BrowseCallback();

  void SaveCallback();

  vtkGetMacro(EditedFlag,int);
  vtkSetMacro(EditedFlag,int);
  vtkSetMacro(IsScalingFlag,int);
  vtkSetMacro(IsMovingFlag,int);

  void OnChar();

  // Handles the events
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);

protected:
  vtkPVAttributeEditor();
  ~vtkPVAttributeEditor();

  vtkClientServerID WriterID;

  vtkCallbackCommand* EventCallbackCommand; //subclasses use one

  int IsScalingFlag;
  int IsMovingFlag;
  int EditedFlag;
  int ForceEdit;

  // I am putting this here so that the display is added to the render module
  // only once.  There may be a better check using the Proxy.
  int DisplayHasBeenAddedToTheRenderModule;
  
  vtkSMPointLabelDisplay* PointLabelDisplay;
  vtkKWFrame *Frame;
  vtkKWFrame *DataFrame;
  vtkKWLabel *LabelWidget;
  vtkKWPushButton *BrowseButton;
  vtkKWPushButton *SaveButton;
  vtkKWEntry *Entry;

  vtkCollection* LabelCollection;
  virtual void Select();
  void UpdateGUI();
  void ClearDataLabels();
  void InsertDataLabel(const char* label, vtkIdType idx,
                       vtkDataSetAttributes* attr, double* x=0);
  int LabelRow;

  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();  

  vtkPVAttributeEditor(const vtkPVAttributeEditor&); // Not implemented
  void operator=(const vtkPVAttributeEditor&); // Not implemented
};

#endif
