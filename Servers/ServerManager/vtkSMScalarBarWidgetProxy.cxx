/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMScalarBarWidgetProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkCoordinate.h"
#include "vtkPVRenderModule.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyProperty.h"

vtkStandardNewMacro(vtkSMScalarBarWidgetProxy);
vtkCxxRevisionMacro(vtkSMScalarBarWidgetProxy, "1.1");

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetProxy::vtkSMScalarBarWidgetProxy()
{
  this->SetPosition1(0.87,0.25);
  this->SetPosition2(0.13,0.5);
  this->Orientation = VTK_ORIENT_VERTICAL;
}

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetProxy::~vtkSMScalarBarWidgetProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);


  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (pm->GetRenderModule())
    {
    vtkClientServerID rendererID = pm->GetRenderModule()->GetRenderer2DID();
    vtkClientServerID interactorID = pm->GetRenderModule()->GetInteractorID();
    this->SetCurrentRenderer(rendererID);
    this->SetInteractor(interactorID);
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    // Set Position1 Coordinate
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetPositionCoordinate"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetValue" << this->Position1[0]
      << this->Position1[1]
      << vtkClientServerStream::End;

    // Set Position2 Coordinate
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetPosition2Coordinate"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetValue" << this->Position2[0]
      << this->Position2[1]
      << vtkClientServerStream::End;

    // Set Orientation
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetOrientation"
      << this->Orientation
      << vtkClientServerStream::End;
    }

  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
  // Build the lookuptable
  /*
  vtkSMLookupTableProxy* lut = vtkSMLookupTableProxy::SafeDownCast(
    this->GetSubProxy("LookupTable"));
  if (lut)
    {
    lut->Build();
    }
  else
    {
    vtkErrorMacro("SubProxy LookupTable is must be defined in the configuration");
    }*/
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetLookupTable(vtkSMProxy *lut)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;

  unsigned cc;
  unsigned  int numObjects = this->GetNumberOfIDs();
  if (!lut)
    {
    return;
    }
  for (cc=0; cc < numObjects; cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetLookupTable"
      << lut->GetID(0)
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetTitle(const char* title)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetTitle"
      << title
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetLabelFormat(const char* format)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetLabelFormat"
      << format
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetTitleFormatColor(double color[3])
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetTitleTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetColor"
      << color[0] << color[1] << color[2]
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetLabelFormatColor(double color[3])
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetLabelTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetColor"
      << color[0] << color[1] << color[2]
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetTitleFormatOpacity(double opacity)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetTitleTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetOpacity"
      << opacity
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetLabelFormatOpacity(double opacity)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetLabelTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetOpacity"
      << opacity
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetTitleFormatFont(int font)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetTitleTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetFontFamily"
      << font 
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }  
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetLabelFormatFont(int font)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetLabelTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetFontFamily"
      << font 
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }  
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetTitleFormatBold(int bold)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetTitleTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetBold"
      << bold 
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }  
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetLabelFormatBold(int bold)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetLabelTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetBold"
      << bold 
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }  
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetTitleFormatItalic(int italic)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetTitleTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetItalic"
      << italic
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }  
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetLabelFormatItalic(int italic)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetLabelTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetItalic"
      << italic
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }  
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetTitleFormatShadow(int shadow)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetTitleTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetShadow"
      << shadow
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }  
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetLabelFormatShadow(int shadow)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc)
      << "GetScalarBarActor"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "GetLabelTextProperty"
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetShadow"
      << shadow
      << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }  
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::ExecuteEvent(vtkObject*obj, unsigned long event, void*p)
{
//  vtkWarningMacro("ExecuteEvent called with event :" << event);
  vtkScalarBarWidget* widget = vtkScalarBarWidget::SafeDownCast(obj);
  if (widget)
    {
    double position[3];
    vtkScalarBarActor* actor = widget->GetScalarBarActor();
    actor->GetPositionCoordinate()->GetValue(position);
    this->SetPosition1(position[0],position[1]);
    actor->GetPosition2Coordinate()->GetValue(position);
    this->SetPosition2(position[0],position[1]);
    this->Orientation = actor->GetOrientation();
    }
  else
    {
    vtkErrorMacro("Widget is not a ScalarBar");
    }
  this->Superclass::ExecuteEvent(obj,event,p);
}
  
//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SaveInBatchScript(ofstream* file)
{
  *file << endl;
  unsigned int cc;
  unsigned int numObjects = this->GetNumberOfIDs();
  vtkSMStringVectorProperty* svp;
  vtkSMIntVectorProperty* ivp;
  vtkSMDoubleVectorProperty* dvp;

  for (cc=0; cc< numObjects; cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    *file << "set pvTemp" << id.ID
      << " [$proxyManager NewProxy scalarbar_widget ScalarBarWidget]"
      << endl;
    *file << "  $proxyManager RegisterProxy scalarbar_widget pvTemp"
      << id.ID << " $pvTemp" << id.ID
      << endl;
    *file << "  $pvTemp" << id.ID << " UnRegister {}"
      << endl;
    *file << "  [$Ren1 GetProperty Displayers] AddProxy $pvTemp"
      << id.ID <<endl; 

    //Set Positions & orientation.
    *file << "  [$pvTemp" << id.ID << " GetProperty Position1]"
      << " SetElements2 "
      << this->Position1[0] << " " << this->Position1[1] << endl;
    *file << "  [$pvTemp" << id.ID << " GetProperty Position2]"
      << " SetElements2 "
      << this->Position2[0] << " " << this->Position2[1] << endl;
    *file << "  [$pvTemp" << id.ID << " GetProperty Orientation]"
      << " SetElements1 "
      << this->Orientation << endl;

    //Set Tile & label format
    svp = vtkSMStringVectorProperty::SafeDownCast(
      this->GetProperty("Title"));
    *file << "  [$pvTemp" << id.ID << " GetProperty Title]"
      << " SetElement 0 \""
      << svp->GetElement(0) << "\"" << endl;

    svp = vtkSMStringVectorProperty::SafeDownCast(
      this->GetProperty("LabelFormat"));
    *file << "  [$pvTemp" << id.ID << " GetProperty LabelFormat]"
      << " SetElement 0 \""
      << svp->GetElement(0) << "\"" << endl;

    //Set visibility
    *file << "  [$pvTemp" << id.ID << " GetProperty Visibility]"
      << " SetElements1 "
      << this->Enabled <<endl;

    //Set Title text property
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("TitleFormatColor"));
    *file << "  [$pvTemp" << id.ID << " GetProperty TitleFormatColor]"
      << " SetElements3 "
      << dvp->GetElement(0) << " "
      << dvp->GetElement(1) << " "
      << dvp->GetElement(2) << endl;

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("TitleFormatOpacity"));
    *file << "  [$pvTemp" << id.ID << " GetProperty TitleFormatOpacity]"
      << " SetElements1 "
      << dvp->GetElement(0) << endl;

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("TitleFormatFont"));
    *file << "  [$pvTemp" << id.ID << " GetProperty TitleFormatFont]"
      << " SetElements1 "
      << ivp->GetElement(0) << endl;

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("TitleFormatBold"));
    *file << "  [$pvTemp" << id.ID << " GetProperty TitleFormatBold]"
      << " SetElements1 "
      << ivp->GetElement(0) << endl;

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("TitleFormatItalic"));
    *file << "  [$pvTemp" << id.ID << " GetProperty TitleFormatItalic]"
      << " SetElements1 "
      << ivp->GetElement(0) << endl;

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("TitleFormatShadow"));
    *file << "  [$pvTemp" << id.ID << " GetProperty TitleFormatShadow]"
      << " SetElements1 "
      << ivp->GetElement(0) << endl;


    //Set Labels text property
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("LabelFormatColor"));
    *file << "  [$pvTemp" << id.ID << " GetProperty LabelFormatColor]"
      << " SetElements3 "
      << dvp->GetElement(0) << " "
      << dvp->GetElement(1) << " "
      << dvp->GetElement(2) << endl;

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("LabelFormatOpacity"));
    *file << "  [$pvTemp" << id.ID << " GetProperty LabelFormatOpacity]"
      << " SetElements1 "
      << dvp->GetElement(0) << endl;

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("LabelFormatFont"));
    *file << "  [$pvTemp" << id.ID << " GetProperty LabelFormatFont]"
      << " SetElements1 "
      << ivp->GetElement(0) << endl;

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("LabelFormatBold"));
    *file << "  [$pvTemp" << id.ID << " GetProperty LabelFormatBold]"
      << " SetElements1 "
      << ivp->GetElement(0) << endl;

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("LabelFormatItalic"));
    *file << "  [$pvTemp" << id.ID << " GetProperty LabelFormatItalic]"
      << " SetElements1 "
      << ivp->GetElement(0) << endl;

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("LabelFormatShadow"));
    *file << "  [$pvTemp" << id.ID << " GetProperty LabelFormatShadow]"
      << " SetElements1 "
      << ivp->GetElement(0) << endl;

    vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
      this->GetProperty("LookupTable"));
    if (pp && pp->GetNumberOfProxies() == 1 && pp->GetProxy(0) )
      {
      *file << "  [$pvTemp" << id.ID << " GetProperty LookupTable]"
        << " RemoveAllProxies" << endl;
      *file << "  [$pvTemp" << id.ID << " GetProperty LookupTable]"
        << " AddProxy $pvTemp" << pp->GetProxy(0)->GetID(0).ID
        << endl;
      }

    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects" << endl;
    *file << endl;
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Position1: " 
    << this->Position1[0] << ", " << this->Position1[1]
    << ", " << this->Position1[2] << endl;
  os << indent << "Position2: " 
    << this->Position2[0] << ", " << this->Position2[1]
    << ", " << this->Position2[2] << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
}
