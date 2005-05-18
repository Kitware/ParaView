/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplicationResources.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVApplication.h"

#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWPushButton.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWText.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWindow.h"

#include "vtkPVOptions.h"
#include "vtkPVWindow.h"

#include <vtkstd/string>

#include "vtkPVSourceInterfaceDirectories.h"

// Buttons

#include "Resources/vtkPV3DCursorButton.h"
#include "Resources/vtkPVCalculatorButton.h"
#include "Resources/vtkPVClipButton.h"
#include "Resources/vtkPVContourButton.h"
#include "Resources/vtkPVCutButton.h"
#include "Resources/vtkPVEditCenterButtonClose.h"
#include "Resources/vtkPVEditCenterButtonOpen.h"
#include "Resources/vtkPVExtractGridButton.h"
#include "Resources/vtkPVFlyButton.h"
#include "Resources/vtkPVGlyphButton.h"
#include "Resources/vtkPVHideCenterButton.h"
#include "Resources/vtkPVPickCenterButton.h"
#include "Resources/vtkPVProbeButton.h"
#include "Resources/vtkPVResetCenterButton.h"
#include "Resources/vtkPVResetViewButton.h"
#include "Resources/vtkPVRotateViewButton.h"
#include "Resources/vtkPVShowCenterButton.h"
#include "Resources/vtkPVStreamTracerButton.h"
#include "Resources/vtkPVThresholdButton.h"
#include "Resources/vtkPVTranslateViewButton.h"
#include "Resources/vtkPVVectorDisplacementButton.h"
#include "Resources/vtkPVPickButton.h"
#include "Resources/vtkPVRulerButton.h"
#include "Resources/vtkPVIntegrateFlowButton.h"
#include "Resources/vtkPVSurfaceVectorsButton.h"
#include "Resources/vtkPVSelectionWindowButton.h"
#include "Resources/vtkPVPullDownArrow.h"
#include "Resources/vtkPVToolbarPullDownArrow.h"
#include "Resources/vtkPVMandelbrotButton.h"
#include "Resources/vtkPVWaveletButton.h"
#include "Resources/vtkPVSphereSourceButton.h"
#include "Resources/vtkPVSuperquadricButton.h"
#include "Resources/vtkPVGroupButton.h"
#include "Resources/vtkPVUngroupButton.h"
#include "Resources/vtkPVLockButton.h"
#include "Resources/vtkPVAMRPartButton.h"
#include "Resources/vtkPVAMROutlineButton.h"
#include "Resources/vtkPVAMRSurfaceButton.h"
#include "Resources/vtkPVRamp.h"
#include "Resources/vtkPVStep.h"
#include "Resources/vtkPVExponential.h"
#include "Resources/vtkPVSinusoid.h"
#include "Resources/vtkPVKeyFrameChanges.h"
#include "Resources/vtkPVInitState.h"
#include "Resources/vtkPVRecord.h"
#include "Resources/vtkPVRecordState.h"
#include "Resources/vtkPVMovie.h"

// Splash screen

#include "Resources/vtkPVSplashScreen.h"

//----------------------------------------------------------------------------
void vtkPVApplication::CreateButtonPhotos()
{
  this->CreatePhoto("PVLockedButton",
                    image_PVLockedButton , 
                    image_PVLockedButton_width, 
                    image_PVLockedButton_height,
                    image_PVLockedButton_pixel_size,
                    image_PVLockedButton_buffer_length);

  this->CreatePhoto("PVUnlockedButton",
                    image_PVUnlockedButton , 
                    image_PVUnlockedButton_width, 
                    image_PVUnlockedButton_height,
                    image_PVUnlockedButton_pixel_size,
                    image_PVUnlockedButton_buffer_length);

  this->CreatePhoto("PVPullDownArrow",
                    image_PVPullDownArrow , 
                    image_PVPullDownArrow_width, 
                    image_PVPullDownArrow_height,
                    image_PVPullDownArrow_pixel_size,
                    image_PVPullDownArrow_buffer_length);

  this->CreatePhoto("PVToolbarPullDownArrow",
                    image_PVToolbarPullDownArrow , 
                    image_PVToolbarPullDownArrow_width, 
                    image_PVToolbarPullDownArrow_height,
                    image_PVToolbarPullDownArrow_pixel_size,
                    image_PVToolbarPullDownArrow_buffer_length);

  this->CreatePhoto("PVResetViewButton", 
                    image_PVResetViewButton, 
                    image_PVResetViewButton_width, 
                    image_PVResetViewButton_height,
                    image_PVResetViewButton_pixel_size,
                    image_PVResetViewButton_buffer_length);

  this->CreatePhoto("PVTranslateViewButton", 
                    image_PVTranslateViewButton, 
                    image_PVTranslateViewButton_width, 
                    image_PVTranslateViewButton_height,
                    image_PVTranslateViewButton_pixel_size,
                    image_PVTranslateViewButton_buffer_length);

  this->CreatePhoto("PVTranslateViewButtonActive", 
                    image_PVTranslateViewButtonActive, 
                    image_PVTranslateViewButtonActive_width, 
                    image_PVTranslateViewButtonActive_height,
                    image_PVTranslateViewButtonActive_pixel_size,
                    image_PVTranslateViewButtonActive_buffer_length);

  this->CreatePhoto("PVFlyButton", 
                    image_PVFlyButton, 
                    image_PVFlyButton_width, 
                    image_PVFlyButton_height,
                    image_PVFlyButton_pixel_size,
                    image_PVFlyButton_buffer_length);

  this->CreatePhoto("PVFlyButtonActive", 
                    image_PVFlyButtonActive, 
                    image_PVFlyButtonActive_width, 
                    image_PVFlyButtonActive_height,
                    image_PVFlyButtonActive_pixel_size,
                    image_PVFlyButtonActive_buffer_length);

  this->CreatePhoto("PVRotateViewButton", 
                    image_PVRotateViewButton, 
                    image_PVRotateViewButton_width, 
                    image_PVRotateViewButton_height,
                    image_PVRotateViewButton_pixel_size,
                    image_PVRotateViewButton_buffer_length);

  this->CreatePhoto("PVRotateViewButtonActive", 
                    image_PVRotateViewButtonActive, 
                    image_PVRotateViewButtonActive_width, 
                    image_PVRotateViewButtonActive_height,
                    image_PVRotateViewButtonActive_pixel_size,
                    image_PVRotateViewButtonActive_buffer_length);

  this->CreatePhoto("PVPickCenterButton", 
                    image_PVPickCenterButton, 
                    image_PVPickCenterButton_width, 
                    image_PVPickCenterButton_height,
                    image_PVPickCenterButton_pixel_size,
                    image_PVPickCenterButton_buffer_length);
  
  this->CreatePhoto("PVResetCenterButton", 
                    image_PVResetCenterButton, 
                    image_PVResetCenterButton_width, 
                    image_PVResetCenterButton_height,
                    image_PVResetCenterButton_pixel_size,
                    image_PVResetCenterButton_buffer_length);
  
  this->CreatePhoto("PVShowCenterButton", 
                    image_PVShowCenterButton, 
                    image_PVShowCenterButton_width, 
                    image_PVShowCenterButton_height,
                    image_PVShowCenterButton_pixel_size,
                    image_PVShowCenterButton_buffer_length);
  
  this->CreatePhoto("PVHideCenterButton", 
                    image_PVHideCenterButton, 
                    image_PVHideCenterButton_width, 
                    image_PVHideCenterButton_height,
                    image_PVHideCenterButton_pixel_size,
                    image_PVHideCenterButton_buffer_length);
  
  this->CreatePhoto("PVEditCenterButtonOpen", 
                    image_PVEditCenterButtonOpen, 
                    image_PVEditCenterButtonOpen_width, 
                    image_PVEditCenterButtonOpen_height,
                    image_PVEditCenterButtonOpen_pixel_size,
                    image_PVEditCenterButtonOpen_buffer_length);
  
  this->CreatePhoto("PVEditCenterButtonClose", 
                    image_PVEditCenterButtonClose, 
                    image_PVEditCenterButtonClose_width, 
                    image_PVEditCenterButtonClose_height,
                    image_PVEditCenterButtonClose_pixel_size,
                    image_PVEditCenterButtonClose_buffer_length);
  
  this->CreatePhoto("PVCalculatorButton", 
                    image_PVCalculatorButton, 
                    image_PVCalculatorButton_width, 
                    image_PVCalculatorButton_height,
                    image_PVCalculatorButton_pixel_size,
                    image_PVCalculatorButton_buffer_length);

  this->CreatePhoto("PVThresholdButton", 
                    image_PVThresholdButton, 
                    image_PVThresholdButton_width, 
                    image_PVThresholdButton_height,
                    image_PVThresholdButton_pixel_size,
                    image_PVThresholdButton_buffer_length);

  this->CreatePhoto("PVContourButton", 
                    image_PVContourButton, 
                    image_PVContourButton_width, 
                    image_PVContourButton_height,
                    image_PVContourButton_pixel_size,
                    image_PVContourButton_buffer_length);

  this->CreatePhoto("PVProbeButton", 
                    image_PVProbeButton, 
                    image_PVProbeButton_width, 
                    image_PVProbeButton_height,
                    image_PVProbeButton_pixel_size,
                    image_PVProbeButton_buffer_length);

  this->CreatePhoto("PVGlyphButton", 
                    image_PVGlyphButton, 
                    image_PVGlyphButton_width, 
                    image_PVGlyphButton_height,
                    image_PVGlyphButton_pixel_size,
                    image_PVGlyphButton_buffer_length);

  this->CreatePhoto("PV3DCursorButton", 
                    image_PV3DCursorButton, 
                    image_PV3DCursorButton_width, 
                    image_PV3DCursorButton_height,
                    image_PV3DCursorButton_pixel_size,
                    image_PV3DCursorButton_buffer_length);

  this->CreatePhoto("PV3DCursorButtonActive", 
                    image_PV3DCursorButtonActive, 
                    image_PV3DCursorButtonActive_width, 
                    image_PV3DCursorButtonActive_height,
                    image_PV3DCursorButtonActive_pixel_size,
                    image_PV3DCursorButtonActive_buffer_length);

  this->CreatePhoto("PVCutButton", 
                    image_PVCutButton, 
                    image_PVCutButton_width, 
                    image_PVCutButton_height,
                    image_PVCutButton_pixel_size,
                    image_PVCutButton_buffer_length);

  this->CreatePhoto("PVClipButton", 
                    image_PVClipButton, 
                    image_PVClipButton_width, 
                    image_PVClipButton_height,
                    image_PVClipButton_pixel_size,
                    image_PVClipButton_buffer_length);

  this->CreatePhoto("PVExtractGridButton", 
                    image_PVExtractGridButton, 
                    image_PVExtractGridButton_width, 
                    image_PVExtractGridButton_height,
                    image_PVExtractGridButton_pixel_size,
                    image_PVExtractGridButton_buffer_length);

  this->CreatePhoto("PVVectorDisplacementButton", 
                    image_PVVectorDisplacementButton, 
                    image_PVVectorDisplacementButton_width, 
                    image_PVVectorDisplacementButton_height,
                    image_PVVectorDisplacementButton_pixel_size,
                    image_PVVectorDisplacementButton_buffer_length);

  this->CreatePhoto("PVStreamTracerButton", 
                    image_PVStreamTracerButton, 
                    image_PVStreamTracerButton_width, 
                    image_PVStreamTracerButton_height,
                    image_PVStreamTracerButton_pixel_size,
                    image_PVStreamTracerButton_buffer_length);

  this->CreatePhoto("PVRulerButton", 
                    image_PVRulerButton, 
                    image_PVRulerButton_width, 
                    image_PVRulerButton_height,
                    image_PVRulerButton_pixel_size,
                    image_PVRulerButton_buffer_length);

  this->CreatePhoto("PVNavigationWindowButton", 
                    image_PVNavigationWindowButton, 
                    image_PVNavigationWindowButton_width, 
                    image_PVNavigationWindowButton_height,
                    image_PVNavigationWindowButton_pixel_size,
                    image_PVNavigationWindowButton_buffer_length);

  this->CreatePhoto("PVSelectionWindowButton", 
                    image_PVSelectionWindowButton, 
                    image_PVSelectionWindowButton_width, 
                    image_PVSelectionWindowButton_height,
                    image_PVSelectionWindowButton_pixel_size,
                    image_PVSelectionWindowButton_buffer_length);

  this->CreatePhoto("PVPickButton", 
                    image_PVPickButton, 
                    image_PVPickButton_width, 
                    image_PVPickButton_height,
                    image_PVPickButton_pixel_size,
                    image_PVPickButton_buffer_length);

  this->CreatePhoto("PVIntegrateFlowButton", 
                    image_PVIntegrateFlowButton, 
                    image_PVIntegrateFlowButton_width, 
                    image_PVIntegrateFlowButton_height,
                    image_PVIntegrateFlowButton_pixel_size,
                    image_PVIntegrateFlowButton_buffer_length);

  this->CreatePhoto("PVSurfaceVectorsButton", 
                    image_PVSurfaceVectorsButton, 
                    image_PVSurfaceVectorsButton_width, 
                    image_PVSurfaceVectorsButton_height,
                    image_PVSurfaceVectorsButton_pixel_size,
                    image_PVSurfaceVectorsButton_buffer_length);

  this->CreatePhoto("PVMandelbrotButton", 
                    image_PVMandelbrotButton, 
                    image_PVMandelbrotButton_width, 
                    image_PVMandelbrotButton_height,
                    image_PVMandelbrotButton_pixel_size,
                    image_PVMandelbrotButton_buffer_length);

  this->CreatePhoto("PVWaveletButton", 
                    image_PVWaveletButton, 
                    image_PVWaveletButton_width, 
                    image_PVWaveletButton_height,
                    image_PVWaveletButton_pixel_size,
                    image_PVWaveletButton_buffer_length);

  this->CreatePhoto("PVSphereSourceButton", 
                    image_PVSphereSourceButton, 
                    image_PVSphereSourceButton_width, 
                    image_PVSphereSourceButton_height,
                    image_PVSphereSourceButton_pixel_size,
                    image_PVSphereSourceButton_buffer_length);

  this->CreatePhoto("PVSuperquadricButton", 
                    image_PVSuperquadricButton, 
                    image_PVSuperquadricButton_width, 
                    image_PVSuperquadricButton_height,
                    image_PVSuperquadricButton_pixel_size,
                    image_PVSuperquadricButton_buffer_length);

  this->CreatePhoto("PVGroupButton", 
                    image_PVGroupButton, 
                    image_PVGroupButton_width, 
                    image_PVGroupButton_height,
                    image_PVGroupButton_pixel_size,
                    image_PVGroupButton_buffer_length);

  this->CreatePhoto("PVUngroupButton", 
                    image_PVUngroupButton, 
                    image_PVUngroupButton_width, 
                    image_PVUngroupButton_height,
                    image_PVUngroupButton_pixel_size,
                    image_PVUngroupButton_buffer_length);

  this->CreatePhoto("PVAMRPartButton", 
                    image_PVAMRPartButton, 
                    image_PVAMRPartButton_width, 
                    image_PVAMRPartButton_height,
                    image_PVAMRPartButton_pixel_size,
                    image_PVAMRPartButton_buffer_length);

  this->CreatePhoto("PVAMROutlineButton", 
                    image_PVAMROutlineButton, 
                    image_PVAMROutlineButton_width, 
                    image_PVAMROutlineButton_height,
                    image_PVAMROutlineButton_pixel_size,
                    image_PVAMROutlineButton_buffer_length);

  this->CreatePhoto("PVAMRSurfaceButton", 
                    image_PVAMRSurfaceButton, 
                    image_PVAMRSurfaceButton_width, 
                    image_PVAMRSurfaceButton_height,
                    image_PVAMRSurfaceButton_pixel_size,
                    image_PVAMRSurfaceButton_buffer_length);

  this->CreatePhoto("PVRamp",
    image_PVRamp,
    image_PVRamp_width,
    image_PVRamp_height,
    image_PVRamp_pixel_size,
    image_PVRamp_buffer_length);

  this->CreatePhoto("PVStep",
    image_PVStep,
    image_PVStep_width,
    image_PVStep_height,
    image_PVStep_pixel_size,
    image_PVStep_buffer_length);

  this->CreatePhoto("PVExponential",
    image_PVExponential,
    image_PVExponential_width,
    image_PVExponential_height,
    image_PVExponential_pixel_size,
    image_PVExponential_buffer_length);
  
  this->CreatePhoto("PVSinusoid",
    image_PVSinusoid,
    image_PVSinusoid_width,
    image_PVSinusoid_height,
    image_PVSinusoid_pixel_size,
    image_PVSinusoid_buffer_length);
  
  this->CreatePhoto("PVKeyFrameChanges",
    image_PVKeyFrameChanges,
    image_PVKeyFrameChanges_width,
    image_PVKeyFrameChanges_height,
    image_PVKeyFrameChanges_pixel_size,
    image_PVKeyFrameChanges_buffer_length);

  this->CreatePhoto("PVInitState",
    image_PVInitState,
    image_PVInitState_width,
    image_PVInitState_height,
    image_PVInitState_pixel_size,
    image_PVInitState_buffer_length);

  this->CreatePhoto("PVRecord",
    image_PVRecord,
    image_PVRecord_width,
    image_PVRecord_height,
    image_PVRecord_pixel_size,
    image_PVRecord_buffer_length);

  this->CreatePhoto("PVRecordState",
    image_PVRecordState,
    image_PVRecordState_width,
    image_PVRecordState_height,
    image_PVRecordState_pixel_size,
    image_PVRecordState_buffer_length);

  this->CreatePhoto("PVMovie",
    image_PVMovie,
    image_PVMovie_width,
    image_PVMovie_height,
    image_PVMovie_pixel_size,
    image_PVMovie_buffer_length);
}

//----------------------------------------------------------------------------
void vtkPVApplication::CreateSplashScreen()
{
  // copy the image from the header file into memory
  unsigned char *buffer = 
    new unsigned char [image_PVSplashScreen_buffer_length];

  int i;
  unsigned char *curPos = buffer;
  for (i = 0; i < image_PVSplashScreen_nb_sections; i++)
    {
    size_t len = strlen((const char*)image_PVSplashScreen_sections[i]);
    memcpy(curPos, image_PVSplashScreen_sections[i], len);
    curPos += len;
    }
  
  this->CreatePhoto("PVSplashScreen", 
                    buffer, 
                    image_PVSplashScreen_width, 
                    image_PVSplashScreen_height,
                    image_PVSplashScreen_pixel_size,
                    image_PVSplashScreen_buffer_length);
  delete [] buffer;

  if (!this->GetSplashScreen()->IsCreated())
    {
    this->GetSplashScreen()->Create(this, NULL);
    }
  this->GetSplashScreen()->SetProgressMessageVerticalOffset(-17);
  this->GetSplashScreen()->SetImageName("PVSplashScreen");
}

//----------------------------------------------------------------------------
void vtkPVApplication::ConfigureAboutDialog()
{
  this->Superclass::ConfigureAboutDialog();

  if (!this->SaveRuntimeInfoButton)
    {
    this->SaveRuntimeInfoButton = vtkKWPushButton::New();
    }
  if (!this->SaveRuntimeInfoButton->IsCreated())
    {
    this->SaveRuntimeInfoButton->SetParent(
      this->AboutDialog->GetBottomFrame());
    this->SaveRuntimeInfoButton->SetText("Save Information");
    this->SaveRuntimeInfoButton->Create(this, "-width 16");
    this->SaveRuntimeInfoButton->SetCommand(this, "SaveRuntimeInformation");
    }
  this->Script("pack %s -side bottom",
               this->SaveRuntimeInfoButton->GetWidgetName());
  this->AboutRuntimeInfo->SetHeight(14);
  this->AboutRuntimeInfo->GetTextWidget()->ConfigureOptions(
    "-font {Helvetica 9}");
}

//----------------------------------------------------------------------------
void vtkPVApplication::SaveRuntimeInformation()
{
  vtkKWWindow *window = this->GetMainWindow();
  vtkKWLoadSaveDialog *dialog = vtkKWLoadSaveDialog::New();
  this->GetApplication()->RetrieveDialogLastPathRegistryValue(dialog, "RuntimeInformationPath");
  dialog->SaveDialogOn();
  dialog->SetParent(this->AboutDialog);
  dialog->SetTitle("Save Runtime Information");
  dialog->SetFileTypes("{{text file} {.txt}}");
  dialog->Create(this, 0);

  if (dialog->Invoke() &&
      strlen(dialog->GetFileName()) > 0)
    {
    const char *filename = dialog->GetFileName();
    ofstream file;
    file.open(filename, ios::out);
    if (file.fail())
      {
      vtkErrorMacro("Could not write file " << filename);
      dialog->Delete();
      return;
      }
    this->AddAboutText(file);
    file << endl;
    this->AddAboutCopyrights(file);
    this->GetApplication()->SaveDialogLastPathRegistryValue(dialog, "RuntimeInformationPath");
    }
  dialog->Delete();
}

//----------------------------------------------------------------------------
void vtkPVApplication::AddAboutText(ostream &os)
{
  os << this->GetName() << " was developed by Kitware Inc." << endl
     << "http://www.paraview.org" << endl
     << "http://www.kitware.com" << endl
     << "This is version " << this->MajorVersion << "." << this->MinorVersion
     << ", release " << this->GetReleaseName() << endl;

  ostrstream str;
  vtkIndent indent;
  this->GetOptions()->PrintSelf( str, indent.GetNextIndent() );
  str << ends;
  vtkstd::string tmp = str.str();
  os << endl << tmp.substr( tmp.find( "Runtime information:" ) ).c_str();
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkPVApplication::CreatePhoto(const char *name, 
                                   const unsigned char *data, 
                                   int width, int height, int pixel_size,
                                   unsigned long buffer_length,
                                   const char *filename)
{
  char dir[1024];
  sprintf(dir, "%s/../GUI/Client/Resources", VTK_PV_SOURCE_CONFIG_DIR);
  if (!vtkKWTkUtilities::UpdateOrLoadPhoto(
        this->GetMainInterp(),
        name, 
        filename ? filename : name,
        dir,
        data, 
        width, height, 
        pixel_size,
        buffer_length))
    {
    vtkWarningMacro("Error updating Tk photo " << name);
    }
}
