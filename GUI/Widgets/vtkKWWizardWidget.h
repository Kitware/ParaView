/*=========================================================================

  Module:    vtkKWWizardWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWizardWidget - a superclass for creating wizards UI.
// .SECTION Description
// This class is the basis for a wizard widget/dialog. It embeds a
// wizard workflow (i.e. a state machine) and tie it to navigation buttons.
// This widget can be inserted directly inside another user interface;
// most of the time, however, people will use a vtkKWWizardDialog, which
// is just an independent toplevel embedding a vtkKWWizardWidget.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWWizardDialog vtkKWWizardStep vtkKWWizardWorkflow

#ifndef __vtkKWWizardWidget_h
#define __vtkKWWizardWidget_h

#include "vtkKWCompositeWidget.h"

class vtkKWPushButton;
class vtkKWLabel;
class vtkKWLabelWithLabel;
class vtkKWFrame;
class vtkKWSeparator;
class vtkKWWizardWorkflow;

class KWWidgets_EXPORT vtkKWWizardWidget : public vtkKWCompositeWidget
{
public:
  static vtkKWWizardWidget* New();
  vtkTypeRevisionMacro(vtkKWWizardWidget,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the wizard workflow instance.
  vtkGetObjectMacro(WizardWorkflow, vtkKWWizardWorkflow);

  // Description:
  // Get the client area. This is where user content should be placed.
  // A wizard workflow is made of steps (vtkKWWizardStep). Each step
  // should set its vtkKWWizardStep::ShowUserInterfaceCommand callback to point
  // to a method that will display this step's UI. Within that method,
  // all widgets should be children of this ClientArea.
  vtkGetObjectMacro(ClientArea, vtkKWFrame);

  // Description:
  // Set the minimum client area height. No effect if called before Create(). 
  virtual void SetClientAreaMinimumHeight(int);

  // Description:
  // Refresh the interface.
  // This important method will refresh the state of the buttons, depending
  // on the current workflow navigation stack. If the workflow's FinishStep
  // step is defined, it will invoke its CanGoToSelfCommand callback to check
  // if it can be reached directly, and enable the Finish button accordingly.
  // This method should be called each time modifying the UI of the current
  // step may have an impact on navigating the workflow. For example, updating
  // the value of a specific entry may forbid the user to move to the Finish
  // step directly. Check the entry's API for callbacks that can
  // be triggered with a small granularity (vtkKWEntry::Command,
  // vtkKWEntry::SetCommandTriggerToAnyChange, vtkKWScale::Command, etc.).
  virtual void Update();

  // Description:
  // Set the title text (usually a few words), located in the top area.
  // Note that this method is called automatically by Update() to display
  // the name of the WizardWorkflow's CurrentStep() step (see the
  // vtkKWWizardStep::GetName() method).
  virtual void SetTitle(const char *);
  virtual char* GetTitle();

  // Description:
  // Set the subtitle text (usually a short sentence or two), located in the 
  // top area below the title.
  // Note that this method is called automatically by Update() to display
  // the description of the WizardWorkflow's CurrentStep() step (see the
  // vtkKWWizardStep::GetDescription() method).
  virtual void SetSubTitle(const char *);
  virtual char* GetSubTitle();

  // Description:
  // Set/Get the background color of the title area.
  virtual void GetTitleAreaBackgroundColor(double *r, double *g, double *b);
  virtual double* GetTitleAreaBackgroundColor();
  virtual void SetTitleAreaBackgroundColor(double r, double g, double b);
  virtual void SetTitleAreaBackgroundColor(double rgb[3])
    { this->SetTitleAreaBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Get the wizard icon, located in the top area right of the title.
  // This can be used to provide a better graphical identity to the wizard.
  vtkGetObjectMacro(TitleIconLabel, vtkKWLabel);

  // Description:
  // Set the pre-text, i.e. the contents of a convenience text section placed
  // just above the client area.
  virtual void SetPreText(const char *);
  virtual char* GetPreText();

  // Description:
  // Set the post-text, i.e. the contents of a convenience text section placed
  // just below the client area.
  virtual void SetPostText(const char *);
  virtual char* GetPostText();

  // Description:
  // Set the error text, i.e. the contents of a convenience text section
  // placed just below the client area. It is prefixed with an error icon.
  // This is typically used by a step's vtkKWWizardStep::ValidationCommand 
  // callback to report an error when validating the UI failed.
  virtual void SetErrorText(const char *);
  virtual char* GetErrorText();

  // Description:
  // Unpack all children in the client-area and set all pre-/post-/title label
  // to empty strings.
  // This is typically used by a step's 
  // vtkKWWizardStep::HideUserInterfaceCommand callback to hide the step's UI
  // or release resources that were allocated specifically for a step's UI.
  virtual void ClearPage();
  
  // Description:
  // Set/Get the visibility of the buttons.
  virtual void SetBackButtonVisibility(int);
  vtkGetMacro(BackButtonVisibility,int);
  vtkBooleanMacro(BackButtonVisibility,int);
  virtual void SetNextButtonVisibility(int);
  vtkGetMacro(NextButtonVisibility,int);
  vtkBooleanMacro(NextButtonVisibility,int);
  virtual void SetFinishButtonVisibility(int);
  vtkGetMacro(FinishButtonVisibility,int);
  vtkBooleanMacro(FinishButtonVisibility,int);
  virtual void SetCancelButtonVisibility(int);
  vtkGetMacro(CancelButtonVisibility,int);
  vtkBooleanMacro(CancelButtonVisibility,int);
  virtual void SetOKButtonVisibility(int);
  vtkGetMacro(OKButtonVisibility,int);
  vtkBooleanMacro(OKButtonVisibility,int);

  // Description:
  // Get and customize some UI elements.
  vtkGetObjectMacro(CancelButton, vtkKWPushButton);
  vtkGetObjectMacro(OKButton, vtkKWPushButton);
  vtkGetObjectMacro(SeparatorBeforeButtons, vtkKWSeparator);
  vtkGetObjectMacro(SubTitleLabel, vtkKWLabel);
  vtkGetObjectMacro(TitleLabel, vtkKWLabel);

  // Description:
  // Add all the default observers needed by that object, or remove
  // all the observers that were added through AddCallbackCommandObserver.
  // Subclasses can override these methods to add/remove their own default
  // observers, but should call the superclass too.
  virtual void AddCallbackCommandObservers();
  virtual void RemoveCallbackCommandObservers();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWWizardWidget();
  ~vtkKWWizardWidget();

  // Description:
  // Create the widget
  virtual void CreateWidget();

  // Description:
  // Pack the buttons.
  virtual void PackButtons();

  int BackButtonVisibility;
  int NextButtonVisibility;
  int FinishButtonVisibility;
  int CancelButtonVisibility;
  int OKButtonVisibility;

  vtkKWWizardWorkflow *WizardWorkflow;

  vtkKWFrame          *TitleFrame;
  vtkKWLabel          *TitleLabel;
  vtkKWLabel          *SubTitleLabel;
  vtkKWLabel          *TitleIconLabel;
  
  vtkKWSeparator      *SeparatorAfterTitleArea;

  vtkKWFrame          *LayoutFrame;
  vtkKWLabel          *PreTextLabel;
  vtkKWFrame          *ClientArea;
  vtkKWLabel          *PostTextLabel;
  vtkKWLabelWithLabel *ErrorTextLabel;

  vtkKWSeparator      *SeparatorBeforeButtons;

  vtkKWFrame          *ButtonFrame;
  vtkKWPushButton     *BackButton;
  vtkKWPushButton     *NextButton;
  vtkKWPushButton     *FinishButton;
  vtkKWPushButton     *CancelButton;
  vtkKWPushButton     *OKButton;

  // Description:
  // Processes the events that are passed through CallbackCommand (or others).
  // Subclasses can oberride this method to process their own events, but
  // should call the superclass too.
  virtual void ProcessCallbackCommandEvents(
    vtkObject *caller, unsigned long event, void *calldata);
  
private:
  vtkKWWizardWidget(const vtkKWWizardWidget&); // Not implemented
  void operator=(const vtkKWWizardWidget&); // Not Implemented
};

#endif
