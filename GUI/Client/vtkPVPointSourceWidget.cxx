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
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVProcessModule.h"
#include "vtkPVScaleFactorEntry.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVXMLElement.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSM3DWidgetProxy.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPointSourceWidget);
vtkCxxRevisionMacro(vtkPVPointSourceWidget, "1.40");

int vtkPVPointSourceWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

vtkCxxSetObjectMacro(vtkPVPointSourceWidget, InputMenu, vtkPVInputMenu);

//-----------------------------------------------------------------------------
vtkPVPointSourceWidget::vtkPVPointSourceWidget()
{
  this->CommandFunction = vtkPVPointSourceWidgetCommand;
  this->SourceProxy = 0;
  this->SourceProxyName = 0;
  
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
  if (this->SourceProxyName)
   {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("source",
      this->SourceProxyName);
    }
  this->SetSourceProxyName(0);
  if(this->SourceProxy)
    {
    this->SourceProxy->Delete();
    this->SourceProxy = 0;
    }
  this->RadiusWidget->Delete();
  this->NumberOfPointsWidget->Delete();
  this->SetInputMenu(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::SaveInBatchScript(ofstream *file)
{
  float rad;
  float num;
  
  if (!this->SourceProxy)
    {
    vtkErrorMacro("Source proxy must be set to save to a batch script.");
    return;
    }
  
  vtkClientServerID sourceID = this->SourceProxy->GetID(0);
  
  if (sourceID.ID == 0)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    } 

  this->WidgetProxy->SaveInBatchScript(file);
  
  *file << endl;
  *file << "set pvTemp" << sourceID
        << " [$proxyManager NewProxy sources PointSource]"
        << endl;
  *file << "  $proxyManager RegisterProxy sources pvTemp"
        << sourceID << " $pvTemp" << sourceID
        << endl;
  *file << "  $pvTemp" << sourceID << " UnRegister {}" << endl;

  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Center"));
  if(dvp)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty Center] "
      << "SetElements3 " 
      << dvp->GetElement(0) << " " 
      << dvp->GetElement(1) << " " 
      << dvp->GetElement(2) << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty Center]"
      << " SetControllerProxy $pvTemp" 
      << this->WidgetProxy->GetID(0) << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty Center]"
      << " SetControllerProperty [$pvTemp"
      << this->WidgetProxy->GetID(0) 
      << " GetProperty Position]" << endl; 
    }

  this->NumberOfPointsWidget->GetValue(&num, 1);
  *file << "  [$pvTemp" << sourceID << " GetProperty NumberOfPoints] "
    << "SetElements1 " << static_cast<int>(num) << endl;
  
  this->RadiusWidget->GetValue(&rad, 1);
  *file << "  [$pvTemp" << sourceID << " GetProperty Radius] "
    << "SetElements1 " << rad << endl;
  *file << "  $pvTemp" << sourceID << " UpdateVTKObjects" << endl;
  *file << endl;

}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags
  this->Superclass::Create(app);

  static int proxyNum = 0;
  vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
  this->SourceProxy = vtkSMSourceProxy::SafeDownCast(
    pm->NewProxy("sources", "PointSource"));
  ostrstream str;
  str << "PointSource" << proxyNum << ends;
  this->SetSourceProxyName(str.str());
  pm->RegisterProxy("sources", this->SourceProxyName, this->SourceProxy);
  proxyNum++;
  str.rdbuf()->freeze(0);

  this->RadiusWidget->SetVariableName("Radius");
  this->RadiusWidget->SetPVSource(this->GetPVSource());
  this->RadiusWidget->SetLabel("Radius");
  this->RadiusWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
    "SetAcceptButtonColorToModified");
  vtkSMProperty *prop = this->SourceProxy->GetProperty("Radius");
  vtkSMBoundsDomain *bd = vtkSMBoundsDomain::New();
  vtkPVInputMenu *input = vtkPVInputMenu::SafeDownCast(
    this->GetPVSource()->GetPVWidget("Input"));
  if (input)
    {
    bd->AddRequiredProperty(input->GetSMProperty(), "Input");
    }
  prop->AddDomain("bounds", bd);
  this->RadiusWidget->SetSMProperty(prop);
  bd->Delete();

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
  this->NumberOfPointsWidget->SetVariableName("NumberOfPoints");
  this->NumberOfPointsWidget->SetPVSource(this->GetPVSource());
  this->NumberOfPointsWidget->SetLabel("Number of Points");
  this->NumberOfPointsWidget->SetModifiedCommand(
    this->GetPVSource()->GetTclName(), "SetAcceptButtonColorToModified");
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("NumberOfPoints"));
  this->NumberOfPointsWidget->SetSMProperty(ivp);

  this->NumberOfPointsWidget->Create(app);
  float numPts = static_cast<float>(this->DefaultNumberOfPoints);
  this->NumberOfPointsWidget->SetValue(&numPts, 1);
  ivp->SetElement(0, this->DefaultNumberOfPoints);
  if (this->ShowEntries)
    {
    this->Script("pack %s -side top -fill both -expand true",
      this->NumberOfPointsWidget->GetWidgetName());
    }

  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
int vtkPVPointSourceWidget::GetModifiedFlag()
{
  if (this->ModifiedFlag)
    {
    return 1;
    }
  if (this->RadiusWidget->GetModifiedFlag() ||
    this->NumberOfPointsWidget->GetModifiedFlag())
    {
    return 1;
    }
  return 0;
}


//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Initialize()
{
  this->PlaceWidget();

  this->Accept();
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::ResetInternal()
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Center"));
  if (dvp)
    {
    this->SetPositionInternal(
      dvp->GetElement(0), dvp->GetElement(1), dvp->GetElement(2));
    }

  this->RadiusWidget->ResetInternal();
  this->NumberOfPointsWidget->ResetInternal();
  this->ModifiedFlag = 0;

  this->Render();
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Accept()
{
  int modFlag = this->GetModifiedFlag();

  if (modFlag)
    {
    vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Center"));
    if (dvp)
      {
      double center[3];
      this->GetPosition(center);
      dvp->SetElement(0, center[0]);
      dvp->SetElement(1, center[1]);
      dvp->SetElement(2, center[2]);
      }
    this->RadiusWidget->Accept();
    this->NumberOfPointsWidget->Accept();
    this->SourceProxy->UpdateVTKObjects();
    this->SourceProxy->UpdatePipeline();
    }

  // I actually want to call vtkPVWidget::Accept, not the Accept method of
  // the superclass (vtkPVLineWidget).
  this->vtkPVWidget::Accept();
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  this->Superclass::Trace(file);
  this->RadiusWidget->Trace(file);
  this->NumberOfPointsWidget->Trace(file);
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

    vtkPVSource *input = this->InputMenu->GetCurrentValue();
    if (input)
      {
      double bds[6];
      input->GetDataInformation()->GetBounds(bds);
      this->PlaceWidget(bds);
      }

    }
}

//----------------------------------------------------------------------------
void vtkPVPointSourceWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->RadiusWidget);
  this->PropagateEnableState(this->NumberOfPointsWidget);

  this->PropagateEnableState(this->InputMenu);
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SourceProxy: " << this->SourceProxy << endl;
  os << indent << "SourceProxyName: " << 
    (this->SourceProxyName? this->SourceProxyName: "None") << endl;
  os << indent << "RadiusWidget: " << this->RadiusWidget << endl;
  os << indent << "NumberOfPointsWidget: " << this->NumberOfPointsWidget << endl;
  os << indent << "DefaultRadius: " << this->DefaultRadius << endl;
  os << indent << "DefaultNumberOfPoints: " << this->DefaultNumberOfPoints
    << endl;
  os << indent << "RadiusScaleFactor: " << this->RadiusScaleFactor << endl;
  os << indent << "ShowEntries: " << this->ShowEntries << endl;
}
