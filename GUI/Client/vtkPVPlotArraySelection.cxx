/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotArraySelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPlotArraySelection.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkDataArraySelection.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkPVPlotArraySelection);
vtkCxxRevisionMacro(vtkPVPlotArraySelection, "1.1.2.2");
vtkCxxSetObjectMacro(vtkPVPlotArraySelection, ColorProperty, 
  vtkSMDoubleVectorProperty);
//-----------------------------------------------------------------------------
vtkPVPlotArraySelection::vtkPVPlotArraySelection()
{
  this->ArrayColorButtons = vtkCollection::New();  
  this->ColorProperty = 0;
}

//-----------------------------------------------------------------------------
vtkPVPlotArraySelection::~vtkPVPlotArraySelection()
{
  this->ArrayColorButtons->Delete();
  this->SetColorProperty(0);
}

//-----------------------------------------------------------------------------
void vtkPVPlotArraySelection::CreateNewGUI()
{
  this->Superclass::CreateNewGUI();

  this->ArrayColorButtons->RemoveAllItems();
  int numArrays = this->Selection->GetNumberOfArrays();
  if (numArrays == 0)
    {
    return;
    }
  int existing_widgets = this->ArrayColorButtons->GetNumberOfItems();
  for (int idx = 0; idx < numArrays; idx++)
    {
    vtkKWChangeColorButton* button = 0;
    if (idx >= existing_widgets)
      {
      button = vtkKWChangeColorButton::New();
      button->SetParent(this->CheckFrame);
      button->Create(this->GetApplication());
      button->SetDialogTitle("Array Color");
      button->SetBalloonHelpString("Change the color use to plot the curve for the Array.");
      button->SetCommand(this, "ArrayColorCallback");
      button->LabelVisibilityOff();
      double r,g,b;
      vtkMath::HSVToRGB(idx * 1.0/numArrays, 1.0, 1.0, &r, &g, &b);
      // clamp the values -- sometimes vtkMath::HSVToRGB return values
      // slightly over 1.0-- it messes up the color shown by tk.
      double range[2] = {0.0, 1.0};
      vtkMath::ClampValue(&r, range);
      vtkMath::ClampValue(&g, range);
      vtkMath::ClampValue(&b, range);
      button->SetColor(r,g,b);
      this->ArrayColorButtons->AddItem(button);
      button->Delete();
      }
    else
      {
      button = vtkKWChangeColorButton::SafeDownCast(
        this->ArrayColorButtons->GetItemAsObject(idx));
      }
    vtkKWCheckButton* checkbutton = vtkKWCheckButton::SafeDownCast(
      this->ArrayCheckButtons->GetItemAsObject(idx));
    if (checkbutton)
      {
      this->Script("grid forget %s", checkbutton->GetWidgetName());
      this->Script("grid %s %s -row %d -sticky w",
        button->GetWidgetName(), checkbutton->GetWidgetName(), idx);
      }
    else
      {
      vtkErrorMacro("There must be same number of checkboxes as color widgets.");
      }
    }
  
}

//-----------------------------------------------------------------------------
void vtkPVPlotArraySelection::ArrayColorCallback(double, double, double)
{
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVPlotArraySelection::Trace(ofstream* file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
  vtkCollectionIterator* colorit = this->ArrayColorButtons->NewIterator();

  for(it->GoToFirstItem(), colorit->GoToFirstItem(); 
      !it->IsDoneWithTraversal() && !colorit->IsDoneWithTraversal(); 
      it->GoToNextItem(), colorit->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetCurrentObject());
    vtkKWChangeColorButton* color = vtkKWChangeColorButton::SafeDownCast(
      colorit->GetCurrentObject());
    double* rgb = color->GetColor();

    *file << "$kw(" << this->GetTclName() << ") SetArrayStatus {"
      << check->GetText() << "} "
      << check->GetSelectedState() << " "
      << rgb[0] << " " << rgb[1] << " " << rgb[2] << endl;
    }
  it->Delete();
  colorit->Delete();

}

//-----------------------------------------------------------------------------
void vtkPVPlotArraySelection::SetArrayStatus(const char* array_name, int status, 
  double r, double g, double b)
{
  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
  vtkCollectionIterator* colorit = this->ArrayColorButtons->NewIterator();

  int success = 0;
  for(it->GoToFirstItem(), colorit->GoToFirstItem(); 
      !it->IsDoneWithTraversal() && !colorit->IsDoneWithTraversal(); 
      it->GoToNextItem(), colorit->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetCurrentObject());
    vtkKWChangeColorButton* color = vtkKWChangeColorButton::SafeDownCast(
      colorit->GetCurrentObject());

    if (strcmp(check->GetText(), array_name) == 0)
      {
      check->SetSelectedState(status);
      color->SetColor(r, g, b);
      success = 1;
      break;
      }
    }
  colorit->Delete();
  it->Delete();

  if (success)
    {
    this->ArrayColorCallback(r,g,b);
    }
  else
    {
    vtkErrorMacro("Could not find array: " << array_name);
    }
    

  
}

//-----------------------------------------------------------------------------
void vtkPVPlotArraySelection::SetPropertyFromGUI()
{
  this->Superclass::SetPropertyFromGUI();

  if (!this->ColorProperty)
    {
    return;
    }
  int elemCount = 0;
  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
  vtkCollectionIterator* colorit = this->ArrayColorButtons->NewIterator();

  for(it->GoToFirstItem(), colorit->GoToFirstItem(); 
      !it->IsDoneWithTraversal() && !colorit->IsDoneWithTraversal(); 
      it->GoToNextItem(), colorit->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetCurrentObject());
    if (check->GetSelectedState())
      {
      vtkKWChangeColorButton* color = vtkKWChangeColorButton::SafeDownCast(
        colorit->GetCurrentObject());
      double *rgb = color->GetColor();

      this->ColorProperty->SetElement(elemCount*3, rgb[0]);
      this->ColorProperty->SetElement(elemCount*3+1, rgb[1]);
      this->ColorProperty->SetElement(elemCount*3+2, rgb[2]);
      elemCount++;
      }
    }
  this->ColorProperty->SetNumberOfElements(elemCount*3);

  it->Delete();
  colorit->Delete();
}


//-----------------------------------------------------------------------------
void vtkPVPlotArraySelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorProperty: " << this->ColorProperty << endl;
    
    
}
