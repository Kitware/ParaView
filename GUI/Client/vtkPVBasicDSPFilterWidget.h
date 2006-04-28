/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVBasicDSPFilterWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBasicDSPFilterWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBasicDSPFilterWidget
// .SECTION Description
// 


#ifndef __vtkPVBasicDSPFilterWidget_h
#define __vtkPVBasicDSPFilterWidget_h

#include "vtkPVWidget.h"

class vtkKWLabel;
class vtkKWFrameWithLabel;
class vtkKWMenuButton;
class vtkKWMenuButton;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWEntry;
class vtkKWFrame;

enum FILTER_WIDGET_FILTER_TYPE 
  {
    FILTER_WIDGET_SMOOTHING,
    FILTER_WIDGET_LOW_PASS,
    FILTER_WIDGET_HIGH_PASS,
    FILTER_WIDGET_USER_DEFINED,
    FILTER_WIDGET_INTEGRAL,
    FILTER_WIDGET_DERIVATIVE
  };

class VTK_EXPORT vtkPVBasicDSPFilterWidget : public vtkPVWidget
{
public:
  static vtkPVBasicDSPFilterWidget* New();
  vtkTypeRevisionMacro(vtkPVBasicDSPFilterWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  void ChangeDSPFilterMode(const char* newMode);
  void ChangeCutoffFreq(const char* newMode);
  void ChangeInputVar(const char* newMode);

  void CreateCutoffFreqToggle(vtkKWWidget *topframe);
  void DisableCutoffFreqToggle();
  void EnableCutoffFreqToggle();

  void CreateInputVarToggle(vtkKWWidget *topframe);
  void CreateLengthTextEntry(vtkKWWidget *topframe);
  void CreateNumeratorWeightsTextEntry(vtkKWWidget *topframe);
  void CreateDenominatorWeightsTextEntry(vtkKWWidget *topframe);
  void CreateForwardNumeratorWeightsTextEntry(vtkKWWidget *topframe);
  void CreateOutputVarTextEntry(vtkKWWidget *topframe);
  void CreateAddThisVarButton(vtkKWWidget *topframe);

  bool UpdateTogglesWithFileInformation();

  void getNumeratorWeightsString(char *a_string, int a_maxLength, bool a_isLowPass, const char *a_cutoff);
  void getDenominatorWeightsString(char *a_string, int a_maxLength, bool a_isLowPass, const char *a_cutoff);

  char *getSmoothingNumeratorWeightsString( int a_filterLength );
  char *getSmoothingForwardNumeratorWeightsString( int a_filterLength );
  double *getSmoothingNumeratorWeights( int a_filterLength );

  int GetFilterLength();
  void SetFilterLength(int len);

  void ResizeOutputVariableList();

  void SetOutputVariableName(int a_which);

  void AddVarFunction();
  void DeleteVarFunction(int which);


  void SetWidgetsToSmoothingFilterMode();
  void SetWidgetsToBasicFilterMode();
  void SetWidgetsToUserDefinedMode();
  void SetWidgetsToIntegralMode();
  void SetWidgetsToCorrelationMode();

  virtual void Accept()
    {
    vtkPVWidget::Accept();
    }

  //BTX
  // Description:
  // Called when the Accept button is pressed.  It moves the widget values to the 
  // VTK calculator filter.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // This method resets the widget values from the VTK filter.
  virtual void ResetInternal();

  // Description:
  // Initialize the widget after creation
  virtual void Initialize();

  // Description:
  // Save this source to a file.  We need more than just the source tcl name.
  virtual void SaveInBatchScript(ofstream *file);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);


  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();





  void AddThisFilterToSource(const char *a_inputName, const char *a_outputName,
    double a_cutoff, FILTER_WIDGET_FILTER_TYPE a_filterType); 

  void RemoveThisFilterFromSource(const char *a_outputName);




protected:

  // Description:
  // Set up the UI for this source
  void CreateWidget();

  //I would have liked to use vectors, but there was an odd compiler error
  //It turns out that using SAF_EXECUTABLE_SRCS instead of SAF_SRCS in
  //the CMakeLists file would have fixed it? XXX see if I can change it? XXX
  int m_numOutputVariables;
  int m_maxNumOutputVariables;
  char **m_outputVariableNames;
  char **m_inputVariableNames;
  FILTER_WIDGET_FILTER_TYPE *m_filterType;
  double *m_outputVariableCutoffs;




  vtkPVBasicDSPFilterWidget();
  ~vtkPVBasicDSPFilterWidget();



  //the frame for all of the widgets defined here
  vtkKWFrameWithLabel* DSPFilterFrame;

  //widget to select the type of filter
  vtkKWFrame* DSPFilterModeSubFrame;
  vtkKWLabel* DSPFilterModeLabel;
  vtkKWMenuButton* DSPFilterModeMenu; 

  //widget to select the cutoff freq for basic filters
  vtkKWFrame* CutoffFreqSubFrame;
  vtkKWLabel* CutoffFreqLabel;
  vtkKWMenuButton* CutoffFreqMenu; 

  //widget to select the input variable
  vtkKWFrame* InputVarSubFrame;
  vtkKWLabel* InputVarLabel;
  vtkKWMenuButton* InputVarMenu; 

  //text entry box to specify numerator weights for user defined filter
  vtkKWFrame *LengthSubFrame;
  vtkKWEntry *LengthEntry;
  vtkKWLabel *LengthLabel;

  //text entry box to specify numerator weights for user defined filter
  vtkKWFrame *NumeratorWeightsSubFrame;
  vtkKWEntry *NumeratorWeightsEntry;
  vtkKWLabel *NumeratorWeightsLabel;

  //text entry box to specify denominator weights for user defined filter
  vtkKWFrame *DenominatorWeightsSubFrame;
  vtkKWEntry *DenominatorWeightsEntry;
  vtkKWLabel *DenominatorWeightsLabel;

  //text entry box to specify forward numerator weights for user defined filter
  vtkKWFrame *ForwardNumeratorWeightsSubFrame;
  vtkKWEntry *ForwardNumeratorWeightsEntry;
  vtkKWLabel *ForwardNumeratorWeightsLabel;

  //text entry box to specify the output variable name
  vtkKWFrame *OutputVarSubFrame;
  vtkKWEntry *OutputVarEntry;
  vtkKWLabel *OutputVarLabel;

  //button to add this var to the list to be calculated
  vtkKWFrame *AddThisVarSubFrame;
  vtkKWPushButton *AddThisVarButton;

  //array showing vars to be calculated with a button to delete each
  vtkKWPushButton **DeleteThisVarButton;
  vtkKWLabel **DeleteThisVarLabel;












private:
  vtkPVBasicDSPFilterWidget(const vtkPVBasicDSPFilterWidget&); // Not implemented
  void operator=(const vtkPVBasicDSPFilterWidget&); // Not implemented



  bool m_gotFileInformation;
};

#endif
