/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOrientScaleWidget.h
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
// .NAME vtkPVOrientScaleWidget - a widget for scaling and orientation
// .SECTION Description
// vtkPVOrientScaleWidget is used by the glyph filter to handle scaling and
// orienting the glyphs.  The scale factor depends on the scale mode and the
// selected scalars and vectors.

#ifndef __vtkPVOrientScaleWidget_h
#define __vtkPVOrientScaleWidget_h

#include "vtkPVWidget.h"

class vtkKWEntry;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWOptionMenu;
class vtkPVDataSetAttributesInformation;
class vtkPVInputMenu;
class vtkPVStringAndScalarListWidgetProperty;

class VTK_EXPORT vtkPVOrientScaleWidget : public vtkPVWidget
{
public:
  static vtkPVOrientScaleWidget* New();
  vtkTypeRevisionMacro(vtkPVOrientScaleWidget, vtkPVWidget);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Move widget state to vtk object or back.
  virtual void AcceptInternal(vtkClientServerID);
  virtual void ResetInternal();
  
  // Description:
  // Save this widget's state into a PVScript.  This method does not initialize
  // trace variable or check modified.
  virtual void Trace(ofstream *file);

  // Description:
  // Enable / disable the array menus depending on which orient and scale
  // modes have been selected.
  void UpdateActiveState();

  // Description:
  // Callbacks
  void ScaleModeMenuCallback();
  void ScalarsMenuEntryCallback();
  void VectorsMenuEntryCallback();
  
  // Description:
  // This input menu supplies the data set.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);
  
  // Description:
  // This is called to update the widget is something (InputMenu) changes.
  virtual void Update();
 
  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty* prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();

  // Description:
  // Set the VTK commands.
  vtkSetStringMacro(ScalarsCommand);
  vtkSetStringMacro(VectorsCommand);
  vtkSetStringMacro(OrientCommand);
  vtkSetStringMacro(ScaleModeCommand);
  vtkSetStringMacro(ScaleFactorCommand);

  // Description:
  // Set default widget values.
  vtkSetMacro(DefaultOrientMode, int);
  vtkSetMacro(DefaultScaleMode, int);
  
  // Description:
  // Methods to set the widgets' values from a script.
  void SetOrientMode(char *mode);
  void SetScaleMode(char *mode);
  void SetScalars(char *scalars);
  void SetVectors(char *vectors);
  void SetScaleFactor(float factor);
  
protected:
  vtkPVOrientScaleWidget();
  ~vtkPVOrientScaleWidget();

  vtkKWLabeledFrame *LabeledFrame;
  vtkKWWidget *ScalarsFrame;
  vtkKWLabel *ScalarsLabel;
  vtkKWOptionMenu *ScalarsMenu;
  vtkKWWidget *VectorsFrame;
  vtkKWLabel *VectorsLabel;
  vtkKWOptionMenu *VectorsMenu;
  vtkKWWidget *OrientModeFrame;
  vtkKWLabel *OrientModeLabel;
  vtkKWOptionMenu *OrientModeMenu;
  vtkKWWidget *ScaleModeFrame;
  vtkKWLabel *ScaleModeLabel;
  vtkKWOptionMenu *ScaleModeMenu;
  vtkKWWidget *ScaleFactorFrame;
  vtkKWLabel *ScaleFactorLabel;
  vtkKWEntry *ScaleFactorEntry;

  vtkPVInputMenu *InputMenu;
  char *ScalarArrayName;
  char *VectorArrayName;
  vtkSetStringMacro(ScalarArrayName);
  vtkSetStringMacro(VectorArrayName);

  vtkPVStringAndScalarListWidgetProperty *Property;

  char *ScalarsCommand;
  char *VectorsCommand;
  char *OrientCommand;
  char *ScaleModeCommand;
  char *ScaleFactorCommand;

  int DefaultOrientMode;
  int DefaultScaleMode;
  
//BTX
  virtual void CopyProperties(vtkPVWidget *clone, vtkPVSource *pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement *element,
                        vtkPVXMLPackageParser *parser);
  
  vtkPVDataSetAttributesInformation* GetPointDataInformation();
  void UpdateArrayMenus();
  void UpdateScaleFactor();
  
private:
  vtkPVOrientScaleWidget(const vtkPVOrientScaleWidget&); // Not implemented
  void operator=(const vtkPVOrientScaleWidget&); // Not implemented
};

#endif
