/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPointSourceWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPointSourceWidget.h"

#include "vtkKWEntry.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVInputMenu.h"
#include "vtkPVPointWidget.h"
#include "vtkPVProcessModule.h"
#include "vtkPVScaleFactorEntry.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWidgetProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"

int vtkPVPointSourceWidget::InstanceCount = 0;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPointSourceWidget);
vtkCxxRevisionMacro(vtkPVPointSourceWidget, "1.30.2.1");

int vtkPVPointSourceWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

vtkCxxSetObjectMacro(vtkPVPointSourceWidget, InputMenu, vtkPVInputMenu);

//-----------------------------------------------------------------------------
vtkPVPointSourceWidget::vtkPVPointSourceWidget()
{
  this->CommandFunction = vtkPVPointSourceWidgetCommand;
  this->PointWidget = vtkPVPointWidget::New();
  this->PointWidget->SetParent(this);
  this->PointWidget->SetTraceReferenceObject(this);
  this->PointWidget->SetTraceReferenceCommand("GetPointWidget");
  this->PointWidget->SetUseLabel(0);

  this->RadiusWidget = vtkPVScaleFactorEntry::New();
  this->RadiusWidget->SetParent(this);
  this->RadiusWidget->SetTraceReferenceObject(this);
  this->RadiusWidget->SetTraceReferenceCommand("GetRadiusWidget");
  
  this->NumberOfPointsWidget = vtkPVVectorEntry::New();
  this->NumberOfPointsWidget->SetParent(this);
  this->NumberOfPointsWidget->SetTraceReferenceObject(this);
  this->NumberOfPointsWidget->SetTraceReferenceCommand(
    "GetNumberOfPointsWidget");
  
  // Start out modified so that accept will set the source
  this->ModifiedFlag = 1;
  
  this->RadiusScaleFactor = 0.1;
  this->DefaultRadius = 0;
  this->DefaultNumberOfPoints = 1;
  this->InputMenu = NULL;
  this->ShowEntries = 1;
}

//-----------------------------------------------------------------------------
vtkPVPointSourceWidget::~vtkPVPointSourceWidget()
{
  this->PointWidget->Delete();
  this->RadiusWidget->Delete();
  this->NumberOfPointsWidget->Delete();
  this->SetInputMenu(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::SaveInBatchScript(ofstream *file)
{
  double pt[3];
  float rad;
  float num;
  
  if (this->OutputID.ID == 0 || this->PointWidget == NULL)
    {
    vtkErrorMacro(<< this->GetClassName() << " must not have SaveInBatchScript method.");
    return;
    } 

  *file << endl;
  *file << "set pvTemp" <<  this->OutputID.ID
        << " [$proxyManager NewProxy sources PointSource]"
        << endl;
  *file << "  $proxyManager RegisterProxy sources pvTemp"
        << this->OutputID.ID << " $pvTemp" << this->OutputID.ID
        << endl;
  *file << " $pvTemp" << this->OutputID.ID << " UnRegister {}" << endl;

  this->PointWidget->GetPosition(pt);
  *file << "  [$pvTemp" << this->OutputID.ID << " GetProperty Center] "
        << "SetElements3 " << pt[0] << " " << pt[1] << " " << pt[2] << endl;
  this->NumberOfPointsWidget->GetValue(&num, 1);
  *file << "  [$pvTemp" << this->OutputID.ID << " GetProperty NumberOfPoints] "
        << "SetElements1 " << static_cast<int>(num) << endl;
  this->RadiusWidget->GetValue(&rad, 1);
  *file << "  [$pvTemp" << this->OutputID.ID << " GetProperty Radius] "
        << "SetElements1 " << rad << endl;
  *file << "  $pvTemp" << this->OutputID.ID << " UpdateVTKObjects" << endl;
  *file << endl;
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  char name[256];
  sprintf(name, "PointSourceWidget%d", vtkPVPointSourceWidget::InstanceCount);

  char outputName[256];
  sprintf(outputName, "PointSourceWidgetOutput%d", 
          vtkPVPointSourceWidget::InstanceCount++);

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  this->RadiusWidget->SetObjectID(this->SourceID);
  this->RadiusWidget->SetVariableName("Radius");
  this->RadiusWidget->SetPVSource(this->GetPVSource());
  this->RadiusWidget->SetLabel("Radius");
  this->RadiusWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
                                       "SetAcceptButtonColorToModified");
  
  this->RadiusWidget->Create(app);
  if (this->RadiusWidget->GetSMPropertyName())
    {
    this->RadiusWidget->SetScaleFactor(this->RadiusScaleFactor);
    }
  else
    {
    this->RadiusWidget->SetValue(&this->DefaultRadius, 1);
    }

  if (this->ShowEntries)
    {
    this->Script("pack %s -side top -fill both -expand true",
                 this->RadiusWidget->GetWidgetName());
    }
  
  if (pvApp)
    {
    this->SourceID = pm->NewStreamObject("vtkPointSource");
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->SourceID << "SetNumberOfPoints"
                    << this->DefaultNumberOfPoints
                    << vtkClientServerStream::End;
    float radius;
    this->RadiusWidget->GetValue(&radius, 1);
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->SourceID << "SetRadius" << radius
                    << vtkClientServerStream::End;

    this->OutputID = pm->NewStreamObject("vtkPolyData");
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->SourceID << "SetOutput" << this->OutputID 
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);
    }

  this->PointWidget->SetObjectID(this->SourceID);
  this->PointWidget->SetVariableName("Center");
  this->PointWidget->SetPVSource(this->GetPVSource());
  this->PointWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
                                       "SetAcceptButtonColorToModified");
  
  this->NumberOfPointsWidget->SetObjectID(this->SourceID);
  this->NumberOfPointsWidget->SetVariableName("NumberOfPoints");
  this->NumberOfPointsWidget->SetPVSource(this->GetPVSource());
  this->NumberOfPointsWidget->SetLabel("Number of Points");
  this->NumberOfPointsWidget->SetModifiedCommand(
    this->GetPVSource()->GetTclName(), "SetAcceptButtonColorToModified");
  
  this->NumberOfPointsWidget->Create(app);
  float numPts = static_cast<float>(this->DefaultNumberOfPoints);
  this->NumberOfPointsWidget->SetValue(&numPts, 1);
  if (this->ShowEntries)
    {
    this->Script("pack %s -side top -fill both -expand true",
                 this->NumberOfPointsWidget->GetWidgetName());
    }
  
  this->PointWidget->Create(app);
  this->Script("pack %s -side top -fill both -expand true",
               this->PointWidget->GetWidgetName());

  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
int vtkPVPointSourceWidget::GetModifiedFlag()
{
  if (this->ModifiedFlag)
    {
    return 1;
    }
  if (this->PointWidget->GetModifiedFlag() ||
      this->RadiusWidget->GetModifiedFlag() ||
      this->NumberOfPointsWidget->GetModifiedFlag())
    {
    return 1;
    }
  return 0;
}
 

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::ResetInternal()
{
  // Ignore the source passed in.  We are updating our
  // own point source.
  this->PointWidget->ResetInternal();
  this->RadiusWidget->ResetInternal();
  this->NumberOfPointsWidget->ResetInternal();
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::AcceptInternal(vtkClientServerID)
{
  // Ignore the source passed in.  We are updating our
  // own point source.
  if (this->GetModifiedFlag())
    {
    this->PointWidget->AcceptInternal(this->SourceID);
    this->AcceptRadiusAndNumberOfPointsInternal();
    }
  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::AcceptRadiusAndNumberOfPointsInternal()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->SourceID.ID && pvApp)
    {
    vtkPVProcessModule *pm = pvApp->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke << this->SourceID
                    << "SetNumberOfPoints"
                    << this->NumberOfPointsWidget->GetEntry(0)->GetValueAsInt()
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke << this->SourceID
                    << "SetRadius"
                    << this->RadiusWidget->GetEntry(0)->GetValueAsFloat()
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    this->RadiusWidget->SetAcceptCalled(1);
    this->NumberOfPointsWidget->SetAcceptCalled(1);
    }
  
  vtkSMDoubleVectorProperty *prop1 = vtkSMDoubleVectorProperty::SafeDownCast(
    this->RadiusWidget->GetSMProperty());
  if (prop1)
    {
    prop1->SetElement(0, this->RadiusWidget->GetEntry(0)->GetValueAsFloat());
    }
  
  vtkSMIntVectorProperty *prop2 = vtkSMIntVectorProperty::SafeDownCast(
    this->NumberOfPointsWidget->GetSMProperty());
  if (prop2)
    {
    prop2->SetElement(0, this->NumberOfPointsWidget->GetEntry(0)->
                      GetValueAsInt());
    }
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  this->PointWidget->Trace(file);
  this->RadiusWidget->Trace(file);
  this->NumberOfPointsWidget->Trace(file);
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Select()
{
  this->PointWidget->Select();
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Deselect()
{
  this->PointWidget->Deselect();
}

//-----------------------------------------------------------------------------
int vtkPVPointSourceWidget::ReadXMLAttributes(vtkPVXMLElement *element,
                                              vtkPVXMLPackageParser *parser)
{
  if (!this->Superclass::ReadXMLAttributes(element, parser))
    {
    return 0;
    }
  
  const char *input_menu = element->GetAttribute("input_menu");
  if (input_menu)
    {
    vtkPVXMLElement *ime = element->LookupElement(input_menu);
    if (!ime)
      {
      vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
      return 0;
      }
    
    vtkPVWidget *w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVInputMenu *imw = vtkPVInputMenu::SafeDownCast(w);
    if (!imw)
      {
      if (w)
        {
        w->Delete();
        }
      vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
      return 0;
      }
    imw->AddDependent(this);
    this->SetInputMenu(imw);
    imw->Delete();
    }
  
  if (!element->GetScalarAttribute("radius_scale_factor",
                                   &this->RadiusScaleFactor))
    {
    this->RadiusScaleFactor = 0.1;
    }
  
  if (!element->GetScalarAttribute("default_radius", &this->DefaultRadius))
    {
    this->DefaultRadius = 0;
    }

  if (!element->GetScalarAttribute("default_number_of_points",
                                   &this->DefaultNumberOfPoints))
    {
    this->DefaultNumberOfPoints = 1;
    }
  
  if (!element->GetScalarAttribute("show_entries", &this->ShowEntries))
    {
    this->ShowEntries = 1;
    }

  const char *radius_property = element->GetAttribute("radius_property");
  if (radius_property)
    {
    this->RadiusWidget->SetSMPropertyName(radius_property);
    }
  
  const char *num_points_property =
    element->GetAttribute("number_of_points_property");
  if (num_points_property)
    {
    this->NumberOfPointsWidget->SetSMPropertyName(num_points_property);
    }
  
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::CopyProperties(
  vtkPVWidget *clone, vtkPVSource *pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVPointSourceWidget *psw = vtkPVPointSourceWidget::SafeDownCast(clone);
  if (psw)
    {
    if (this->InputMenu)
      {
      vtkPVInputMenu *im = this->InputMenu->ClonePrototype(pvSource, map);
      psw->SetInputMenu(im);
      im->Delete();
      }
    psw->SetRadiusScaleFactor(this->RadiusScaleFactor);
    psw->SetDefaultRadius(this->DefaultRadius);
    psw->SetDefaultNumberOfPoints(this->DefaultNumberOfPoints);
    psw->SetShowEntries(this->ShowEntries);
    psw->GetRadiusWidget()->SetSMPropertyName(
      this->RadiusWidget->GetSMPropertyName());
    psw->GetNumberOfPointsWidget()->SetDataType(VTK_INT);
    psw->GetNumberOfPointsWidget()->SetSMPropertyName(
      this->NumberOfPointsWidget->GetSMPropertyName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Update()
{
  if (this->InputMenu)
    {
    this->RadiusWidget->Update();
    }
}

//----------------------------------------------------------------------------
void vtkPVPointSourceWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->PointWidget);

  this->PropagateEnableState(this->RadiusWidget);
  this->PropagateEnableState(this->NumberOfPointsWidget);

  this->PropagateEnableState(this->InputMenu);
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Point widget: " << this->PointWidget << endl;
  os << indent << "RadiusWidget: " << this->RadiusWidget << endl;
  os << indent << "NumberOfPointsWidget: " << this->NumberOfPointsWidget << endl;
  os << indent << "DefaultRadius: " << this->DefaultRadius << endl;
  os << indent << "DefaultNumberOfPoints: " << this->DefaultNumberOfPoints
     << endl;
  os << indent << "RadiusScaleFactor: " << this->RadiusScaleFactor << endl;
  os << indent << "ShowEntries: " << this->ShowEntries << endl;
}
