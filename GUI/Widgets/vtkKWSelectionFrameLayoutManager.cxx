/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "vtkKWSelectionFrameLayoutManager.h"

#include "vtkImageData.h"
#include "vtkKWApplication.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWSaveImageDialog.h"
#include "vtkKWSelectionFrame.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWToolbar.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkKWSimpleEntryDialog.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWEntry.h"

// Readers / Writers

#include "vtkErrorCode.h"
#include "vtkBMPWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkTIFFWriter.h"

#include "vtkWindowToImageFilter.h"
#include "vtkImageAppend.h"
#include "vtkImageConstantPad.h"

#include <vtkstd/list>
#include <vtkstd/vector>
#include <vtkstd/string>

#include "Resources/KWWindowLayout.h"

#define VTK_KW_SFLMGR_LABEL_PATTERN "%d x %d"
#define VTK_KW_SFLMGR_HELP_PATTERN "Set window layout to %d column(s) by %d row(s)"
#define VTK_KW_SFLMGR_ICON_PATTERN "KWWindowLayout%dx%d"
#define VTK_KW_SFLMGR_ICON_SELECTED_PATTERN "KWWindowLayout%dx%dc"
#define VTK_KW_SFLMGR_RESOLUTIONS {{ 1, 1}, { 1, 2}, { 2, 1}, { 2, 2}, { 2, 3}, { 3, 2}}
#define VTK_KW_SFLMGR_MAX_SIZE 100

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWSelectionFrameLayoutManager);
vtkCxxRevisionMacro(vtkKWSelectionFrameLayoutManager, "1.3");

//----------------------------------------------------------------------------
class vtkKWSelectionFrameLayoutManagerInternals
{
public:
  struct PoolNode
  {
    vtkstd::string Name;
    vtkKWSelectionFrame *Widget;
    int Position[2];
  };

  typedef vtkstd::vector<PoolNode> PoolType;
  typedef vtkstd::vector<PoolNode>::iterator PoolIterator;

  PoolType Pool;
};

//----------------------------------------------------------------------------
vtkKWSelectionFrameLayoutManager::vtkKWSelectionFrameLayoutManager()
{
  this->Internals = new vtkKWSelectionFrameLayoutManagerInternals;

  this->Window = NULL;

  this->Resolution[0] = 0;
  this->Resolution[1] = 0;
  
  this->ResolutionEntriesMenu    = NULL;
  this->ResolutionEntriesToolbar = NULL;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrameLayoutManager::~vtkKWSelectionFrameLayoutManager()
{
  // Delete all widgets

  this->DeleteAllWidgets();

  // Delete our pool

  delete this->Internals;

  // Delete the menu

  if (this->ResolutionEntriesMenu)
    {
    this->ResolutionEntriesMenu->Delete();
    this->ResolutionEntriesMenu = NULL;
    }

  // Delete the toolbar

  if (this->ResolutionEntriesToolbar)
    {
    this->ResolutionEntriesToolbar->Delete();
    this->ResolutionEntriesToolbar = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SetWindow(vtkKWWindow *arg)
{
  if (this->Window == arg)
    {
    return;
    }

  this->Window = arg;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::Create(vtkKWApplication *app, 
                                             const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", "-bg #333333"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ConfigureOptions(args);

  // Pack

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::Pack()
{
  if (!this->IsAlive())
    {
    return;
    }

  // Unpack everything

  this->UnpackChildren();

  // Pack each widgets, column first

  ostrstream tk_cmd;
  int i, j;

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget)
      {
      this->CreateWidget(it->Widget);
      if (it->Widget->IsCreated())
        {
        if (it->Position[0] < this->Resolution[0] && 
            it->Position[1] < this->Resolution[1])
          {
          tk_cmd << "grid " << it->Widget->GetWidgetName() 
                 << " -sticky news "
                 << " -column " << it->Position[0] 
                 << " -row " << it->Position[1] << endl;
          }
        }
      }
    }

  // columns and rows can resize
  // Make sure we reset the columns/rows that are not used (even if we
  // unpacked the children, those settings are kept since they are set
  // on the master)

  int nb_of_cols = 10, nb_of_rows = 10;
  vtkKWTkUtilities::GetGridSize(
    this->GetApplication()->GetMainInterp(),
    this->GetWidgetName(),
    &nb_of_cols, &nb_of_rows);

  for (j = 0; j < this->Resolution[1]; j++)
    {
    tk_cmd << "grid rowconfigure " << this->GetWidgetName() << " " << j 
           << " -weight 1" << endl;
    }
  for (j = this->Resolution[1]; j < nb_of_rows; j++)
    {
    tk_cmd << "grid rowconfigure " << this->GetWidgetName() << " " << j 
           << " -weight 0" << endl;
    }
  for (i = 0; i < this->Resolution[0]; i++)
    {
    tk_cmd << "grid columnconfigure " << this->GetWidgetName() << " " << i
           << " -weight 1" << endl;
    }
  for (i = this->Resolution[0]; i < nb_of_cols; i++)
    {
    tk_cmd << "grid columnconfigure " << this->GetWidgetName() << " " << i
           << " -weight 0" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SetWidgetPosition(
  const char *name, int pos[2])
{
  return this->SetWidgetPosition(this->GetWidget(name), pos);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SetWidgetPosition(
  vtkKWSelectionFrame *widget, int pos[2])
{
  if (pos && widget)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && it->Widget == widget)
        {
        it->Position[0] = pos[0];
        it->Position[1] = pos[1];
        this->Pack();
        return 1;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::GetWidgetPosition(
  const char *name, int pos[2])
{
  return this->GetWidgetPosition(this->GetWidget(name), pos);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::GetWidgetPosition(
  vtkKWSelectionFrame *widget, int pos[2])
{
  if (pos && widget)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && it->Widget == widget)
        {
        pos[0] = it->Position[0];
        pos[1] = it->Position[1];
        return 1;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* 
vtkKWSelectionFrameLayoutManager::GetWidgetAtPosition(int pos[2])
{
  if (pos)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && 
          it->Position[0] == pos[0] && it->Position[1] == pos[1])
        {
        return it->Widget;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::ReorganizeWidgetPositions()
{
  // Given the resolution, fill in the corresponding grid with 
  // widgets that have a valid position inside that grid

  vtkstd::vector<int> grid;
  grid.assign(this->Resolution[0] * this->Resolution[1], 0);

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget &&
        (it->Position[0] >= 0 && it->Position[0] < this->Resolution[0] && 
         it->Position[1] >= 0 && it->Position[1] < this->Resolution[1]))
      {
      grid[it->Position[1] * this->Resolution[0] + it->Position[0]] = 1;
      }
    }

  // Fill the holes in the grid with whatever widgets
  // which positions were out of the grid

  it = this->Internals->Pool.begin();
  int i, j;
  for (j = 0; j < this->Resolution[1] && it != end; j++)
    {
    for (i = 0; i < this->Resolution[0] && it != end; i++)
      {
      if (grid[j * this->Resolution[0] + i] == 0)
        {
        while (it != end)
          {
          if (it->Widget &&
              (it->Position[0] < 0 || it->Position[0] >= this->Resolution[0] ||
               it->Position[1] < 0 || it->Position[1] >= this->Resolution[1]))
            {
            it->Position[0] = i;
            it->Position[1] = j;
            ++it;
            break;
            }
          ++it;
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SetResolution(int i, int j)
{
  if (i < 0 || j < 0 || 
      (i == this->Resolution[0] && j == this->Resolution[1]))
    {
    return;
    }

  this->Resolution[0] = i;
  this->Resolution[1] = j;

  this->UpdateResolutionEntriesMenu();
  this->UpdateResolutionEntriesToolbar();

  this->ReorganizeWidgetPositions();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::AdjustResolution()
{
  int i = this->Resolution[0];
  int j = this->Resolution[1];

  int pool_size = (int)this->Internals->Pool.size();

  // Increase the resolution so that all widgets can potentially be shown
  // If there is the same number of row/column, add a row first

  while (pool_size && 
         (i * j) < pool_size)
    {
    if (i < j)
      {
      i++;
      }
    else
      {
      j++;
      }
    }

  // Decrease the resolution so that all widgets can potentially be shown
  // without extra columns or holes.
  // If there is the same number of row/column, remove a row first

  while (pool_size &&
         (pool_size <= ((i - 1) * j)  ||
          pool_size <= (i * (j - 1))))
    {
    if (i > j)
      {
      i--;
      }
    else
      {
      j--;
      }
    }

  this->SetResolution(i, j);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::CreateResolutionEntriesMenu(
  vtkKWMenu *parent)
{
  if (!parent)
    {
    return;
    }

  if (!this->ResolutionEntriesMenu)
    {
    this->ResolutionEntriesMenu = vtkKWMenu::New();
    }

  if (!this->ResolutionEntriesMenu->IsCreated())
    {
    this->ResolutionEntriesMenu->SetParent(parent);
    this->ResolutionEntriesMenu->Create(parent->GetApplication(), NULL);
    }

  // Allowed resolutions

  char *rbv = 
    this->ResolutionEntriesMenu->CreateRadioButtonVariable(this, "reschoice");

  char label[64], command[128], help[128];  

  int res[][2] = VTK_KW_SFLMGR_RESOLUTIONS;
  for (int idx = 0; idx < sizeof(res) / sizeof(res[0]); idx++)
    {
    sprintf(label, VTK_KW_SFLMGR_LABEL_PATTERN, 
            res[idx][0], res[idx][1]);
    sprintf(command, "SetResolution %d %d", res[idx][0], res[idx][1]);
    sprintf(help, VTK_KW_SFLMGR_HELP_PATTERN, 
            res[idx][0], res[idx][1]);
    int value = 
      ((res[idx][0] - 1) * VTK_KW_SFLMGR_MAX_SIZE + res[idx][1] - 1);
    this->ResolutionEntriesMenu->AddRadioButton(
      value, label, rbv, this, command, 0, help);
    }

  delete [] rbv;

  this->UpdateResolutionEntriesMenu();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::UpdateResolutionEntriesMenu()
{
  if (!this->ResolutionEntriesMenu ||
      !this->ResolutionEntriesMenu->IsCreated())
    {
    return;
    }

  // Enabled/Disabled some resolutions

  int normal_state = this->Enabled ? vtkKWMenu::Normal : vtkKWMenu::Disabled;
  size_t size = this->Internals->Pool.size();

  char label[64];

  int res[][2] = VTK_KW_SFLMGR_RESOLUTIONS;
  for (int idx = 0; idx < sizeof(res) / sizeof(res[0]); idx++)
    {
    sprintf(label, VTK_KW_SFLMGR_LABEL_PATTERN, res[idx][0], res[idx][1]);
    this->ResolutionEntriesMenu->SetState(
      label, 
      (size_t)(res[idx][0] * res[idx][1]) <= 
      (size + (res[idx][0] != 1 && res[idx][1] != 1 ? 1 : 0))
      ? normal_state : vtkKWMenu::Disabled);
    }

  // Select the right one

  int value = 
    (this->Resolution[0]-1) * VTK_KW_SFLMGR_MAX_SIZE + this->Resolution[1]-1;
  this->ResolutionEntriesMenu->CheckRadioButton(this, "reschoice", value);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::CreateResolutionEntriesToolbar(
  vtkKWWidget *parent)
{
  if (!parent)
    {
    return;
    }

  if (!this->ResolutionEntriesToolbar)
    {
    this->ResolutionEntriesToolbar = vtkKWToolbar::New();
    }

  if (!this->ResolutionEntriesToolbar->IsCreated())
    {
    this->ResolutionEntriesToolbar->SetParent(parent);
    this->ResolutionEntriesToolbar->Create(parent->GetApplication());
    }

  // Got to create the icons

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout1x1",
    NULL,
    NULL,
    image_KWWindowLayout1x1, 
    image_KWWindowLayout1x1_width, 
    image_KWWindowLayout1x1_height,
    image_KWWindowLayout1x1_pixel_size,
    image_KWWindowLayout1x1_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout1x1c",
    NULL,
    NULL,
    image_KWWindowLayout1x1c, 
    image_KWWindowLayout1x1c_width, 
    image_KWWindowLayout1x1c_height,
    image_KWWindowLayout1x1c_pixel_size,
    image_KWWindowLayout1x1c_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout1x2",
    NULL,
    NULL,
    image_KWWindowLayout1x2, 
    image_KWWindowLayout1x2_width, 
    image_KWWindowLayout1x2_height,
    image_KWWindowLayout1x2_pixel_size,
    image_KWWindowLayout1x2_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout1x2c",
    NULL,
    NULL,
    image_KWWindowLayout1x2c, 
    image_KWWindowLayout1x2c_width, 
    image_KWWindowLayout1x2c_height,
    image_KWWindowLayout1x2c_pixel_size,
    image_KWWindowLayout1x2c_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout2x1",
    NULL,
    NULL,
    image_KWWindowLayout2x1, 
    image_KWWindowLayout2x1_width, 
    image_KWWindowLayout2x1_height,
    image_KWWindowLayout2x1_pixel_size,
    image_KWWindowLayout2x1_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout2x1c",
    NULL,
    NULL,
    image_KWWindowLayout2x1c, 
    image_KWWindowLayout2x1c_width, 
    image_KWWindowLayout2x1c_height,
    image_KWWindowLayout2x1c_pixel_size,
    image_KWWindowLayout2x1c_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout2x2",
    NULL,
    NULL,
    image_KWWindowLayout2x2, 
    image_KWWindowLayout2x2_width, 
    image_KWWindowLayout2x2_height,
    image_KWWindowLayout2x2_pixel_size,
    image_KWWindowLayout2x2_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout2x2c",
    NULL,
    NULL,
    image_KWWindowLayout2x2c, 
    image_KWWindowLayout2x2c_width, 
    image_KWWindowLayout2x2c_height,
    image_KWWindowLayout2x2c_pixel_size,
    image_KWWindowLayout2x2c_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout2x3",
    NULL,
    NULL,
    image_KWWindowLayout2x3, 
    image_KWWindowLayout2x3_width, 
    image_KWWindowLayout2x3_height,
    image_KWWindowLayout2x3_pixel_size,
    image_KWWindowLayout2x3_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout2x3c",
    NULL,
    NULL,
    image_KWWindowLayout2x3c, 
    image_KWWindowLayout2x3c_width, 
    image_KWWindowLayout2x3c_height,
    image_KWWindowLayout2x3c_pixel_size,
    image_KWWindowLayout2x3c_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout3x2",
    NULL,
    NULL,
    image_KWWindowLayout3x2, 
    image_KWWindowLayout3x2_width, 
    image_KWWindowLayout3x2_height,
    image_KWWindowLayout3x2_pixel_size,
    image_KWWindowLayout3x2_buffer_length);

  vtkKWTkUtilities::UpdateOrLoadPhoto(
    parent->GetApplication()->GetMainInterp(),
    "KWWindowLayout3x2c",
    NULL,
    NULL,
    image_KWWindowLayout3x2c, 
    image_KWWindowLayout3x2c_width, 
    image_KWWindowLayout3x2c_height,
    image_KWWindowLayout3x2c_pixel_size,
    image_KWWindowLayout3x2c_buffer_length);

  // Allowed resolutions

  char *rbv = 
    this->ResolutionEntriesMenu->CreateRadioButtonVariable(this, "reschoice");

  char command[128], help[128], icon[128], icon_selected[128];  

  int res[][2] = VTK_KW_SFLMGR_RESOLUTIONS;
  for (int idx = 0; idx < sizeof(res) / sizeof(res[0]); idx++)
    {
    sprintf(command, "SetResolution %d %d", res[idx][0], res[idx][1]);
    sprintf(help, VTK_KW_SFLMGR_HELP_PATTERN, 
            res[idx][0], res[idx][1]);
    sprintf(icon, VTK_KW_SFLMGR_ICON_PATTERN, 
            res[idx][0], res[idx][1]);
    sprintf(icon, VTK_KW_SFLMGR_ICON_PATTERN, 
            res[idx][0], res[idx][1]);
    sprintf(icon_selected, VTK_KW_SFLMGR_ICON_SELECTED_PATTERN, 
            res[idx][0], res[idx][1]);
    int value = 
      ((res[idx][0] - 1) * VTK_KW_SFLMGR_MAX_SIZE + res[idx][1] - 1);
    this->ResolutionEntriesToolbar->AddRadioButtonImage(
      value, icon, icon_selected, rbv, this, command, help);
    }

  delete [] rbv;

  this->UpdateResolutionEntriesToolbar();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::UpdateResolutionEntriesToolbar()
{
  if (!this->ResolutionEntriesToolbar ||
      !this->ResolutionEntriesToolbar->IsCreated())
    {
    return;
    }

  // Enabled/Disabled some resolutions

  size_t size = this->Internals->Pool.size();
  char icon[128];

  int res[][2] = VTK_KW_SFLMGR_RESOLUTIONS;
  for (int idx = 0; idx < sizeof(res) / sizeof(res[0]); idx++)
    {
    sprintf(icon, VTK_KW_SFLMGR_ICON_PATTERN, res[idx][0], res[idx][1]);
    vtkKWWidget *w = this->ResolutionEntriesToolbar->GetWidget(icon);
    if (w)
      {
      w->SetEnabled(
        (size_t)(res[idx][0] * res[idx][1]) <= 
        (size + (res[idx][0] != 1 && res[idx][1] != 1 ? 1 : 0)) 
        ? this->Enabled : 0);
      }
    }

  // Select the right one

  int value = 
    (this->Resolution[0]-1) * VTK_KW_SFLMGR_MAX_SIZE + this->Resolution[1]-1;
  this->ResolutionEntriesMenu->CheckRadioButton(this, "reschoice", value);
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetWidget(
  const char *name)
{
  if (name)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && !it->Name.compare(name))
        {
        return it->Widget;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetNthWidget(
  int index)
{
  if (index < 0 || index >= (int)this->Internals->Pool.size())
    {
    return NULL;
    }

#if 0  
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  while (index != 0)
    {
    ++it;
    index--;
    }
  return it->Widget;
#else
  return this->Internals->Pool[index].Widget;
#endif
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetWidgetWithTitle(
  const char *title)
{
  if (title)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget && 
          it->Widget->GetTitle() && 
          !strcmp(title, it->Widget->GetTitle()))
        {
        return it->Widget;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
const char * vtkKWSelectionFrameLayoutManager::GetNthWidgetName(
  int index)
{
  if (index < 0 || index >= (int)this->Internals->Pool.size())
    {
    return NULL;
    }
  
#if 0
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  while (index != 0)
    {
    ++it;
    index--;
    }
  return it->Name.c_str();
#else
  return this->Internals->Pool[index].Name.c_str();
#endif
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::GetNumberOfWidgets()
{
  return this->Internals->Pool.size();
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::HasWidget(const char *name)
{
  return this->GetWidget(name) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::HasWidget(
  vtkKWSelectionFrame *widget)
{
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget == widget)
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::AddWidget(
  const char *name)
{
  if (!name || !*name)
    {
    return NULL;
    }

  // If we have that widget already

  vtkKWSelectionFrame *widget = this->GetWidget(name);
  if (widget)
    {
    return NULL;
    }

  // Create a new node and allocate a new widget

  vtkKWSelectionFrameLayoutManagerInternals::PoolNode node;
  node.Name.assign(name); 
  node.Widget = this->AllocateNewWidget();
  node.Widget->Register(this);
  node.Widget->Delete();

  // Create the widget, set its window title, callback

  this->CreateWidget(node.Widget);
  node.Widget->SetTitle(name);
  node.Widget->SetSelectionListCommand(this, "SwitchWidgetCallback");

  // Unitialize its position. It will be updated automatically the first
  // time this widget is packed.

  node.Position[0] = node.Position[1] = -1;

  // Add it to the pool

  this->Internals->Pool.push_back(node);

  this->NumberOfWidgetsHasChanged();

  return node.Widget;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::AllocateNewWidget()
{
  return vtkKWSelectionFrame::New();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::CreateWidget(
  vtkKWSelectionFrame *widget)
{
  if (this->IsCreated() && widget && !widget->IsCreated())
    {
    widget->SetParent(this);
    widget->Create(this->GetApplication(), "-width 350 -height 350");
    this->PropagateEnableState(widget);
    widget->SetCloseCommand(
      this, "CloseWidgetCallback");
    widget->ShowCloseOn();
    widget->SetChangeTitleCommand(
      this, "ChangeWidgetTitleCallback");
    widget->ShowChangeTitleOn();
    widget->SetSelectCommand(
      this, "SelectWidgetCallback");
    widget->SetDoubleClickCommand(
      this, "SelectAndMaximizeWidgetCallback");
    }
}

//----------------------------------------------------------------------------
vtkKWRenderWidget* vtkKWSelectionFrameLayoutManager::GetVisibleRenderWidget(
  vtkKWSelectionFrame *widget)
{
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::NumberOfWidgetsHasChanged()
{
  // Update all selection lists, so that this new widget can be selected

  this->UpdateSelectionLists();

  // Adjust the resolution

  this->AdjustResolution();
  this->ReorganizeWidgetPositions();
  this->UpdateEnableState();

  // Pack

  this->Pack();
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::DeleteWidget(
  vtkKWSelectionFrame *widget)
{
  if (this->Internals && widget)
    {
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget == widget)
        {
        vtkKWSelectionFrame *sel = this->GetSelectedWidget();
        this->Internals->Pool.erase(it);
        if (sel == widget)
          {
          this->SelectWidget(this->GetNthWidget(0));
          }
        widget->Delete();
        this->NumberOfWidgetsHasChanged();
        return 1;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::DeleteWidget(const char *name)
{
  return this->DeleteWidget(this->GetWidget(name));
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::DeleteAllWidgets()
{
  if (this->Internals)
    {
    this->SelectWidget((vtkKWSelectionFrame*)NULL);
    
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
      this->Internals->Pool.begin();
    vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
      this->Internals->Pool.end();
    for (; it != end; ++it)
      {
      if (it->Widget)
        {
        it->Widget->Delete();
        }
      }
    
    this->Internals->Pool.clear();
    this->NumberOfWidgetsHasChanged();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::RenameWidget(
  vtkKWSelectionFrame *widget, 
  const char *new_name)
{
  // Valid new name ?

  if (!widget || !new_name || !*new_name)
    {
    return 0;
    }

  // If we have a widget with that name already

  if (this->GetWidget(new_name))
    {
    return 0;
    }

  // OK, rename it

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget == widget)
      {
      it->Name = new_name;
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::RenameWidget(
  const char *old_name, 
  const char *new_name)
{
  return this->RenameWidget(this->GetWidget(old_name), new_name);
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame* vtkKWSelectionFrameLayoutManager::GetSelectedWidget()
{
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget->GetSelected())
      {
      return it->Widget;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkKWSelectionFrameLayoutManager::GetSelectedWidgetName()
{
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget->GetSelected())
      {
      return it->Name.c_str();
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SelectWidget(
  vtkKWSelectionFrame *widget)
{
  // Deselect all widgets and select the right one (if any)

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget && it->Widget != widget)
      {
      it->Widget->SelectedOff();
      }
    }
  if (widget)
    {
    widget->SelectedOn();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SelectWidget(const char *name)
{
  this->SelectWidget(this->GetWidget(name));
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SelectWidgetCallback(
  vtkKWSelectionFrame *selection)
{
  this->SelectWidget(selection);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SelectAndMaximizeWidgetCallback(
  vtkKWSelectionFrame *selection)
{
  // Select the widget to maximize
  // Set the resolution to full (1, 1)
  // then switch whichever dataset was at [0, 0] with our selection

  this->SelectWidget(selection);

  this->SetResolution(1, 1);

  int pos[2] = {0, 0};
  this->SwitchWidgetCallback(selection->GetTitle(), 
                             this->GetWidgetAtPosition(pos));
}

//---------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::SwitchWidgetCallback(
  const char *title, vtkKWSelectionFrame *widget)
{
  // Get the widget we want to see in place of the current
  // widget

  vtkKWSelectionFrame *new_widget = this->GetWidgetWithTitle(title);
  if (!new_widget || new_widget == widget)
    {
    return;
    }

  // Switch both

  int pos[2], new_pos[2];
  if (!this->GetWidgetPosition(widget, pos) ||
      !this->GetWidgetPosition(new_widget, new_pos))
    {
    return;
    }
  
  this->SetWidgetPosition(widget, new_pos);
  this->SetWidgetPosition(new_widget, pos);

  // Select the new one

  new_widget->SelectCallback();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::CloseWidgetCallback(
  vtkKWSelectionFrame *widget)
{
  this->DeleteWidget(widget);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::ChangeWidgetTitleCallback(
  vtkKWSelectionFrame *widget)
{
  if (!widget)
    {
    return 0;
    }

  // Create a dialog to ask for a new title

  vtkKWSimpleEntryDialog *dlg = vtkKWSimpleEntryDialog::New();
  dlg->SetMasterWindow(this->Window);
  dlg->InvokeAtPointerOn();
  dlg->SetTitle("Change frame title");
  dlg->SetStyleToOkCancel();
  dlg->Create(this->GetApplication(), 0);
  dlg->GetEntry()->SetLabel("Name:");
  dlg->SetText("Enter a new value for this frame title");

  int ok = dlg->Invoke();
  if (ok)
    {
    char *new_title = dlg->GetEntry()->GetEntry()->GetValue();
    ok = this->CanWidgetTitleBeChanged(widget, new_title);
    if (!ok)
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetApplication(), this->Window, "Change frame title - Error",
        "There was a problem with the new title you provided.\n",
        vtkKWMessageDialog::ErrorIcon);
      }
    else
      {
      widget->SetTitle(new_title);
      this->UpdateSelectionLists();
      }
    }

  dlg->Delete();
  return ok; 
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::CanWidgetTitleBeChanged(
    vtkKWSelectionFrame *widget, const char *new_title)
{
  return (widget && 
          new_title && 
          *new_title && 
          (!widget->GetTitle() || strcmp(widget->GetTitle(), new_title)));
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    this->PropagateEnableState(it->Widget);
    }

  this->PropagateEnableState(this->ResolutionEntriesMenu);
  this->PropagateEnableState(this->ResolutionEntriesToolbar);

  // Enable/Disable some entries

  this->UpdateResolutionEntriesMenu();
  this->UpdateResolutionEntriesToolbar();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::UpdateSelectionLists()
{
  if (!this->Internals ||
      !this->Internals->Pool.size())
    {
    return;
    }
  
  // Allocate array of names

  const char **names_list = new const char *[this->Internals->Pool.size()];

  // Set the selection list for each widget

  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it = 
    this->Internals->Pool.begin();
  vtkKWSelectionFrameLayoutManagerInternals::PoolIterator end = 
    this->Internals->Pool.end();
  for (; it != end; ++it)
    {
    if (it->Widget)
      {
      int nb_names = 0;
      vtkKWSelectionFrameLayoutManagerInternals::PoolIterator it2 = 
        this->Internals->Pool.begin();
      for (; it2 != end; ++it2)
        {
        if (it2->Widget &&
            it2->Widget != it->Widget &&
            it2->Widget->GetTitle())
          {
          names_list[nb_names++] = it2->Widget->GetTitle();
          }
        }
      it->Widget->SetSelectionList(nb_names, names_list);
      }
    }

  // Free names

  delete [] names_list;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AppendWidgetsToImageData(
  vtkImageData *image, int selection_only)
{
  int nb_slots = this->Resolution[0] * this->Resolution[1];

  // We need a window to image filter for each widget in the grid

  vtkstd::vector<vtkWindowToImageFilter*> w2i_filters;
  w2i_filters.assign(nb_slots, NULL);

  // We also need a pad filter to add a small margin for each widget 

  vtkstd::vector<vtkImageConstantPad*> pad_filters;
  pad_filters.assign(nb_slots, NULL);

  // We need an append filter for each row in the grid, to append
  // widgets horizontally

  vtkstd::vector<vtkImageAppend*> append_filters;
  append_filters.assign(this->Resolution[1], NULL);

  // We need an append filter to append each rows (see above) and form
  // the final picture

  vtkImageAppend *append_all = vtkImageAppend::New();
  append_all->SetAppendAxis(1);

  int spacing = 4;

  // Build the whole pipeline

  int i, j;
  for (j = this->Resolution[1] - 1; j >= 0; j--)
    {
    append_filters[j] = vtkImageAppend::New();
    append_filters[j]->SetAppendAxis(0);
    for (i = 0; i < this->Resolution[0]; i++)
      {
      int pos[2]; pos[0] = i; pos[1] = j;
      vtkKWSelectionFrame *widget = this->GetWidgetAtPosition(pos);
      if (widget && (!selection_only || widget->GetSelected()))
        {
        vtkKWRenderWidget *rwwidget = this->GetVisibleRenderWidget(widget);
        if (rwwidget)
          {
          int offscreen = rwwidget->GetOffScreenRendering();
          rwwidget->SetOffScreenRendering(1);
          int idx = j * this->Resolution[0] + i;
          w2i_filters[idx] = vtkWindowToImageFilter::New();
          w2i_filters[idx]->SetInput(rwwidget->GetRenderWindow());
          w2i_filters[idx]->Update();
          rwwidget->SetOffScreenRendering(offscreen);

          int ext[6];
          w2i_filters[idx]->GetOutput()->GetWholeExtent(ext);
          pad_filters[idx] = vtkImageConstantPad::New();
          pad_filters[idx]->SetInput(w2i_filters[idx]->GetOutput());
          pad_filters[idx]->SetConstant(255);
          pad_filters[idx]->SetOutputWholeExtent(
            ext[0] - spacing, ext[1] + spacing,
            ext[2] - spacing, ext[3] + spacing,
            ext[4], ext[5]);
          pad_filters[idx]->Update();

          append_filters[j]->AddInput(pad_filters[idx]->GetOutput());
          }
        }
      }

    if (append_filters[j]->GetNumberOfInputConnections(0))
      {
      append_all->AddInput(append_filters[j]->GetOutput());
      append_filters[j]->Update();
      }
    }

  // Create the final output

  if (append_all->GetNumberOfInputConnections(0))
    {
    append_all->Update();
    image->ShallowCopy(append_all->GetOutput());
    }

  // Deallocate

  append_all->Delete();
  
  for (j = 0; j < this->Resolution[1]; j++)
    {
    append_filters[j]->Delete();
    for (i = 0; i < this->Resolution[0]; i++)
      {
      int pos[2]; pos[0] = i; pos[1] = j;
      vtkKWSelectionFrame *widget = this->GetWidgetAtPosition(pos);
      if (widget && (!selection_only || widget->GetSelected()))
        {
        vtkKWRenderWidget *rwwidget = this->GetVisibleRenderWidget(widget);
        if (rwwidget)
          {
          rwwidget->Render();
          }
        }
      int idx = j * this->Resolution[0] + i;
      if (w2i_filters[idx])
        {
        w2i_filters[idx]->Delete();
        }
      if (pad_filters[idx])
        {
        pad_filters[idx]->Delete();
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AppendAllWidgetsToImageData(
  vtkImageData *image)
{
  return this->AppendWidgetsToImageData(image, 0);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::AppendSelectedWidgetToImageData(
  vtkImageData *image)
{
  return this->AppendWidgetsToImageData(image, 1);
}

//---------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SaveScreenshotAllWidgets()
{
  if (!this->Window || !this->IsCreated())
    {
    return 0;
    }

  vtkKWSaveImageDialog *save_dialog = vtkKWSaveImageDialog::New();
  save_dialog->SetParent(this->Window);
  save_dialog->Create(this->GetApplication(), NULL);
  save_dialog->SetTitle("Save Screenshot");
  this->Window->RetrieveLastPath(save_dialog, "SavePath");
  
  int res = 0;
  if (save_dialog->Invoke() && 
      this->SaveScreenshotAllWidgets(save_dialog->GetFileName()))
    {
    this->Window->SaveLastPath(save_dialog, "SavePath");
    res = 1;
    }

  save_dialog->Delete();

  return res;
}

//---------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::SaveScreenshotAllWidgets(
  const char* fname)
{
  if (!fname)
    {
    return 0;
    }

  // Append all widgets to an image

  vtkImageData *iData = vtkImageData::New();
  if (!this->AppendAllWidgetsToImageData(iData))
    {
    iData->Delete();
    return 0;
    }

  int extent[6];
  iData->GetExtent(extent);
  if (extent[0] > extent[1] && extent[2] > extent[3] && extent[4] > extent[5])
    {
    iData->Delete();
    return 0;
    }

  // Now save it

  const char *ext = fname + strlen(fname) - 4;
  
  int success = 1;

  if (!strcmp(ext, ".bmp"))
    {
    vtkBMPWriter *bmp = vtkBMPWriter::New();
    bmp->SetInput(iData);
    bmp->SetFileName(fname);
    bmp->Write();
    if (bmp->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    bmp->Delete();
    }
  else if (!strcmp(ext, ".tif"))
    {
    vtkTIFFWriter *tif = vtkTIFFWriter::New();
    tif->SetInput(iData);
    tif->SetFileName(fname);
    tif->Write();
    if (tif->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    tif->Delete();
    }
  else if (!strcmp(ext, ".ppm"))
    {
    vtkPNMWriter *pnm = vtkPNMWriter::New();
    pnm->SetInput(iData);
    pnm->SetFileName(fname);
    pnm->Write();
    if (pnm->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    pnm->Delete();
    }
  else if (!strcmp(ext, ".png"))
    {
    vtkPNGWriter *png = vtkPNGWriter::New();
    png->SetInput(iData);
    png->SetFileName(fname);
    png->Write();
    if (png->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    png->Delete();
    }
  else if (!strcmp(ext, ".jpg"))
    {
    vtkJPEGWriter *jpg = vtkJPEGWriter::New();
    jpg->SetInput(iData);
    jpg->SetFileName(fname);
    jpg->Write();
    if (jpg->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
      success = 0;
      }
    jpg->Delete();
    }
  
  if (!success)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this->Window, "Write Error",
      "There was a problem writing the image file.\n"
      "Please check the location and make sure you have write\n"
      "permissions and enough disk space.",
      vtkKWMessageDialog::ErrorIcon);
    }
  iData->Delete();

  return success;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::CopyScreenshotAllWidgetsToClipboard()
{
  // Append all widgets to an image

  vtkImageData *iData = vtkImageData::New();
  if (!this->AppendAllWidgetsToImageData(iData))
    {
    iData->Delete();
    return 0;
    }

  int *extent = iData->GetExtent();
  if (extent[0] > extent[1] && extent[2] > extent[3] && extent[4] > extent[5])
    {
    iData->Delete();
    return 0;
    }

  // Save to clipboard

#ifdef _WIN32

  vtkKWSelectionFrame *widget = this->GetSelectedWidget();
  if (!widget)
    {
    return 0;
    }

  vtkKWRenderWidget *rwwidget = this->GetVisibleRenderWidget(widget);
  if (!rwwidget)
    {
    return 0;
    }

  if (::OpenClipboard((HWND)rwwidget->GetRenderWindow()->GetGenericWindowId()))
    {
    extent = iData->GetWholeExtent();

    int size[2];
    size[0] = extent[1] - extent[0] + 1;
    size[1] = extent[3] - extent[2] + 1;

    int data_width = ((size[0] * 3 + 3) / 4) * 4;
    int src_width = size[0] * 3;
  
    EmptyClipboard();

    DWORD dwLen = sizeof(BITMAPINFOHEADER) + data_width * size[1];
    HANDLE hDIB = ::GlobalAlloc(GHND, dwLen);
    LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(hDIB);
    
    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth = size[0];
    lpbi->biHeight = size[1];
    lpbi->biPlanes = 1;
    lpbi->biBitCount = 24;
    lpbi->biCompression = BI_RGB;
    lpbi->biClrUsed = 0;
    lpbi->biClrImportant = 0;
    lpbi->biSizeImage = data_width * size[1];
    
    // Copy the data to the clipboard

    unsigned char *ptr = (unsigned char *)(iData->GetScalarPointer());
    unsigned char *dest = (unsigned char *)lpbi + lpbi->biSize;

    int i,j;
    for (i = 0; i < size[1]; i++)
      {
      for (j = 0; j < size[0]; j++)
        {
        *dest++ = ptr[2];
        *dest++ = ptr[1];
        *dest++ = *ptr;
        ptr += 3;
        }
      dest = dest + (data_width - src_width);
      }
    
    SetClipboardData (CF_DIB, hDIB);
    ::GlobalUnlock(hDIB);
    CloseClipboard();
    }           
#endif

  iData->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintWidgets(
  double dpi, int selection_only)
{
#ifdef _WIN32

  vtkKWSelectionFrame *first_widget = this->GetNthWidget(0);
  if (!first_widget)
    {
    }

  PRINTDLG pd;
  DOCINFO di;
  RECT rcDest = { 0, 0, 0, 0};
  
  memset((void *)&pd, 0, sizeof(PRINTDLG));

  pd.lStructSize = sizeof(PRINTDLG);
  vtkKWRenderWidget *first_rwwidget = 
    this->GetVisibleRenderWidget(first_widget);
  if (first_rwwidget)
    {
    pd.hwndOwner = (HWND)first_rwwidget->GetRenderWindow()->GetGenericWindowId();
    }
  pd.Flags = PD_RETURNDC;
  pd.hInstance = NULL;
  
  PrintDlg(&pd);
  HDC ghdc = pd.hDC;

  if (!ghdc)
    {
    return 0;
    }

  if (pd.hDevMode)
    {
    GlobalFree(pd.hDevMode);
    }
  if (pd.hDevNames)
    {
    GlobalFree(pd.hDevNames);
    }
  
  if (this->IsCreated())
    {
    this->Script("%s configure -cursor watch; update", this->GetWidgetName());
    }
  
  di.cbSize = sizeof(DOCINFO);
  di.lpszDocName = "Kitware Test";
  di.lpszOutput = NULL;
  
  StartDoc(ghdc, &di);
  StartPage(ghdc);

  // Get size of printer page in pixels

  int cxPage = GetDeviceCaps(ghdc, HORZRES);
  int cyPage = GetDeviceCaps(ghdc, VERTRES);

  // Get printer DPI

  int cxInch = GetDeviceCaps(ghdc, LOGPIXELSX);
  int cyInch = GetDeviceCaps(ghdc, LOGPIXELSY);

  double scale = (double)cxInch / dpi;
  
  SetStretchBltMode(ghdc, HALFTONE);
 
  // If only the selection is to be printed, set the res to 1, 1

  int res[2];
  if (selection_only)
    {
    res[0] = res[1] = 1;
    }
  else
    {
    res[0] = this->Resolution[0];
    res[1] = this->Resolution[1];
    }

  // First pass to compute the total size (i.e. the resolution * biggest win)

  int max_size[2] = { -1, -1 };

  int i, j;
  for (j = 0; j < this->Resolution[1]; j++)
    {
    for (i = 0; i < this->Resolution[0]; i++)
      {
      int pos[2]; pos[0] = i; pos[1] = j;
      vtkKWSelectionFrame *widget = this->GetWidgetAtPosition(pos);
      if (widget && (!selection_only || widget->GetSelected()))
        {
        vtkKWRenderWidget *rwwidget = this->GetVisibleRenderWidget(widget);
        if (rwwidget)
          {
          int *size = rwwidget->GetRenderWindow()->GetSize();
          if (max_size[0] < size[0])
            {
            max_size[0] = size[0];
            }
          if (max_size[1] < size[1])
            {
            max_size[1] = size[1];
            }
          }
        }
      }
    }

  int spacing = 4;

  int total_size[2];
  total_size[0] = res[0] * (max_size[0] + 2 * spacing);
  total_size[1] = res[1] * (max_size[1] + 2 * spacing);

  double ratio[2];
  ratio[0] = (double)max_size[0] / (double)total_size[0];
  ratio[1] = (double)max_size[1] / (double)total_size[1];

  // Print each widget (or the selection only)

  for (j = 0; j < this->Resolution[1]; j++)
    {
    for (i = 0; i < this->Resolution[0]; i++)
      {
      int pos[2]; pos[0] = i; pos[1] = j;
      vtkKWSelectionFrame *widget = this->GetWidgetAtPosition(pos);
      if (widget && (!selection_only || widget->GetSelected()))
        {
        vtkKWRenderWidget *rwwidget = this->GetVisibleRenderWidget(widget);
        if (rwwidget)
          {
          int i2, j2;
          if (selection_only)
            {
            i2 = j2 = 0;
            }
          else
            {
            i2 = i;
            j2 = j;
            }
          int printing = rwwidget->GetPrinting();
          rwwidget->SetPrinting(1);
          rwwidget->SetupPrint(
            rcDest, ghdc, cxPage, cyPage, cxInch, cyInch,
            ratio[0], ratio[1], total_size[0], total_size[1]);
          rwwidget->Render();

          StretchBlt(
            ghdc, 
            (double)rcDest.right * 
            (spacing + i2 * (max_size[0] + 2 * spacing))/(double)total_size[0],
            (double)rcDest.top * 
            (spacing + j2 * (max_size[1] + 2 * spacing))/(double)total_size[1],
            (double)rcDest.right * ratio[0], 
            (double)rcDest.top * ratio[1],
            (HDC)rwwidget->GetMemoryDC(), 
            0, 
            0,
            (double)rcDest.right / scale * ratio[0], 
            (double)rcDest.top / scale * ratio[1], 
            SRCCOPY);

          rwwidget->SetPrinting(printing);
          }
        }
      }
    }
  
  // Close the page

  EndPage(ghdc);
  EndDoc(ghdc);
  DeleteDC(ghdc);

  if (this->IsCreated())
    {
    this->Script("%s configure -cursor {}", this->GetWidgetName());
    }

  // At that point the Print Dialog does not seem to disappear.
  // Let's Render()

  if (this->IsCreated())
    {
    this->Script("update");
    }

#endif

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintAllWidgets()
{
  if (this->Window)
    {
    return this->PrintAllWidgets(this->Window->GetPrintTargetDPI());
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintAllWidgets(double dpi)
{
  return this->PrintWidgets(dpi, 0);
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintSelectedWidget()
{
  if (this->Window)
    {
    return this->PrintSelectedWidget(this->Window->GetPrintTargetDPI());
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrameLayoutManager::PrintSelectedWidget(double dpi)
{
  return this->PrintWidgets(dpi, 1);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrameLayoutManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Window: " << this->Window << endl;

  os << indent << "Resolution: " << this->Resolution[0] << " x " 
     << this->Resolution[1] << endl;

  os << indent << "ResolutionEntriesMenu: " << this->ResolutionEntriesMenu << endl;
  os << indent << "ResolutionEntriesToolbar: " << this->ResolutionEntriesToolbar << endl;
}
