/*=========================================================================

  Module:    vtkKWHSVColorSelector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWHSVColorSelector.h"

#include "vtkKWApplication.h"
#include "vtkKWCanvas.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWTkUtilities.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkKWHSVColorSelector, "1.4");
vtkStandardNewMacro(vtkKWHSVColorSelector);

#define VTK_KW_HSV_SEL_POINT_RADIUS_MIN     2

#define VTK_KW_HSV_SEL_HS_WHEEL_RADIUS_MIN   20
#define VTK_KW_HSV_SEL_VALUE_BOX_WIDTH_MIN  10

#define VTK_KW_HSV_SEL_HS_CURSOR_RADIUS_MIN 2
#define VTK_KW_HSV_SEL_V_CURSOR_MARGIN_MIN  0

#define VTK_KW_HSV_SEL_IMAGE_TAG            "image"
#define VTK_KW_HSV_SEL_SELECTION_TAG        "selection"

#define VTK_KW_HSV_SEL_DISABLED_OPACITY     0.25

//----------------------------------------------------------------------------
vtkKWHSVColorSelector::vtkKWHSVColorSelector()
{
  this->HueSatWheelRadius         = 64;
  this->ValueBoxWidth            = 20;
  this->HueSatCursorRadius       = 4;
  this->ValueCursorMargin        = 2;
  this->Selected                 = 0;
  this->ModificationOnly         = 0;
  this->HideValue                = 0;

  this->SelectedColor[0]         = 0.0;
  this->SelectedColor[1]         = 0.0;
  this->SelectedColor[2]         = 0.5;

  this->SelectionChangedCommand  = NULL;
  this->SelectionChangingCommand = NULL;

  this->HueSatWheelCanvas         = vtkKWCanvas::New();
  this->ValueBoxCanvas           = vtkKWCanvas::New();
  this->HueSatLabel              = vtkKWLabel::New();
  this->ValueLabel               = vtkKWLabel::New();
}

//----------------------------------------------------------------------------
vtkKWHSVColorSelector::~vtkKWHSVColorSelector()
{
  // Commands

  if (this->SelectionChangedCommand)
    {
    delete [] this->SelectionChangedCommand;
    this->SelectionChangedCommand = NULL;
    }

  if (this->SelectionChangingCommand)
    {
    delete [] this->SelectionChangingCommand;
    this->SelectionChangingCommand = NULL;
    }

  // GUI

  if (this->HueSatWheelCanvas)
    {
    this->HueSatWheelCanvas->Delete();
    this->HueSatWheelCanvas = NULL;
    }

  if (this->ValueBoxCanvas)
    {
    this->ValueBoxCanvas->Delete();
    this->ValueBoxCanvas = NULL;
    }

  if (this->HueSatLabel)
    {
    this->HueSatLabel->Delete();
    this->HueSatLabel = NULL;
    }

  if (this->ValueLabel)
    {
    this->ValueLabel->Delete();
    this->ValueLabel = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::Create(vtkKWApplication *app, 
                                   const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(
        app, "frame", "-relief flat -bd 0 -highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ConfigureOptions(args);

  // Create the Hue/Sat canvas

  const char *canv_options = 
    "-height 0 -width 0 -highlightthickness 0 -relief solid -bd 0";

  this->HueSatWheelCanvas->SetParent(this);
  this->HueSatWheelCanvas->Create(app, canv_options);

  // Create the Value canvas

  this->ValueBoxCanvas->SetParent(this);
  this->ValueBoxCanvas->Create(app, canv_options);

  // Create the Hue/Sat label

  this->HueSatLabel->SetParent(this);
  this->HueSatLabel->Create(app, "-bd 0");
  this->HueSatLabel->SetText("Hue/Saturation:");

  // Create the Value label

  this->ValueLabel->SetParent(this);
  this->ValueLabel->Create(app, "-bd 0");
  this->ValueLabel->SetText("Value:");

  // Set the bindings

  this->Bind();

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::Update()
{
  this->UpdateEnableState();

  this->Redraw();

  // No selection, disable Value box

  if (!this->HasSelection())
    {
    if (this->ValueBoxCanvas)
      {
      this->ValueBoxCanvas->SetEnabled(0);
      }
    if (this->ValueLabel)
      {
      this->ValueLabel->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->HueSatWheelCanvas->UnpackSiblings();

  // Repack everything

  int row = 0;
  const char *col0 = " -column 1 ";
  const char *col1 = " -column 0 ";

  ostrstream tk_cmd;

  // Hue/Sat label

  tk_cmd << "grid " << this->HueSatLabel->GetWidgetName() 
         << " -sticky ew -row " << row << col0 << endl;
  
  // Value label

  if (!this->HideValue)
    {
    tk_cmd << "grid " << this->ValueLabel->GetWidgetName() 
           << " -sticky ew -row " << row << col1 << endl;
    }
  
  row++;

  // Hue/Sat Box Canvas

  tk_cmd << "grid " << this->HueSatWheelCanvas->GetWidgetName() 
         << " -sticky ew -row " << row << col0 << endl;

  // Value Box Canvas

  if (!this->HideValue)
    {
    tk_cmd << "grid " << this->ValueBoxCanvas->GetWidgetName() 
           << " -sticky ew -row " << row << col1 << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::Bind()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  // Hue/Sat Box Canvas

  if (this->HueSatWheelCanvas && this->HueSatWheelCanvas->IsAlive())
    {
    const char *canv = this->HueSatWheelCanvas->GetWidgetName();

    tk_cmd << "bind " << canv
           << " <ButtonPress-1> {" << this->GetTclName() 
           << " HueSatPickCallback %%x %%y}" << endl;

    tk_cmd << "bind " << canv
           << " <B1-Motion> {" << this->GetTclName() 
           << " HueSatMoveCallback %%x %%y}" << endl;

    tk_cmd << "bind " << canv
           << " <ButtonRelease-1> {" << this->GetTclName() 
           << " HueSatReleaseCallback}" << endl;
    }

  if (this->ValueBoxCanvas && this->ValueBoxCanvas->IsAlive())
    {
    const char *canv = this->ValueBoxCanvas->GetWidgetName();

    tk_cmd << "bind " << canv
           << " <ButtonPress-1> {" << this->GetTclName() 
           << " ValuePickCallback %%x %%y}" << endl;

    tk_cmd << "bind " << canv
           << " <B1-Motion> {" << this->GetTclName() 
           << " ValueMoveCallback %%x %%y}" << endl;

    tk_cmd << "bind " << canv
           << " <ButtonRelease-1> {" << this->GetTclName() 
           << " ValueReleaseCallback}" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::UnBind()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  // Hue/Sat Box Canvas

  if (this->HueSatWheelCanvas && this->HueSatWheelCanvas->IsAlive())
    {
    const char *canv = this->HueSatWheelCanvas->GetWidgetName();

    tk_cmd << "bind " << canv
           << " <ButtonPress-1> {}" << endl;

    tk_cmd << "bind " << canv
           << " <B1-Motion> {}" << endl;

    tk_cmd << "bind " << canv
           << " <ButtonRelease-1> {}" << endl;
    }

  if (this->ValueBoxCanvas && this->ValueBoxCanvas->IsAlive())
    {
    const char *canv = this->ValueBoxCanvas->GetWidgetName();

    tk_cmd << "bind " << canv
           << " <ButtonPress-1> {}" << endl;

    tk_cmd << "bind " << canv
           << " <B1-Motion> {}" << endl;

    tk_cmd << "bind " << canv
           << " <ButtonRelease-1> {}" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetHueSatWheelRadius(int arg)
{
  if (this->HueSatWheelRadius == arg || 
      arg < VTK_KW_HSV_SEL_HS_WHEEL_RADIUS_MIN)
    {
    return;
    }

  this->HueSatWheelRadius = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetValueBoxWidth(int arg)
{
  if (this->ValueBoxWidth == arg || 
      arg < VTK_KW_HSV_SEL_VALUE_BOX_WIDTH_MIN)
    {
    return;
    }

  this->ValueBoxWidth = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetHueSatCursorRadius(int arg)
{
  if (this->HueSatCursorRadius == arg || 
      arg < VTK_KW_HSV_SEL_HS_CURSOR_RADIUS_MIN)
    {
    return;
    }

  this->HueSatCursorRadius = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetValueCursorMargin(int arg)
{
  if (this->ValueCursorMargin == arg || 
      arg < VTK_KW_HSV_SEL_V_CURSOR_MARGIN_MIN)
    {
    return;
    }

  this->ValueCursorMargin = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetHideValue(int arg)
{
  if (this->HideValue == arg)
    {
    return;
    }

  this->HideValue = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetSelectedColor(
  double h, double s, double v)
{
  if ((h < 0.0 || h > 1.0 ||
       s < 0.0 || s > 1.0 ||
       v < 0.0 || v > 1.0) ||
      (this->Selected && 
       (h == this->SelectedColor[0] &&
        s == this->SelectedColor[1] &&
        v == this->SelectedColor[2])))
    {
    return;
    }

  this->SelectedColor[0] = h;
  this->SelectedColor[1] = s;
  this->SelectedColor[2] = v;

  this->Selected = 1;

  this->Modified();

  this->UpdateValueBoxImage();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::ClearSelection()
{
  if (!this->Selected)
    {
    return;
    }

  this->Selected = 0;

  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
int vtkKWHSVColorSelector::HasSelection()
{
  return this->Selected;
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::InvokeCommand(const char *command)
{
  if (command && *command)
    {
    this->Script("eval %s", command);
    }
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::InvokeSelectionChangedCommand()
{
  this->InvokeCommand(this->SelectionChangedCommand);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::InvokeSelectionChangingCommand()
{
  this->InvokeCommand(this->SelectionChangingCommand);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetSelectionChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->SelectionChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetSelectionChangingCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->SelectionChangingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->HueSatWheelCanvas)
    {
    this->HueSatWheelCanvas->SetEnabled(this->Enabled);
    }

  if (this->ValueBoxCanvas)
    {
    this->ValueBoxCanvas->SetEnabled(this->Enabled);
    }

  if (this->HueSatLabel)
    {
    this->HueSatLabel->SetEnabled(this->Enabled);
    }

  if (this->ValueLabel)
    {
    this->ValueLabel->SetEnabled(this->Enabled);
    }

  if (this->Enabled)
    {
    this->Bind();
    }
  else
    {
    this->UnBind();
    }
}

// ---------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetBalloonHelpString(
  const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->HueSatWheelCanvas)
    {
    this->HueSatWheelCanvas->SetBalloonHelpString(string);
    }

  if (this->ValueBoxCanvas)
    {
    this->ValueBoxCanvas->SetBalloonHelpString(string);
    }

  if (this->HueSatLabel)
    {
    this->HueSatLabel->SetBalloonHelpString(string);
    }

  if (this->ValueLabel)
    {
    this->ValueLabel->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWHSVColorSelector::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->HueSatWheelCanvas)
    {
    this->HueSatWheelCanvas->SetBalloonHelpJustification(j);
    }

  if (this->ValueBoxCanvas)
    {
    this->ValueBoxCanvas->SetBalloonHelpJustification(j);
    }

  if (this->HueSatLabel)
    {
    this->HueSatLabel->SetBalloonHelpJustification(j);
    }

  if (this->ValueLabel)
    {
    this->ValueLabel->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
int vtkKWHSVColorSelector::CanvasHasTag(const char *canvas, const char *tag)
{
  if (!this->IsCreated() || !canvas || !tag || !*tag)
    {
    return 0;
    }
  
  return atoi(this->Script("llength [%s find withtag %s]", canvas, tag));
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::Redraw()
{
  this->RedrawHueSatWheelCanvas();
  this->RedrawValueBoxCanvas();
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::RedrawHueSatWheelCanvas()
{
  if (!this->HueSatWheelCanvas || !this->HueSatWheelCanvas->IsAlive())
    {
    return;
    }

  ostrstream tk_cmd;

  const char *canv = this->HueSatWheelCanvas->GetWidgetName();

  // If the image is not in the canvas, create it and add it to the canvas
  
  ostrstream img_name;
  img_name << this->HueSatWheelCanvas->GetWidgetName() 
           << "." << VTK_KW_HSV_SEL_IMAGE_TAG << ends;

  if (!this->CanvasHasTag(canv, VTK_KW_HSV_SEL_IMAGE_TAG))
    {
    ostrstream img_name_d;
    img_name_d << this->HueSatWheelCanvas->GetWidgetName() 
               << "." << VTK_KW_HSV_SEL_IMAGE_TAG << "_disabled" << ends;

    tk_cmd << "image create photo " << img_name.str() << " -width 0 -height 0"
           << endl;
    tk_cmd << "image create photo " << img_name_d.str()<< " -width 0 -height 0"
           << endl;
    tk_cmd << canv << " create image 0 0 -anchor nw "
           << " -image " << img_name.str()
           << " -disabledimage " << img_name_d.str()
           << " -tags {" << VTK_KW_HSV_SEL_IMAGE_TAG << "}"
           << endl;

    img_name_d.rdbuf()->freeze(0);
    }

  // Update the image coordinates
  // Leave some margin for the cursor

  int height_offset = this->HueSatCursorRadius;
  int width_offset = height_offset;

  tk_cmd << canv << " coords " << VTK_KW_HSV_SEL_IMAGE_TAG
         << " " << width_offset << " " << height_offset << endl;

  // Reconfigure the canvas given the image size and margins

  int c_height = 2 * (this->HueSatWheelRadius + height_offset);
  int c_width  = 2 * (this->HueSatWheelRadius + width_offset);

  tk_cmd << canv << " config "
         << " -width " << c_width << " -height " << c_height
         << " -scrollregion {0 0 " << c_width << " " << c_height << "}"
         << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // Check if the image needs to be redrawn

  int i_height = 2 * this->HueSatWheelRadius;
  int i_width = i_height;
  
  if (atoi(this->Script("%s cget -width", img_name.str())) != i_width ||
      atoi(this->Script("%s cget -height", img_name.str())) !=  i_height)
    {
    this->UpdateHueSatWheelImage();
    }

  img_name.rdbuf()->freeze(0);

  // Update the selection

  this->UpdateHueSatWheelSelection();
}

//----------------------------------------------------------------------------
int vtkKWHSVColorSelector::GetHueSatFromCoordinates(
  int x, int y, double &hue, double &sat)
{
  // x and y are coordinates within the hue/sat wheel image

  int diameter = 2 * this->HueSatWheelRadius;
  y = (diameter - 1) - y;

  double radius = this->HueSatWheelRadius;
  double dx = (double)x + 0.5 - radius;
  double dy = (double)y + 0.5 - radius;

  int inside_wheel;
  sat = sqrt(dx * dx + dy * dy) / radius;
  if (sat > 1.0)
    {
    sat = 1.0;
    inside_wheel = 0;
    }
  else
    {
    inside_wheel = 1;
    }

  hue = atan2(dy, dx) / (vtkMath::Pi() * 2.0);
  if (hue < 0.0) 
    {
    hue = 1.0 + hue;
    }

  return inside_wheel;
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::UpdateHueSatWheelImage()
{
  if (!this->HueSatWheelCanvas || !this->HueSatWheelCanvas->IsAlive())
    {
    return;
    }

  // Create the Hue/Sat image and update (as well as the disable image)

  int diameter = 2 * this->HueSatWheelRadius;

  unsigned char *buffer = new unsigned char [diameter * diameter * 4];
  unsigned char *ptr = buffer;

  unsigned char *buffer_d = new unsigned char [diameter * diameter * 4];
  unsigned char *ptr_d = buffer_d;

  double hue, sat, r, g, b;

  for (int y = 0; y < diameter; y++)
    {
    for (int x = 0; x < diameter; x++)
      {
      if (this->GetHueSatFromCoordinates(x, y, hue, sat))
        {
        vtkMath::HSVToRGB(hue, sat, 1.0, &r, &g, &b);
        r *= 255.0;
        g *= 255.0;
        b *= 255.0;

        *ptr++ = (unsigned char)r;
        *ptr++ = (unsigned char)g;
        *ptr++ = (unsigned char)b;
        *ptr++ = 255;

        *ptr_d++ = (unsigned char)r;
        *ptr_d++ = (unsigned char)g;
        *ptr_d++ = (unsigned char)b;
        *ptr_d++ = (unsigned char)(VTK_KW_HSV_SEL_DISABLED_OPACITY * 255.0);
        }
      else
        {
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;

        *ptr_d++ = 0;
        *ptr_d++ = 0;
        *ptr_d++ = 0;
        *ptr_d++ = 0;
        }
      }
    }

  // Update the image

  ostrstream img_name;
  img_name << this->HueSatWheelCanvas->GetWidgetName() 
           << "." << VTK_KW_HSV_SEL_IMAGE_TAG << ends;

  vtkKWTkUtilities::UpdatePhoto(this->GetApplication()->GetMainInterp(),
                                img_name.str(),
                                buffer,
                                diameter, diameter, 4,
                                diameter * diameter * 4,
                                this->HueSatWheelCanvas->GetWidgetName());

  delete [] buffer;
  img_name.rdbuf()->freeze(0);

  // Update the image (disabled state)

  ostrstream img_name_d;
  img_name_d << this->HueSatWheelCanvas->GetWidgetName() 
             << "." << VTK_KW_HSV_SEL_IMAGE_TAG << "_disabled" << ends;

  vtkKWTkUtilities::UpdatePhoto(this->GetApplication()->GetMainInterp(),
                                img_name_d.str(),
                                buffer_d,
                                diameter, diameter, 4,
                                diameter * diameter * 4,
                                this->HueSatWheelCanvas->GetWidgetName());

  delete [] buffer_d;
  img_name_d.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::UpdateHueSatWheelSelection()
{
  if (!this->HueSatWheelCanvas || !this->HueSatWheelCanvas->IsAlive())
    {
    return;
    }

  ostrstream tk_cmd;

  const char *canv = this->HueSatWheelCanvas->GetWidgetName();

  int has_tag = this->CanvasHasTag(canv, VTK_KW_HSV_SEL_SELECTION_TAG);

  // Remove the cursor, or update its coordinates given the selection

  if (!this->HasSelection())
    {
    if (has_tag)
      {
      tk_cmd << canv << " delete " << VTK_KW_HSV_SEL_SELECTION_TAG << endl;
      }
    }
  else
    {
    if (!has_tag)
      {
      tk_cmd << canv << " create oval 0 0 0 0 -fill white -tag "
             << VTK_KW_HSV_SEL_SELECTION_TAG << endl;
      }
    double wheel_radius = this->HueSatWheelRadius;
    int x = (int)(wheel_radius - 0.5 + wheel_radius * this->SelectedColor[1] * 
                  cos(this->SelectedColor[0] * vtkMath::Pi() * 2.0));
    int y = (int)(wheel_radius - 0.5 - wheel_radius * this->SelectedColor[1] * 
                  sin(this->SelectedColor[0] * vtkMath::Pi() * 2.0));
    int height_offset = this->HueSatCursorRadius;
    int width_offset = height_offset;
    tk_cmd << canv << " coords " << VTK_KW_HSV_SEL_SELECTION_TAG
           << " " << width_offset + x - this->HueSatCursorRadius 
           << " " << height_offset + y - this->HueSatCursorRadius 
           << " " << width_offset + x + this->HueSatCursorRadius 
           << " " << height_offset + y + this->HueSatCursorRadius 
           << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::RedrawValueBoxCanvas()
{
  if (!this->ValueBoxCanvas || !this->ValueBoxCanvas->IsAlive())
    {
    return;
    }

  ostrstream tk_cmd;

  const char *canv = this->ValueBoxCanvas->GetWidgetName();

  // If the image is not in the canvas, create it and add it to the canvas
  
  ostrstream img_name;
  img_name << this->ValueBoxCanvas->GetWidgetName() 
           << "." << VTK_KW_HSV_SEL_IMAGE_TAG << ends;

  if (!this->CanvasHasTag(canv, VTK_KW_HSV_SEL_IMAGE_TAG))
    {
    ostrstream img_name_d;
    img_name_d << this->ValueBoxCanvas->GetWidgetName() 
               << "." << VTK_KW_HSV_SEL_IMAGE_TAG << "_disabled" << ends;

    tk_cmd << "image create photo " << img_name.str() << " -width 0 -height 0"
           << endl;
    tk_cmd << "image create photo " << img_name_d.str() <<" -width 0 -height 0"
           << endl;
    tk_cmd << canv << " create image 0 0 -anchor nw "
           << " -image " << img_name.str()
           << " -disabledimage " << img_name_d.str()
           << " -tags {" << VTK_KW_HSV_SEL_IMAGE_TAG << "}"
           << endl;

    img_name_d.rdbuf()->freeze(0);
    }

  // Update the image coordinates
  // Leave some margin for the cursor

  int height_offset = this->HueSatCursorRadius;
  int width_offset = this->ValueCursorMargin;

  tk_cmd << canv << " coords " << VTK_KW_HSV_SEL_IMAGE_TAG
         << " " << width_offset << " " << height_offset << endl;

  // Reconfigure the canvas given the image size and margins

  int c_height = 2 * (this->HueSatWheelRadius + height_offset);
  int c_width = this->ValueBoxWidth + 2 * width_offset;

  tk_cmd << canv << " config "
         << " -width " << c_width << " -height " << c_height
         << " -scrollregion {0 0 " << c_width - 1 << " " << c_height - 1 << "}"
         << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // Check if the image needs to be redrawn

  int i_height = 2 * this->HueSatWheelRadius;
  int i_width = this->ValueBoxWidth;
  
  if (atoi(this->Script("%s cget -width", img_name.str())) != i_width ||
      atoi(this->Script("%s cget -height", img_name.str())) !=  i_height)
    {
    this->UpdateValueBoxImage();
    }

  img_name.rdbuf()->freeze(0);

  // Update the selection

  this->UpdateValueBoxSelection();
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::GetValueFromCoordinate(int y, double &value)
{
  // y is coordinate within the value box image

  int height = 2 * this->HueSatWheelRadius;
  y = (height - 1) - y;

  value = (double)y / (double)(height - 1);
  if (value < 0.0)
    {
    value = 0.0;
    }
  else if (value > 1.0)
    {
    value = 1.0;
    }
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::UpdateValueBoxImage()
{
  if (!this->ValueBoxCanvas || !this->ValueBoxCanvas->IsAlive())
    {
    return;
    }

  // Create the Value image and update

  int height = 2 * this->HueSatWheelRadius;
  int width = this->ValueBoxWidth;

  unsigned char *buffer = new unsigned char [width * height * 3];
  unsigned char *ptr = buffer;

  unsigned char *buffer_d = new unsigned char [width * height * 4];
  unsigned char *ptr_d = buffer_d;

  double value, r, g, b;

  for (int y = 0; y < height; y++)
    {
    this->GetValueFromCoordinate(y, value);
    vtkMath::HSVToRGB(this->SelectedColor[0], this->SelectedColor[1], value, 
                      &r, &g, &b);
    r *= 255.0;
    g *= 255.0;
    b *= 255.0;

    for (int x = 0; x < width; x++)
      {
      *ptr++ = (unsigned char)r;
      *ptr++ = (unsigned char)g;
      *ptr++ = (unsigned char)b;

      *ptr_d++ = (unsigned char)r;
      *ptr_d++ = (unsigned char)g;
      *ptr_d++ = (unsigned char)b;
      *ptr_d++ = (unsigned char)(VTK_KW_HSV_SEL_DISABLED_OPACITY * 255.0);
      }
    }

  // Update image

  ostrstream img_name;
  img_name << this->ValueBoxCanvas->GetWidgetName() 
           << "." << VTK_KW_HSV_SEL_IMAGE_TAG << ends;

  vtkKWTkUtilities::UpdatePhoto(this->GetApplication()->GetMainInterp(),
                                img_name.str(),
                                buffer,
                                width, height, 3,
                                width * height * 3,
                                this->ValueBoxCanvas->GetWidgetName());

  delete [] buffer;
  img_name.rdbuf()->freeze(0);

  // Update disabled image

  ostrstream img_name_d;
  img_name_d << this->ValueBoxCanvas->GetWidgetName() 
             << "." << VTK_KW_HSV_SEL_IMAGE_TAG << "_disabled" << ends;

  vtkKWTkUtilities::UpdatePhoto(this->GetApplication()->GetMainInterp(),
                                img_name_d.str(),
                                buffer_d,
                                width, height, 4,
                                width * height * 4,
                                this->ValueBoxCanvas->GetWidgetName());

  delete [] buffer_d;
  img_name_d.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::UpdateValueBoxSelection()
{
  if (!this->ValueBoxCanvas || !this->ValueBoxCanvas->IsAlive())
    {
    return;
    }

  ostrstream tk_cmd;

  const char *canv = this->ValueBoxCanvas->GetWidgetName();

  int has_tag = this->CanvasHasTag(canv, VTK_KW_HSV_SEL_SELECTION_TAG);

  // Delete the cursor or update its coordinates given the selection

  if (!this->HasSelection())
    {
    if (has_tag)
      {
      tk_cmd << canv << " delete " << VTK_KW_HSV_SEL_SELECTION_TAG << endl;
      }
    }
  else
    {
    if (!has_tag)
      {
      tk_cmd << canv << " create rectangle 0 0 0 0 -fill white -tag "
             << VTK_KW_HSV_SEL_SELECTION_TAG << endl;
      }
    int height_offset = this->HueSatCursorRadius;
    int width_offset = this->ValueCursorMargin;
    int height = this->HueSatWheelRadius * 2;
    int y = (int)((1.0 - this->SelectedColor[2]) * (height - 1));
    tk_cmd << canv << " coords " << VTK_KW_HSV_SEL_SELECTION_TAG
           << " " << width_offset - this->ValueCursorMargin
           << " " << height_offset + y - 1
           << " " 
           << width_offset + this->ValueBoxWidth - 1 + this->ValueCursorMargin 
           << " " << height_offset + y + 1
           << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::HueSatPickCallback(int x, int y)
{
  if (this->ModificationOnly && !this->HasSelection())
    {
    return;
    }

  this->PreviouslySelectedColor[0] = this->SelectedColor[0];
  this->PreviouslySelectedColor[1] = this->SelectedColor[1];
  this->PreviouslySelectedColor[2] = this->SelectedColor[2];

  int height_offset = this->HueSatCursorRadius;
  int width_offset = height_offset;

  y -= height_offset;
  x -= width_offset;

  double hue, sat;
  this->GetHueSatFromCoordinates(x, y, hue, sat);
  this->SetSelectedColor(hue, sat, this->SelectedColor[2]);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::HueSatMoveCallback(int x, int y)
{
  if (!this->HasSelection())
    {
    return;
    }

  int height_offset = this->HueSatCursorRadius;
  int width_offset = height_offset;

  y -= height_offset;
  x -= width_offset;

  double hue, sat;
  this->GetHueSatFromCoordinates(x, y, hue, sat);
  this->SetSelectedColor(hue, sat, this->SelectedColor[2]);

  if (this->PreviouslySelectedColor[0] != this->SelectedColor[0] ||
      this->PreviouslySelectedColor[1] != this->SelectedColor[1] ||
      this->PreviouslySelectedColor[2] != this->SelectedColor[2])
    {
    this->InvokeSelectionChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::HueSatReleaseCallback()
{
  if (!this->HasSelection())
    {
    return;
    }

  if (this->PreviouslySelectedColor[0] != this->SelectedColor[0] ||
      this->PreviouslySelectedColor[1] != this->SelectedColor[1] ||
      this->PreviouslySelectedColor[2] != this->SelectedColor[2])
    {
    this->InvokeSelectionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::ValuePickCallback(int vtkNotUsed(x), int y)
{
  if (this->ModificationOnly && !this->HasSelection())
    {
    return;
    }

  this->PreviouslySelectedColor[0] = this->SelectedColor[0];
  this->PreviouslySelectedColor[1] = this->SelectedColor[1];
  this->PreviouslySelectedColor[2] = this->SelectedColor[2];

  int height_offset = this->HueSatCursorRadius;
  y -= height_offset;

  double value;
  this->GetValueFromCoordinate(y, value);
  this->SetSelectedColor(
    this->SelectedColor[0], this->SelectedColor[1], value);
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::ValueMoveCallback(int vtkNotUsed(x), int y)
{
  if (!this->HasSelection())
    {
    return;
    }

  int height_offset = this->HueSatCursorRadius;
  y -= height_offset;

  double value;
  this->GetValueFromCoordinate(y, value);
  this->SetSelectedColor(
    this->SelectedColor[0], this->SelectedColor[1], value);

  if (this->PreviouslySelectedColor[0] != this->SelectedColor[0] ||
      this->PreviouslySelectedColor[1] != this->SelectedColor[1] ||
      this->PreviouslySelectedColor[2] != this->SelectedColor[2])
    {
    this->InvokeSelectionChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::ValueReleaseCallback()
{
  if (!this->HasSelection())
    {
    return;
    }

  if (this->PreviouslySelectedColor[0] != this->SelectedColor[0] ||
      this->PreviouslySelectedColor[1] != this->SelectedColor[1] ||
      this->PreviouslySelectedColor[2] != this->SelectedColor[2])
    {
    this->InvokeSelectionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWHSVColorSelector::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "HueSatWheelRadius: "<< this->HueSatWheelRadius << endl;
  os << indent << "ValueBoxWidth: "<< this->ValueBoxWidth << endl;
  os << indent << "HueSatCursorRadius: "
     << this->HueSatCursorRadius << endl;
  os << indent << "ValueCursorMargin: "<< this->ValueCursorMargin << endl;
  os << indent << "ModificationOnly: "
     << (this->ModificationOnly ? "On" : "Off") << endl;
  os << indent << "SelectedColor: (" 
     << this->SelectedColor[0] << ", "
     << this->SelectedColor[1] << ", "
     << this->SelectedColor[2] << ") " << endl;
  os << indent << "HideValue: "
     << (this->HideValue ? "On" : "Off") << endl;
  os << indent << "HueSatWheelCanvas: ";
  if (this->HueSatWheelCanvas)
    {
    os << endl;
    this->HueSatWheelCanvas->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "ValueBoxCanvas: ";
  if (this->ValueBoxCanvas)
    {
    os << endl;
    this->ValueBoxCanvas->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}

