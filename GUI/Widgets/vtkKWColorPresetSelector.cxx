/*=========================================================================

  Module:    vtkKWColorPresetSelector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWColorPresetSelector.h"

#include "vtkColorTransferFunction.h"
#include "vtkKWLabel.h"
#include "vtkKWApplication.h"
#include "vtkKWIcon.h"
#include "vtkKWMenu.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"

#include <vtkstd/list>
#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWColorPresetSelector);
vtkCxxRevisionMacro(vtkKWColorPresetSelector, "1.3");

vtkCxxSetObjectMacro(vtkKWColorPresetSelector,ColorTransferFunction,vtkColorTransferFunction);

//----------------------------------------------------------------------------
class vtkKWColorPresetSelectorInternals
{
public:
  struct PresetNode
  {
    vtkstd::string Name;
    vtkColorTransferFunction *ColorTransferFunction;
  };

  typedef vtkstd::list<PresetNode> PresetContainer;
  typedef vtkstd::list<PresetNode>::iterator PresetContainerIterator;

  PresetContainer Presets;
};

//----------------------------------------------------------------------------
vtkKWColorPresetSelector::vtkKWColorPresetSelector()
{
  // Create a default transfer function

  this->ColorTransferFunction = NULL;
  this->SetColorTransferFunction(vtkColorTransferFunction::New());
  this->ColorTransferFunction->Delete();

  // Set a default scalar range

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;  

  // Internal structs (the presets)

  this->Internals = new vtkKWColorPresetSelectorInternals;

  this->PreviewSize                 = 12;
  this->HideSolidColorPresets       = 0;
  this->HideGradientPresets         = 0;
  this->ApplyPresetBetweenEndPoints = 0;

  this->PresetSelectedCommand = NULL;

  this->CreateDefaultPresets();
}

//----------------------------------------------------------------------------
vtkKWColorPresetSelector::~vtkKWColorPresetSelector()
{
  this->SetColorTransferFunction(NULL);

  if (this->PresetSelectedCommand)
    {
    delete [] this->PresetSelectedCommand;
    this->PresetSelectedCommand = NULL;
    }

  // Delete all presets

  vtkKWColorPresetSelectorInternals::PresetContainerIterator it = 
    this->Internals->Presets.begin();
  vtkKWColorPresetSelectorInternals::PresetContainerIterator end = 
    this->Internals->Presets.end();
  for (; it != end; ++it)
    {
    if (it->ColorTransferFunction)
      {
      it->ColorTransferFunction->Delete();
      it->ColorTransferFunction = NULL;
      }
    }

  // Delete our container

  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkColorTransferFunction* 
vtkKWColorPresetSelector::GetPresetColorTransferFunction(const char *name)
{
  if (name)
    {
    vtkKWColorPresetSelectorInternals::PresetContainerIterator it = 
      this->Internals->Presets.begin();
    vtkKWColorPresetSelectorInternals::PresetContainerIterator end = 
      this->Internals->Presets.end();
    for (; it != end; ++it)
      {
      if (!it->Name.compare(name))
        {
        return it->ColorTransferFunction;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::MapColorTransferFunction(
  vtkColorTransferFunction *source, double source_range[2],
  vtkColorTransferFunction *target, double target_range[2])
{
  if (!source || !source_range || !target || !target_range)
    {
    return 0;
    }

  target->RemoveAllPoints();
  target->SetClamping(source->GetClamping());
  target->SetColorSpace(source->GetColorSpace());

  double source_span = source_range[1] - source_range[0];
  double target_span = target_range[1] - target_range[0];

  double *source_ptr = source->GetDataPointer();
  double *source_ptr_end = source_ptr + source->GetSize() * 4;
  
  double norm_val, target_val;

  while (source_ptr < source_ptr_end)
    {
    norm_val = (source_ptr[0] - source_range[0]) / source_span;
    target_val = norm_val * target_span + target_range[0];
    target->AddRGBPoint(
      target_val, source_ptr[1], source_ptr[2], source_ptr[3]);
    source_ptr += 4;
    }

  return 1;
}


//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::HasPreset(const char *name)
{
  return this->GetPresetColorTransferFunction(name) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AllocatePreset(const char *name)
{
  if (!name || this->HasPreset(name))
    {
    return 0;
    }

  // Create a new node and allocate a new color tfunc

  vtkKWColorPresetSelectorInternals::PresetNode node;
  node.Name.assign(name); 
  node.ColorTransferFunction = vtkColorTransferFunction::New();
  node.ColorTransferFunction->Register(this);
  node.ColorTransferFunction->Delete();

  // Add it to the container

  this->Internals->Presets.push_back(node);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::RemovePreset(const char *name)
{
  if (name)
    {
    vtkKWColorPresetSelectorInternals::PresetContainerIterator it = 
      this->Internals->Presets.begin();
    vtkKWColorPresetSelectorInternals::PresetContainerIterator end = 
      this->Internals->Presets.end();
    for (; it != end; ++it)
      {
      if (!it->Name.compare(name))
        {
        if (it->ColorTransferFunction)
          {
          it->ColorTransferFunction->Delete();
          it->ColorTransferFunction = NULL;
          }
        this->Internals->Presets.erase(it);
        this->PopulatePresetMenu();
        return 1;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddPreset(
  const char *name, vtkColorTransferFunction *func, double range[2])
{
  // Check parameters, check if preset is not present, add it otherwise

  if (!name || !func || !range || 
      this->HasPreset(name) || 
      !this->AllocatePreset(name))
    {
    return 0;
    }

  // Parse the func, normalize and set the points of the preset func

  vtkColorTransferFunction *preset_func = 
    this->GetPresetColorTransferFunction(name);
  double preset_range[2] = {0.0, 1.0};

  int res = this->MapColorTransferFunction(
    func, range, preset_func, preset_range);

  // Repopulate the menu

  this->PopulatePresetMenu();

  return res;
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddSolidRGBPreset(
  const char *name, double r, double g, double b)
{
  return this->AddGradientRGBPreset(name, r, g, b, r, g, b);
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddSolidRGBPreset(
  const char *name, double rgb[3])
{
  return this->AddGradientRGBPreset(name, rgb, rgb);
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddSolidHSVPreset(
  const char *name, double h, double s, double v)
{
  return this->AddGradientHSVPreset(name, h, s, v, h, s, v);
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddSolidHSVPreset(
  const char *name, double hsv[3])
{
  return this->AddGradientHSVPreset(name, hsv, hsv);
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddGradientRGBPreset(
  const char *name, double rgb1[3], double rgb2[3])
{
  if (!rgb1 || !rgb2)
    {
    return 0;
    }
  return this->AddGradientRGBPreset(
    name, rgb1[0], rgb1[1], rgb1[2], rgb2[0], rgb2[1], rgb2[2]);
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddGradientRGBPreset(
  const char *name, 
  double r1, double g1, double b1, 
  double r2, double g2, double b2)
{
  double range[2] = {0.0, 1.0};
  vtkColorTransferFunction *func = vtkColorTransferFunction::New();
  func->SetColorSpaceToRGB();
  func->AddRGBPoint(range[0], r1, g1, b1);
  func->AddRGBPoint(range[1], r2, g2, b2);
  
  int res = this->AddPreset(name, func, range);
  func->Delete();
  return res;
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddGradientHSVPreset(
  const char *name, double hsv1[3], double hsv2[3])
{
  if (!hsv1 || !hsv2)
    {
    return 0;
    }
  return this->AddGradientHSVPreset(
    name, hsv1[0], hsv1[1], hsv1[2], hsv2[0], hsv2[1], hsv2[2]);
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddGradientHSVPreset(
  const char *name, 
  double h1, double s1, double v1, 
  double h2, double s2, double v2)
{
  double range[2] = {0.0, 1.0};
  vtkColorTransferFunction *func = vtkColorTransferFunction::New();
  func->SetColorSpaceToHSV();
  func->AddHSVPoint(range[0], h1, s1, v1);
  func->AddHSVPoint(range[1], h2, s2, v2);
  
  int res = this->AddPreset(name, func, range);
  func->Delete();
  return res;
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::AddFlagRGBPreset(
  const char *name, int nb_colors, double **rgb, int repeat)
{
  if (!name || nb_colors < 1 || !rgb || repeat < 1)
    {
    return 0;
    }

  double range[2] = { 0.0, 1.0 };

  vtkColorTransferFunction *func = vtkColorTransferFunction::New();
  func->SetColorSpaceToRGB();

  double flag_width = 1.0 / (double)repeat;
  double color_width = flag_width / (double)nb_colors;
  double flag_start = 0.0;
  double tol = 0.0001;
  double *color;

  // Force the first end-point to match the first color

  color = rgb[0];
  func->AddRGBPoint(0.0, color[0], color[1], color[2]);

  // Insert each color of the flag, for each repetition
  // Do not insert the first and last one, as we will take care of it
  // manually so that they exactly match the endpoints (not within a tolerance)

  for (int j = 0; j < repeat; j++)
    {
    double color_start = flag_start;
    for (int i = 0; i < nb_colors; i++)
      {
      color = rgb[i];
      if (color)
        {
        if (!(j == 0 && i == 0))
          {
          func->AddRGBPoint(color_start + tol, 
                            color[0], color[1], color[2]);
          }
        if (!(j == repeat - 1 && i == nb_colors - 1))
          {
          func->AddRGBPoint(color_start + color_width - tol, 
                            color[0], color[1], color[2]);
          }
        }
      color_start += color_width;
      }
    flag_start += flag_width;
    }

  // Force the last end-point to match the last color

  color = rgb[nb_colors - 1];
  func->AddRGBPoint(1.0, color[0], color[1], color[2]);
  
  int res = this->AddPreset(name, func, range);
  func->Delete();
  return res;
}

//----------------------------------------------------------------------------
void vtkKWColorPresetSelector::CreateDefaultPresets()
{
  vtkColorTransferFunction *func;

  double range[2] = { 0.0, 1.0 };
  double r0 = range[0];
  double r1 = range[1];
  double rd = r1 - r0;

  double black[3]       = { 0.0, 0.0, 0.0 };
  double white[3]       = { 1.0, 1.0, 1.0 };
  double red[3]         = { 1.0, 0.3, 0.3 };
  double red_pure[3]    = { 1.0, 0.0, 0.0 };
  double green[3]       = { 0.3, 1.0, 0.3 };
  double green_pure[3]  = { 0.0, 1.0, 0.0 };
  double blue[3]        = { 0.3, 0.3, 1.0 };
  double blue_pure[3]   = { 0.0, 0.0, 1.0 };
  double yellow[3]      = { 1.0, 1.0, 0.3 };
  double yellow_pure[3] = { 1.0, 1.0, 0.0 };
  double magenta[3]     = { 1.0, 0.3, 1.0 };
  double cyan[3]        = { 0.3, 1.0, 1.0 };
  double orange[3]      = { 1.0, 0.5, 0.0 };
  double violet[3]      = { 2.0 / 3.0, 0.0, 1.0 };

  double p1, p2, p3, p4;
  double f1, f2;

  // Rainbow
  // Varies the hue component of HSV color model
  
  func = vtkColorTransferFunction::New();
  func->SetColorSpaceToHSV();
  func->AddRGBPoint(r0, blue[0], blue[1], blue[2]);
  func->AddRGBPoint(r0 + rd * 0.5, green[0], green[1], green[2]);
  func->AddRGBPoint(r1, red[0], red[1], red[2]);
  this->AddPreset("Rainbow", func, range);
  func->Delete();

  // Jet (a variant of Rainbow, more saturated)
  // Varies the hue component of HSV color model
  
  func = vtkColorTransferFunction::New();
  func->SetColorSpaceToHSV();
  func->AddRGBPoint(r0, blue_pure[0], blue_pure[1], blue_pure[2]);
  func->AddRGBPoint(r0 + rd * 0.5,green_pure[0], green_pure[1], green_pure[2]);
  func->AddRGBPoint(r1, red_pure[0], red_pure[1], red_pure[2]);
  this->AddPreset("Jet", func, range);
  func->Delete();

  // HSV (a variant of Rainbow or Jet)
  // Varies the hue component of HSV color model

  func = vtkColorTransferFunction::New();
  func->SetColorSpaceToHSV();
  func->AddHSVPoint(r0, 0.0, 1.0, 1.0);
  func->AddHSVPoint(r0 + rd * 0.5, 0.5, 1.0, 1.0);
  func->AddHSVPoint(r1, 0.999999, 1.0, 1.0);
  this->AddPreset("HSV", func, range);
  func->Delete();
  
  // Spring (shades of magenta and yellow color map)

  this->AddGradientRGBPreset("Spring", 1, 0, 1, 1, 1, 0);
  
  // Summer (shades of green and yellow colormap)

  this->AddGradientRGBPreset("Summer", 0, 0.5, 0.4, 1, 1, 0.4);
  
  // Autumn (shades of red and yellow color map)

  this->AddGradientRGBPreset("Autumn", 1, 0, 0, 1, 1, 0);
  
  // Winter (shades of blue and green color map)

  this->AddGradientRGBPreset("Winter", 0, 0, 0.75, 0, 1, 0.25);
  
  // Hot (black-red-yellow-white color map)
  
  func = vtkColorTransferFunction::New();
  func->SetColorSpaceToRGB();
  p1 = 0.0 / 8.0;
  p2 = 3.0 / 8.0;
  p3 = 6.0 / 8.0;
  p4 = 8.0 / 8.0;
  func->AddRGBPoint(r0,           0, 0, 0);
  func->AddRGBPoint(r0 + rd * p2, 1, 0, 0);
  func->AddRGBPoint(r0 + rd * p3, 1, 1, 0);
  func->AddRGBPoint(r1,           1, 1, 1);
  this->AddPreset("Hot", func, range);
  func->Delete();
  
  // Cool (shades of cyan and magenta color map)

  this->AddGradientRGBPreset("Cool", 0, 1, 1, 1, 0, 1);
  
  // Bone (gray-scale with a tinge of blue color map)

  f1 = 7.0 / 8.0;
  f2 = 1.0 / 8.0;
  func = vtkColorTransferFunction::New();
  func->SetColorSpaceToRGB();
  func->AddRGBPoint(r0, 
                    f1 * p1 + f2 * 0, f1 * p1 + f2 * 0, f1 * p1 + f2 * 0);
  func->AddRGBPoint(r0 + rd * p2, 
                    f1 * p2 + f2 * 0, f1 * p2 + f2 * 0, f1 * p2 + f2 * 1);
  func->AddRGBPoint(r0 + rd * p3, 
                    f1 * p3 + f2 * 0, f1 * p3 + f2 * 1, f1 * p3 + f2 * 1);
  func->AddRGBPoint(r1, 
                    f1 * p4 + f2 * 1, f1 * p4 + f2 * 1, f1 * p4 + f2 * 1);
  this->AddPreset("Bone", func, range);
  func->Delete();
  
  // Pink (pastel shades of pink color map)

  f1 = 2.0 / 3.0;
  f2 = 1.0 / 3.0;
  func = vtkColorTransferFunction::New();
  func->SetColorSpaceToHSV();
  func->AddRGBPoint(r0, 
                    sqrt(f1 * p1 + f2 * 0), 
                    sqrt(f1 * p1 + f2 * 0), 
                    sqrt(f1 * p1 + f2 * 0));
  func->AddRGBPoint(r0 + rd * p2, 
                    sqrt(f1 * p2 + f2 * 1), 
                    sqrt(f1 * p2 + f2 * 0), 
                    sqrt(f1 * p2 + f2 * 0));
  func->AddRGBPoint(r0 + rd * p3, 
                    sqrt(f1 * p3 + f2 * 1), 
                    sqrt(f1 * p3 + f2 * 1), 
                    sqrt(f1 * p3 + f2 * 0));
  func->AddRGBPoint(r1, 
                    sqrt(f1 * p4 + f2 * 1), 
                    sqrt(f1 * p4 + f2 * 1), 
                    sqrt(f1 * p4 + f2 * 1));
  this->AddPreset("Pink", func, range);
  func->Delete();
  
  // Copper (linear copper-tone color map)

  p1 = 1.0 / 1.25;
  func = vtkColorTransferFunction::New();
  func->SetColorSpaceToRGB();
  func->AddRGBPoint(r0, 0.0, 0.0, 0.0);
  func->AddRGBPoint(r0 + rd * p1, 
                    p1 * 1.25, p1 * 0.7812, p1 * 0.4975);
  func->AddRGBPoint(r1, 1.0, 0.7812, 0.4975);
  this->AddPreset("Copper", func, range);
  func->Delete();
  
  // Grayscale (linear gray-scale color map)

  this->AddGradientRGBPreset("Gray", black, white);
  
  // Solid colors

  this->AddSolidRGBPreset("White", white);

  this->AddSolidRGBPreset("Red", red);

  this->AddSolidRGBPreset("Green", green);

  this->AddSolidRGBPreset("Blue", blue);

  this->AddSolidRGBPreset("Cyan", cyan);

  this->AddSolidRGBPreset("Magenta", magenta);

  this->AddSolidRGBPreset("Yellow", yellow);

  // Flag (alternating red, white, blue, and black color map)

  // Flag (alternating red, white, blue, and black color map)

  double *flag_colors[6];
  flag_colors[0] = red_pure;
  flag_colors[1] = white;
  flag_colors[2] = blue_pure;
  flag_colors[3] = black;

  this->AddFlagRGBPreset("Flag", 4, flag_colors, 8);

  // Flag (alternating red, orange, yellow, green, blue, violet)

  flag_colors[0] = red_pure;
  flag_colors[1] = orange;
  flag_colors[2] = yellow_pure;
  flag_colors[3] = green_pure;
  flag_colors[4] = blue_pure;
  flag_colors[5] = violet;

  this->AddFlagRGBPreset("Prism", 6, flag_colors, 6);
}

// ---------------------------------------------------------------------------
void vtkKWColorPresetSelector::Create(vtkKWApplication *app, 
                                      const char *args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created.");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create(app, args);

  if (!this->HasLabel() || !this->GetLabel()->GetLabel())
    {
    this->SetLabel("Color Presets:");
    }
  this->SetBalloonHelpString(
    "Preset transfer functions for mapping scalar value to color. Select a "
    "preset to use it.");

  this->OptionMenu->IndicatorOff();
  this->OptionMenu->SetImageOption(vtkKWIcon::ICON_FOLDER);

  this->PopulatePresetMenu();
}

//----------------------------------------------------------------------------
void vtkKWColorPresetSelector::PresetSelectedCallback(const char *name)
{
  if (!name || !this->ColorTransferFunction)
    {
    return;
    }

  vtkColorTransferFunction *preset_func = 
    this->GetPresetColorTransferFunction(name);
  double preset_range[2] = {0.0, 1.0};

  double scalar_range[2];
  if (this->ApplyPresetBetweenEndPoints &&
      this->ColorTransferFunction->GetSize() >= 2)
    {
    this->ColorTransferFunction->GetRange(scalar_range);
    }
  else
    {
    scalar_range[0] = this->ScalarRange[0];
    scalar_range[1] = this->ScalarRange[1];
    }

  if (this->MapColorTransferFunction(
        preset_func, preset_range, 
        this->ColorTransferFunction, scalar_range))
    {
    if (this->PresetSelectedCommand)
      {
      this->Script("eval %s", this->PresetSelectedCommand);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWColorPresetSelector::SetPreviewSize(int arg)
{
  if (arg < 3)
    {
    arg = 3;
    }

  if (this->PreviewSize == arg)
    {
    return;
    }

  this->PreviewSize = arg;

  this->Modified();

  this->PopulatePresetMenu();
}

//----------------------------------------------------------------------------
void vtkKWColorPresetSelector::SetHideSolidColorPresets(int arg)
{
  if (this->HideSolidColorPresets == arg)
    {
    return;
    }

  this->HideSolidColorPresets = arg;

  this->Modified();

  this->PopulatePresetMenu();
}

//----------------------------------------------------------------------------
void vtkKWColorPresetSelector::SetHideGradientPresets(int arg)
{
  if (this->HideGradientPresets == arg)
    {
    return;
    }

  this->HideGradientPresets = arg;

  this->Modified();

  this->PopulatePresetMenu();
}

// ---------------------------------------------------------------------------
void vtkKWColorPresetSelector::PopulatePresetMenu()
{
  if (!this->IsCreated())
    {
    return;
    }

  vtkKWMenu *menu = this->OptionMenu->GetMenu();
  menu->DeleteAllMenuItems();

  Tcl_Interp *interp = this->GetApplication()->GetMainInterp();

  vtkstd::string callback, preset_label, img_name;
  char func_addr[128];

  double *data_start, *data_ptr, *data_ptr_end;
  int count = 0;

  vtkKWColorPresetSelectorInternals::PresetContainerIterator it = 
    this->Internals->Presets.begin();
  vtkKWColorPresetSelectorInternals::PresetContainerIterator end = 
    this->Internals->Presets.end();
  for (; it != end; ++it)
    {
    if (it->ColorTransferFunction && it->Name.size())
      {
      // Check if the preset should be shown or not

      int show_preset = 1;
      if (this->HideSolidColorPresets || this->HideGradientPresets)
        {
        int is_solid_color = 1;
        data_start = it->ColorTransferFunction->GetDataPointer() + 1;
        data_ptr = data_start;
        data_ptr_end = data_ptr + it->ColorTransferFunction->GetSize() * 4;
        while (data_ptr < data_ptr_end)
          {
          if (data_ptr[0] != data_start[0] ||
              data_ptr[1] != data_start[1] ||
              data_ptr[2] != data_start[2])
            {
            is_solid_color = 0;
            break;
            }
          data_ptr += 4;
          }
        if ((is_solid_color && this->HideSolidColorPresets) ||
            (!is_solid_color && this->HideGradientPresets))
          {
          show_preset = 0;
          }
        }

      // Get the label and preview image name, add the entry

      preset_label = " ";
      preset_label += it->Name;
      preset_label += "   ";
      
      if (show_preset)
        {

        callback = "PresetSelectedCallback {";
        callback += it->Name;
        callback += "}";
        menu->AddCommand(preset_label.c_str(), this, callback.c_str());

        // Switch to next column every n-th preset

        ++count;
        if (!(count % 9))
          {
          menu->ConfigureItem(menu->GetIndex(preset_label.c_str()), 
                              "-columnbreak 1");
          }

        // Do we have a preview, if not create one, if yes check its size

        sprintf(func_addr, "%p", it->ColorTransferFunction);
        img_name = this->GetWidgetName();
        img_name += func_addr;

        int update_preview = 1;
        Tk_PhotoHandle photo = Tk_FindPhoto(interp, img_name.c_str());
        if (photo)
          {
          int img_height = vtkKWTkUtilities::GetPhotoHeight(
            interp, img_name.c_str());
          if (img_height == this->PreviewSize)
            {
            update_preview = 0;
            }
          }
        
        if (update_preview)
          {
          this->CreateColorTransferFunctionPreview(
            it->ColorTransferFunction, img_name.c_str());
          }
        
        menu->SetItemCompoundImage(preset_label.c_str(), img_name.c_str());
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWColorPresetSelector::CreateColorTransferFunctionPreview(
  vtkColorTransferFunction *func, const char *img_name)
{
  if (!this->IsCreated() || !func || !img_name || this->PreviewSize < 3)
    {
    return 0;
    }
  
  // Get a ramp (leave 2 pixels for the black frame border)

  const unsigned char *rgb_table = 
    func->GetTable(0.0, 1.0, this->PreviewSize - 2);

  // Allocate a buffer for the whole image

  int bytes_in_row = this->PreviewSize * 3;
  int buffer_length = this->PreviewSize * bytes_in_row;

  unsigned char *buffer = new unsigned char [buffer_length];
  unsigned char *ptr = buffer;

  // Fill the first row (black frame), it will be used for the last one too

  unsigned char *first_row = ptr;
  memset(first_row, 0, bytes_in_row);
  ptr += bytes_in_row;

  // Fill the second row (black frame border + ramp), it will be used for all
  // others row (but the last one)

  unsigned char *second_row = ptr;
  memset(second_row, 0, 3);
  memcpy(second_row + 3, rgb_table, (this->PreviewSize - 2) * 3);
  memset(second_row + bytes_in_row - 3, 0, 3);
  ptr += bytes_in_row;

  // Fill all rows but the last one, using the second row

  int j;
  for (j = 2; j < this->PreviewSize - 1; j++)
    {
    memcpy(ptr, second_row, bytes_in_row);
    ptr += bytes_in_row;
    }

  // Fill the last row using the first one

  memcpy(ptr, first_row, bytes_in_row);

  // Update the Tk image

  vtkKWTkUtilities::UpdatePhoto(this->GetApplication()->GetMainInterp(),
                                img_name,
                                buffer,
                                this->PreviewSize, this->PreviewSize, 3,
                                buffer_length);

  delete [] buffer;

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWColorPresetSelector::SetPresetSelectedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PresetSelectedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWColorPresetSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->ColorTransferFunction)
    {
    os << indent << "Color Transfer Function:\n";
    this->ColorTransferFunction->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Color Transfer Function: (none)\n";
    }

  os << indent << "ScalarRange: " 
     << this->ScalarRange[0] << "..." <<  this->ScalarRange[1] << endl;

  os << indent << "ApplyPresetBetweenEndPoints: " 
     << (this->ApplyPresetBetweenEndPoints ? "On" : "Off") << endl;

  os << indent << "HideSolidColorPresets: " 
     << (this->HideSolidColorPresets ? "On" : "Off") << endl;

  os << indent << "HideGradientPresets: " 
     << (this->HideGradientPresets ? "On" : "Off") << endl;

  os << indent << "PreviewSize: " << this->PreviewSize << endl;
}

