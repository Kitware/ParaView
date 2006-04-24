/*=========================================================================

  Module:    vtkKWSimpleAnimationWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSimpleAnimationWidget.h"

#include "vtkKWApplication.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkKWComboBoxWithLabel.h"
#include "vtkKWComboBox.h"
#include "vtkKWIcon.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWLabelWithLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWPushButton.h"
#include "vtkKWPushButtonSet.h"
#include "vtkRenderer.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWScaleWithEntrySet.h"
#include "vtkMPEG2Writer.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkKWRenderWidget.h"
#include "vtkWindowToImageFilter.h"
#include "vtkKWWindowBase.h"
#include "vtkToolkits.h"
#include "vtkImageWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkTIFFWriter.h"
#include "vtkKWWidgetsBuildConfigure.h" // VTK_USE_VIDEO_FOR_WINDOWS

#ifdef VTK_USE_VIDEO_FOR_WINDOWS 
#include "vtkAVIWriter.h"
#else
#ifdef VTK_USE_FFMPEG_ENCODER
#include "vtkFFMPEGWriter.h"
#endif
#endif

#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/string>

//----------------------------------------------------------------------------

#define VTK_VV_ANIMATION_BUTTON_PREVIEW_ID 0
#define VTK_VV_ANIMATION_BUTTON_CREATE_ID  1
#define VTK_VV_ANIMATION_BUTTON_CANCEL_ID  2

#define VTK_VV_ANIMATION_SCALE_NB_OF_FRAMES_ID 0
#define VTK_VV_ANIMATION_SCALE_SLICE_START_ID  1
#define VTK_VV_ANIMATION_SCALE_SLICE_END_ID    2
#define VTK_VV_ANIMATION_SCALE_AZIMUTH_ID      3
#define VTK_VV_ANIMATION_SCALE_ELEVATION_ID    4
#define VTK_VV_ANIMATION_SCALE_ROLL_ID         5
#define VTK_VV_ANIMATION_SCALE_ZOOM_ID         6

#define VTK_VV_ANIMATION_SCALE_NB_FRAMES       500

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWSimpleAnimationWidget);
vtkCxxRevisionMacro(vtkKWSimpleAnimationWidget, "1.28");

//----------------------------------------------------------------------------
vtkKWSimpleAnimationWidget::vtkKWSimpleAnimationWidget()
{
  this->RenderWidget = NULL;

  this->AnimationType = vtkKWSimpleAnimationWidget::AnimationTypeCamera;

  this->AnimationStatus = vtkKWSimpleAnimationWidget::AnimationStopped;
  
  this->Parameters = NULL;
  this->AnimationButtonSet = NULL;
  this->HelpLabel = NULL;

  this->CameraPostAnimationCommand = NULL;
  this->SlicePostAnimationCommand  = NULL;
  this->SliceGetCommand            = NULL;
  this->SliceSetCommand            = NULL;
}

//----------------------------------------------------------------------------
vtkKWSimpleAnimationWidget::~vtkKWSimpleAnimationWidget()
{
  if (this->Parameters)
    {
    this->Parameters->Delete();
    this->Parameters = NULL;
    }

  if (this->AnimationButtonSet)
    {
    this->AnimationButtonSet->Delete();
    this->AnimationButtonSet = NULL;
    }
  
  if (this->HelpLabel)
    {
    this->HelpLabel->Delete();
    this->HelpLabel = NULL;
    }

  if (this->CameraPostAnimationCommand)
    {
    delete [] this->CameraPostAnimationCommand;
    this->CameraPostAnimationCommand = NULL;
    }

  if (this->SlicePostAnimationCommand)
    {
    delete [] this->SlicePostAnimationCommand;
    this->SlicePostAnimationCommand = NULL;
    }

  if (this->SliceGetCommand)
    {
    delete [] this->SliceGetCommand;
    this->SliceGetCommand = NULL;
    }

  if (this->SliceSetCommand)
    {
    delete [] this->SliceSetCommand;
    this->SliceSetCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::Create()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkKWSimpleAnimationWidget already created.");
    return;
    }
  
  // Call the superclass to create the whole widget

  this->Superclass::Create();
  
  ostrstream tk_cmd;

  vtkKWPushButton *pb;
  vtkKWScaleWithEntry *scale;

  int entry_width = 5;
  int label_width = 18;
  int i;

  // --------------------------------------------------------------
  // Parameters

  if (!this->Parameters)
    {
    this->Parameters = vtkKWScaleWithEntrySet::New();
    }

  this->Parameters->SetParent(this);
  this->Parameters->Create();
  this->Parameters->PackHorizontallyOff();
  this->Parameters->ExpandWidgetsOn();
  
  tk_cmd << "pack " << this->Parameters->GetWidgetName()
         << " -side top -anchor w -expand y -fill x" << endl;

  // Number of frames

  scale = this->Parameters->AddWidget(VTK_VV_ANIMATION_SCALE_NB_OF_FRAMES_ID);
  scale->SetLabelText(ks_("Animation|Number of frames:"));
  scale->SetResolution(1);
  scale->SetRange(1, VTK_VV_ANIMATION_SCALE_NB_FRAMES);
  scale->SetValue(20);
  scale->SetBalloonHelpString(
    k_("Specify the number of frames for this animation"));

  // 3D animation : Azimuth scale

  double rotate_max = 720.0;
  double res = 10.0;

  scale = this->Parameters->AddWidget(VTK_VV_ANIMATION_SCALE_AZIMUTH_ID);
  scale->SetResolution(res);
  scale->SetRange(-rotate_max, rotate_max);
  scale->SetValue(0.0);
  scale->SetLabelText(ks_("Animation|X rotation:"));
  scale->SetBalloonHelpString(
    k_("Set the total amount of rotation in X (in degrees)"));

  // 3D animation : Elevation scale

  scale = this->Parameters->AddWidget(VTK_VV_ANIMATION_SCALE_ELEVATION_ID);
  scale->SetResolution(res);
  scale->SetRange(-rotate_max, rotate_max);
  scale->SetLabelText(ks_("Animation|Y rotation:"));
  scale->SetBalloonHelpString(
    k_("Set the total amount of rotation in Y (in degrees)"));
  
  // 3D animation : Roll scale

  scale = this->Parameters->AddWidget(VTK_VV_ANIMATION_SCALE_ROLL_ID);
  scale->SetResolution(res);
  scale->SetRange(-rotate_max, rotate_max);
  scale->SetValue(0.0);
  scale->SetLabelText(ks_("Animation|Z Rotation:"));
  scale->SetBalloonHelpString(
    k_("Set the total amount of rotation in Z (in degrees)"));

  // 3D animation : Zoom scale

  scale = this->Parameters->AddWidget(VTK_VV_ANIMATION_SCALE_ZOOM_ID);
  scale->SetResolution(0.01);
  scale->SetRange(scale->GetResolution(), 10.0);
  scale->SetValue(1.0);
  scale->SetLabelText(ks_("Animation|Zoom factor:"));
  scale->SetBalloonHelpString(k_("Set the total zoom factor"));

  // 2D animation : Starting slice scale

  scale = this->Parameters->AddWidget(VTK_VV_ANIMATION_SCALE_SLICE_START_ID);
  scale->SetValue(0);
  scale->SetLabelText(ks_("Animation|Starting slice:"));
  scale->SetBalloonHelpString(
    k_("Set the slice number with which to begin the animation"));
  
  // 2D animation : Ending slice scale

  scale = this->Parameters->AddWidget(VTK_VV_ANIMATION_SCALE_SLICE_END_ID);
  scale->SetValue(0);
  scale->SetLabelText(ks_("Animation|Ending slice:"));
  scale->SetBalloonHelpString(
    k_("Set the slice number with which to end the animation"));

  for (i = 0; i < this->Parameters->GetNumberOfWidgets(); i++)
    {
    scale = this->Parameters->GetWidget(this->Parameters->GetIdOfNthWidget(i));
    if (scale)
      {
      scale->SetEntryWidth(entry_width);
      scale->SetLabelWidth(label_width);
      }
    }

  // --------------------------------------------------------------
  // Animation buttons: Preview, Create, and Cancel buttons

  if (!this->AnimationButtonSet)
    {
    this->AnimationButtonSet = vtkKWPushButtonSet::New();
    }

  this->AnimationButtonSet->SetParent(this);
  this->AnimationButtonSet->PackHorizontallyOn();
  this->AnimationButtonSet->SetWidgetsPadX(2);
  this->AnimationButtonSet->SetWidgetsPadY(2);
  this->AnimationButtonSet->Create();

  tk_cmd << "pack " << this->AnimationButtonSet->GetWidgetName()
         << " -side top -anchor w -expand y -fill x -pady 2" << endl;

  // Preview, Create, and Cancel buttons

  pb = this->AnimationButtonSet->AddWidget(VTK_VV_ANIMATION_BUTTON_PREVIEW_ID);
  pb->SetText(ks_("Animation|Button|Preview"));
  pb->SetCommand(this, "PreviewAnimationCallback");
  pb->SetBalloonHelpString(
    k_("Preview the animation you are about to create"));

  pb = this->AnimationButtonSet->AddWidget(VTK_VV_ANIMATION_BUTTON_CREATE_ID);
  pb->SetText(ks_("Animation|Button|Create..."));
  pb->SetCommand(this, "CreateAnimationCallback");
  pb->SetBalloonHelpString(k_("Create the animation"));

  pb = this->AnimationButtonSet->AddWidget(VTK_VV_ANIMATION_BUTTON_CANCEL_ID);
  pb->SetText(ks_("Animation|Button|Cancel"));
  pb->SetCommand(this, "CancelAnimationCallback");
  pb->SetBalloonHelpString(
    k_("Cancel the preview or creation of an animation"));

  // --------------------------------------------------------------
  // Label that is visible regardless of the animation type

  if (!this->HelpLabel)
    {
    this->HelpLabel = vtkKWLabelWithLabel::New();
    }

  this->HelpLabel->SetParent(this);
  this->HelpLabel->Create();
  this->HelpLabel->GetLabel()->SetImageToPredefinedIcon(
    vtkKWIcon::IconHelpBubble);
  this->HelpLabel->ExpandWidgetOn();
  this->HelpLabel->GetWidget()->AdjustWrapLengthToWidthOn();
  this->HelpLabel->GetWidget()->SetText(
    k_("Preview images will be generated using a low level-of-detail. When "
       "the animation is created, the best available level-of-detail will be "
       "used."));
  
  tk_cmd << "pack " << this->HelpLabel->GetWidgetName()
         << " -side top -anchor w -expand y -fill x" << endl;

  // --------------------------------------------------------------
  // Pack 

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // Update according to the current render widget

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::SetAnimationType(int val)
{
  if (val < vtkKWSimpleAnimationWidget::AnimationTypeCamera)
    {
    val = vtkKWSimpleAnimationWidget::AnimationTypeCamera;
    }
  if (val > vtkKWSimpleAnimationWidget::AnimationTypeSlice)
    {
    val = vtkKWSimpleAnimationWidget::AnimationTypeSlice;
    }

  if (this->AnimationType == val)
    {
    return;
    }

  this->AnimationType = val;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::Update()
{
  this->UpdateEnableState();

  if (!this->IsCreated())
    {
    return;
    }

  int is_cam = 
    this->AnimationType == vtkKWSimpleAnimationWidget::AnimationTypeCamera;
  int is_slice = 
    this->AnimationType == vtkKWSimpleAnimationWidget::AnimationTypeSlice;

  int is_complete = 
    (this->RenderWidget &&
     (is_cam || (is_slice &&
                 this->SliceSetCommand && *this->SliceSetCommand)));

  if (this->Parameters)
    {
    // Show or hide the rotation + zoom parameters

    this->Parameters->SetWidgetVisibility(
      VTK_VV_ANIMATION_SCALE_AZIMUTH_ID, is_cam);
    this->Parameters->SetWidgetVisibility(
      VTK_VV_ANIMATION_SCALE_ELEVATION_ID, is_cam);
    this->Parameters->SetWidgetVisibility(
      VTK_VV_ANIMATION_SCALE_ROLL_ID, is_cam);
    this->Parameters->SetWidgetVisibility(
      VTK_VV_ANIMATION_SCALE_ZOOM_ID, is_cam);

    // Show or hide the scale parameters

    this->Parameters->SetWidgetVisibility(
      VTK_VV_ANIMATION_SCALE_SLICE_START_ID, is_slice);
    this->Parameters->SetWidgetVisibility(
      VTK_VV_ANIMATION_SCALE_SLICE_END_ID, is_slice);

    // Update scale range
    
    if (is_slice)
      {
      vtkKWScaleWithEntry *scale_start = 
        this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_SLICE_START_ID);
      scale_start->SetEnabled(is_complete ? this->Parameters->GetEnabled():0);

      vtkKWScaleWithEntry *scale_end = 
        this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_SLICE_END_ID);
      scale_end->SetEnabled(is_complete ? this->Parameters->GetEnabled():0);
      }
    }

  if (this->AnimationButtonSet && !is_complete)
    {
    this->AnimationButtonSet->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::SetSliceRange(int min, int max)
{
  vtkKWScaleWithEntry *scale_start = 
    this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_SLICE_START_ID);
  if (scale_start)
    {
    scale_start->SetRange(min, max);
    int v = (int)scale_start->GetValue();
    if (v < min || v > max)
      {
      scale_start->SetValue(min);
      }
    }
  
  vtkKWScaleWithEntry *scale_end = 
    this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_SLICE_END_ID);
  if (scale_end)
    {
    scale_end->SetRange(min, max);
    int v = (int)scale_end->GetValue();
    if (v < min || v > max)
      {
      scale_end->SetValue(max);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::SetMaximumNumberOfFrames(int max)
{
  vtkKWScaleWithEntry *scale_nb_of_frames = 
    this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_NB_OF_FRAMES_ID);
  if (scale_nb_of_frames)
    {
    scale_nb_of_frames->SetRange(scale_nb_of_frames->GetRangeMin(), max);
    }
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::DisableButtonsButCancel()
{
  if (!this->IsCreated())
    {
    return;
    }

  // It seems the grab has no impact on the menubar, so try to disable
  // it manually

  vtkKWTopLevel *win = this->GetParentTopLevel();
  if (win)
    {
    win->GetMenu()->SetEnabled(0);
    }

  // Disable "Create" and "Preview"

  this->AnimationButtonSet
    ->GetWidget(VTK_VV_ANIMATION_BUTTON_PREVIEW_ID)->SetEnabled(0);
  this->AnimationButtonSet
    ->GetWidget(VTK_VV_ANIMATION_BUTTON_CREATE_ID)->SetEnabled(0);
  this->AnimationButtonSet
    ->GetWidget(VTK_VV_ANIMATION_BUTTON_CANCEL_ID)->SetEnabled(
      this->GetEnabled());
  this->AnimationButtonSet
    ->GetWidget(VTK_VV_ANIMATION_BUTTON_CANCEL_ID)->Grab();
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::EnableButtonsButCancel()
{
  if (!this->IsCreated())
    {
    return;
    }

  // It seems the grab has no impact on the menubar, so try to re-enable
  // it since we disabled it manually in DisableButtonsButCancel

  vtkKWWindowBase *win = vtkKWWindowBase::SafeDownCast(
    this->GetParentTopLevel());
  if (win)
    {
    win->UpdateMenuState();
    }

  // Enable "Create" and "Preview"

  this->AnimationButtonSet
    ->GetWidget(VTK_VV_ANIMATION_BUTTON_PREVIEW_ID)->SetEnabled(
      this->GetEnabled());
  this->AnimationButtonSet
    ->GetWidget(VTK_VV_ANIMATION_BUTTON_CREATE_ID)->SetEnabled(
      this->GetEnabled());
  this->AnimationButtonSet
    ->GetWidget(VTK_VV_ANIMATION_BUTTON_CANCEL_ID)->SetEnabled(0);
  this->AnimationButtonSet
    ->GetWidget(VTK_VV_ANIMATION_BUTTON_CANCEL_ID)->ReleaseGrab();
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::PreviewAnimationCallback()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Disable buttons but preview

  this->DisableButtonsButCancel();

  // Run preview

  if (this->AnimationType == 
      vtkKWSimpleAnimationWidget::AnimationTypeCamera)
    {
    this->PreviewCameraAnimation();
    }
  else if (this->AnimationType == 
           vtkKWSimpleAnimationWidget::AnimationTypeSlice)
    {
    this->PreviewSliceAnimation();
    }

  // Reenable buttons

  this->EnableButtonsButCancel();
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::CreateAnimationCallback()
{
  if (!this->IsCreated())
    {
    return;
    }

  int res;
  vtksys_stl::string filename;

  vtkKWLoadSaveDialog *save_dialog = vtkKWLoadSaveDialog::New();
  save_dialog->SetParent(this->GetParentTopLevel());
  save_dialog->RetrieveLastPathFromRegistry("SavePath");
  save_dialog->Create();
  save_dialog->SetTitle(
    ks_("Animation|Save Animation Dialog|Title|Save Animation"));
  save_dialog->SaveDialogOn();
#if defined(VTK_USE_VIDEO_FOR_WINDOWS) || defined(VTK_USE_FFMPEG_ENCODER)
  save_dialog->SetFileTypes("{{AVI} {.avi}} {{MPEG2} {.mpg}} {{JPEG} {.jpg}} {{TIFF} {.tif}}");
  save_dialog->SetDefaultExtension(".avi");
#else
  save_dialog->SetFileTypes("{{MPEG2} {.mpg}} {{JPEG} {.jpg}} {{TIFF} {.tif}}");
  save_dialog->SetDefaultExtension(".mpg");
#endif

  res = save_dialog->Invoke();
  if (res)
    {
    filename = save_dialog->GetFileName();
    save_dialog->SaveLastPathToRegistry("SavePath");
    }

  save_dialog->Delete();
  if (!res)
    {
    return;
    }

  // Split into root and extension.

  vtksys_stl::string ext = 
    vtksys::SystemTools::GetFilenameLastExtension(filename.c_str());

  vtksys_stl::string filename_path = 
    vtksys::SystemTools::GetFilenamePath(filename.c_str());

  vtksys_stl::string file_root(filename_path);
  file_root += '/';
  file_root +=
    vtksys::SystemTools::GetFilenameWithoutLastExtension(filename.c_str());

  if (!ext.size())
    {
    vtkErrorMacro(<< "Could not find extension in " << filename.c_str());
    return;
    }

  int orig_width = this->RenderWidget->GetRenderWindow()->GetSize()[0];
  int orig_height = this->RenderWidget->GetRenderWindow()->GetSize()[1];

  int width, height;

  // Is this a video format

  int is_mpeg = 
    (!strcmp(ext.c_str(), ".mpg") || !strcmp(ext.c_str(), ".mpeg") ||
     !strcmp(ext.c_str(), ".MPG") || !strcmp(ext.c_str(), ".MPEG") ||
     !strcmp(ext.c_str(), ".MP2") || !strcmp(ext.c_str(), ".mp2"));
  int is_avi = 
    (!strcmp(ext.c_str(), ".avi") || !strcmp(ext.c_str(), ".AVI"));

  // Prompt for the size of the movie
    
  vtkKWMessageDialog *msg_dialog = vtkKWMessageDialog::New();
  msg_dialog->SetMasterWindow(this->GetParentTopLevel());
  msg_dialog->SetTitle(
    ks_("Animation|Create Animation Dialog|Title|Frame Size"));
  msg_dialog->SetStyleToOkCancel();
  msg_dialog->Create();

  vtksys_stl::string msg(
      k_("Specify the width and height of each frame to be saved from this "
         "animation."));
  
  if (is_mpeg)
    {
    msg += " ";
    msg += 
      k_("The width must be a multiple of 32 and the height a "
         "multiple of 8. Each will be resized to the next smallest multiple "
         "if it does not meet this criterion. The maximum size allowed is "
         "1920 by 1080.");
    }
  else if (is_avi)
    { 
    msg += " ";
    msg += 
      k_("Each dimension must be a multiple of 4. Each will be "
         "resized to the next smallest multiple of 4 if it does not meet this "
         "criterion.");
    }

  msg_dialog->SetText(msg.c_str());
  
  vtkKWFrame *frame = vtkKWFrame::New();
  frame->SetParent(msg_dialog->GetTopFrame());
  frame->Create();

  char buffer[1024];
  int nb_scanned = 0, key_w, key_h;

  vtkKWApplication *app = this->GetApplication();

  if (app->HasRegistryValue(2, "RunTime", "SimpleAnimationSize") &&
      app->GetRegistryValue(2, "RunTime", "SimpleAnimationSize", buffer))
    {
    nb_scanned = sscanf(buffer, "%dx%d", &key_w, &key_h);
    }

  vtkKWComboBox *combobox;

  vtkKWComboBoxWithLabel *width_combobox = vtkKWComboBoxWithLabel::New();
  width_combobox->SetParent(frame);
  width_combobox->Create();
  width_combobox->SetLabelText(
    ks_("Animation|Create Animation Dialog|Frame Size|Width:"));

  combobox = width_combobox->GetWidget();
  combobox->SetValueAsInt(orig_width);
  if (nb_scanned == 2)
    {
    combobox->AddValueAsInt(key_w);
    }
  combobox->AddValueAsInt(640);
  combobox->AddValueAsInt(800);
  combobox->AddValueAsInt(1024);
  combobox->AddValueAsInt(1280);
  combobox->AddValueAsInt(1680);
    
  vtkKWComboBoxWithLabel *height_combobox = vtkKWComboBoxWithLabel::New();
  height_combobox->SetParent(frame);
  height_combobox->Create();
  height_combobox->SetLabelText(
    ks_("Animation|Create Animation Dialog|Frame Size|Height:"));

  combobox = height_combobox->GetWidget();
  combobox->SetValueAsInt(orig_height);
  if (nb_scanned == 2)
    {
    combobox->AddValueAsInt(key_h);
    }
  combobox->AddValueAsInt(600);
  combobox->AddValueAsInt(768);
  combobox->AddValueAsInt(800);
  combobox->AddValueAsInt(1024);
  combobox->AddValueAsInt(1050);

  this->Script("pack %s %s -side left -fill both -expand t",
               width_combobox->GetWidgetName(), 
               height_combobox->GetWidgetName());
    
  this->Script("pack %s -side top -pady 5", frame->GetWidgetName());
    
  res = msg_dialog->Invoke();
    
  width = width_combobox->GetWidget()->GetValueAsInt();
  height = height_combobox->GetWidget()->GetValueAsInt();

  width_combobox->Delete();
  height_combobox->Delete();
  frame->Delete();
  msg_dialog->Delete();

  if (!res)
    {
    return;
    }

  app->SetRegistryValue(
    2, "RunTime", "SimpleAnimationSize", "%dx%d", width, height);

  // Fix the size

  if (is_mpeg)
    {
    if ((width % 32) > 0)
      {
      width -= width % 32;
      }
    if ((height % 8) > 0)
      {
      height -= height % 8;
      }
    if (width > 1920)
      {
      width = 1920;
      }
    if (height > 1080)
      {
      height = 1080;
      }      
    }
  else if (is_avi)
    {
    if ((width % 4) > 0)
      {
      width -= width % 4;
      }
    if ((height % 4) > 0)
      {
      height -= height % 4;
      }
    }
  
  // Disable buttons but preview

  this->DisableButtonsButCancel();

  // Create the animation

  if (this->AnimationType == 
      vtkKWSimpleAnimationWidget::AnimationTypeCamera)
    {
    this->CreateCameraAnimation(file_root.c_str(), ext.c_str(), width, height);
    }
  else if (this->AnimationType == 
           vtkKWSimpleAnimationWidget::AnimationTypeSlice)
    {
    this->CreateSliceAnimation(file_root.c_str(), ext.c_str(), width, height);
    }
  
  // Reenable buttons

  this->EnableButtonsButCancel();
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::CancelAnimationCallback()
{
  this->AnimationStatus = vtkKWSimpleAnimationWidget::AnimationCanceled;
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::PreviewCameraAnimation()
{
  this->PerformCameraAnimation(NULL, NULL, -1, -1);
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::CreateCameraAnimation(const char *file_root,
                                              const char *ext,
                                              int width, int height)
{
  this->PerformCameraAnimation(file_root, ext, width, height);
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::PerformCameraAnimation(const char *file_root,
                                                        const char *ext,
                                                        int width, int height)
{
  if (!this->IsCreated() || !this->RenderWidget)
    {
    return;
    }

  int previewing = !file_root;
  vtkKWWindowBase *win = vtkKWWindowBase::SafeDownCast(
    this->GetParentTopLevel());

  int old_render_mode = 0, old_size[2], status;

  vtkWindowToImageFilter *w2i = NULL;
  vtkGenericMovieWriter *movie_writer = NULL;
  vtkImageWriter *image_writer = 0;
  char *image_filename = 0;

  if (previewing)
    {
    old_render_mode = this->RenderWidget->GetRenderMode();
    this->RenderWidget->SetRenderModeToInteractive();
    if (win)
      {
      win->SetStatusText(ks_("Progress|Previewing animation"));
      }
    status = vtkKWSimpleAnimationWidget::AnimationPreviewing;
    }
  else
    {
    if (ext)
      {
      if (!strcmp(ext, ".mpg"))
        {
        movie_writer = vtkMPEG2Writer::New();
        }
#ifdef VTK_USE_VIDEO_FOR_WINDOWS 
      else if (!strcmp(ext, ".avi"))
        {
        movie_writer = vtkAVIWriter::New();
        }
#else
#ifdef VTK_USE_FFMPEG_ENCODER
      else if (!strcmp(ext, ".avi"))
        {
        movie_writer = vtkFFMPEGWriter::New();
        }
#endif
#endif
      else if (!strcmp(ext, ".jpg"))
        {
        image_writer = vtkJPEGWriter::New();
        }
      else if (!strcmp(ext, ".tif"))
        {
        image_writer = vtkTIFFWriter::New();
        }
      }

    this->RenderWidget->OffScreenRenderingOn();
    old_size[0] = this->RenderWidget->GetRenderWindow()->GetSize()[0];
    old_size[1] = this->RenderWidget->GetRenderWindow()->GetSize()[1];
    if (width > 0)
      {
      this->RenderWidget->GetRenderWindow()->SetSize(width, height);
      }
    if (win)
      {
      win->SetStatusText(
        ks_("Progress|Generating animation (rendering to memory; please wait)"));
      }
    status = vtkKWSimpleAnimationWidget::AnimationCreating;

    w2i = vtkWindowToImageFilter::New();
    w2i->SetInput(this->RenderWidget->GetRenderWindow());
    if(movie_writer)
      {
      movie_writer->SetInput(w2i->GetOutput());
      vtksys_stl::string filename(file_root);
      filename += ext;
      movie_writer->SetFileName(filename.c_str());
      movie_writer->Start();
      }
    else if(image_writer)
      {
      image_writer->SetInput(w2i->GetOutput());
      image_filename = new char[strlen(file_root) + strlen(ext) + 25];
      }
    }

  this->AnimationStatus = status;

  // Save the camera state

  double pos[3], view_up[3], angle, parallel_scale;

  vtkCamera *cam = this->RenderWidget->GetRenderer()->GetActiveCamera();
  cam->GetPosition(pos);
  cam->GetViewUp(view_up);
  angle = cam->GetViewAngle();
  parallel_scale = cam->GetParallelScale();

  // Get the animation parameters

  vtkKWScaleWithEntry *scale;

  scale = this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_NB_OF_FRAMES_ID);
  int num_frames = (int)scale->GetValue();
  
  scale = this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_AZIMUTH_ID);
  double azimuth = scale->GetValue() / (double)num_frames;

  scale = this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_ELEVATION_ID);
  double elev = scale->GetValue() / (double)num_frames;

  scale = this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_ROLL_ID);
  double roll = scale->GetValue() / (double)num_frames;

  scale = this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_ZOOM_ID);
  double zoom = pow(scale->GetValue(), (double)1.0 / (double)num_frames);

  // Perform the animation

  if (!movie_writer || movie_writer->GetError() == 0)
    {
    for (int i = 0; 
         i < num_frames && 
           this->AnimationStatus != 
           vtkKWSimpleAnimationWidget::AnimationCanceled; i++)
      {
      if (win)
        {
        win->GetProgressGauge()->SetValue((int)(100.0 * i / num_frames));
        }
      // process pending events... necessary for being able to interrupt
      this->GetApplication()->ProcessPendingEvents();
      cam->Azimuth(azimuth);
      cam->Elevation(elev);
      cam->Roll(roll);
      cam->Zoom(zoom);
      cam->OrthogonalizeViewUp();
      this->RenderWidget->Render();
      if (w2i)
        {
        w2i->Modified();
        if (movie_writer)
          {
          movie_writer->Write();
          }
        else if(image_writer)
          {
          sprintf(image_filename, "%s.%04d%s", file_root, i, ext);
          image_writer->SetFileName(image_filename);
          image_writer->Write();
          }
        }
      }

    if (movie_writer)
      {
      movie_writer->End();
      movie_writer->SetInput(0);
      }
    }

  // Update status

  if (win)
    {
    vtksys_stl::string end_msg(win->GetStatusText());
    end_msg += " -- ";
    if (this->AnimationStatus != status)
      {
      end_msg += ks_("Progress|Canceled");
      }
    else
      {
      end_msg += ks_("Progress|Done");
      }
    win->SetStatusText(end_msg.c_str());
    win->GetProgressGauge()->SetValue(0);
    }
  
  this->AnimationStatus = vtkKWSimpleAnimationWidget::AnimationStopped;

  // Restore camera state

  cam->SetPosition(pos);
  cam->SetViewUp(view_up);
  cam->SetViewAngle(angle);
  cam->SetParallelScale(parallel_scale);

  // Switch back to the previous render mode / widget state

  if (previewing)
    {
    this->RenderWidget->SetRenderMode(old_render_mode);
    }
  else
    {
    this->RenderWidget->GetRenderWindow()->SetSize(old_size);
    this->RenderWidget->OffScreenRenderingOff();
    }

  this->InvokeCameraPostAnimationCommand();

  this->RenderWidget->Render();

  // Cleanup

  if (w2i)
    {
    w2i->Delete();
    }
  if (movie_writer)
    {
    movie_writer->Delete();
    }
  if (image_writer)
    {
    delete [] image_filename;
    image_writer->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::PreviewSliceAnimation()
{
  this->PerformSliceAnimation( NULL, NULL, -1, -1);
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::CreateSliceAnimation(const char *file_root,
                                                      const char *ext,
                                                      int width, int height)
{
  this->PerformSliceAnimation(file_root, ext, width, height);
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::PerformSliceAnimation(const char *file_root,
                                                       const char *ext,
                                                       int width, int height)
{
  if (!this->IsCreated() || !this->RenderWidget)
    {
    return;
    }

  int previewing = !file_root;
  vtkKWWindowBase *win = vtkKWWindowBase::SafeDownCast(
    this->GetParentTopLevel());
  
  int slice = this->InvokeSliceGetCommand();
  int old_size[2], status;

  vtkWindowToImageFilter *w2i = NULL;
  vtkGenericMovieWriter *movie_writer = 0;
  vtkImageWriter *image_writer = 0;
  char *image_filename = 0;

  if (previewing)
    {
    if (win)
      {
      win->SetStatusText(ks_("Progress|Previewing animation"));
      }
    status = vtkKWSimpleAnimationWidget::AnimationPreviewing;
    }
  else
    {
    if (ext)
      {
      if (!strcmp(ext, ".mpg"))
        {
        movie_writer = vtkMPEG2Writer::New();
        }
#ifdef VTK_USE_VIDEO_FOR_WINDOWS 
      else if (!strcmp(ext, ".avi"))
        {
        movie_writer = vtkAVIWriter::New();
        }
#else
#ifdef VTK_USE_FFMPEG_ENCODER
      else if (!strcmp(ext, ".avi"))
        {
        movie_writer = vtkFFMPEGWriter::New();
        }
#endif
#endif
      else if (!strcmp(ext, ".jpg"))
        {
        image_writer = vtkJPEGWriter::New();
        }
      else if (!strcmp(ext, ".tif"))
        {
        image_writer = vtkTIFFWriter::New();
        }
      }

    this->RenderWidget->OffScreenRenderingOn();
    old_size[0] = this->RenderWidget->GetRenderWindow()->GetSize()[0];
    old_size[1] = this->RenderWidget->GetRenderWindow()->GetSize()[1];
    this->RenderWidget->GetRenderWindow()->SetSize(width, height);
    if (width > 0)
      {
      this->RenderWidget->GetRenderWindow()->SetSize(width, height);
      }
    if (win)
      {
      win->SetStatusText(
        ks_("Progress|Generating animation (rendering to memory; please wait)"));
      }
    status = vtkKWSimpleAnimationWidget::AnimationCreating;

    w2i = vtkWindowToImageFilter::New();
    w2i->SetInput(this->RenderWidget->GetRenderWindow());
    if(movie_writer)
      {
      movie_writer->SetInput(w2i->GetOutput());
      vtksys_stl::string filename(file_root);
      filename += ext;
      movie_writer->SetFileName(filename.c_str());
      movie_writer->Start();
      }
    else if(image_writer)
      {
      image_writer->SetInput(w2i->GetOutput());
      image_filename = new char[strlen(file_root) + strlen(ext) + 25];
      }
    }

  this->AnimationStatus = status;

  // Save the camera state

  double pos[3], fp[3], parallel_scale;

  vtkCamera *cam = this->RenderWidget->GetRenderer()->GetActiveCamera();
  cam->GetPosition(pos);
  cam->GetFocalPoint(fp);
  parallel_scale = cam->GetParallelScale();

  // Get the animation parameters

  vtkKWScaleWithEntry *scale;

  scale = this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_NB_OF_FRAMES_ID);
  int num_frames = (int)scale->GetValue();

  scale = this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_SLICE_START_ID);
  int min = (int)scale->GetValue();

  scale = this->Parameters->GetWidget(VTK_VV_ANIMATION_SCALE_SLICE_END_ID);
  int max = (int)scale->GetValue();
  
  int dir = (min < max) ? 1 : -1;
  double inc = dir * (abs(max - min) + 1)/ (double)(num_frames - 1);

  // Perform the animation

  if (!movie_writer || movie_writer->GetError() == 0)
    {
    //this->RenderWidget->Reset();
    for (int i = 0; 
         i < num_frames && 
           this->AnimationStatus != 
           vtkKWSimpleAnimationWidget::AnimationCanceled; i++)
      {
      if (win)
        {
        win->GetProgressGauge()->SetValue((int)(100.0 * i / num_frames));
        }
      // process pending events... necessary for being able to interrupt
      this->GetApplication()->ProcessPendingEvents();
      int slice_num = (int)(min + inc * i);
      if ((slice_num > max && dir > 0) || (slice_num < max && dir < 0))
        {
        slice_num = max;
        }
      this->InvokeSliceSetCommand(slice_num);
      if (w2i)
        {
        w2i->Modified();
        if (movie_writer)
          {
          movie_writer->Write();
          }
        else if(image_writer)
          {
          sprintf(image_filename, "%s.%04d%s", file_root, i, ext);
          image_writer->SetFileName(image_filename);
          image_writer->Write();
          }
        }
      }

    if (movie_writer)
      {
      movie_writer->End();
      movie_writer->SetInput(0);
      }
    }

  // Update status

  if (win)
    {
    vtksys_stl::string end_msg(win->GetStatusText());
    end_msg += " -- ";
    if (this->AnimationStatus != status)
      {
      end_msg += ks_("Progress|Canceled");
      }
    else
      {
      end_msg += ks_("Progress|Done");
      }
    win->SetStatusText(end_msg.c_str());
    win->GetProgressGauge()->SetValue(0);
    }
  
  this->AnimationStatus = vtkKWSimpleAnimationWidget::AnimationStopped;

  // Restore camera state

  cam->SetPosition(pos);
  cam->SetParallelScale(parallel_scale);
  cam->SetFocalPoint(fp);

  // Switch back to the previous render mode / widget state

  if (!previewing)
    {
    this->RenderWidget->GetRenderWindow()->SetSize(old_size);
    this->RenderWidget->OffScreenRenderingOff();
    }

  this->InvokeSliceSetCommand(slice);

  this->InvokeSlicePostAnimationCommand();

  this->RenderWidget->Render();

  // Cleanup

  if (w2i)
    {
    w2i->Delete();
    }
  if (movie_writer)
    {
    movie_writer->Delete();
    }
  if (image_writer)
    {
    delete [] image_filename;
    image_writer->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::SetRenderWidget(vtkKWRenderWidget *arg)
{
  if (this->RenderWidget == arg)
    {
    return;
    }
  this->RenderWidget = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::SetCameraPostAnimationCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->CameraPostAnimationCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::InvokeCameraPostAnimationCommand()
{
  this->InvokeObjectMethodCommand(this->CameraPostAnimationCommand);
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::SetSlicePostAnimationCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->SlicePostAnimationCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::InvokeSlicePostAnimationCommand()
{
  this->InvokeObjectMethodCommand(this->SlicePostAnimationCommand);
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::SetSliceGetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->SliceGetCommand, object, method);
  this->Update();
}

//----------------------------------------------------------------------------
int vtkKWSimpleAnimationWidget::InvokeSliceGetCommand()
{
  if (this->SliceGetCommand && *this->SliceGetCommand && 
      this->GetApplication())
    {
    return atoi(this->Script(this->SliceGetCommand));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::SetSliceSetCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->SliceSetCommand, object, method);
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::InvokeSliceSetCommand(int slice)
{
  if (this->SliceSetCommand && *this->SliceSetCommand && 
      this->GetApplication())
    {
    this->Script("%s %d", this->SliceSetCommand, slice);
    }
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Parameters);
  this->PropagateEnableState(this->HelpLabel);
  this->PropagateEnableState(this->AnimationButtonSet);
  
  if (this->AnimationButtonSet &&
      !(this->AnimationStatus & vtkKWSimpleAnimationWidget::AnimationPlaying))
    {
    this->AnimationButtonSet
      ->GetWidget(VTK_VV_ANIMATION_BUTTON_CANCEL_ID)->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWSimpleAnimationWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "RenderWidget: " << this->RenderWidget << endl;

  if (this->AnimationType == vtkKWSimpleAnimationWidget::AnimationTypeCamera)
    {
    os << indent << "AnimationType: Camera\n";
    }
  else
    {
    os << indent << "AnimationType: Slice\n";
    }
}
