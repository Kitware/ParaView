/*=========================================================================

  Module:    vtkKWSpinButtons.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWSpinButtons.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWSpinButtons );
vtkCxxRevisionMacro(vtkKWSpinButtons, "1.1");

/* 
 * Resource generated for file:
 *    spin_up.png (zlib, base64) (image file)
 */
static const unsigned int  image_spin_up_width          = 8;
static const unsigned int  image_spin_up_height         = 4;
static const unsigned int  image_spin_up_pixel_size     = 4;
static const unsigned long image_spin_up_length         = 76;
static const unsigned long image_spin_up_decoded_length = 128;

static const unsigned char image_spin_up[] = 
  "eNr7//8/w38oBoLpIPwfSQxJbj4Qv4fi+Whys4H4NRD/hmIQezZULhuItwPxcTQMEssGAI"
  "0RMok=";

/* 
 * Resource generated for file:
 *    spin_down.png (zlib, base64) (image file)
 */
static const unsigned int  image_spin_down_width          = 8;
static const unsigned int  image_spin_down_height         = 4;
static const unsigned int  image_spin_down_pixel_size     = 4;
static const unsigned long image_spin_down_length         = 80;
static const unsigned long image_spin_down_decoded_length = 128;

static const unsigned char image_spin_down[] = 
  "eNpjYGDIZmBg2A7Ex9EwSCz7////QIphNhC/BuLfUAxizwbJwTAQzAfi91A8H1kOSc10EE"
  "YWAwDjtzKJ";

/* 
 * Resource generated for file:
 *    spin_left.png (zlib, base64) (image file)
 */
static const unsigned int  image_spin_left_width          = 4;
static const unsigned int  image_spin_left_height         = 8;
static const unsigned int  image_spin_left_pixel_size     = 4;
static const unsigned long image_spin_left_length         = 64;
static const unsigned long image_spin_left_decoded_length = 128;

static const unsigned char image_spin_left[] = 
  "eNr7//8/w38oBoJsJPZsIN4OZc8H4tdAfByIpwPxeyD+jY2Prh6becj2AQATAzKJ";

/* 
 * Resource generated for file:
 *    spin_right.png (zlib, base64) (image file)
 */
static const unsigned int  image_spin_right_width          = 4;
static const unsigned int  image_spin_right_height         = 8;
static const unsigned int  image_spin_right_pixel_size     = 4;
static const unsigned long image_spin_right_length         = 64;
static const unsigned long image_spin_right_decoded_length = 128;

static const unsigned char image_spin_right[] = 
  "eNpjYGDI/v//PwMMA8F2IJ6NxD8OxK+BeD4S/zcQvwfi6Tj4yOrRzUOxDwBd1DKJ";


//----------------------------------------------------------------------------
vtkKWSpinButtons::vtkKWSpinButtons()
{
  this->DecrementButton = vtkKWPushButton::New();
  this->IncrementButton = vtkKWPushButton::New();

  this->Orientation = vtkKWSpinButtons::OrientationVertical;
}

//----------------------------------------------------------------------------
vtkKWSpinButtons::~vtkKWSpinButtons()
{
  if (this->DecrementButton)
    {
    this->DecrementButton->Delete();
    this->DecrementButton = NULL;
    }

  if (this->IncrementButton)
    {
    this->IncrementButton->Delete();
    this->IncrementButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  this->DecrementButton->SetParent(this);
  this->DecrementButton->Create(app);
  this->DecrementButton->SetPadX(0);
  this->DecrementButton->SetPadY(this->DecrementButton->GetPadX());

  this->IncrementButton->SetParent(this);
  this->IncrementButton->Create(app);
  this->IncrementButton->SetPadX(this->DecrementButton->GetPadX());
  this->IncrementButton->SetPadY(this->DecrementButton->GetPadY());
  
  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetOrientation(int val)
{
  if (val < vtkKWSpinButtons::OrientationHorizontal)
    {
    val = vtkKWSpinButtons::OrientationHorizontal;
    }
  if (val > vtkKWSpinButtons::OrientationVertical)
    {
    val = vtkKWSpinButtons::OrientationVertical;
    }

  if (this->Orientation == val)
    {
    return;
    }

  this->Orientation = val;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetDecrementCommand(
  vtkObject *object, const char *method)
{
  if (this->DecrementButton)
    {
    this->DecrementButton->SetCommand(object, method);
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::SetIncrementCommand(
  vtkObject *object, const char *method)
{
  if (this->IncrementButton)
    {
    this->IncrementButton->SetCommand(object, method);
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->Orientation == vtkKWSpinButtons::OrientationVertical)  
    {
    if (this->IncrementButton && this->IncrementButton->IsCreated())
      {
      this->IncrementButton->SetImageToPixels(
        image_spin_up, 
        image_spin_up_width, image_spin_up_height, 
        image_spin_up_pixel_size,
        image_spin_up_length);
      this->Script("pack %s -side top -expand y -fill y",
                   this->IncrementButton->GetWidgetName());
      }
    if (this->DecrementButton && this->DecrementButton->IsCreated())
      {
      this->DecrementButton->SetImageToPixels(
        image_spin_down, 
        image_spin_down_width, image_spin_down_height, 
        image_spin_down_pixel_size,
        image_spin_down_length);
      this->Script("pack %s -side bottom -expand y -fill y",
                   this->DecrementButton->GetWidgetName());
      }
    }
  else
    {
    if (this->DecrementButton && this->DecrementButton->IsCreated())
      {
      this->DecrementButton->SetImageToPixels(
        image_spin_left, 
        image_spin_left_width, image_spin_left_height, 
        image_spin_left_pixel_size,
        image_spin_left_length);
      this->Script("pack %s -side left -expand y -fill y",
                   this->DecrementButton->GetWidgetName());
      }
    if (this->IncrementButton && this->IncrementButton->IsCreated())
      {
      this->IncrementButton->SetImageToPixels(
        image_spin_right, 
        image_spin_right_width, image_spin_right_height, 
        image_spin_right_pixel_size,
        image_spin_right_length);
      this->Script("pack %s -side right -expand y -fill y",
                   this->IncrementButton->GetWidgetName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->DecrementButton);
  this->PropagateEnableState(this->IncrementButton);
}

//----------------------------------------------------------------------------
void vtkKWSpinButtons::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DecrementButton: " << this->DecrementButton << endl;
  os << indent << "IncrementButton: " << this->IncrementButton << endl;
  if (this->Orientation == vtkKWSpinButtons::OrientationHorizontal)
    {
    os << indent << "Orientation: Horizontal\n";
    }
  else
    {
    os << indent << "Orientation: Vertical\n";
    }
}

