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

#include "vtkKWImageLabel.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWTkUtilities.h"

#include "vtkPNGReader.h"

#include <sys/stat.h>

#include "vtkPVSourceInterfaceDirectories.h"

// Buttons

#include "Resources/vtkKWFlyButton.h"
#include "Resources/vtkKWPickCenterButton.h"
#include "Resources/vtkKWResetViewButton.h"
#include "Resources/vtkKWRotateViewButton.h"
#include "Resources/vtkKWTranslateViewButton.h"
#include "Resources/vtkPV3DCursor.h"
#include "Resources/vtkPVCalculatorButton.h"
#include "Resources/vtkPVClipButton.h"
#include "Resources/vtkPVContourButton.h"
#include "Resources/vtkPVCutButton.h"
#include "Resources/vtkPVExtractGridButton.h"
#include "Resources/vtkPVGlyphButton.h"
#include "Resources/vtkPVProbeButton.h"
#include "Resources/vtkPVThresholdButton.h"
#include "Resources/vtkPVVectorDisplacementButton.h"

// Splash screen

#include "Resources/vtkPVSplashScreen.h"

//----------------------------------------------------------------------------
void vtkPVApplication::CreateButtonPhotos()
{
  this->CreatePhoto("KWResetViewButton", 
                    KW_RESET_VIEW_BUTTON, 
                    KW_RESET_VIEW_BUTTON_WIDTH, 
                    KW_RESET_VIEW_BUTTON_HEIGHT);

  this->CreatePhoto("KWTranslateViewButton", 
                    KW_TRANSLATE_VIEW_BUTTON, 
                    KW_TRANSLATE_VIEW_BUTTON_WIDTH, 
                    KW_TRANSLATE_VIEW_BUTTON_HEIGHT);

  this->CreatePhoto("KWActiveTranslateViewButton", 
                    KW_ACTIVE_TRANSLATE_VIEW_BUTTON, 
                    KW_ACTIVE_TRANSLATE_VIEW_BUTTON_WIDTH, 
                    KW_ACTIVE_TRANSLATE_VIEW_BUTTON_HEIGHT);

  this->CreatePhoto("KWFlyButton", 
                    KW_FLY_BUTTON, 
                    KW_FLY_BUTTON_WIDTH, 
                    KW_FLY_BUTTON_HEIGHT);

  this->CreatePhoto("KWActiveFlyButton", 
                    KW_ACTIVE_FLY_BUTTON, 
                    KW_ACTIVE_FLY_BUTTON_WIDTH, 
                    KW_ACTIVE_FLY_BUTTON_HEIGHT);

  this->CreatePhoto("KWRotateViewButton", 
                    KW_ROTATE_VIEW_BUTTON, 
                    KW_ROTATE_VIEW_BUTTON_WIDTH, 
                    KW_ROTATE_VIEW_BUTTON_HEIGHT);

  this->CreatePhoto("KWActiveRotateViewButton", 
                    KW_ACTIVE_ROTATE_VIEW_BUTTON, 
                    KW_ACTIVE_ROTATE_VIEW_BUTTON_WIDTH, 
                    KW_ACTIVE_ROTATE_VIEW_BUTTON_HEIGHT);

  this->CreatePhoto("KWPickCenterButton", 
                    KW_PICK_CENTER_BUTTON, 
                    KW_PICK_CENTER_BUTTON_WIDTH, 
                    KW_PICK_CENTER_BUTTON_HEIGHT);
  
  /* I'm using the same one here */

  this->CreatePhoto("KWResetCenterButton", 
                    KW_PICK_CENTER_BUTTON, 
                    KW_PICK_CENTER_BUTTON_WIDTH, 
                    KW_PICK_CENTER_BUTTON_HEIGHT);
  
  /* I'm using the same one here */

  this->CreatePhoto("KWShowCenterButton", 
                    KW_PICK_CENTER_BUTTON, 
                    KW_PICK_CENTER_BUTTON_WIDTH, 
                    KW_PICK_CENTER_BUTTON_HEIGHT);
  
  /* I'm using the same one here */

  this->CreatePhoto("KWHideCenterButton", 
                    KW_PICK_CENTER_BUTTON, 
                    KW_PICK_CENTER_BUTTON_WIDTH, 
                    KW_PICK_CENTER_BUTTON_HEIGHT);
  
  /* I'm using the same one here */

  this->CreatePhoto("KWEditCenterButtonOpen", 
                    KW_PICK_CENTER_BUTTON, 
                    KW_PICK_CENTER_BUTTON_WIDTH, 
                    KW_PICK_CENTER_BUTTON_HEIGHT);
  
  /* I'm using the same one here */

  this->CreatePhoto("KWEditCenterButtonClose", 
                    KW_PICK_CENTER_BUTTON, 
                    KW_PICK_CENTER_BUTTON_WIDTH, 
                    KW_PICK_CENTER_BUTTON_HEIGHT);
  
  this->CreatePhoto("PVCalculatorButton", 
                    PV_CALCULATOR_BUTTON,
                    PV_CALCULATOR_BUTTON_WIDTH, 
                    PV_CALCULATOR_BUTTON_HEIGHT);

  this->CreatePhoto("PVThresholdButton", 
                    PV_THRESHOLD_BUTTON,
                    PV_THRESHOLD_BUTTON_WIDTH, 
                    PV_THRESHOLD_BUTTON_HEIGHT);

  this->CreatePhoto("PVContourButton", 
                    PV_CONTOUR_BUTTON,
                    PV_CONTOUR_BUTTON_WIDTH, 
                    PV_CONTOUR_BUTTON_HEIGHT);

  this->CreatePhoto("PVProbeButton", 
                    PV_PROBE_BUTTON,
                    PV_PROBE_BUTTON_WIDTH, 
                    PV_PROBE_BUTTON_HEIGHT);

  this->CreatePhoto("PVGlyphButton", 
                    PV_GLYPH_BUTTON,
                    PV_GLYPH_BUTTON_WIDTH, 
                    PV_GLYPH_BUTTON_HEIGHT);

  this->CreatePhoto("PV3DCursorButton", 
                    PV_3D_CURSOR_BUTTON,
                    PV_3D_CURSOR_BUTTON_WIDTH, 
                    PV_3D_CURSOR_BUTTON_HEIGHT);

  this->CreatePhoto("PVActive3DCursorButton", 
                    PV_ACTIVE_3D_CURSOR_BUTTON,
                    PV_ACTIVE_3D_CURSOR_BUTTON_WIDTH, 
                    PV_ACTIVE_3D_CURSOR_BUTTON_HEIGHT);

  this->CreatePhoto("PVCutButton", 
                    PV_CUT_BUTTON,
                    PV_CUT_BUTTON_WIDTH, 
                    PV_CUT_BUTTON_HEIGHT);

  this->CreatePhoto("PVClipButton", 
                    PV_CLIP_BUTTON,
                    PV_CLIP_BUTTON_WIDTH, 
                    PV_CLIP_BUTTON_HEIGHT);

  this->CreatePhoto("PVExtractGridButton", 
                    PV_EXTRACT_GRID_BUTTON,
                    PV_EXTRACT_GRID_BUTTON_WIDTH, 
                    PV_EXTRACT_GRID_BUTTON_HEIGHT);

  this->CreatePhoto("PVVectorDisplacementButton", 
                    PV_VECTOR_DISPLACEMENT_BUTTON,
                    PV_VECTOR_DISPLACEMENT_BUTTON_WIDTH, 
                    PV_VECTOR_DISPLACEMENT_BUTTON_HEIGHT);

  /* Yes, I'm using the Vector displacement attributes because the .h
     has not been generated for that button, it will once everybody is
     happy with those new icons :)
  */
  this->CreatePhoto("PVStreamTracerButton", 
                    PV_VECTOR_DISPLACEMENT_BUTTON,
                    PV_VECTOR_DISPLACEMENT_BUTTON_WIDTH, 
                    PV_VECTOR_DISPLACEMENT_BUTTON_HEIGHT);
}

//----------------------------------------------------------------------------
void vtkPVApplication::CreateSplashScreen()
{
  this->SplashScreen->Create(this, "-bg #FFFFFF");
  vtkKWImageLabel *image = this->SplashScreen->GetImage();
  image->SetImageData(image_vtkPVSplashScreen,
                      image_vtkPVSplashScreen_width, 
                      image_vtkPVSplashScreen_height,
                      3);
  this->SplashScreen->Show();
}

//----------------------------------------------------------------------------
void vtkPVApplication::CreatePhoto(char *name, 
                                   unsigned char *data, 
                                   int width, int height,
                                   char *filename)
{
  // Try to use the filename if provided

  if (filename)
    {
    if (this->EvaluateBooleanExpression(
      "catch {image create photo %s -file {%s}}", name, filename))
      {
      vtkWarningMacro("Error creating photo from file " << filename);
      }
    return;
    }

  // Otherwise try to find a file with the same name in the Resources dir

  this->Script("image create photo %s", name);

  struct stat fs;
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
                                     name, data, width, height, 3))
    {
    vtkWarningMacro("Error updating Tk photo " << name);
    }
}
