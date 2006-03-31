/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
/*=========================================================================

Program:   ParaView
Module:    vtkPVBasicDSPFilterWidget.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBasicDSPFilterWidget.h"
#include <vtkDSPFilterDefinition.h>

#include "vtkArrayCalculator.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenuButton.h"
#include "vtkKWPushButton.h"
#include "vtkKWEntry.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkSource.h"
#include "vtkStringList.h"

#include <stdlib.h>
#include <vtkstd/map>
#include <vtkstd/algorithm>
#include <vtkstd/vector>
#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVBasicDSPFilterWidget);
vtkCxxRevisionMacro(vtkPVBasicDSPFilterWidget, "1.7");

//20 weights, 5 cutoff freqs(.3, .4, .5, .6, .7)
const double g_butter_lp_numerator_coeffs[5][20]={
    { 
    6.728999e-09,1.278510e-07,1.150659e-06,6.520400e-06,2.608160e-05,
    7.824480e-05,1.825712e-04,3.390608e-04,5.085912e-04,6.216115e-04,
    6.216115e-04,5.085912e-04,3.390608e-04,1.825712e-04,7.824480e-05,
    2.608160e-05,6.520400e-06,1.150659e-06,1.278510e-07,6.728999e-09 
    },
    { 
    5.426836e-07,1.031099e-05,9.279890e-05,5.258604e-04,2.103442e-03,
    6.310325e-03,1.472409e-02,2.734474e-02,4.101711e-02,5.013203e-02,
    5.013203e-02,4.101711e-02,2.734474e-02,1.472409e-02,6.310325e-03,
    2.103442e-03,5.258604e-04,9.279890e-05,1.031099e-05,5.426836e-07 
    },
    {
    1.532235e-05,2.911246e-04,2.620121e-03,1.484735e-02,5.938941e-02,
    1.781682e-01,4.157259e-01,7.720623e-01,1.158093e+00,1.415448e+00,
    1.415448e+00,1.158093e+00,7.720623e-01,4.157259e-01,1.781682e-01,
    5.938941e-02,1.484735e-02,2.620121e-03,2.911246e-04,1.532235e-05 
    },
    { 
    2.347399e-04,4.460058e-03,4.014053e-02,2.274630e-01,9.098519e-01,
    2.729556e+00,6.368963e+00,1.182807e+01,1.774211e+01,2.168480e+01,
    2.168480e+01,1.774211e+01,1.182807e+01,6.368963e+00,2.729556e+00,
    9.098519e-01,2.274630e-01,4.014053e-02,4.460058e-03,2.347399e-04 
    },
    { 
    2.464897e-03,4.683304e-02,4.214974e-01,2.388485e+00,9.553940e+00,
    2.866182e+01,6.687758e+01,1.242012e+02,1.863018e+02,2.277022e+02,
    2.277022e+02,1.863018e+02,1.242012e+02,6.687758e+01,2.866182e+01,
    9.553940e+00,2.388485e+00,4.214974e-01,4.683304e-02,2.464897e-03 
    }
};

const double g_butter_lp_denominator_coeffs[5][20]={
    { 
    1.000000e+00,-7.593463e+00,2.911257e+01,-7.382643e+01,1.376640e+02,
    -1.993865e+02,2.316075e+02,-2.200609e+02,1.730898e+02,-1.134452e+02,
    6.209662e+01,-2.834171e+01,1.072687e+01,-3.333303e+00,8.371334e-01,
    -1.658866e-01,2.498993e-02,-2.691521e-03,1.847415e-04,-6.075716e-06 
    },
    { 
    1.000000e+00,-3.795965e+00,9.197865e+00,-1.558958e+01,2.067092e+01,
    -2.202384e+01,1.943812e+01,-1.435913e+01,8.968465e+00,-4.746696e+00,
    2.130598e+00,-8.078102e-01,2.570137e-01,-6.783423e-02,1.460810e-02,
    -2.503336e-03,3.286926e-04,-3.107581e-05,1.884751e-06,-5.510253e-08 
    },
    { 
    1.000000e+00,-4.135345e-15,2.582051e+00,-9.661999e-15,2.629015e+00,
    -8.608043e-15,1.364366e+00,-3.400246e-15,3.901539e-01,-4.442945e-16,
    6.217307e-02,-3.031696e-17,5.333174e-03,-4.875880e-17,2.255304e-04,
    -1.293553e-17,3.915040e-06,-6.517327e-19,1.784284e-08,-9.904768e-25 
    },
    { 
    1.000000e+00,3.795965e+00,9.197865e+00,1.558958e+01,2.067092e+01,
    2.202384e+01,1.943812e+01,1.435913e+01,8.968465e+00,4.746696e+00,
    2.130598e+00,8.078102e-01,2.570137e-01,6.783423e-02,1.460810e-02,
    2.503336e-03,3.286926e-04,3.107581e-05,1.884751e-06,5.510253e-08 
    },
    { 
    1.000000e+00,7.593463e+00,2.911257e+01,7.382643e+01,1.376640e+02,
    1.993865e+02,2.316075e+02,2.200609e+02,1.730898e+02,1.134452e+02,
    6.209662e+01,2.834171e+01,1.072687e+01,3.333303e+00,8.371334e-01,
    1.658866e-01,2.498993e-02,2.691521e-03,1.847415e-04,6.075716e-06 
    }
};

const double g_butter_hp_numerator_coeffs[5][20]={
    { 
    2.464897e-03,-4.683304e-02,4.214974e-01,-2.388485e+00,9.553940e+00,
    -2.866182e+01,6.687758e+01,-1.242012e+02,1.863018e+02,-2.277022e+02,
    2.277022e+02,-1.863018e+02,1.242012e+02,-6.687758e+01,2.866182e+01,
    -9.553940e+00,2.388485e+00,-4.214974e-01,4.683304e-02,-2.464897e-03 
    },
    { 
    2.347399e-04,-4.460058e-03,4.014053e-02,-2.274630e-01,9.098519e-01,
    -2.729556e+00,6.368963e+00,-1.182807e+01,1.774211e+01,-2.168480e+01,
    2.168480e+01,-1.774211e+01,1.182807e+01,-6.368963e+00,2.729556e+00,
    -9.098519e-01,2.274630e-01,-4.014053e-02,4.460058e-03,-2.347399e-04 
    },
    { 
    1.532235e-05,-2.911246e-04,2.620121e-03,-1.484735e-02,5.938941e-02,
    -1.781682e-01,4.157259e-01,-7.720623e-01,1.158093e+00,-1.415448e+00,
    1.415448e+00,-1.158093e+00,7.720623e-01,-4.157259e-01,1.781682e-01,
    -5.938941e-02,1.484735e-02,-2.620121e-03,2.911246e-04,-1.532235e-05 
    },
    { 
    5.426836e-07,-1.031099e-05,9.279890e-05,-5.258604e-04,2.103442e-03,
    -6.310325e-03,1.472409e-02,-2.734474e-02,4.101711e-02,-5.013203e-02,
    5.013203e-02,-4.101711e-02,2.734474e-02,-1.472409e-02,6.310325e-03,
    -2.103442e-03,5.258604e-04,-9.279890e-05,1.031099e-05,-5.426836e-07 
    },
    { 
    6.728999e-09,-1.278510e-07,1.150659e-06,-6.520400e-06,2.608160e-05,
    -7.824480e-05,1.825712e-04,-3.390608e-04,5.085912e-04,-6.216115e-04,
    6.216115e-04,-5.085912e-04,3.390608e-04,-1.825712e-04,7.824480e-05,
    -2.608160e-05,6.520400e-06,-1.150659e-06,1.278510e-07,-6.728999e-09
    }
};

const double g_butter_hp_denominator_coeffs[5][20]={
    { 
    1.000000e+00,-7.593463e+00,2.911257e+01,-7.382643e+01,1.376640e+02,
    -1.993865e+02,2.316075e+02,-2.200609e+02,1.730898e+02,-1.134452e+02,
    6.209662e+01,-2.834171e+01,1.072687e+01,-3.333303e+00,8.371334e-01,
    -1.658866e-01,2.498993e-02,-2.691521e-03,1.847415e-04,-6.075716e-06
    },
    { 
    1.000000e+00,-3.795965e+00,9.197865e+00,-1.558958e+01,2.067092e+01,
    -2.202384e+01,1.943812e+01,-1.435913e+01,8.968465e+00,-4.746696e+00,
    2.130598e+00,-8.078102e-01,2.570137e-01,-6.783423e-02,1.460810e-02,
    -2.503336e-03,3.286926e-04,-3.107581e-05,1.884751e-06,-5.510253e-08
    },
    { 
    1.000000e+00,-3.258732e-15,2.582051e+00,-7.317450e-15,2.629015e+00,
    -6.436023e-15,1.364366e+00,-2.631408e-15,3.901539e-01,-5.939375e-16,
    6.217307e-02,-1.880158e-16,5.333174e-03,-5.599617e-17,2.255304e-04,
    -1.167858e-18,3.915040e-06,5.483311e-19,1.784284e-08,-9.904768e-25
    },
    { 
    1.000000e+00,3.795965e+00,9.197865e+00,1.558958e+01,2.067092e+01,
    2.202384e+01,1.943812e+01,1.435913e+01,8.968465e+00,4.746696e+00,
    2.130598e+00,8.078102e-01,2.570137e-01,6.783423e-02,1.460810e-02,
    2.503336e-03,3.286926e-04,3.107581e-05,1.884751e-06,5.510253e-08
    },
    { 
    1.000000e+00,7.593463e+00,2.911257e+01,7.382643e+01,1.376640e+02,
    1.993865e+02,2.316075e+02,2.200609e+02,1.730898e+02,1.134452e+02,
    6.209662e+01,2.834171e+01,1.072687e+01,3.333303e+00,8.371334e-01,
    1.658866e-01,2.498993e-02,2.691521e-03,1.847415e-04,6.075716e-06
    }
};

//----------------------------------------------------------------------------
#ifdef HAS_snprintf
void vtkPVBasicDSPFilterWidget::getNumeratorWeightsString(char *a_string, int a_maxLength, bool a_isLowPass, 
  const char *a_cutoff)
#else
void vtkPVBasicDSPFilterWidget::getNumeratorWeightsString(char *a_string, int vtkNotUsed(a_maxLength), bool a_isLowPass, 
  const char *a_cutoff)
#endif
{
  const double *l_ptr=NULL;
  if( a_isLowPass )
    {
    if(!strcmp(".3",a_cutoff)) l_ptr = g_butter_lp_numerator_coeffs[0];
    else if(!strcmp(".4",a_cutoff)) l_ptr = g_butter_lp_numerator_coeffs[1];
    else if(!strcmp(".5",a_cutoff)) l_ptr = g_butter_lp_numerator_coeffs[2];
    else if(!strcmp(".6",a_cutoff)) l_ptr = g_butter_lp_numerator_coeffs[3];
    else if(!strcmp(".7",a_cutoff)) l_ptr = g_butter_lp_numerator_coeffs[4];
    }
  else
    {
    if(!strcmp(".3",a_cutoff)) l_ptr = g_butter_hp_numerator_coeffs[0];
    else if(!strcmp(".4",a_cutoff)) l_ptr = g_butter_hp_numerator_coeffs[1];
    else if(!strcmp(".5",a_cutoff)) l_ptr = g_butter_hp_numerator_coeffs[2];
    else if(!strcmp(".6",a_cutoff)) l_ptr = g_butter_hp_numerator_coeffs[3];
    else if(!strcmp(".7",a_cutoff)) l_ptr = g_butter_hp_numerator_coeffs[4];
    }
  if(l_ptr)
    {
#ifdef HAS_snprintf
    snprintf(a_string,a_maxLength-1,"%e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e",
      l_ptr[0],l_ptr[1],l_ptr[2],l_ptr[3],
      l_ptr[4],l_ptr[5],l_ptr[6],l_ptr[7],
      l_ptr[8],l_ptr[9],l_ptr[10],l_ptr[11],
      l_ptr[12],l_ptr[13],l_ptr[14],l_ptr[15],
      l_ptr[16],l_ptr[17],l_ptr[18],l_ptr[19] );
#else
    sprintf(a_string,"%e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e",
      l_ptr[0],l_ptr[1],l_ptr[2],l_ptr[3],
      l_ptr[4],l_ptr[5],l_ptr[6],l_ptr[7],
      l_ptr[8],l_ptr[9],l_ptr[10],l_ptr[11],
      l_ptr[12],l_ptr[13],l_ptr[14],l_ptr[15],
      l_ptr[16],l_ptr[17],l_ptr[18],l_ptr[19] );
#endif

    }
  else
    {
    sprintf(a_string,"error");
    }
}

//----------------------------------------------------------------------------
#ifdef HAS_snprintf
void vtkPVBasicDSPFilterWidget::getDenominatorWeightsString(char *a_string, int a_maxLength, bool a_isLowPass, 
  const char *a_cutoff)
#else
void vtkPVBasicDSPFilterWidget::getDenominatorWeightsString(char *a_string, int vtkNotUsed(a_maxLength), bool a_isLowPass, 
  const char *a_cutoff)
#endif
{
  const double *l_ptr=NULL;
  if( a_isLowPass )
    {
    if(!strcmp(".3",a_cutoff)) l_ptr = g_butter_lp_denominator_coeffs[0];
    else if(!strcmp(".4",a_cutoff)) l_ptr = g_butter_lp_denominator_coeffs[1];
    else if(!strcmp(".5",a_cutoff)) l_ptr = g_butter_lp_denominator_coeffs[2];
    else if(!strcmp(".6",a_cutoff)) l_ptr = g_butter_lp_denominator_coeffs[3];
    else if(!strcmp(".7",a_cutoff)) l_ptr = g_butter_lp_denominator_coeffs[4];
    }
  else
    {
    if(!strcmp(".3",a_cutoff)) l_ptr = g_butter_hp_denominator_coeffs[0];
    else if(!strcmp(".4",a_cutoff)) l_ptr = g_butter_hp_denominator_coeffs[1];
    else if(!strcmp(".5",a_cutoff)) l_ptr = g_butter_hp_denominator_coeffs[2];
    else if(!strcmp(".6",a_cutoff)) l_ptr = g_butter_hp_denominator_coeffs[3];
    else if(!strcmp(".7",a_cutoff)) l_ptr = g_butter_hp_denominator_coeffs[4];
    }
  if(l_ptr)
    {
#ifdef HAS_snprintf
    snprintf(a_string,a_maxLength-1,"%e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e",
      l_ptr[0],l_ptr[1],l_ptr[2],l_ptr[3],
      l_ptr[4],l_ptr[5],l_ptr[6],l_ptr[7],
      l_ptr[8],l_ptr[9],l_ptr[10],l_ptr[11],
      l_ptr[12],l_ptr[13],l_ptr[14],l_ptr[15],
      l_ptr[16],l_ptr[17],l_ptr[18],l_ptr[19] );
#else
    sprintf(a_string,"%e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e %e",
      l_ptr[0],l_ptr[1],l_ptr[2],l_ptr[3],
      l_ptr[4],l_ptr[5],l_ptr[6],l_ptr[7],
      l_ptr[8],l_ptr[9],l_ptr[10],l_ptr[11],
      l_ptr[12],l_ptr[13],l_ptr[14],l_ptr[15],
      l_ptr[16],l_ptr[17],l_ptr[18],l_ptr[19] );
#endif
    }
  else
    {
    sprintf(a_string,"error");
    }
}


//----------------------------------------------------------------------------
double *vtkPVBasicDSPFilterWidget::getSmoothingNumeratorWeights( int a_filterLength )
{
  int i;
  double *l_weights=new double[a_filterLength];
  double l_sum=0.0;

  for(i=0;i<a_filterLength;i++)
    {

#if 0
    double x = 2.0 * fabs( (double)i / (double)a_filterLength );//looks more correct
#else
    double x = 2.0 * fabs( (double)i / (double)(a_filterLength+1) );//the way i did it for ecl
#endif

    if( x < 1.0 )
      {
      l_weights[i] =  .5*x*x*x - x*x + 2.0/3.0;
      }
    else if( x < 2.0 )
      {
      l_weights[i] = -(1.0/6.0)*x*x*x + x*x -2.0*x + 4.0/3.0;
      }

    if( !i )
      {
      l_sum += l_weights[i];
      }
    else
      { //add it twice, because it will be used in + and - directions
      l_sum += 2*l_weights[i];
      }
    }
  for(i=0;i<a_filterLength;i++)
    {//normalize the coeffs, so that the filter doesnt add DC content
    l_weights[i] /= l_sum;

    // printf("   smoothing_weights[%d]=%f\n",i,l_weights[i]);
    }

  return(l_weights);
}
//----------------------------------------------------------------------------
char *vtkPVBasicDSPFilterWidget::getSmoothingNumeratorWeightsString( int a_filterLength )
{
  double *l_weights = this->getSmoothingNumeratorWeights(a_filterLength);
  const int l_maxCharsPerEntry=32;
  char *l_str = new char[l_maxCharsPerEntry*a_filterLength];
  l_str[0] = '\0';
  char *l_ptr;
  for(int i=0;i<a_filterLength;i++)
    {
    l_ptr = l_str + strlen(l_str);
    sprintf(l_ptr,"%f ",l_weights[i]);
    }

  delete[] l_weights;
  return(l_str);
}
//----------------------------------------------------------------------------
char *vtkPVBasicDSPFilterWidget::getSmoothingForwardNumeratorWeightsString( int a_filterLength )
{ //SAME AS NUM WEIGHTS BUT WITHOUT 1ST ENTRY

  double *l_weights = this->getSmoothingNumeratorWeights(a_filterLength);
  const int l_maxCharsPerEntry=32;
  char *l_str = new char[l_maxCharsPerEntry*a_filterLength];
  l_str[0] = '\0';
  char *l_ptr;
  for(int i=1;i<a_filterLength;i++)
    {
    l_ptr = l_str + strlen(l_str);
    sprintf(l_ptr,"%f ",l_weights[i]);
    }

  delete[] l_weights;
  return(l_str);
}


//----------------------------------------------------------------------------
vtkPVBasicDSPFilterWidget::vtkPVBasicDSPFilterWidget()
{
  //the frame for all of the widgets defined here
  this->DSPFilterFrame = vtkKWFrameWithLabel::New();

  //widget to select the type of filter
  this->DSPFilterModeSubFrame = vtkKWFrame::New();
  this->DSPFilterModeLabel = vtkKWLabel::New();
  this->DSPFilterModeMenu = vtkKWMenuButton::New(); 

  //widget to select the cutoff freq for basic filters
  this->CutoffFreqSubFrame = vtkKWFrame::New();
  this->CutoffFreqLabel = vtkKWLabel::New();
  this->CutoffFreqMenu = vtkKWMenuButton::New(); 

  //widget to select the input variable
  this->InputVarSubFrame = vtkKWFrame::New();
  this->InputVarLabel = vtkKWLabel::New();
  this->InputVarMenu = vtkKWMenuButton::New(); 

  //text entry box to specify length of smoothing filter
  this->LengthSubFrame = vtkKWFrame::New();
  this->LengthEntry = vtkKWEntry::New();
  this->LengthLabel = vtkKWLabel::New();

  //text entry box to specify numerator weights for user defined filter
  this->NumeratorWeightsSubFrame = vtkKWFrame::New();
  this->NumeratorWeightsEntry = vtkKWEntry::New();
  this->NumeratorWeightsLabel = vtkKWLabel::New();

  //text entry box to specify denominator weights for user defined filter
  this->DenominatorWeightsSubFrame = vtkKWFrame::New();
  this->DenominatorWeightsEntry = vtkKWEntry::New();
  this->DenominatorWeightsLabel = vtkKWLabel::New();

  //text entry box to specify forward numerator weights for user defined filter
  this->ForwardNumeratorWeightsSubFrame = vtkKWFrame::New();
  this->ForwardNumeratorWeightsEntry = vtkKWEntry::New();
  this->ForwardNumeratorWeightsLabel = vtkKWLabel::New();

  //text entry box to specify the output variable name
  this->OutputVarSubFrame = vtkKWFrame::New();
  this->OutputVarEntry = vtkKWEntry::New();
  this->OutputVarLabel = vtkKWLabel::New();

  //button to add this var to the list to be calculated
  this->AddThisVarSubFrame = vtkKWFrame::New();
  this->AddThisVarButton = vtkKWPushButton::New();










  this->m_gotFileInformation=false;

  this->m_numOutputVariables=0;
  this->m_maxNumOutputVariables=10;
  this->m_outputVariableNames=new char*[m_maxNumOutputVariables];
  this->m_inputVariableNames=new char*[m_maxNumOutputVariables];
  this->m_filterType=new FILTER_WIDGET_FILTER_TYPE[m_maxNumOutputVariables];
  this->m_outputVariableCutoffs=new double[m_maxNumOutputVariables];

  this->DeleteThisVarButton = new vtkKWPushButton*[m_maxNumOutputVariables];
  this->DeleteThisVarLabel = new vtkKWLabel*[m_maxNumOutputVariables];

  for(int i=0;i<m_maxNumOutputVariables;i++)
    {
    this->DeleteThisVarButton[i]=vtkKWPushButton::New();
    this->DeleteThisVarLabel[i]=vtkKWLabel::New();
    this->m_outputVariableNames[i]=NULL;
    this->m_inputVariableNames[i]=NULL;
    }




}

//----------------------------------------------------------------------------
vtkPVBasicDSPFilterWidget::~vtkPVBasicDSPFilterWidget()
{


  //the frame for all of the widgets defined here
  this->DSPFilterFrame->Delete();

  //widget to select the type of filter
  this->DSPFilterModeSubFrame->Delete();
  this->DSPFilterModeLabel->Delete();
  this->DSPFilterModeMenu->Delete();

  //widget to select the cutoff freq for basic filters
  this->CutoffFreqSubFrame->Delete();
  this->CutoffFreqLabel->Delete();
  this->CutoffFreqMenu->Delete();

  //widget to select the input variable
  this->InputVarSubFrame->Delete();
  this->InputVarLabel->Delete();
  this->InputVarMenu->Delete();

  //text entry box to specify numerator weights for user defined filter
  this->LengthSubFrame->Delete();
  this->LengthEntry->Delete();
  this->LengthLabel->Delete();

  //text entry box to specify numerator weights for user defined filter
  this->NumeratorWeightsSubFrame->Delete();
  this->NumeratorWeightsEntry->Delete();
  this->NumeratorWeightsLabel->Delete();

  //text entry box to specify denominator weights for user defined filter
  this->DenominatorWeightsSubFrame->Delete();
  this->DenominatorWeightsEntry->Delete();
  this->DenominatorWeightsLabel->Delete();

  //text entry box to specify numerator weights for user defined filter
  this->ForwardNumeratorWeightsSubFrame->Delete();
  this->ForwardNumeratorWeightsEntry->Delete();
  this->ForwardNumeratorWeightsLabel->Delete();

  //text entry box to specify the output variable name
  this->OutputVarSubFrame->Delete();
  this->OutputVarEntry->Delete();
  this->OutputVarLabel->Delete();

  //button to add this var to the list to be calculated
  this->AddThisVarSubFrame->Delete();
  this->AddThisVarButton->Delete();



  for(int i=0;i<m_maxNumOutputVariables;i++)
    {
    this->DeleteThisVarButton[i]->Delete();
    this->DeleteThisVarLabel[i]->Delete();
    }

  delete[] this->m_outputVariableNames;
  delete[] this->m_inputVariableNames;
  delete[] this->m_filterType;
  delete[] this->m_outputVariableCutoffs;
  delete[] this->DeleteThisVarButton;
  delete[] this->DeleteThisVarLabel;







}




//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::CreateCutoffFreqToggle(vtkKWWidget *topframe)
{

  this->CutoffFreqSubFrame->SetParent(topframe);
  this->CutoffFreqSubFrame->Create();
  this->Script("pack %s -side top -fill x",
    this->CutoffFreqSubFrame->GetWidgetName());


  this->CutoffFreqLabel->SetParent(this->CutoffFreqSubFrame);
  this->CutoffFreqLabel->Create();
  this->CutoffFreqLabel->SetJustificationToRight();
  this->CutoffFreqLabel->SetWidth(18);
  this->CutoffFreqLabel->SetText("Cutoff Frequency");
  this->CutoffFreqLabel->SetBalloonHelpString("Select the normalized cutoff frequency");
  this->CutoffFreqMenu->SetParent(this->CutoffFreqSubFrame);
  this->CutoffFreqMenu->Create();
  this->CutoffFreqMenu->GetMenu()->AddRadioButton(
    ".3", this, "ChangeCutoffFreq 3");
  this->CutoffFreqMenu->GetMenu()->AddRadioButton(
    ".4", this, "ChangeCutoffFreq 4");
  this->CutoffFreqMenu->GetMenu()->AddRadioButton(
    ".5", this, "ChangeCutoffFreq 5");
  this->CutoffFreqMenu->GetMenu()->AddRadioButton(
    ".6", this, "ChangeCutoffFreq 6");
  this->CutoffFreqMenu->GetMenu()->AddRadioButton(
    ".7", this, "ChangeCutoffFreq 7");
  this->CutoffFreqMenu->SetValue(".5");
  this->CutoffFreqMenu->SetBalloonHelpString("Select the normalized cutoff frequency");
  this->Script("pack %s %s -side left -pady 1m",
    this->CutoffFreqLabel->GetWidgetName(),
    this->CutoffFreqMenu->GetWidgetName());
}



//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::CreateInputVarToggle(vtkKWWidget *topframe)
{
  this->InputVarSubFrame->SetParent(topframe);
  this->InputVarSubFrame->Create();
  this->Script("pack %s -side top -fill x",
    this->InputVarSubFrame->GetWidgetName());

  this->InputVarLabel->SetParent(this->InputVarSubFrame);
  this->InputVarLabel->Create();
  this->InputVarLabel->SetJustificationToRight();
  this->InputVarLabel->SetWidth(18);
  this->InputVarLabel->SetText("Input Variable");
  this->InputVarLabel->SetBalloonHelpString("Select the input variable");
  this->InputVarMenu->SetParent(this->InputVarSubFrame);
  this->InputVarMenu->Create();

  this->InputVarMenu->SetBalloonHelpString("Select the input variable");
  this->Script("pack %s %s -side left -pady 1m",
    this->InputVarLabel->GetWidgetName(),
    this->InputVarMenu->GetWidgetName());
}



//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::CreateOutputVarTextEntry(vtkKWWidget *topframe)
{
  this->OutputVarSubFrame->SetParent(topframe);
  this->OutputVarSubFrame->Create();
  this->Script("pack %s -side top -fill x",
    this->OutputVarSubFrame->GetWidgetName());

  this->OutputVarLabel->SetParent(this->OutputVarSubFrame);
  this->OutputVarLabel->Create();
  this->OutputVarLabel->SetJustificationToRight();
  this->OutputVarLabel->SetWidth(18);
  this->OutputVarLabel->SetText("Output Variable");
  this->OutputVarLabel->SetBalloonHelpString("Enter the output variable's name");
  this->OutputVarEntry->SetParent(this->OutputVarSubFrame);
  this->OutputVarEntry->Create();
  this->OutputVarEntry->SetValue("name");

  this->OutputVarEntry->SetBalloonHelpString("Enter the output variable's name");
  this->Script("pack %s %s -side left -pady 1m",
    this->OutputVarLabel->GetWidgetName(),
    this->OutputVarEntry->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::CreateLengthTextEntry(vtkKWWidget *topframe)
{
  this->LengthSubFrame->SetParent(topframe);
  this->LengthSubFrame->Create();
  this->Script("pack %s -side top -fill x",
    this->LengthSubFrame->GetWidgetName());

  this->LengthLabel->SetParent(this->LengthSubFrame);
  this->LengthLabel->Create();
  this->LengthLabel->SetJustificationToRight();
  this->LengthLabel->SetWidth(18);
  this->LengthLabel->SetText("Filter Length");

  this->LengthLabel->SetBalloonHelpString("Enter the integer length of the smoothing filter (2 to N allowed).");

  this->LengthEntry->SetParent(this->LengthSubFrame);
  this->LengthEntry->Create();
  this->LengthEntry->SetWidth(12);
  this->LengthEntry->SetValue("");

  this->LengthEntry->SetBalloonHelpString("Enter the integer length of the smoothing filter (2 to N allowed).");
  this->Script("pack %s %s -side left -pady 1m",
    this->LengthLabel->GetWidgetName(),
    this->LengthEntry->GetWidgetName());
}



//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::CreateNumeratorWeightsTextEntry(vtkKWWidget *topframe)
{
  this->NumeratorWeightsSubFrame->SetParent(topframe);
  this->NumeratorWeightsSubFrame->Create();
  this->Script("pack %s -side top -fill x",
    this->NumeratorWeightsSubFrame->GetWidgetName());

  this->NumeratorWeightsLabel->SetParent(this->NumeratorWeightsSubFrame);
  this->NumeratorWeightsLabel->Create();
  this->NumeratorWeightsLabel->SetJustificationToRight();
  this->NumeratorWeightsLabel->SetWidth(18);
  this->NumeratorWeightsLabel->SetText("Numerator Weights");

  this->NumeratorWeightsLabel->SetBalloonHelpString("Enter the space-separated list of numerator weights. These "
    "are the b() entries in the equation: a(0)*y(n) = b(0)*x(n) + b(1)*x(n-1) + ... "
    " - a(1)*y(n-1) - a(2)*y(n-2) - ...");

  this->NumeratorWeightsEntry->SetParent(this->NumeratorWeightsSubFrame);
  this->NumeratorWeightsEntry->Create();
  this->NumeratorWeightsEntry->SetWidth(32);
  this->NumeratorWeightsEntry->SetValue("");

  this->NumeratorWeightsEntry->SetBalloonHelpString("Enter the space-separated list of numerator weights");
  this->Script("pack %s %s -side left -pady 1m",
    this->NumeratorWeightsLabel->GetWidgetName(),
    this->NumeratorWeightsEntry->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::CreateForwardNumeratorWeightsTextEntry(vtkKWWidget *topframe)
{
  this->ForwardNumeratorWeightsSubFrame->SetParent(topframe);
  this->ForwardNumeratorWeightsSubFrame->Create();
  this->Script("pack %s -side top -fill x",
    this->ForwardNumeratorWeightsSubFrame->GetWidgetName());

  this->ForwardNumeratorWeightsLabel->SetParent(this->ForwardNumeratorWeightsSubFrame);
  this->ForwardNumeratorWeightsLabel->Create();
  this->ForwardNumeratorWeightsLabel->SetJustificationToRight();
  this->ForwardNumeratorWeightsLabel->SetWidth(18);
  this->ForwardNumeratorWeightsLabel->SetText("Forward Numer Weights");

  this->ForwardNumeratorWeightsLabel->SetBalloonHelpString("Enter the space-separated list of numerator weights. These "
    "are the c() entries in the equation: a(0)*y(n) = b(0)*x(n) + b(1)*x(n-1) + ... "
    " - a(1)*y(n-1) - a(2)*y(n-2) - ... + c(0)*x(n+1) + c(1)*x(n+2) + ...");

  this->ForwardNumeratorWeightsEntry->SetParent(this->ForwardNumeratorWeightsSubFrame);
  this->ForwardNumeratorWeightsEntry->Create();
  this->ForwardNumeratorWeightsEntry->SetWidth(32);
  this->ForwardNumeratorWeightsEntry->SetValue("");

  this->ForwardNumeratorWeightsEntry->SetBalloonHelpString("Enter the space-separated list of forward numerator weights");
  this->Script("pack %s %s -side left -pady 1m",
    this->ForwardNumeratorWeightsLabel->GetWidgetName(),
    this->ForwardNumeratorWeightsEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::CreateDenominatorWeightsTextEntry(vtkKWWidget *topframe)
{
  this->DenominatorWeightsSubFrame->SetParent(topframe);
  this->DenominatorWeightsSubFrame->Create();
  this->Script("pack %s -side top -fill x",
    this->DenominatorWeightsSubFrame->GetWidgetName());

  this->DenominatorWeightsLabel->SetParent(this->DenominatorWeightsSubFrame);
  this->DenominatorWeightsLabel->Create();
  this->DenominatorWeightsLabel->SetJustificationToRight();
  this->DenominatorWeightsLabel->SetWidth(18);
  this->DenominatorWeightsLabel->SetText("Denominator Weights");



  this->DenominatorWeightsLabel->SetBalloonHelpString("Enter the space-separated list of denominator weights. These "
    "are the a() entries in the equation: a(0)*y(n) = b(0)*x(n) + b(1)*x(n-1) + ... "
    " - a(1)*y(n-1) - a(2)*y(n-2) - ...");



  this->DenominatorWeightsEntry->SetParent(this->DenominatorWeightsSubFrame);
  this->DenominatorWeightsEntry->Create();
  this->DenominatorWeightsEntry->SetWidth(32);
  this->DenominatorWeightsEntry->SetValue("");

  this->DenominatorWeightsEntry->SetBalloonHelpString("Enter the space-separated list of denominator weights");
  this->Script("pack %s %s -side left -pady 1m",
    this->DenominatorWeightsLabel->GetWidgetName(),
    this->DenominatorWeightsEntry->GetWidgetName());
}





//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::CreateAddThisVarButton(vtkKWWidget *topframe)
{
  this->AddThisVarSubFrame->SetParent(topframe);
  this->AddThisVarSubFrame->Create();
  this->Script("pack %s -side top -fill x",
    this->AddThisVarSubFrame->GetWidgetName());


  this->AddThisVarButton->SetText("Add Output Variable");
  this->AddThisVarButton->SetParent(this->AddThisVarSubFrame);
  this->AddThisVarButton->Create();

  this->AddThisVarButton->SetCommand(this, "AddVarFunction");


  this->AddThisVarButton->SetBalloonHelpString("Add this output to the list of variables to be calculated");


  this->Script("grid %s -row 0 -column 0 -columnspan 2", 
    this->AddThisVarButton->GetWidgetName());



#if 1 
  //put the first delete/label pair in, but disable it,
  //so that the alignment doesnt change later

  int which=0;
  this->DeleteThisVarButton[which]->SetText("Remove");
  this->DeleteThisVarButton[which]->SetParent(this->AddThisVarSubFrame);
  this->DeleteThisVarButton[which]->Create();
  this->DeleteThisVarButton[which]->SetCommand(this, "DeleteVarFunction 0");
  this->DeleteThisVarButton[which]->SetBalloonHelpString("Remove this output variable");
  this->DeleteThisVarLabel[which]->SetParent(this->AddThisVarSubFrame);
  this->DeleteThisVarLabel[which]->Create();
  this->DeleteThisVarLabel[which]->SetJustificationToRight();
  this->DeleteThisVarLabel[which]->SetWidth(18);
  this->DeleteThisVarLabel[which]->SetBalloonHelpString("Output Variable Description");
  this->DeleteThisVarLabel[which]->SetText("");
  this->Script("grid %s %s -row %d", 
    this->DeleteThisVarButton[which]->GetWidgetName(),
    this->DeleteThisVarLabel[which]->GetWidgetName(),
    which+1 );
  this->DeleteVarFunction(which);
  m_numOutputVariables=1;
#endif



  this->Script("grid columnconfigure %s 1 -minsize 50",
    this->AddThisVarSubFrame->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -minsize 80",
    this->AddThisVarSubFrame->GetWidgetName());

}



//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::SetWidgetsToSmoothingFilterMode()
{
  //widget to select the cutoff freq for basic filters
  this->CutoffFreqLabel->SetEnabled(0);
  this->CutoffFreqMenu->SetEnabled(0);

  //
  this->LengthEntry->SetEnabled(1);
  this->LengthLabel->SetEnabled(1);

  //text entry box to specify numerator weights for user defined filter
  this->NumeratorWeightsEntry->SetEnabled(0);
  this->NumeratorWeightsLabel->SetEnabled(0);

  //text entry box to specify denominator weights for user defined filter
  this->DenominatorWeightsEntry->SetEnabled(0);
  this->DenominatorWeightsLabel->SetEnabled(0);

  //text entry box to specify numerator weights for user defined filter
  this->ForwardNumeratorWeightsEntry->SetEnabled(0);
  this->ForwardNumeratorWeightsLabel->SetEnabled(0);
}

//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::SetWidgetsToBasicFilterMode()
{
  //widget to select the cutoff freq for basic filters
  this->CutoffFreqLabel->SetEnabled(1);
  this->CutoffFreqMenu->SetEnabled(1);

  //
  this->LengthEntry->SetEnabled(0);
  this->LengthLabel->SetEnabled(0);

  //text entry box to specify numerator weights for user defined filter
  this->NumeratorWeightsEntry->SetEnabled(0);
  this->NumeratorWeightsLabel->SetEnabled(0);

  //text entry box to specify denominator weights for user defined filter
  this->DenominatorWeightsEntry->SetEnabled(0);
  this->DenominatorWeightsLabel->SetEnabled(0);

  //text entry box to specify numerator weights for user defined filter
  this->ForwardNumeratorWeightsEntry->SetEnabled(0);
  this->ForwardNumeratorWeightsLabel->SetEnabled(0);
}
//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::SetWidgetsToUserDefinedMode()
{
  //widget to select the cutoff freq for basic filters
  this->CutoffFreqLabel->SetEnabled(0);
  this->CutoffFreqMenu->SetEnabled(0);

  //
  this->LengthEntry->SetEnabled(0);
  this->LengthLabel->SetEnabled(0);

  //text entry box to specify numerator weights for user defined filter
  this->NumeratorWeightsEntry->SetEnabled(1);
  this->NumeratorWeightsLabel->SetEnabled(1);

  //text entry box to specify denominator weights for user defined filter
  this->DenominatorWeightsEntry->SetEnabled(1);
  this->DenominatorWeightsLabel->SetEnabled(1);

  //text entry box to specify numerator weights for user defined filter
  this->ForwardNumeratorWeightsEntry->SetEnabled(1);
  this->ForwardNumeratorWeightsLabel->SetEnabled(1);
}
//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::SetWidgetsToIntegralMode()
{
  //widget to select the cutoff freq for basic filters
  this->CutoffFreqLabel->SetEnabled(0);
  this->CutoffFreqMenu->SetEnabled(0);

  //
  this->LengthEntry->SetEnabled(0);
  this->LengthLabel->SetEnabled(0);

  //text entry box to specify numerator weights for user defined filter
  this->NumeratorWeightsEntry->SetEnabled(0);
  this->NumeratorWeightsLabel->SetEnabled(0);

  //text entry box to specify denominator weights for user defined filter
  this->DenominatorWeightsEntry->SetEnabled(0);
  this->DenominatorWeightsLabel->SetEnabled(0);

  //text entry box to specify numerator weights for user defined filter
  this->ForwardNumeratorWeightsEntry->SetEnabled(0);
  this->ForwardNumeratorWeightsLabel->SetEnabled(0);
}
//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::SetWidgetsToCorrelationMode()
{
  //widget to select the cutoff freq for basic filters
  this->CutoffFreqLabel->SetEnabled(0);
  this->CutoffFreqMenu->SetEnabled(0);

  //
  this->LengthEntry->SetEnabled(0);
  this->LengthLabel->SetEnabled(0);

  //text entry box to specify numerator weights for user defined filter
  this->NumeratorWeightsEntry->SetEnabled(0);
  this->NumeratorWeightsLabel->SetEnabled(0);

  //text entry box to specify denominator weights for user defined filter
  this->DenominatorWeightsEntry->SetEnabled(0);
  this->DenominatorWeightsLabel->SetEnabled(0);

  //text entry box to specify numerator weights for user defined filter
  this->ForwardNumeratorWeightsEntry->SetEnabled(0);
  this->ForwardNumeratorWeightsLabel->SetEnabled(0);
}


//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::DeleteVarFunction(int which)
{
  /*
  printf("--vtkPVBasicDSPFilterWidget del var fn, which=%d          GetEnabled= %d %d\n",
  which,
  this->DeleteThisVarButton[which]->GetEnabled(),
  this->DeleteThisVarLabel[which]->GetEnabled() );
  */
  //this->DeleteThisVarButton[which]->SetStateOption(0);
  //this->DeleteThisVarLabel[which]->SetStateOption(0);


  if( this->DeleteThisVarLabel[which]->GetEnabled() )
    {
    this->RemoveThisFilterFromSource(m_outputVariableNames[which]);
    this->DeleteThisVarButton[which]->SetEnabled(0);
    this->DeleteThisVarLabel[which]->SetEnabled(0);

    this->ModifiedCallback();
    }

}

//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::SetOutputVariableName(int a_which)
{
  int i;
  int l_count=0;
  bool l_isUnique=false;
  char *l_testName = new char[ sizeof(this->OutputVarEntry->GetValue())+10 ];

  sprintf(l_testName,"%s",this->OutputVarEntry->GetValue());
  while(!l_isUnique)
    {
    l_isUnique=true;
    //first, check the name against all other output vars
    for(i=0;i<m_numOutputVariables;i++)
      {
      if(this->DeleteThisVarButton[i]->GetEnabled())
        {
        if(!strcmp(l_testName, m_outputVariableNames[i]))
          {
          l_isUnique=false;//this name is already being used
          break;
          }
        }
      }
    //second, check the name against input variables
    if(l_isUnique)
      {
      int l_numInputVars = this->InputVarMenu->GetMenu()->GetNumberOfItems();
      for(i=0;i<l_numInputVars;i++)
        {
        if(!strcmp(l_testName, this->InputVarMenu->GetMenu()->GetItemLabel(i)))
          {
          l_isUnique=false;//this name is already being used
          break;
          }
        }
      }
    //third, if the name is not unique, change it
    if(!l_isUnique)
      {
      l_count++;
      sprintf(l_testName,"%s_%d",this->OutputVarEntry->GetValue(),l_count);
      }
    }

  m_outputVariableNames[a_which]=l_testName;
}


//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::ResizeOutputVariableList()
{
  int l_newMaxNum = m_maxNumOutputVariables*2;


  char **l_outputVariableNames=new char*[l_newMaxNum];
  char **l_inputVariableNames=new char*[l_newMaxNum];
  FILTER_WIDGET_FILTER_TYPE *l_filterType=new FILTER_WIDGET_FILTER_TYPE[l_newMaxNum];
  double *l_outputVariableCutoffs=new double[l_newMaxNum];
  vtkKWPushButton **l_DeleteThisVarButton = new vtkKWPushButton*[l_newMaxNum];
  vtkKWLabel **l_DeleteThisVarLabel = new vtkKWLabel*[l_newMaxNum];

  int i;
  for(i=0;i<m_maxNumOutputVariables;i++)
    {
    l_outputVariableNames[i] = m_outputVariableNames[i];
    l_inputVariableNames[i] = m_inputVariableNames[i];
    l_filterType[i] = m_filterType[i];
    l_outputVariableCutoffs[i] = m_outputVariableCutoffs[i];
    l_DeleteThisVarButton[i] = DeleteThisVarButton[i];
    l_DeleteThisVarLabel[i] = DeleteThisVarLabel[i];
    }

  delete[] m_outputVariableNames;
  m_outputVariableNames = l_outputVariableNames;
  delete[] m_inputVariableNames;
  m_inputVariableNames = l_inputVariableNames;
  delete[] m_filterType;
  m_filterType = l_filterType;
  delete[] m_outputVariableCutoffs;
  m_outputVariableCutoffs = l_outputVariableCutoffs;
  delete[] DeleteThisVarButton;
  DeleteThisVarButton = l_DeleteThisVarButton;
  delete[] DeleteThisVarLabel;
  DeleteThisVarLabel = l_DeleteThisVarLabel;

  for(i=m_maxNumOutputVariables;i<l_newMaxNum;i++)
    {
    DeleteThisVarButton[i]=vtkKWPushButton::New();
    DeleteThisVarLabel[i]=vtkKWLabel::New();
    }

  m_maxNumOutputVariables=l_newMaxNum;
}
//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::AddVarFunction()
{
  const char *l_filtertype = this->DSPFilterModeMenu->GetValue();
  const char *l_cutoffstring = this->CutoffFreqMenu->GetValue();
  const char *l_inputvarname = this->InputVarMenu->GetValue();


  //printf("--vtkPVBasicDSPFilterWidget::AddVarFunction %s %s %s %s\n",
  // this->OutputVarEntry->GetValue(),l_filtertype,l_cutoffstring,l_inputvarname);

  int which = 0;
  while( which<m_numOutputVariables && this->DeleteThisVarButton[which]->GetEnabled() )
    {
    which++;
    }

  if( which==m_numOutputVariables &&  m_numOutputVariables>=m_maxNumOutputVariables )
    {
    this->ResizeOutputVariableList();
    }


  this->SetOutputVariableName(which);
  m_inputVariableNames[which]=strdup(l_inputvarname);

  if( !strcmp("Low Pass Filter",l_filtertype) )
    {
    m_filterType[which]=FILTER_WIDGET_LOW_PASS;
    }
  else if( !strcmp("High Pass Filter",l_filtertype) )
    {
    m_filterType[which]=FILTER_WIDGET_HIGH_PASS;
    }
  else if( !strcmp("User Defined Filter",l_filtertype) )
    {
    m_filterType[which]=FILTER_WIDGET_USER_DEFINED;
    }
  else if( !strcmp("Integral",l_filtertype) )
    {
    m_filterType[which]=FILTER_WIDGET_INTEGRAL;
    }
  else if( !strcmp("Derivative",l_filtertype) )
    {
    m_filterType[which]=FILTER_WIDGET_DERIVATIVE;
    }
  else if( !strcmp("Smoothing",l_filtertype) )
    {
    m_filterType[which]=FILTER_WIDGET_SMOOTHING;
    }




  m_outputVariableCutoffs[which]=atof(l_cutoffstring);


  int l_strlen = strlen(this->OutputVarEntry->GetValue())+strlen(l_inputvarname)+100;
  char *l_str = new char[l_strlen];

  if(which==m_numOutputVariables)
    {
    this->DeleteThisVarButton[which]->SetText("Remove");
    this->DeleteThisVarButton[which]->SetParent(this->AddThisVarSubFrame);
    this->DeleteThisVarButton[which]->Create();
    sprintf(l_str,"DeleteVarFunction %d",which );
    this->DeleteThisVarButton[which]->SetCommand(this, l_str);
    this->DeleteThisVarButton[which]->SetBalloonHelpString("Remove this output variable");
    this->DeleteThisVarLabel[which]->SetParent(this->AddThisVarSubFrame);
    this->DeleteThisVarLabel[which]->Create();
    this->DeleteThisVarLabel[which]->SetJustificationToRight();
    this->DeleteThisVarLabel[which]->SetWidth(18);
    this->DeleteThisVarLabel[which]->SetBalloonHelpString("Output Variable Description");
    }


  sprintf(l_str,"%s",
    m_outputVariableNames[which]
  );


  this->DeleteThisVarLabel[which]->SetText(l_str);

  delete[] l_str;

  if(which==m_numOutputVariables)
    {
    this->Script("grid %s %s -row %d", 
      this->DeleteThisVarButton[which]->GetWidgetName(),
      this->DeleteThisVarLabel[which]->GetWidgetName(),
      which+1 );

    this->Script("grid columnconfigure %s 1 -minsize 50",
      this->AddThisVarSubFrame->GetWidgetName());
    this->Script("grid columnconfigure %s 2 -minsize 80",
      this->AddThisVarSubFrame->GetWidgetName());


    m_numOutputVariables++;
    }
  else
    {
    this->DeleteThisVarButton[which]->SetEnabled(1);
    this->DeleteThisVarLabel[which]->SetEnabled(1);;
    }


  ///////////

  this->AddThisFilterToSource(m_inputVariableNames[which],m_outputVariableNames[which],
    m_outputVariableCutoffs[which], m_filterType[which]);




  this->ModifiedCallback();
}




//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::DisableCutoffFreqToggle()
{
  this->CutoffFreqMenu->SetEnabled(0);
  this->CutoffFreqLabel->SetEnabled(0);
}

//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::EnableCutoffFreqToggle()
{
  this->CutoffFreqMenu->SetEnabled(this->GetEnabled());
  this->CutoffFreqLabel->SetEnabled(this->GetEnabled());
}

//----------------------------------------------------------------------------
bool vtkPVBasicDSPFilterWidget::UpdateTogglesWithFileInformation()
{

  ////////////////////////////////

  vtkPVSource *l_pvsource = this->GetPVSource();
  vtkProcessModule* pm = l_pvsource->GetPVApplication()->GetProcessModule();
  int numSources = l_pvsource->GetNumberOfVTKSources();

  if(numSources != 1)
    {
    //this should be an error XXX
    return false;
    }

  int l_whichSource=0;

  vtkClientServerStream stream;

  int l_numVarArrays = -1;
  stream << vtkClientServerStream::Invoke 
    <<  l_pvsource->GetVTKSourceID(l_whichSource)
    << "GetNumberOfVariableArrays"
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
    vtkProcessModule::DATA_SERVER, stream);
  pm->GetLastResult(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
    vtkProcessModule::DATA_SERVER).GetArgument(0, 0, &l_numVarArrays);


  if(l_numVarArrays<0)
    {
    //there is no reader data yet?
    return false;
    }


  for( int i=0; i<l_numVarArrays; i++ )
    {
    char *l_name=NULL;
    stream << vtkClientServerStream::Invoke 
      <<  l_pvsource->GetVTKSourceID(l_whichSource)
      << "GetVariableArrayName"
      << i
      << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);
    pm->GetLastResult(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER).GetArgument(0, 0, &l_name);


    char *l_command = (char *)malloc( strlen(l_name)+64 );
    sprintf(l_command,"ChangeInputVar %s",l_name);

    this->InputVarMenu->GetMenu()->AddRadioButton(l_name, this, l_command);
    if(!i) this->InputVarMenu->SetValue(l_name);

    free(l_command);
    }

  return true;
}




//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  this->DSPFilterFrame->SetParent(this);
  this->DSPFilterFrame->Create();
  this->DSPFilterFrame->SetLabelText("Basic DSP Filtering");
  this->Script("pack %s -fill x -expand t -side top",
    this->DSPFilterFrame->GetWidgetName());

  //this->DSPFilterFrame->PerformShowHideFrame();//start out minimized




  this->DSPFilterModeSubFrame->SetParent(this->DSPFilterFrame->GetFrame());
  this->DSPFilterModeSubFrame->Create();
  this->Script("pack %s -side top -fill x",
    this->DSPFilterModeSubFrame->GetWidgetName());



  this->DSPFilterModeLabel->SetParent(this->DSPFilterModeSubFrame);
  this->DSPFilterModeLabel->Create();
  this->DSPFilterModeLabel->SetJustificationToRight();
  this->DSPFilterModeLabel->SetWidth(18);
  this->DSPFilterModeLabel->SetText("Filtering Mode");
  this->DSPFilterModeLabel->SetBalloonHelpString("Select the type of filter");
  this->DSPFilterModeMenu->SetParent(this->DSPFilterModeSubFrame);
  this->DSPFilterModeMenu->Create();



  this->DSPFilterModeMenu->GetMenu()->AddRadioButton(
    "Smoothing Filter (BSpline)", this, "ChangeDSPFilterMode smoothing");
  this->DSPFilterModeMenu->GetMenu()->AddRadioButton(
    "Low Pass Filter (19th order Butterworth)", this, "ChangeDSPFilterMode lowpass");
  this->DSPFilterModeMenu->GetMenu()->AddRadioButton(
    "High Pass Filter (19th order Butterworth)", this, "ChangeDSPFilterMode highpass");
  this->DSPFilterModeMenu->GetMenu()->AddRadioButton(
    "User Defined Filter", this, "ChangeDSPFilterMode userdef");
  this->DSPFilterModeMenu->GetMenu()->AddRadioButton(
    "Integral", this, "ChangeDSPFilterMode integral");
  this->DSPFilterModeMenu->GetMenu()->AddRadioButton(
    "Derivative", this, "ChangeDSPFilterMode derivative");



  this->DSPFilterModeMenu->SetBalloonHelpString("Select the type of filter");
  this->Script("pack %s %s -side left -pady 1m",
    this->DSPFilterModeLabel->GetWidgetName(),
    this->DSPFilterModeMenu->GetWidgetName());


  CreateLengthTextEntry(this->DSPFilterFrame->GetFrame());
  CreateCutoffFreqToggle(this->DSPFilterFrame->GetFrame());
  CreateNumeratorWeightsTextEntry(this->DSPFilterFrame->GetFrame());
  CreateDenominatorWeightsTextEntry(this->DSPFilterFrame->GetFrame());
  CreateForwardNumeratorWeightsTextEntry(this->DSPFilterFrame->GetFrame());
  CreateInputVarToggle(this->DSPFilterFrame->GetFrame());
  CreateOutputVarTextEntry(this->DSPFilterFrame->GetFrame());
  CreateAddThisVarButton(this->DSPFilterFrame->GetFrame());


  //SET START TO smoothing
  this->LengthEntry->SetValue("3");
  this->DSPFilterModeMenu->SetValue("Smoothing");

  char *l_str;
  l_str=getSmoothingNumeratorWeightsString(this->GetFilterLength());
  this->NumeratorWeightsEntry->SetValue(l_str);
  delete[] l_str;

  this->DenominatorWeightsEntry->SetValue("");

  l_str=getSmoothingForwardNumeratorWeightsString(this->GetFilterLength());
  this->ForwardNumeratorWeightsEntry->SetValue(l_str);
  delete[] l_str;


  this->SetWidgetsToSmoothingFilterMode();

}



//---------------------------------------------------------------------------


void vtkPVBasicDSPFilterWidget::ChangeDSPFilterMode(const char* newMode)
{
  char *l_weightsString = new char[2048];


  if (!strcmp(newMode, "smoothing"))
    {
    this->DSPFilterModeMenu->SetValue("Smoothing");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeDSPFilterMode {%s}",
      this->GetTclName(), newMode);
    this->SetWidgetsToSmoothingFilterMode();

    char *l_str;

    l_str=getSmoothingNumeratorWeightsString(this->GetFilterLength());
    this->NumeratorWeightsEntry->SetValue(l_str);
    delete[] l_str;

    this->DenominatorWeightsEntry->SetValue("");

    l_str=getSmoothingForwardNumeratorWeightsString(this->GetFilterLength());
    this->ForwardNumeratorWeightsEntry->SetValue(l_str);
    delete[] l_str;

    }
  else if (!strcmp(newMode, "lowpass"))
    {
    this->DSPFilterModeMenu->SetValue("Low Pass Filter");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeDSPFilterMode {%s}",
      this->GetTclName(), newMode);
    this->SetWidgetsToBasicFilterMode();

    getNumeratorWeightsString(l_weightsString,2048,true,this->CutoffFreqMenu->GetValue());
    this->NumeratorWeightsEntry->SetValue(l_weightsString);
    getDenominatorWeightsString(l_weightsString,2048,true,this->CutoffFreqMenu->GetValue());
    this->DenominatorWeightsEntry->SetValue(l_weightsString);
    this->ForwardNumeratorWeightsEntry->SetValue("");
    }
  else if (!strcmp(newMode, "highpass"))
    {
    this->DSPFilterModeMenu->SetValue("High Pass Filter");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeDSPFilterMode {%s}",
      this->GetTclName(), newMode);
    this->SetWidgetsToBasicFilterMode();

    getNumeratorWeightsString(l_weightsString,2048,false,this->CutoffFreqMenu->GetValue());
    this->NumeratorWeightsEntry->SetValue(l_weightsString);
    getDenominatorWeightsString(l_weightsString,2048,false,this->CutoffFreqMenu->GetValue());
    this->DenominatorWeightsEntry->SetValue(l_weightsString);
    this->ForwardNumeratorWeightsEntry->SetValue("");
    }
  else if (!strcmp(newMode, "userdef"))
    {
    this->DSPFilterModeMenu->SetValue("User Defined Filter");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeDSPFilterMode {%s}",
      this->GetTclName(), newMode);
    this->SetWidgetsToUserDefinedMode();

    this->NumeratorWeightsEntry->SetValue("");
    this->DenominatorWeightsEntry->SetValue("");
    this->ForwardNumeratorWeightsEntry->SetValue("");
    }
  else if (!strcmp(newMode, "integral"))
    {
    this->DSPFilterModeMenu->SetValue("Integral");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeDSPFilterMode {%s}",
      this->GetTclName(), newMode);
    this->SetWidgetsToIntegralMode();

    this->NumeratorWeightsEntry->SetValue("0 1");
    this->DenominatorWeightsEntry->SetValue("1 -1");
    this->ForwardNumeratorWeightsEntry->SetValue("");
    }
  else if (!strcmp(newMode, "derivative"))
    {
    this->DSPFilterModeMenu->SetValue("Derivative");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeDSPFilterMode {%s}",
      this->GetTclName(), newMode);
    this->SetWidgetsToIntegralMode();

    this->NumeratorWeightsEntry->SetValue("-1");
    this->DenominatorWeightsEntry->SetValue("1");
    this->ForwardNumeratorWeightsEntry->SetValue("1");
    }

  delete[] l_weightsString;

  this->ModifiedCallback();
}






//---------------------------------------------------------------------------


void vtkPVBasicDSPFilterWidget::ChangeCutoffFreq(const char* newMode)
{
  if (!strcmp(newMode, "3"))
    {
    this->CutoffFreqMenu->SetValue(".3");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeCutoffFreq {%s}",
      this->GetTclName(), newMode);
    }
  if (!strcmp(newMode, "4"))
    {
    this->CutoffFreqMenu->SetValue(".4");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeCutoffFreq {%s}",
      this->GetTclName(), newMode);
    }
  if (!strcmp(newMode, "5"))
    {
    this->CutoffFreqMenu->SetValue(".5");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeCutoffFreq {%s}",
      this->GetTclName(), newMode);
    }

  if (!strcmp(newMode, "6"))
    {
    this->CutoffFreqMenu->SetValue(".6");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeCutoffFreq {%s}",
      this->GetTclName(), newMode);
    }
  if (!strcmp(newMode, "7"))
    {
    this->CutoffFreqMenu->SetValue(".7");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeCutoffFreq {%s}",
      this->GetTclName(), newMode);
    }


  char *l_weightsString = new char[2048];
  getNumeratorWeightsString(l_weightsString,2048,
    strcmp("Low Pass Filter",this->DSPFilterModeMenu->GetValue())?false:true,
    this->CutoffFreqMenu->GetValue());
  this->NumeratorWeightsEntry->SetValue(l_weightsString);
  getDenominatorWeightsString(l_weightsString,2048,
    strcmp("Low Pass Filter",this->DSPFilterModeMenu->GetValue())?false:true,
    this->CutoffFreqMenu->GetValue());
  this->DenominatorWeightsEntry->SetValue(l_weightsString);
  this->ForwardNumeratorWeightsEntry->SetValue("");
  delete[] l_weightsString;


  this->ModifiedCallback();
}




//---------------------------------------------------------------------------



void vtkPVBasicDSPFilterWidget::ChangeInputVar(const char* newMode)
{
  if (!strcmp(newMode, "3"))
    {
    this->InputVarMenu->SetValue(".3");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeInputVar {%s}",
      this->GetTclName(), newMode);
    }
  if (!strcmp(newMode, "4"))
    {
    this->InputVarMenu->SetValue(".4");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeInputVar {%s}",
      this->GetTclName(), newMode);
    }
  if (!strcmp(newMode, "5"))
    {
    this->InputVarMenu->SetValue(".5");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeInputVar {%s}",
      this->GetTclName(), newMode);
    }

  if (!strcmp(newMode, "6"))
    {
    this->InputVarMenu->SetValue(".6");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeInputVar {%s}",
      this->GetTclName(), newMode);
    }
  if (!strcmp(newMode, "7"))
    {
    this->InputVarMenu->SetValue(".7");
    this->GetTraceHelper()->AddEntry("$kw(%s) ChangeInputVar {%s}",
      this->GetTclName(), newMode);
    }



  this->ModifiedCallback();
}


//---------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::Trace(ofstream *file)
{
  const char *val;

  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  // dsp filter mode widget
  val = this->DSPFilterModeMenu->GetValue();
  if (!strcmp(val, "Smoothing"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeDSPFilterMode smoothing" << endl;
    }
  else if (!strcmp(val, "Low Pass Filter"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeDSPFilterMode lowpass" << endl;
    }
  else if (!strcmp(val, "High Pass Filter"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeDSPFilterMode highpass" << endl;
    }
  else if (!strcmp(val, "User Defined Filter"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeDSPFilterMode userdef" << endl;
    }
  else if (!strcmp(val, "Integral"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeDSPFilterMode integral" << endl;
    }
  else if (!strcmp(val, "Derivative"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeDSPFilterMode derivative" << endl;
    }

  // cut off freq widget
  val = this->CutoffFreqMenu->GetValue();
  if (!strcmp(val, ".3"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeCutoffFreq 3" << endl;
    }
  else if (!strcmp(val, ".4"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeCutoffFreq 4" << endl;
    }
  else if (!strcmp(val, ".5"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeCutoffFreq 5" << endl;
    }
  else if (!strcmp(val, ".6"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeCutoffFreq 6" << endl;
    }
  else if (!strcmp(val, ".7"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeCutoffFreq 7" << endl;
    }

  // filter length widget
  *file << "$kw(" << this->GetTclName() << ") SetFilterLength "
    << this->GetFilterLength() << endl;
}


//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::AcceptInternal(vtkClientServerID vtkNotUsed(vtkSourceID))
{
  //printf("doing nothing in vtkPVBasicDSPFilterWidget::AcceptInternal\n");
}


//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::Initialize()
{
  vtkClientServerStream stream;

  ////////////////////////////////
  if(!this->m_gotFileInformation)
    {
    int l_numBlockArrays = -1;
    char *l_filename = NULL;


    vtkPVSource *l_pvsource = this->GetPVSource();
    vtkProcessModule* pm = l_pvsource->GetPVApplication()->GetProcessModule();
    //note there should only be one source.....should check this XXX
    int numSources = l_pvsource->GetNumberOfVTKSources();
    int i;
    for(i = 0; i < numSources; ++i)
      {
      stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(i)
        << "GetFileName"
        << vtkClientServerStream::End;
      }
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);
    pm->GetLastResult(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER).GetArgument(0, 0, &l_filename);

    for(i = 0; i < numSources; ++i)
      {
      stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(i)
        << "GetNumberOfBlockArrays"
        << vtkClientServerStream::End;
      }
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);
    pm->GetLastResult(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER).GetArgument(0, 0, &l_numBlockArrays);

    if( l_filename && l_numBlockArrays )
      {
      if( UpdateTogglesWithFileInformation() )
        {
        this->m_gotFileInformation=true;
        }
      }

    }
}

//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::ResetInternal()
{

  //printf("--vtkPVBasicDSPFilterWidget reset internal------this->ModifiedFlag=%d\n",this->ModifiedFlag);

  this->Initialize();


  if(this->m_gotFileInformation)
    {
    this->ModifiedFlag = 0; //???
    }
  else
    {
    this->Modified(); //??? is this right?
    }

  //////////////////////////////////




  /*

  if (this->AcceptCalled)
  {
  this->ModifiedFlag = 0;
  }
  */
}


//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::SaveInBatchScript(ofstream * vtkNotUsed(file))
{

  //printf("--vtkPVBasicDSPFilterWidget save in batch script\n");


}



//-----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::UpdateEnableState()
{

  //printf("--vtkPVBasicDSPFilterWidget update enable state\n");
  /*
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->AttributeModeFrame);
  this->PropagateEnableState(this->AttributeModeLabel);
  this->PropagateEnableState(this->AttributeModeMenu);
  this->PropagateEnableState(this->CalculatorFrame);
  this->PropagateEnableState(this->FunctionLabel);
  this->PropagateEnableState(this->ButtonClear);
  */
}

//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::AddThisFilterToSource(const char *a_inputName, const char *a_outputName,
  double a_cutoff, FILTER_WIDGET_FILTER_TYPE a_filterType)
{      
  printf("vtkPVBasicDSPFilterWidget::AddThisFilterToSource add dsp var %s -> %s, cutoff %f  type=%d\n",
    a_inputName,a_outputName,a_cutoff,(int)a_filterType );


  vtkPVSource *l_pvsource = this->GetPVSource();
  vtkProcessModule* pm = l_pvsource->GetPVApplication()->GetProcessModule();
  //note there should only be one source.....should check this XXX
  int numSources = l_pvsource->GetNumberOfVTKSources();
  if(numSources!=1)
    {
    vtkErrorMacro("vtkPVBasicDSPFilterWidget::AddThisFilterToSource should have 1 source only");
    return;
    }

  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
    << "EnableDSPFiltering"
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
    << "StartAddingFilter"
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
    << "AddFilterInputVar"
    << a_inputName
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
    << "AddFilterOutputVar"
    << a_outputName
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
    vtkProcessModule::DATA_SERVER, stream);


  //ADD THE FILTER WEIGHTS
  int j,l_whichCutoff=2;
  if(a_cutoff<=.3) l_whichCutoff=0;
  else if(a_cutoff<=.4) l_whichCutoff=1;
  else if(a_cutoff<=.5) l_whichCutoff=2;
  else if(a_cutoff<=.6) l_whichCutoff=3;
  else if(a_cutoff<=.7) l_whichCutoff=4;
  if( a_filterType==FILTER_WIDGET_LOW_PASS )
    {
    for(j=0;j<20;j++)
      {
      stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
        << "AddFilterNumeratorWeight"
        << g_butter_lp_numerator_coeffs[l_whichCutoff][j]
        << vtkClientServerStream::End;
      }
    for(j=0;j<20;j++)
      {
      stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
        << "AddFilterDenominatorWeight"
        << g_butter_lp_denominator_coeffs[l_whichCutoff][j]
        << vtkClientServerStream::End;
      }
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);
    }
  else if( a_filterType==FILTER_WIDGET_HIGH_PASS )
    {
    for(j=0;j<20;j++)
      {
      stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
        << "AddFilterNumeratorWeight"
        << g_butter_hp_numerator_coeffs[l_whichCutoff][j]
        << vtkClientServerStream::End;
      }
    for(j=0;j<20;j++)
      {
      stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
        << "AddFilterDenominatorWeight"
        << g_butter_hp_denominator_coeffs[l_whichCutoff][j]
        << vtkClientServerStream::End;
      }
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);
    }
  else if( a_filterType==FILTER_WIDGET_USER_DEFINED )
    {
      { //handle numerator weights
      vtkstd::string l_numString = this->NumeratorWeightsEntry->GetValue();
      vtkstd::string l_number;

      //printf("\nnumerator weights string=%s\n",l_numString.c_str());

      do 
        {
        const size_t l_error = vtkstd::string::npos;
        size_t l_pos = l_numString.find_first_of(' ');
        size_t l_otherpos = l_numString.find_first_of(',');

        //printf("          l_pos=%d l_otherpos=%d\n",l_pos,l_otherpos);

        if( l_otherpos!=l_error && (l_pos==l_error || l_otherpos<l_pos) ) l_pos = l_otherpos;
        l_otherpos = l_numString.find_first_of(';');
        if( l_otherpos!=l_error && (l_pos==l_error || l_otherpos<l_pos) ) l_pos = l_otherpos;
        if(l_pos==0 || l_pos==l_error) 
          {
          l_pos=l_numString.size();//just read until end
          }       
        l_number="";
        for(j=0;j<(int)l_pos;j++)
          {
          l_number.append( 1, (char) l_numString[0] );
          l_numString.erase( l_numString.begin() );
          }
        while( l_numString.size() && ( l_numString[0]==' ' ||
            l_numString[0]==',' ||
            l_numString[0]==';' ) )
          {
          l_numString.erase( l_numString.begin() );
          }
        double l_weight = atof(l_number.c_str());

        //printf("  AddFilterNumeratorWeight %f\n",l_weight);

        stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
          << "AddFilterNumeratorWeight"
          << l_weight
          << vtkClientServerStream::End;
        pm->SendStream(
          vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
          vtkProcessModule::DATA_SERVER, stream);

        } while( l_numString.size() );
      }

      { //handle denominator weights
      vtkstd::string l_numString = this->DenominatorWeightsEntry->GetValue();
      vtkstd::string l_number;

      //printf("\ndenominator weights string=%s\n",l_numString.c_str());

      do {
        const size_t l_error = vtkstd::string::npos;
        size_t l_pos = l_numString.find_first_of(' ');
        size_t l_otherpos = l_numString.find_first_of(',');
        if( l_otherpos!=l_error && (l_pos==l_error || l_otherpos<l_pos) ) l_pos = l_otherpos;
        l_otherpos = l_numString.find_first_of(';');
        if( l_otherpos!=l_error && (l_pos==l_error || l_otherpos<l_pos) ) l_pos = l_otherpos;
        if(l_pos==0 || l_pos==l_error) 
          {
          l_pos=l_numString.size();//just read until end
          }       
        l_number="";
        for(j=0;j<(int)l_pos;j++)
          {
          l_number.append( 1, (char) l_numString[0] );
          l_numString.erase( l_numString.begin() );
          }
        while( l_numString.size() && ( l_numString[0]==' ' ||
            l_numString[0]==',' ||
            l_numString[0]==';' ) )
          {
          l_numString.erase( l_numString.begin() );
          }
        double l_weight = atof(l_number.c_str());

        //printf("  AddFilterDenominatorWeight %f\n",l_weight);

        stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
          << "AddFilterDenominatorWeight"
          << l_weight
          << vtkClientServerStream::End;
        pm->SendStream(
          vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
          vtkProcessModule::DATA_SERVER, stream);

      } while( l_numString.size() );
      }



      { //handle forward numerator weights
      vtkstd::string l_numString = this->ForwardNumeratorWeightsEntry->GetValue();
      vtkstd::string l_number;

      //printf("\nforward numerator weights string=%s\n",l_numString.c_str());

      do {
        const size_t l_error = vtkstd::string::npos;
        size_t l_pos = l_numString.find_first_of(' ');
        size_t l_otherpos = l_numString.find_first_of(',');
        if( l_otherpos!=l_error && (l_pos==l_error || l_otherpos<l_pos) ) l_pos = l_otherpos;
        l_otherpos = l_numString.find_first_of(';');
        if( l_otherpos!=l_error && (l_pos==l_error || l_otherpos<l_pos) ) l_pos = l_otherpos;
        if(l_pos==0 || l_pos==l_error) 
          {
          l_pos=l_numString.size();//just read until end
          }       
        l_number=""; 
        for(j=0;j<(int)l_pos;j++)
          {
          l_number.append( 1, (char) l_numString[0] );
          l_numString.erase( l_numString.begin() );
          }
        while( l_numString.size() && ( l_numString[0]==' ' ||
            l_numString[0]==',' ||
            l_numString[0]==';' ) )
          {
          l_numString.erase( l_numString.begin() );
          }
        double l_weight = atof(l_number.c_str());

        //printf("  AddFilterForwardNumeratorWeight %f\n",l_weight);

        stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
          << "AddFilterForwardNumeratorWeight"
          << l_weight
          << vtkClientServerStream::End;
        pm->SendStream(
          vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
          vtkProcessModule::DATA_SERVER, stream);

      } while( l_numString.size() );
      }



    }
  else if( a_filterType==FILTER_WIDGET_INTEGRAL )
    {
    //The integral at instance n is defined as
    // y[n] = x[n-1]*(t[n]-t[n-1]) + y[n-1]
    //Since we have no times here, delta time is 1.
    double l_weight=0;
    stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
      << "AddFilterNumeratorWeight"
      << l_weight
      << vtkClientServerStream::End;
    l_weight=1;
    stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
      << "AddFilterNumeratorWeight"
      << l_weight
      << vtkClientServerStream::End;
    l_weight=1;
    stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
      << "AddFilterDenominatorWeight"
      << l_weight
      << vtkClientServerStream::End;
    l_weight=-1;
    stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
      << "AddFilterDenominatorWeight"
      << l_weight
      << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);
    }
  else if( a_filterType==FILTER_WIDGET_DERIVATIVE )
    {
    //The derivative at instance n is defined as
    //y[n] = (x[n+1]-x[n])/(t[n+1]-t[n])
    //Since we have no times here, delta time is 1.

    //do the x[n+1] term
    double l_weight=1;
    stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
      << "AddFilterForwardNumeratorWeight"
      << l_weight
      << vtkClientServerStream::End;

    //do the x[n] term
    l_weight=-1;
    stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
      << "AddFilterNumeratorWeight"
      << l_weight
      << vtkClientServerStream::End;

    //do the y[n] term
    l_weight=1;
    stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
      << "AddFilterDenominatorWeight"
      << l_weight
      << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);
    }
  else if( a_filterType==FILTER_WIDGET_SMOOTHING )
    {
    int i;
    int l_length = this->GetFilterLength();
    double *l_weights = this->getSmoothingNumeratorWeights( l_length );



    char *l_str;
    l_str=getSmoothingNumeratorWeightsString(l_length);
    this->NumeratorWeightsEntry->SetValue(l_str);
    delete[] l_str;

    this->DenominatorWeightsEntry->SetValue("");

    l_str=getSmoothingForwardNumeratorWeightsString(this->GetFilterLength());
    this->ForwardNumeratorWeightsEntry->SetValue(l_str);
    delete[] l_str;


    //do the x[n+1] terms
    for(i=1;i<l_length;i++)
      {
      stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
        << "AddFilterForwardNumeratorWeight"
        << l_weights[i]
        << vtkClientServerStream::End;
      }
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);

    //do the x[n] terms
    for(i=0;i<l_length;i++)
      {
      stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
        << "AddFilterNumeratorWeight"
        << l_weights[i]
        << vtkClientServerStream::End;
      }
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);

    //do the y[n] term
    double l_weight=1;
    stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
      << "AddFilterDenominatorWeight"
      << l_weight
      << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);

    delete[] l_weights;
    }
  else 
    {
    printf("\n\nerror WHAT KIND OF FILTER IS THIS?\n\n");
    }


  stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
    << "FinishAddingFilter"
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
    vtkProcessModule::DATA_SERVER, stream);

}
//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::RemoveThisFilterFromSource(const char *a_outputName)
{
  if(a_outputName)
    {
    printf("vtkPVBasicDSPFilterWidget::RemoveThisFilterFromSource output var name %s\n",
      a_outputName );


    vtkPVSource *l_pvsource = this->GetPVSource();
    vtkProcessModule* pm = l_pvsource->GetPVApplication()->GetProcessModule();
    //note there should only be one source.....should check this XXX
    int numSources = l_pvsource->GetNumberOfVTKSources();
    if(numSources!=1)
      {
      vtkErrorMacro("vtkPVBasicDSPFilterWidget::RemoveThisFilterFromSource should have 1 source only");
      return;
      }

    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke <<  l_pvsource->GetVTKSourceID(0)
      << "RemoveFilter"
      << a_outputName
      << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);
    }
}

//----------------------------------------------------------------------------
int vtkPVBasicDSPFilterWidget::GetFilterLength()
{
  const char *l_str = this->LengthEntry->GetValue();

  int l_len = atoi(l_str);

  if(l_len<2)
    {
    l_len=2;
    }
  else if(l_len>1000)
    {
    //There is no real reason to limit the size of the smoothing
    //filter, other than that very high lengths would use a lot
    //of memory, and also that it is unlikely that such a long
    //filter would be useful to anyone.
    //
    //Limiting it here will prevent someone from accidently or
    //experimentally crashing paraview for lack of memory.
    l_len=1000;
    }

  char l_fixed[64];
  sprintf(l_fixed,"%d",l_len);
  this->LengthEntry->SetValue(l_fixed);


  return(l_len);
}

//----------------------------------------------------------------------------
void vtkPVBasicDSPFilterWidget::SetFilterLength(int l_len)
{
  if(l_len<2)
    {
    l_len=2;
    }
  else if(l_len>1000)
    {
    //There is no real reason to limit the size of the smoothing
    //filter, other than that very high lengths would use a lot
    //of memory, and also that it is unlikely that such a long
    //filter would be useful to anyone.
    //
    //Limiting it here will prevent someone from accidently or
    //experimentally crashing paraview for lack of memory.
    l_len=1000;
    }

  char l_fixed[64];
  sprintf(l_fixed,"%d",l_len);
  this->LengthEntry->SetValue(l_fixed);
}
