/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContainerWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVContainerWidget.h"
#include "vtkObjectFactory.h"
#include "vtkArrayMap.txx"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkPVWidgetProperty.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVContainerWidget);
vtkCxxRevisionMacro(vtkPVContainerWidget, "1.27");

int vtkPVContainerWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

#ifndef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION

template class VTK_EXPORT vtkAbstractList<vtkPVWidget*>;

#endif

//----------------------------------------------------------------------------
vtkPVContainerWidget::vtkPVContainerWidget()
{
  this->CommandFunction = vtkPVContainerWidgetCommand;

  this->WidgetProperties = vtkCollection::New();

  this->PackDirection = 0;
  this->SetPackDirection("top");
}

//----------------------------------------------------------------------------
vtkPVContainerWidget::~vtkPVContainerWidget()
{
  this->WidgetProperties->Delete();
  this->WidgetProperties = NULL;
  this->SetPackDirection(0);
}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    if (!widget->GetApplication())
      {
      widget->Create(app);
      this->Script("pack %s -side %s -fill both -expand true",
                   widget->GetWidgetName(), this->PackDirection);
      }
    it->GoToNextItem();
    }
  it->Delete();
}


//----------------------------------------------------------------------------
int vtkPVContainerWidget::GetModifiedFlag()
{
  if (this->ModifiedFlag)
    {
    return 1;
    }

  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    if (widget->GetModifiedFlag())
      {
      it->Delete();
      return 1;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::PostAccept()
{
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    if (widget)
      {
      widget->PostAccept();
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::AcceptInternal(vtkClientServerID sourceID)
{
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    if (widget->GetSMProperty())
      {
      widget->Accept();
      }
    else
      {
      widget->AcceptInternal(sourceID);
      }
    it->GoToNextItem();
    }
  it->Delete();

  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
void vtkPVContainerWidget::AddAnimationScriptsToMenu(vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai)
{
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();

  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;

  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    widget->AddAnimationScriptsToMenu(menu, ai);
    it->GoToNextItem();
    }
  it->Delete();
}

//---------------------------------------------------------------------------
void vtkPVContainerWidget::Trace(ofstream *file)
{
  vtkCollectionIterator *it;
  
  vtkPVWidget* widget;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  int i;
  vtkPVWidgetProperty *prop;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    widget->Trace(file);
    it->GoToNextItem();
    }
  it->Delete();

}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::ResetInternal()
{
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    widget->ResetInternal();
    it->GoToNextItem();
    }
  it->Delete();

  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::Select()
{
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    widget->Select();
    it->GoToNextItem();
    }
  it->Delete();

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::Deselect()
{
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    widget->Deselect();
    it->GoToNextItem();
    }
  it->Delete();

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::AddPVWidget(vtkPVWidget *pvw)
{
  char str[512];
  vtkPVWidgetProperty *prop = pvw->CreateAppropriateProperty();
  prop->SetWidget(pvw);
  this->WidgetProperties->AddItem(prop);
  prop->Delete();
  
  if (pvw->GetTraceName() == NULL)
    {
    vtkWarningMacro("TraceName not set.");
    return;
    }

  pvw->SetTraceReferenceObject(this);
  sprintf(str, "GetPVWidget {%s}", pvw->GetTraceName());
  pvw->SetTraceReferenceCommand(str); 
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVContainerWidget::GetPVWidget(vtkIdType i)
{
  vtkPVWidget* widget = 0;
  vtkPVWidgetProperty *prop =
    static_cast<vtkPVWidgetProperty*>(this->WidgetProperties->GetItemAsObject(i));
  if (!prop)
    {
    widget = 0;
    }
  else
    {
    widget = prop->GetWidget();
    }
  return widget;
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVContainerWidget::GetPVWidget(const char* traceName)
{
  if (!traceName)
    {
    return 0;
    }

  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    if (widget->GetTraceName() && 
        strcmp(traceName,widget->GetTraceName()) == 0) 
      {
      it->Delete();
      return widget;
      }
    it->GoToNextItem();
    }
  it->Delete();
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::SaveInBatchScript(ofstream *file)
{
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  vtkPVWidget* widget;
  vtkPVWidgetProperty *prop;
  int i;
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    widget = prop->GetWidget();
    widget->SaveInBatchScript(file);
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
vtkPVContainerWidget* vtkPVContainerWidget::ClonePrototype(
  vtkPVSource* pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVContainerWidget::SafeDownCast(clone);
}

vtkPVWidget* vtkPVContainerWidget::ClonePrototypeInternal(
  vtkPVSource* pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* pvWidget = 0;
  // Check if a clone of this widget has already been created
  if ( map->GetItem(this, pvWidget) != VTK_OK )
    {
    // If not, create one and add it to the map
    pvWidget = this->NewInstance();
    map->SetItem(this, pvWidget);
    // Now copy all the properties
    this->CopyProperties(pvWidget, pvSource, map);

    vtkPVContainerWidget* pvCont = vtkPVContainerWidget::SafeDownCast(
      pvWidget);
    if (!pvCont)
      {
      vtkErrorMacro("Internal error. Could not downcast pointer.");
      pvWidget->Delete();
      return 0;
      }
    
    vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
    it->InitTraversal();
    
    vtkPVWidget* widget;
    vtkPVWidgetProperty *prop;
    vtkPVWidget* clone;
    int i;
    
    for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
      {
      prop = static_cast<vtkPVWidgetProperty*>(it->GetObject());
      widget = prop->GetWidget();
      clone = widget->ClonePrototype(pvSource, map);
      clone->SetParent(pvCont);
      pvCont->AddPVWidget(clone);
      clone->Delete();
      it->GoToNextItem();
      }
    it->Delete();
    }
  else
    {
    // Increment the reference count. This is necessary
    // to make the behavior same whether a widget is created
    // or returned from the map. Always call Delete() after
    // cloning.
    pvWidget->Register(this);
    }

  // note pvCont == pvWidget
  return pvWidget;
}

void vtkPVContainerWidget::CopyProperties(
  vtkPVWidget* clone, vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVContainerWidget* pvcw = vtkPVContainerWidget::SafeDownCast(clone);
  if (pvcw)
    {
    pvcw->SetPackDirection(this->PackDirection);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to "
                  "PVContainerWidget.");
    }
}

//----------------------------------------------------------------------------
int vtkPVContainerWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                            vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Extract the list of items.
  unsigned int i;
  for(i=0;i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* item = element->GetNestedElement(i);
    if(strcmp(item->GetName(), "Item") != 0)
      {
      vtkErrorMacro("Found non-Item element in ContainerWidget.");
      return 0;
      }
    else if(item->GetNumberOfNestedElements() != 1)
      {
      vtkErrorMacro("Item element doesn't have exactly 1 widget.");
      return 0;
      }
    vtkPVXMLElement* we = item->GetNestedElement(0);
    vtkPVWidget* widget = this->GetPVWidgetFromParser(we, parser);
    if (widget)
      {
      this->AddPVWidget(widget);
      widget->Delete();
      }
    }  

  const char* pack_direction  = element->GetAttribute("pack_direction");
  if (pack_direction)
    {
    this->SetPackDirection(pack_direction);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkCollectionIterator* it = this->WidgetProperties->NewIterator();
  for ( it->InitTraversal();
    !it->IsDoneWithTraversal();
    it->GoToNextItem() )
    {
    vtkPVWidgetProperty* prop = vtkPVWidgetProperty::SafeDownCast(it->GetObject());
    if ( prop && prop->GetWidget() )
      {
      prop->GetWidget()->SetEnabled(this->Enabled);
      }
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVContainerWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PackDirection: " 
     << (this->PackDirection?this->PackDirection:"(none)") << endl;
}
