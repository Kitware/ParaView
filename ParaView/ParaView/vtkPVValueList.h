/*=========================================================================

  Program:   ParaView
  Module:    vtkPVValueList.h
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
// .NAME vtkPVValueList maintains a list of floats.
// .SECTION Description
// This widget lets the user add or delete floats from a list.
// It is used for contours, cut and clip plane offsets..

#ifndef __vtkPVValueList_h
#define __vtkPVValueList_h

#include "vtkPVWidget.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWListBox;
class vtkKWPushButton;
class vtkKWRange;
class vtkKWScale;
class vtkPVContourWidgetProperty;

class VTK_EXPORT vtkPVValueList : public vtkPVWidget
{
public:
  static vtkPVValueList* New();
  vtkTypeRevisionMacro(vtkPVValueList, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the label.  The label can be used to get this widget
  // from a script.
  void SetLabel (const char* label);
  const char* GetLabel();
  
  // Description:
  // Access to this widget from a script.
  void AddValue(float val);
  void RemoveAllValues();

  // Description:
  // Button callbacks.
  void AddValueCallback();
  void DeleteValueCallback();
  void DeleteAllValuesCallback();
  void GenerateValuesCallback();

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Gets called when the accept button is pressed. The sub-classes
  // should first call this and then do their own thing.
  virtual void AcceptInternal(const char* sourceTclName);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Update UI from Property object. This is an internal
  // method to be only used by the tracing interface. Use at
  // your own risk.
  void Update();
  
protected:
  vtkPVValueList();
  ~vtkPVValueList();

  static const int MAX_NUMBER_ENTRIES;

  vtkPVContourWidgetProperty *Property;
  
  vtkKWLabeledFrame* ContourValuesFrame;
  vtkKWFrame* ContourValuesFrame2;
  vtkKWListBox* ContourValuesList;

  vtkKWFrame* ContourValuesButtonsFrame;
  vtkKWPushButton* DeleteValueButton;
  vtkKWPushButton* DeleteAllButton;

  vtkKWLabeledFrame* NewValueFrame;
  vtkKWLabel* NewValueLabel;
  vtkKWScale* NewValueEntry;
  vtkKWPushButton* AddValueButton;

  vtkKWLabeledFrame* GenerateFrame;
  vtkKWFrame* GenerateNumberFrame;
  vtkKWFrame* GenerateRangeFrame;

  vtkKWLabel* GenerateLabel;
  vtkKWLabel* GenerateRangeLabel;
  vtkKWScale* GenerateEntry;
  vtkKWPushButton* GenerateButton;

  vtkKWRange* GenerateRangeWidget;

  virtual int ComputeWidgetRange() {return 0;}
  
  vtkPVValueList(const vtkPVValueList&); // Not implemented
  void operator=(const vtkPVValueList&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,      
                        vtkPVXMLPackageParser* parser);

};

#endif
