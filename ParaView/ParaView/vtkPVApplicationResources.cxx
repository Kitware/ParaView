/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplicationResources.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVApplication.h"

#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWTkUtilities.h"

#include "vtkPNGReader.h"

#include <sys/stat.h>

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

#include "Resources/vtkPVSelectionWindowButton.h"

// Splash screen

#include "Resources/vtkPVSplashScreen.h"

//----------------------------------------------------------------------------
void vtkPVApplication::CreateButtonPhotos()
{
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
}

//----------------------------------------------------------------------------
void vtkPVApplication::CreateSplashScreen()
{
  unsigned char *buffer = new unsigned char [ image_PVSplashScreen_buffer_length ];
  memcpy(buffer, image_PVSplashScreen1, sizeof(image_PVSplashScreen1));
  memcpy(buffer+sizeof(image_PVSplashScreen1), image_PVSplashScreen2, 
    sizeof(image_PVSplashScreen2));
  this->CreatePhoto("PVSplashScreen", 
                    buffer, 
                    image_PVSplashScreen_width, 
                    image_PVSplashScreen_height,
                    image_PVSplashScreen_pixel_size,
                    image_PVSplashScreen_buffer_length);

  this->SplashScreen->Create(this, "");
  this->SplashScreen->SetProgressMessageVerticalOffset(-17);
  this->SplashScreen->SetImageName("PVSplashScreen");
  // this->SplashScreen->Show();
  delete [] buffer;
}

//----------------------------------------------------------------------------
void vtkPVApplication::AddAboutText(ostream &os)
{
  os << this->GetApplicationName() << " was developed by Kitware Inc." << endl
     << "http://www.paraview.org" << endl
     << "http://www.kitware.com" << endl
     << "This is version " << this->MajorVersion << "." << this->MinorVersion
     << ", release " << this->GetApplicationReleaseName() << endl;
}

//----------------------------------------------------------------------------
void vtkPVApplication::CreatePhoto(char *name, 
                                   unsigned char *data, 
                                   int width, int height, int pixel_size,
                                   unsigned long buffer_length,
                                   char *filename)
{
  struct stat fs;

  // Try to use the filename directly (provided that Tk will support 
  // this image format)

  if (filename)
    {
    if (stat(filename, &fs) != 0 || this->EvaluateBooleanExpression(
      "catch {image create photo %s -file {%s}}", name, filename))
      {
      vtkWarningMacro("Error creating photo from file " << filename);
      }
    return;
    }

  // Otherwise try to find a PNG file with the same name in the Resources dir

  char buffer[1024];
  sprintf(buffer, "%s/../ParaView/Resources/%s.png", 
          VTK_PV_SOURCE_CONFIG_DIR, name);

  if (stat(buffer, &fs) == 0)
    {
    vtkPNGReader *png_reader = vtkPNGReader::New();
    png_reader->SetFileName(buffer);
    if (!vtkKWTkUtilities::UpdatePhoto(this->GetMainInterp(),
                                       name, 
                                       png_reader->GetOutput()))
      {
      vtkWarningMacro("Error creating photo from file " << buffer);
      }
    png_reader->Delete();
    return;
    }

  // Otherwise use the provided data

  if (!vtkKWTkUtilities::UpdatePhoto(this->GetMainInterp(),
                                     name, 
                                     data, 
                                     width, height, pixel_size,
                                     buffer_length))
    {
    vtkWarningMacro("Error updating Tk photo " << name);
    }
}
