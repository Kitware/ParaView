/*=========================================================================

  Program:   ParaView
  Module:    vtkPVColorMap.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVColorMap.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVTextPropertyEditor.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMScalarBarWidgetProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkTextProperty.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVColorMap);
vtkCxxRevisionMacro(vtkPVColorMap, "1.142");

//===========================================================================
//***************************************************************************
class vtkPVColorMapObserver : public vtkCommand
{
public:
  static vtkPVColorMapObserver* New() 
    {return new vtkPVColorMapObserver;};

  vtkPVColorMapObserver()
    {
    this->PVColorMap = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void* calldata)
    {
      if ( this->PVColorMap )
        {
        this->PVColorMap->ExecuteEvent(wdg, event, calldata);
        }
    }

  vtkPVColorMap* PVColorMap;
};

#define VTK_PV_COLOR_MAP_RED_HUE 0.0
#define VTK_PV_COLOR_MAP_RED_SATURATION 1.0
#define VTK_PV_COLOR_MAP_RED_VALUE 1.0
#define VTK_PV_COLOR_MAP_BLUE_HUE 0.6667
#define VTK_PV_COLOR_MAP_BLUE_SATURATION 1.0
#define VTK_PV_COLOR_MAP_BLUE_VALUE 1.0
#define VTK_PV_COLOR_MAP_BLACK_HUE 0.0
#define VTK_PV_COLOR_MAP_BLACK_SATURATION 0.0
#define VTK_PV_COLOR_MAP_BLACK_VALUE 0.0
#define VTK_PV_COLOR_MAP_WHITE_HUE 0.0
#define VTK_PV_COLOR_MAP_WHITE_SATURATION 0.0
#define VTK_PV_COLOR_MAP_WHITE_VALUE 1.0

#define VTK_USE_LAB_COLOR_MAP 1.1

//***************************************************************************
//===========================================================================
//vtkCxxSetObjectMacro(vtkPVColorMap,PVRenderView,vtkPVRenderView);
//----------------------------------------------------------------------------
// No register count because of reference loop.
void vtkPVColorMap::SetPVRenderView(vtkPVRenderView *rv)
{
  this->PVRenderView = rv;
}

//----------------------------------------------------------------------------
vtkPVColorMap::vtkPVColorMap()
{
  this->UseCount = 0;
  
  this->ScalarBarVisibility = 0;
  this->InternalScalarBarVisibility = 0;
  this->Initialized = 0;
  this->PVRenderView = NULL;
  this->ScalarBarObserver = NULL;

  // Stuff for setting the range of the color map.
  this->ScalarRange[0] = this->WholeScalarRange[0] = VTK_LARGE_FLOAT;
  this->ScalarRange[1] = this->WholeScalarRange[1] = -VTK_LARGE_FLOAT;
  this->ScalarRangeLock = 0;

  this->MapData = NULL;
  this->MapDataSize = 0;
  this->MapHeight = 25;
  this->MapWidth = 20;

  this->VisitedFlag = 0;

  this->LabelTextProperty = vtkTextProperty::New();
  this->TitleTextProperty = vtkTextProperty::New();
  
  this->ScalarBarProxy = 0;
  this->ScalarBarProxyName = 0;

  this->LookupTableProxy = 0;
  this->LookupTableProxyName = 0;
  
  this->ScalarBarTitle = NULL;
  this->VectorMagnitudeTitle = new char[12];
  strcpy(this->VectorMagnitudeTitle, "Magnitude");
  this->VectorComponentTitles = NULL;
  this->ScalarBarVectorTitle = NULL;
  this->NumberOfVectorComponents = 0;
  this->VectorComponent = 0;

  this->StartColor[0] = this->StartColor[1] = this->StartColor[2] = 0;
  this->EndColor[0] = this->EndColor[1] = this->EndColor[2] = 0;

  this->Displayed = 0;
}

//----------------------------------------------------------------------------
vtkPVColorMap::~vtkPVColorMap()
{
  // Used to be in vtkPVActorComposite........

  this->SetPVRenderView(NULL);

  if (this->ScalarBarObserver)
    {
    this->ScalarBarObserver->Delete();
    this->ScalarBarObserver = NULL;
    }
    
  if (this->MapData)
    {
    delete [] this->MapData;
    this->MapDataSize = 0;
    this->MapWidth = 0;
    this->MapHeight = 0;
    }

  if (this->ScalarBarProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("displays", this->ScalarBarProxyName);
    }
  this->SetScalarBarProxyName(0);
  if (this->ScalarBarProxy)
    {
    vtkSMRenderModuleProxy* rm = this->GetPVApplication()->GetRenderModuleProxy();
    vtkSMProxyProperty* pp =  (rm)? vtkSMProxyProperty::SafeDownCast(
      rm->GetProperty("Displays")): 0;
    if (pp)
      {
      pp->RemoveProxy(this->ScalarBarProxy);
      rm->UpdateVTKObjects();
      }
    this->ScalarBarProxy->Delete();
    this->ScalarBarProxy = 0;
    }
 
  if (this->LookupTableProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("lookup_tables", 
      this->LookupTableProxyName);
    }
  this->SetLookupTableProxyName(0);
  
  if (this->LookupTableProxy)
    {
    this->LookupTableProxy->Delete();
    this->LookupTableProxy = 0;
    }
  
  if (this->ScalarBarTitle)
    {
    delete [] this->ScalarBarTitle;
    this->ScalarBarTitle = 0;
    }
  if (this->VectorMagnitudeTitle)
    {
    delete [] this->VectorMagnitudeTitle;
    this->VectorMagnitudeTitle = 0;
    }
  if (this->ScalarBarVectorTitle)
    {
    delete[] this->ScalarBarVectorTitle;
    this->ScalarBarVectorTitle = 0;
    }
  // This will delete the vector component titles
  this->SetNumberOfVectorComponents(0);

  this->LabelTextProperty->Delete();
  this->TitleTextProperty->Delete();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();
  
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(
    this->GetApplication());

  this->CreateParallelTclObjects(pvApp);

  // Scalar bar : Label control

  this->SetScalarBarLabelFormat(this->GetLabelFormatInternal());

  this->SetColorSchemeToBlueRed();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::GetLabelTextPropertyInternal()
{
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelColor"));
  if (dvp)
    {
    this->LabelTextProperty->SetColor(dvp->GetElements());
    }
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelOpacity"));
  if (dvp)
    {
    this->LabelTextProperty->SetOpacity(dvp->GetElement(0));
    }
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelFontFamily"));
  if (ivp)
    {
    this->LabelTextProperty->SetFontFamily(ivp->GetElement(0));
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelBold"));
  if (ivp)
    {
    this->LabelTextProperty->SetBold(ivp->GetElement(0));
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelItalic"));
  if (ivp)
    {
    this->LabelTextProperty->SetItalic(ivp->GetElement(0));
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelShadow"));
  if (ivp)
    {
    this->LabelTextProperty->SetShadow(ivp->GetElement(0));
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::GetTitleTextPropertyInternal()
{
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleColor"));
  if (dvp)
    {
    this->TitleTextProperty->SetColor(dvp->GetElements());
    }
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleOpacity"));
  if (dvp)
    {
    this->TitleTextProperty->SetOpacity(dvp->GetElement(0));
    }
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleFontFamily"));
  if (ivp)
    {
    this->TitleTextProperty->SetFontFamily(ivp->GetElement(0));
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleBold"));
  if (ivp)
    {
    this->TitleTextProperty->SetBold(ivp->GetElement(0));
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleItalic"));
  if (ivp)
    {
    this->TitleTextProperty->SetItalic(ivp->GetElement(0));
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleShadow"));
  if (ivp)
    {
    this->TitleTextProperty->SetShadow(ivp->GetElement(0));
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetNumberOfVectorComponents(int  num)
{
  int idx;

  if (this->IsCreated() && num != 0)
    {
    vtkErrorMacro("You must set the number of vector components before "
      "you create this color map.");
    return;
    }

  if (num == this->NumberOfVectorComponents)
    {
    return;
    }

  // Get rid of old arrays.
  // Use for delete.  This number shold not be changed after creation.
  if (this->VectorComponentTitles)
    {
    for (idx = 0; idx < this->NumberOfVectorComponents; ++idx)
      {
      delete [] this->VectorComponentTitles[idx];
      this->VectorComponentTitles[idx] = NULL;
      }
    }

  delete[] this->VectorComponentTitles;
  this->VectorComponentTitles = NULL;

  this->NumberOfVectorComponents = num;

  // Set defaults for component titles.
  if (num > 0)
    {
    this->VectorComponentTitles = new char* [num];
    }
  for (idx = 0; idx < num; ++idx)
    {
    this->VectorComponentTitles[idx] = new char[4];  
    }
  if (num == 3)
    { // Use XYZ for default of three component vectors.
    strcpy(this->VectorComponentTitles[0], "X");
    strcpy(this->VectorComponentTitles[1], "Y");
    strcpy(this->VectorComponentTitles[2], "Z");
    }
  else
    {
    for (idx = 0; idx < num; ++idx)
      {
      sprintf(this->VectorComponentTitles[idx], "%d", idx+1);
      }
    }
  const char* arrayname = this->GetArrayName();
  if ( arrayname != NULL)
    {
    char *str2;
    str2 = new char [strlen(arrayname) + 128];
    sprintf(str2, "GetPVColorMap {%s} %d", arrayname, 
      this->NumberOfVectorComponents);
    this->GetTraceHelper()->SetReferenceCommand(str2);
    delete [] str2;
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  this->vtkKWObject::SetApplication(pvApp);
  
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  static int proxyNum = 0;
  // Create LookupTableProxy
  this->LookupTableProxy = vtkSMLookupTableProxy::SafeDownCast(
    pxm->NewProxy("lookup_tables","LookupTable"));
  if (!this->LookupTableProxy)
    {
    vtkErrorMacro("Failed to create LookupTableProxy");
    return;
    }
  this->LookupTableProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  ostrstream str1;
  str1 << "PVColorMap_LookupTable" << proxyNum << ends;
  this->SetLookupTableProxyName(str1.str());
  str1.rdbuf()->freeze(0);
  proxyNum++;
  pxm->RegisterProxy("lookup_tables", this->LookupTableProxyName , 
    this->LookupTableProxy);
  this->LookupTableProxy->CreateVTKObjects(1);
    
  // Create ScalarBarProxy
  this->ScalarBarProxy = vtkSMScalarBarWidgetProxy::SafeDownCast(
    pxm->NewProxy("displays","ScalarBarWidget"));
  if (!this->ScalarBarProxy)
    {
    vtkErrorMacro("Failed to create ScalarBarWidget proxy");
    return;
    }
  ostrstream str;
  str << "PVColorMap_ScalarBarWidget" << proxyNum << ends;
  this->SetScalarBarProxyName(str.str());
  str.rdbuf()->freeze(0);
  proxyNum++;
  pxm->RegisterProxy("displays",this->ScalarBarProxyName,this->ScalarBarProxy);
  this->ScalarBarProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->ScalarBarProxy->UpdateVTKObjects();
  this->InitializeObservers();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LookupTable"));
  if (!pp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property LookupTable");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->LookupTableProxy);
  this->ScalarBarProxy->UpdateVTKObjects();

  // Add to rendermodule.
  vtkSMRenderModuleProxy* rm = this->GetPVApplication()->GetRenderModuleProxy();
  pp =  vtkSMProxyProperty::SafeDownCast(
    rm->GetProperty("Displays"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Displays on RenderModuleProxy.");
    }
  else
    {
    pp->AddProxy(this->ScalarBarProxy);
    rm->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkPVColorMap::GetProxyByName(const char* name)
{
  if (strcmp(name,"LookupTable") == 0)
    {
    return this->LookupTableProxy;
    }
  if (strcmp(name,"ScalarBarWidget") == 0)
    {
    return this->ScalarBarProxy;
    }
  vtkErrorMacro("Unknow proxy name : " << name);
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVColorMap::InitializeObservers()
{
  this->ScalarBarObserver = vtkPVColorMapObserver::New();
  this->ScalarBarObserver->PVColorMap = this;

  this->ScalarBarProxy->AddObserver(vtkCommand::InteractionEvent,
    this->ScalarBarObserver);
  this->ScalarBarProxy->AddObserver(vtkCommand::StartInteractionEvent, 
    this->ScalarBarObserver);
  this->ScalarBarProxy->AddObserver(vtkCommand::EndInteractionEvent, 
    this->ScalarBarObserver);
  this->ScalarBarProxy->AddObserver(vtkCommand::WidgetModifiedEvent,
    this->ScalarBarObserver);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::RenderView()
{
  if (this->PVRenderView)
    {
    this->PVRenderView->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetArrayName(const char* str)
{

  this->SetArrayNameInternal(str);

  if (str != NULL)
    {
    char *str2;
    str2 = new char [strlen(str) + 128];
    sprintf(str2, "GetPVColorMap {%s} %d", str, this->NumberOfVectorComponents);
    this->GetTraceHelper()->SetReferenceCommand(str2);
    delete [] str2;
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetArrayNameInternal(const char* str)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("ArrayName"));
  if (!svp)
    {
    vtkErrorMacro("LookupTableProxy does not have property ArrayName");
    return;
    }
  svp->SetElement(0,str);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
int vtkPVColorMap::MatchArrayName(const char* str, int numberOfComponents)
{
  const char* arrayname = this->GetArrayName();
  if (str == NULL || arrayname == NULL)
    {
    return 0;
    }
  if (strcmp(str, arrayname) == 0 &&
    numberOfComponents == this->NumberOfVectorComponents)
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarTitle(const char* name)
{
  this->SetScalarBarTitleNoTrace(name);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarTitle {%s}", 
    this->GetTclName(), name);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarTitleNoTrace(const char* name)
{
  if (this->ScalarBarTitle == NULL && name == NULL) 
    { 
    return;
    }

  if (this->ScalarBarTitle && name && (!strcmp(this->ScalarBarTitle, name))) 
    { 
    return;
    }

  if (this->ScalarBarTitle) 
    { 
    delete [] this->ScalarBarTitle; 
    this->ScalarBarTitle = NULL;
    }

  if (name)
    {
    this->ScalarBarTitle = new char[strlen(name) + 1];
    strcpy(this->ScalarBarTitle, name);
    } 
  this->UpdateScalarBarTitle();
  this->RenderView();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitle(const char* title)
{
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("Title"));
  if (!svp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property Title");
    return;
    }
  svp->SetElement(0,title);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarVectorTitle(const char* name)
{
  if (this->GetVectorModeInternal() == vtkLookupTable::MAGNITUDE)
    {
    if (this->VectorMagnitudeTitle == NULL && name == NULL) 
      { 
      return;
      }

    if (this->VectorMagnitudeTitle && name && 
      (!strcmp(this->VectorMagnitudeTitle, name)))
      {
      return;
      }

    if (this->VectorMagnitudeTitle)
      {
      delete [] this->VectorMagnitudeTitle;
      this->VectorMagnitudeTitle = NULL;
      }

    if (name)
      {
      this->VectorMagnitudeTitle = new char[strlen(name) + 1];
      strcpy(this->VectorMagnitudeTitle, name);
      }

    this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarVectorTitle {%s}", 
      this->GetTclName(), name);
    }
  else
    {
    if (this->VectorComponentTitles == NULL)
      {
      return;
      }

    if (this->VectorComponentTitles[this->VectorComponent] == NULL && 
      name == NULL) 
      { 
      return;
      }

    if (this->VectorComponentTitles[this->VectorComponent] && 
      name && 
      (!strcmp(this->VectorComponentTitles[this->VectorComponent], name)))
      {
      return;
      }

    if (this->VectorComponentTitles[this->VectorComponent])
      {
      delete [] this->VectorComponentTitles[this->VectorComponent];
      this->VectorComponentTitles[this->VectorComponent] = NULL;
      }

    if (name)
      {
      this->VectorComponentTitles[this->VectorComponent] = 
        new char[strlen(name) + 1];
      strcpy(this->VectorComponentTitles[this->VectorComponent], name);
      }

    this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarVectorTitle {%s}", 
      this->GetTclName(), name);
    }
    this->UpdateScalarBarTitle();
    this->Modified();
    this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarLabelFormat(const char* name)
{
  this->SetLabelFormatInternal(name);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarLabelFormat {%s}", 
                      this->GetTclName(), name);
  this->RenderView();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelFormatInternal(const char* format)
{
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelFormat"));
  if (!svp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property LabelFormat");
    return;
    }
  svp->SetElement(0,format);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
// Access for trace files.
void vtkPVColorMap::SetNumberOfColors(int num)
{
  this->SetNumberOfColorsInternal(num);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetNumberOfColors %d", this->GetTclName(),
    num);
  this->UpdateMap();
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetNumberOfColorsInternal(int num)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("NumberOfTableValues"));
  if (!ivp)
    {
    vtkErrorMacro("LookupTableProxy does not have property NumberOfTableValues");
    return;
    }
  ivp->SetElement(0,num);
  this->LookupTableProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
int vtkPVColorMap::GetNumberOfColorsInternal()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("NumberOfTableValues"));
  if (!ivp)
    {
    vtkErrorMacro("LookupTableProxy does not have property NumberOfTableValues");
    return 0;
    }
  return ivp->GetElement(0);
}
//----------------------------------------------------------------------------
void vtkPVColorMap::SetColorSchemeToRedBlue()
{
  double hr[2],sr[2],vr[2];
  hr[0] = VTK_PV_COLOR_MAP_RED_HUE;
  sr[0] = VTK_PV_COLOR_MAP_RED_SATURATION;
  vr[0] = VTK_PV_COLOR_MAP_RED_VALUE;
  
  hr[1] = VTK_PV_COLOR_MAP_BLUE_HUE;
  sr[1] = VTK_PV_COLOR_MAP_BLUE_SATURATION;
  vr[1] = VTK_PV_COLOR_MAP_BLUE_VALUE;

  this->SetStartColor(1.0, 0.0, 0.0);
  this->SetEndColor(0.0, 0.0, 1.0);

  this->SetHSVRangesInternal(hr,sr,vr);

  this->GetTraceHelper()->AddEntry("$kw(%s) SetColorSchemeToRedBlue", this->GetTclName());
  this->UpdateMap();
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetColorSchemeToBlueRed()
{
  double hr[2],sr[2],vr[2];
  hr[0] = VTK_PV_COLOR_MAP_BLUE_HUE;
  sr[0] = VTK_PV_COLOR_MAP_BLUE_SATURATION;
  vr[0] = VTK_PV_COLOR_MAP_BLUE_VALUE;
  hr[1] = VTK_PV_COLOR_MAP_RED_HUE;
  sr[1] = VTK_PV_COLOR_MAP_RED_SATURATION;
  vr[1] = VTK_PV_COLOR_MAP_RED_VALUE;

  this->SetStartColor(0.0, 0.0, 1.0);
  this->SetEndColor(1.0, 0.0, 0.0);

  this->SetHSVRangesInternal(hr,sr,vr);

  this->GetTraceHelper()->AddEntry("$kw(%s) SetColorSchemeToBlueRed", this->GetTclName());
  this->UpdateMap();
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetColorSchemeToGrayscale()
{
  double hr[2],sr[2],vr[2];
  hr[0] = VTK_PV_COLOR_MAP_BLACK_HUE;
  sr[0] = VTK_PV_COLOR_MAP_BLACK_SATURATION;
  vr[0] = VTK_PV_COLOR_MAP_BLACK_VALUE;
  hr[1] = VTK_PV_COLOR_MAP_WHITE_HUE;
  sr[1] = VTK_PV_COLOR_MAP_WHITE_SATURATION;
  vr[1] = VTK_PV_COLOR_MAP_WHITE_VALUE;

  this->SetStartColor(0.0, 0.0, 0.0);
  this->SetEndColor(1.0, 1.0, 1.0);

  this->SetHSVRangesInternal(hr,sr,vr);

  this->GetTraceHelper()->AddEntry("$kw(%s) SetColorSchemeToGrayscale", this->GetTclName());
  this->UpdateMap();
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetColorSchemeToLabBlueRed()
{
  double hr[2],sr[2],vr[2];
  double startHSV[3];
  double endHSV[3];
  //first calculate the gradient values
  hr[0]=57.93;
  sr[0]=-26.85;
  vr[0]=-30.21;
  startHSV[0] = hr[0];
  startHSV[1] = sr[0];
  startHSV[2] = vr[0];
  
  hr[1]=57.93;
  sr[1]=26.85;
  vr[1]=30.21;
  endHSV[0] = hr[1];
  endHSV[1] = sr[1];
  endHSV[2] = vr[1];

  double rgb[3];
  double xyz[3];
  vtkMath::LabToXYZ(startHSV,xyz);
  vtkMath::XYZToRGB(xyz,rgb);
  this->SetStartColor(rgb[0],rgb[1],rgb[2]);
  vtkMath::LabToXYZ(endHSV,xyz);
  vtkMath::XYZToRGB(xyz,rgb);
  this->SetEndColor(rgb[0],rgb[1],rgb[2]);

  this->SetHSVRangesInternal(hr,sr,vr);

  this->GetTraceHelper()->AddEntry("$kw(%s) SetColorSchemeToLabBlueRed", this->GetTclName());
  this->UpdateMap();
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::StartColorButtonCallback(double r, double g, double b)
{
  double rgb[3];
  double hsv[3];

  // Convert RGB to HSV.
  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
  vtkMath::RGBToHSV(rgb, hsv);

  this->SetStartHSV(hsv[0], hsv[1], hsv[2]);
  
  // Lab color map uses hue > 1.1 as a flag.
  // We need to make sure the end color hue is not > 1.
  double hrange[2];
  this->GetHueRangeInternal(hrange);
  if (hrange[1] > 1.1)
    {
    double *tmp = this->GetEndColor();
    vtkMath::RGBToHSV(tmp, hsv);
    this->SetEndHSV(hsv[0], hsv[1], hsv[2]);
    }  
}

//----------------------------------------------------------------------------
// Access for trace files.
void vtkPVColorMap::SetStartHSV(double h, double s, double v)
{
  double hr[2],sr[2],vr[2];
  double hsv[3];
  double rgb[3];

  this->GetHueRangeInternal(hr);
  this->GetSaturationRangeInternal(sr);
  this->GetValueRangeInternal(vr);

  if ( h == hr[0] && s == sr[0] && v == vr[0])
    {
    return;
    }

  // Change color button (should have no callback effect...)
  hsv[0] = h;  hsv[1] = s;  hsv[2] = v;
  if (hsv[0] > 1.1)
    { // Detect the Sandia hack for changing the interpolation.
    double xyz[3];
    vtkMath::LabToXYZ(hsv,xyz);
    vtkMath::XYZToRGB(xyz,rgb);
    this->SetStartColor(rgb[0],rgb[1],rgb[2]);    
    }
  else
    {
    vtkMath::HSVToRGB(hsv, rgb);
    this->SetStartColor(rgb);
    }
  hr[0] = h; sr[0] = s; vr[0] = v;
  this->SetHSVRangesInternal(hr,sr,vr);

  this->GetTraceHelper()->AddEntry("$kw(%s) SetStartHSV %g %g %g", 
                      this->GetTclName(), h, s, v);

  this->UpdateMap();
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetHSVRangesInternal(double hrange[2],
  double srange[2], double vrange[2])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("HueRange"));
  if (!dvp)
    {
    vtkErrorMacro("LookupTableProxy does not have property HueRange");
    }
  dvp->SetElements(hrange);

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("SaturationRange"));
  if (!dvp)
    {
    vtkErrorMacro("LookupTableProxy does not have property SaturationRange");
    }
  dvp->SetElements(srange);

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("ValueRange"));
  if (!dvp)
    {
    vtkErrorMacro("LookupTableProxy does not have property ValueRange");
    }
  dvp->SetElements(vrange);

  this->LookupTableProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::EndColorButtonCallback(double r, double g, double b)
{
  double rgb[3];
  double hsv[3];

  // Convert RGB to HSV.
  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
  vtkMath::RGBToHSV(rgb, hsv);

  this->SetEndHSV(hsv[0], hsv[1], hsv[2]);

  // Lab color map uses hue > 1.1 as a flag.
  // We need to make sure the start color hue is not > 1.
  double hrange[2];
  this->GetHueRangeInternal(hrange);
  if (hrange[0] > 1.1)
    {
    double *tmp = this->GetStartColor();
    vtkMath::RGBToHSV(tmp, hsv);
    this->SetStartHSV(hsv[0], hsv[1], hsv[2]);
    }
}

//----------------------------------------------------------------------------
// Access for trace files.
void vtkPVColorMap::SetEndHSV(double h, double s, double v)
{
  double hr[2],sr[2],vr[2];
  double hsv[3];
  double rgb[3];
  
  this->GetHueRangeInternal(hr);
  this->GetSaturationRangeInternal(sr);
  this->GetValueRangeInternal(vr);
  
  if ( hr[1] == h && sr[1] == s && vr[1] == v)
    {
    return;
    }

  // Change color button (should have no callback effect...)
  hsv[0] = h;  hsv[1] = s;  hsv[2] = v;
  if (hsv[0] > 1.1)
    { // Detect the Sandia hack for changing the interpolation.
    double xyz[3];
    vtkMath::LabToXYZ(hsv,xyz);
    vtkMath::XYZToRGB(xyz,rgb);
    this->SetEndColor(rgb[0],rgb[1],rgb[2]);    
    }
  else
    {
    vtkMath::HSVToRGB(hsv, rgb);
    this->SetEndColor(rgb);
    }
  hr[1] = h; sr[1] = s; vr[1] = v;
  this->SetHSVRangesInternal(hr,sr,vr);
  //this->SetHueRangeInternal(hr);
  //this->SetSaturationRangeInternal(sr);
  //this->SetValueRangeInternal(vr);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetEndHSV %g %g %g", 
                      this->GetTclName(), h, s, v);
  this->UpdateMap();
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarRangeLock(int val)
{
  if (this->ScalarRangeLock == val)
    {
    return;
    }
  this->ScalarRangeLock = val;
  if ( ! val)
    {
    this->ResetScalarRange();
    }

  // Trace.
  this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarRangeLock %d", 
                      this->GetTclName(), val);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetVectorComponent(int component)
{
  this->SetVectorComponentInternal(component);
  this->UpdateScalarBarTitle();
  this->GetTraceHelper()->AddEntry("$kw(%s) SetVectorComponent %d", 
                      this->GetTclName(), component);
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetVectorComponentInternal(int component)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("VectorComponent"));
  if (!ivp)
    {
    vtkErrorMacro("LookupTableProxy does not have property VectorComponent");
    return;
    }
  ivp->SetElement(0,component);
  this->VectorComponent = component;
  this->LookupTableProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::ResetScalarRange()
{
  this->ResetScalarRangeInternal();
  this->GetTraceHelper()->AddEntry("$kw(%s) ResetScalarRange", this->GetTclName());
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVColorMap::GetPVApplication()
{
  if (this->GetApplication() == NULL)
    {
    return NULL;
    }
  
  if (this->GetApplication()->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->GetApplication());
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
void vtkPVColorMap::IncrementUseCount()
{
  ++this->UseCount;
  this->UpdateInternalScalarBarVisibility();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::DecrementUseCount()
{
  --this->UseCount;
  this->UpdateInternalScalarBarVisibility();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarVisibility(int val)
{
  if (this->ScalarBarVisibility == val)
    {
    return;
    }
  this->ScalarBarVisibility = val;
  
  this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarVisibility %d", this->GetTclName(),
                      val);
  
  this->UpdateInternalScalarBarVisibility();

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::UpdateInternalScalarBarVisibility()
{
  int visible = this->ScalarBarVisibility;

  if (this->UseCount == 0)
    {
    visible = 0;
    }

  if (this->InternalScalarBarVisibility == visible)
    {
    return;
    }
  this->InternalScalarBarVisibility = visible;


  if (!this->GetPVRenderView())
    {
    return;
    }
  
  this->SetVisibilityInternal(visible);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetVisibilityInternal(int visible)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property Visibility");
    return;
    }
  ivp->SetElement(0,visible);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SaveInBatchScript(ofstream *file)
{
  if (this->VisitedFlag)
    {
    return;
    }
  this->VisitedFlag = 1;
  this->LookupTableProxy->SaveInBatchScript(file);
  this->ScalarBarProxy->SaveInBatchScript(file);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::GetHueRangeInternal(double range[2])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("HueRange"));
  if (!dvp || dvp->GetNumberOfElements() != 2)
    {
    vtkErrorMacro("LookupTableProxy does not have property HueRange");
    range[0] = range[1] = 0.0;
    return;
    }
  range[0] = dvp->GetElement(0);
  range[1] = dvp->GetElement(1);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::GetSaturationRangeInternal(double range[2])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("SaturationRange"));
  if (!dvp || dvp->GetNumberOfElements() != 2)
    {
    vtkErrorMacro("LookupTableProxy does not have property SaturationRange");
    range[0] = range[1] = 0.0;
    return;
    }
  range[0] = dvp->GetElement(0);
  range[1] = dvp->GetElement(1);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::GetValueRangeInternal(double range[2])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("ValueRange"));
  if (!dvp || dvp->GetNumberOfElements() != 2)
    {
    vtkErrorMacro("LookupTableProxy does not have property ValueRange");
    range[0] = range[1] = 0.0;
    return;
    }
  range[0] = dvp->GetElement(0);
  range[1] = dvp->GetElement(1);
}

//----------------------------------------------------------------------------
int vtkPVColorMap::GetVectorModeInternal()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("VectorMode"));
  if (!ivp)
    {
    vtkErrorMacro("LookupTableProxy does not have property VectorMode");
    return 0;
    }
  return ivp->GetElement(0);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::UpdateScalarBarTitle()
{
  if (this->ScalarBarTitle == NULL || this->GetPVApplication() == NULL)
    {
    return;
    }
  
  if (this->GetVectorModeInternal() == vtkLookupTable::MAGNITUDE &&
      this->NumberOfVectorComponents > 1)
    {
    ostrstream ostr;
    ostr << this->ScalarBarTitle << " " << this->VectorMagnitudeTitle << ends;
    this->SetTitle(ostr.str()); 
    ostr.rdbuf()->freeze(0);    
    }
  else if (this->NumberOfVectorComponents == 1)
    {
    this->SetTitle(this->ScalarBarTitle);
    }
  else
    {
    ostrstream ostr;
    ostr << this->ScalarBarTitle << " " 
         << this->VectorComponentTitles[this->VectorComponent] << ends;
    this->SetTitle(ostr.str());
    ostr.rdbuf()->freeze(0);    
    }
  
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetVectorMode(int mode)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("VectorMode"));
  if (!ivp)
    {
    vtkErrorMacro("LookupTableProxy does not have property VectorMode");
    return;
    }
  ivp->SetElement(0,mode);
  this->LookupTableProxy->UpdateVTKObjects();

  this->ResetScalarRangeInternal();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::UpdateMap()
{
  if (this->MapWidth && this->MapHeight)
    {
    this->UpdateMap(this->MapWidth,this->MapHeight);
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::UpdateMap(int width, int height)
{
  int size;
  int i, j;
  double *range;
  double *wholeRange;
  double val, step;
  unsigned char *rgba;  
  unsigned char *ptr;  

  size = width*height;
  if (this->MapDataSize < size)
    {
    if (this->MapData)
      {
      delete [] this->MapData;
      }
    this->MapData = new unsigned char[size*3];
    this->MapDataSize = size;
    }
  this->MapWidth = width;
  this->MapHeight = height;

  vtkProcessModule* pm =vtkProcessModule::GetProcessModule();
  vtkLookupTable* lut = NULL;
  if (this->LookupTableProxy)
    {
    lut = vtkLookupTable::SafeDownCast(
      pm->GetObjectFromID(this->LookupTableProxy->GetID(0)));
    }
  if (!lut)
    {
    return;
    }

  range = this->ScalarRange;
  wholeRange = this->WholeScalarRange;

  //step = (range[1]-range[0])/(double)(width);
  step = (wholeRange[1]-wholeRange[0])/(double)(width);
  ptr = this->MapData;
  for (j = 0; j < height; ++j)
    {
    for (i = 0; i < width; ++i)
      {
      // Lets have x be constant whole range.
      // Color map shrinks as map range changes.
      val = wholeRange[0] + ((double)(i)*step);
      
      // Colors clamp to min or max out of color map range.
      if (val < range[0])
        {
        val = range[0];
        }
      if (val > range[1])
        {
        val = range[1];
        }
        
      rgba = lut->MapValue(val);
      ptr[0] = rgba[0];
      ptr[1] = rgba[1];
      ptr[2] = rgba[2];
      ptr += 3;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarPosition1(float x, float y)
{
  this->SetPosition1Internal(x,y);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarPosition1 %f %f", 
                      this->GetTclName(), x, y);
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetPosition1Internal(double x, double y)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("Position"));
  if (!dvp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property Position");
    return;
    }
  dvp->SetElement(0, x);
  dvp->SetElement(1, y);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::GetPosition1Internal(double pos[2])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("Position"));
  if (!dvp || dvp->GetNumberOfElements() != 2 )
    {
    vtkErrorMacro("ScalarBarProxy does not have property Position"
      " or it does not have two elements");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
}  

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarPosition2(float x, float y)
{
  this->SetPosition2Internal(x,y);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarPosition2 %f %f", 
                      this->GetTclName(), x, y);
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetPosition2Internal(double x, double y)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("Position2"));
  if (!dvp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property Position2");
    return;
    }
  dvp->SetElement(0, x);
  dvp->SetElement(1, y);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::GetPosition2Internal(double pos[2])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("Position2"));
  if (!dvp && dvp->GetNumberOfElements() != 2 )
    {
    vtkErrorMacro("ScalarBarProxy does not have property Position2"
      " or it does not have two elements");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarOrientation(int o)
{
  this->SetOrientationInternal(o);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarOrientation %d", 
                      this->GetTclName(), o);
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetOrientationInternal(int orientation)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("Orientation"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property Orientation");
    return;
    }
  ivp->SetElement(0, orientation);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
int vtkPVColorMap::GetOrientationInternal()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("Orientation"));
  if (!ivp && ivp->GetNumberOfElements() != 1 )
    {
    vtkErrorMacro("ScalarBarProxy does not have property Orientation"
      " or it does not have 1 element");
    return 0;
    }
  return ivp->GetElement(0); 
}
  
//----------------------------------------------------------------------------
const char* vtkPVColorMap::GetArrayName() 
{ 
  if (this->GetPVApplication() && this->ScalarBarProxy)
    {
    return this->GetArrayNameInternal();
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkPVColorMap::GetArrayNameInternal()
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("ArrayName"));
  if (!svp || svp->GetNumberOfElements() != 1)
    {
    vtkErrorMacro("LookupTableProxy does not have property ArrayName");
    return NULL;
    }
  return svp->GetElement(0);
}

//----------------------------------------------------------------------------
int vtkPVColorMap::GetNumberOfVectorComponents()
{
  return this->NumberOfVectorComponents;
}

//----------------------------------------------------------------------------
const char* vtkPVColorMap::GetScalarBarLabelFormat()
{
  return this->GetLabelFormatInternal();
}

//----------------------------------------------------------------------------
const char* vtkPVColorMap::GetLabelFormatInternal()
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelFormat"));
  if (!svp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property LabelFormat");
    return "";
    }
  return svp->GetElement(0);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleColor(double r, double g, double b)
{
  this->SetTitleColorInternal(r,g,b);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetTitleColor %g %g %g", this->GetTclName(),
                      r, g, b);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleOpacity(double opacity)
{
  this->SetTitleOpacityInternal(opacity);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetTitleOpacity %g ", this->GetTclName(),
                      opacity);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleFontFamily(int font)
{
  this->SetTitleFontFamilyInternal(font);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetTitleFontFamily %d ", this->GetTclName(),
                      font);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleBold(int bold)
{
  this->SetTitleBoldInternal(bold);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetTitleBold %d ", this->GetTclName(),
                      bold);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleItalic(int italic)
{
  this->SetTitleItalicInternal(italic);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetTitleItalic %d ", this->GetTclName(),
                      italic);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleShadow(int shadow)
{
  this->SetTitleShadowInternal(shadow);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetTitleShadow %d ", this->GetTclName(),
                      shadow);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleColorInternal(double r, double g, double b)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleColor"));
  if (!dvp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property TitleColor");
    return;
    }
  dvp->SetElement(0,r);
  dvp->SetElement(1,g);
  dvp->SetElement(2,b);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleOpacityInternal(double opacity)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleOpacity"));
  if (!dvp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property TitleFormatOpacity");
    return;
    }
  dvp->SetElement(0,opacity);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleFontFamilyInternal(int font)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleFontFamily"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property TitleFontFamily");
    return;
    }
  ivp->SetElement(0, font);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleBoldInternal(int bold)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleBold"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property TitleBold");
    return;
    }
  ivp->SetElement(0,bold);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleItalicInternal(int italic)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleItalic"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property TitleItalic");
    return;
    }
  ivp->SetElement(0,italic);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetTitleShadowInternal(int shadow)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("TitleShadow"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property TitleShadow");
    return;
    }
  ivp->SetElement(0,shadow);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelColor(double r, double g, double b)
{
  this->SetLabelColorInternal(r,g,b);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLabelColor %g %g %g", this->GetTclName(),
                      r, g, b);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelOpacity(double opacity)
{
  this->SetLabelOpacityInternal(opacity);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLabelOpacity %g ", this->GetTclName(),
                      opacity);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelFontFamily(int font)
{
  this->SetLabelFontFamilyInternal(font);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLabelFontFamily %d ", this->GetTclName(),
                      font);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelBold(int bold)
{
  this->SetLabelBoldInternal(bold);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLabelBold %d ", this->GetTclName(),
                      bold);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelItalic(int italic)
{
  this->SetLabelItalicInternal(italic);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLabelItalic %d ", this->GetTclName(),
                      italic);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelShadow(int shadow)
{
  this->SetLabelShadowInternal(shadow);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLabelShadow %d ", this->GetTclName(),
                      shadow);
  this->Modified();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelColorInternal(double r, double g, double b)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelColor"));
  if (!dvp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property LabelColor");
    return;
    }

  dvp->SetElement(0,r);
  dvp->SetElement(1,g);
  dvp->SetElement(2,b);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelOpacityInternal(double opacity)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelOpacity"));
  if (!dvp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property LabelOpacity");
    return;
    }
  dvp->SetElement(0,opacity);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelFontFamilyInternal(int font)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelFontFamily"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property LabelFontFamily");
    return;
    }
  ivp->SetElement(0,font);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelBoldInternal(int bold)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelBold"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property LabelBold");
    return;
    }
  ivp->SetElement(0,bold);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelItalicInternal(int italic)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelItalic"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property LabelItalic");
    return;
    }
  ivp->SetElement(0,italic);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLabelShadowInternal(int shadow)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("LabelShadow"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property LabelShadow");
    return;
    }
  ivp->SetElement(0,shadow);
  this->ScalarBarProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::ExecuteEvent(vtkObject* vtkNotUsed(wdg), 
                                 unsigned long event,  
                                 void* vtkNotUsed(calldata))
{
  switch ( event )
    {
    case vtkCommand::WidgetModifiedEvent:
      // the only things that have changed are the positions.
      // since we have no GUI for positions, we don't have any work to do here.
      //this->RenderView();
      // We don;t call EventuallyRender since that leads to 
      // non-smooth movement.
      this->GetPVApplication()->GetMainWindow()->GetInteractor()->Render();
      this->Modified();
      break;

    case vtkCommand::StartInteractionEvent:
      this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOn();
      this->RenderView();
      break;

    case vtkCommand::EndInteractionEvent:
      this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOff();
      this->RenderView();
      
      double pos1[2], pos2[2];
      this->GetPosition1Internal(pos1);
      this->GetPosition2Internal(pos2);
      
      this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarPosition1 %lf %lf", 
                          this->GetTclName(), pos1[0], pos1[1]);
      this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarPosition2 %lf %lf", 
                          this->GetTclName(), pos2[0], pos2[1]);
      this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarBarOrientation %d",
                          this->GetTclName(), this->GetOrientationInternal());
      break;
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SaveState(ofstream *file)
{

  *file << "set kw(" << this->GetTclName() << ") [$kw(" 
        << this->PVRenderView->GetPVWindow()->GetTclName() 
        << ") GetPVColorMap {" 
        << this->GetArrayNameInternal() << "} " 
        << this->NumberOfVectorComponents << "]\n";

  *file << "$kw(" << this->GetTclName() << ") SetScalarBarTitle {"
    << this->ScalarBarTitle << "}\n";
  
  *file << "$kw(" << this->GetTclName() << ") SetScalarBarVectorTitle {" 
    << this->VectorMagnitudeTitle << "}\n"; 
  
  *file << "$kw(" << this->GetTclName() << ") SetScalarBarLabelFormat {" 
    << this->GetLabelFormatInternal() << "}\n"; 
 
  double hr[2],sr[2],vr[2];
  this->GetHueRangeInternal(hr);
  this->GetSaturationRangeInternal(sr);
  this->GetValueRangeInternal(vr);

  *file << "$kw(" << this->GetTclName() << ") SetStartHSV " 
    << hr[0] << " " << sr[0] << " " << vr[0] << endl;
  *file << "$kw(" << this->GetTclName() << ") SetEndHSV " 
    << hr[1] << " " << sr[1] << " " << vr[1] << endl;

  *file << "$kw(" << this->GetTclName() << ") SetNumberOfColors " 
    << this->GetNumberOfColorsInternal() << endl;
  
  *file << "$kw(" << this->GetTclName() << ") SetScalarRange " 
    << this->ScalarRange[0] << " " << this->ScalarRange[1] << endl;
        
  *file << "$kw(" << this->GetTclName() << ") SetScalarRangeLock " 
        << this->ScalarRangeLock << "\n"; 

  *file << "$kw(" << this->GetTclName() << ") SetScalarBarVisibility " 
        << this->ScalarBarVisibility << endl;

  double pos1[2], pos2[2];
  this->GetPosition1Internal(pos1);
  this->GetPosition2Internal(pos2);

  *file << "$kw(" << this->GetTclName() << ") SetScalarBarPosition1 " 
    << pos1[0] << " " << pos1[1] << endl; 
  *file << "$kw(" << this->GetTclName() << ") SetScalarBarPosition2 " 
    << pos2[0] << " " << pos2[1] << endl;
  *file << "$kw(" << this->GetTclName() << ") SetScalarBarOrientation " 
    << this->GetOrientationInternal() << endl;
}

//----------------------------------------------------------------------------
int vtkPVColorMap::ComputeWholeScalarRange(double range[2])
{
  double tmp[2];
  vtkPVSource *pvs;
  vtkPVSourceCollection *sourceList;

  if (this->GetApplication() == NULL || this->PVRenderView == NULL)
    {
    vtkErrorMacro("Trying to reset scalar range without application and view.");
    return 0;
    }

  // Compute global scalar range ...
  range[0] = VTK_LARGE_FLOAT;
  range[1] = -VTK_LARGE_FLOAT;
  sourceList = this->PVRenderView->GetPVWindow()->GetSourceList("Sources");
  sourceList->InitTraversal();
  while ( (pvs = sourceList->GetNextPVSource()) )
    {    
    this->ComputeScalarRangeForSource(pvs, tmp);
    if (range[0] > tmp[0])
      {
      range[0] = tmp[0];
      }
    if (range[1] < tmp[1])
      {
      range[1] = tmp[1];
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVColorMap::ResetScalarRangeInternal()
{
  double range[2];
  if (!this->ComputeWholeScalarRange(range))
    {
    return;
    }

  // Get the computed range from the whole range.
  this->SetWholeScalarRange(range[0], range[1]);
  this->SetScalarRangeInternal(range[0], range[1]);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::ResetScalarRangeInternal(vtkPVSource* pvs)
{
  if (this->GetApplication() == NULL || this->PVRenderView == NULL)
    {
    vtkErrorMacro("Trying to reset scalar range without application and view.");
    return;
    }
    
  double range[2];
  this->ComputeScalarRangeForSource(pvs, range);
  this->SetScalarRangeInternal(range[0], range[1]);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::UpdateForSource(vtkPVSource* pvs)
{
  double range[2];

  this->ComputeScalarRangeForSource(pvs, range);
  // Do we need to expand the whole range?
  if (range[0] > this->WholeScalarRange[0])
    {
    range[0] = this->WholeScalarRange[0];
    }
  if (range[1] < this->WholeScalarRange[1])
    {
    range[1] = this->WholeScalarRange[1];
    }
  
  // This will expand range if not locked.
  this->SetWholeScalarRange(range[0], range[1]);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::ComputeScalarRange(
  vtkPVDataSetAttributesInformation* attrInfo, double* range)
{
  double tmp[2];

  const char* arrayname = this->GetArrayName();
  int component = this->VectorComponent;
  if (this->GetVectorModeInternal() == vtkLookupTable::MAGNITUDE)
    {
    component = -1;
    }

  vtkPVArrayInformation *ai = attrInfo->GetArrayInformation(arrayname);
  if (ai)
    {
    ai->GetComponentRange(component, tmp);
    if (tmp[0] < range[0])
      {
      range[0] = tmp[0];
      }
    if (tmp[1] > range[1])
      {
      range[1] = tmp[1];
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::ComputeScalarRangeForSource(vtkPVSource* pvs, 
                                                double* range)
{
  if (this->GetApplication() == NULL || this->PVRenderView == NULL)
    {
    vtkErrorMacro("Trying to reset scalar range without application and view.");
    return;
    }

  range[0] = VTK_DOUBLE_MAX;
  range[1] = -VTK_DOUBLE_MAX;

  // First check the geometry information.
  vtkSMDisplayProxy* dproxy = pvs->GetDisplayProxy();
  if (dproxy)
    {
    vtkPVDataInformation* geomInfo = dproxy->GetGeometryInformation();
    this->ComputeScalarRange(
      geomInfo->GetPointDataInformation(), range);
    this->ComputeScalarRange(
      geomInfo->GetCellDataInformation(), range);
    }

  this->ComputeScalarRange(
    pvs->GetDataInformation()->GetPointDataInformation(), range);
  this->ComputeScalarRange(
    pvs->GetDataInformation()->GetCellDataInformation(), range);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarRange(double min, double max)
{
  if (min == this->ScalarRange[0] && max == this->ScalarRange[1])
    {
    return;
    }
  this->SetScalarRangeInternal(min, max);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetScalarRange %g %g", this->GetTclName(),
                      min, max);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarWidgetScalarRangeInternal(double min, double max)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("ScalarRange"));
  if (!dvp)
    {
    vtkErrorMacro("LookupTableProxy does not have property ScalarRange");
    return;
    }
  dvp->SetElement(0,min);
  dvp->SetElement(1,max);
  this->LookupTableProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarRangeInternal(double min, double max)
{
  // Make sure new whole range does not cause problems. 
  if (max < min)
    {
    min = 0.0;
    max = 1.0;
    }
  if (min == max)
    {
    max = min + 0.0001;
    }

  // This will terminate any recursion.
  if (min == this->ScalarRange[0] && max == this->ScalarRange[1])
    {
    return;
    }
  this->ScalarRange[0] = min;
  this->ScalarRange[1] = max;
  this->SetScalarBarWidgetScalarRangeInternal(min,max);    
  
  // Expand whole range if necessary.
  if (min < WholeScalarRange[0] || max > this->WholeScalarRange[1])
    {
    if (min > WholeScalarRange[0])
      {
      min = WholeScalarRange[0];
      }
    if (max < WholeScalarRange[1])
      {
      max = WholeScalarRange[1];
      }
    this->SetWholeScalarRange(min, max);
    }    
    
  // User modified the scalar range.  Lets lock it 
  // so paraview will not over ride the users slection.
  // Also lock if user resets to range of data.
  if (this->ScalarRange[0] > this->WholeScalarRange[0] ||
      this->ScalarRange[1] < this->WholeScalarRange[1] )
    {
    this->ScalarRangeLock = 1;
    }    
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetWholeScalarRange(double min, double max)
{
  // Make sure new whole range does not cause problems. 
  if (max < min)
    {
    min = 0.0;
    max = 1.0;
    }
  if (min == max)
    {
    max = min + 0.0001;
    }

  // This will terminate any recursion.
  if (min == this->WholeScalarRange[0] && max == this->WholeScalarRange[1])
    {
    return;
    }
    
  // Do not allow the whole range to shink smaller than a locked range.
  if (this->ScalarRangeLock)
    {
    if (min > this->ScalarRange[0])
      {
      min = this->ScalarRange[0];
      }
    if (max < this->ScalarRange[1])
      {
      max = this->ScalarRange[1];
      }
    }  
  this->WholeScalarRange[0] = min;
  this->WholeScalarRange[1] = max;
  
  // We might change the range also.
  double newRange[2];
  newRange[0] = this->ScalarRange[0];
  newRange[1] = this->ScalarRange[1];
  // Do not let the range outside of the whole range.
  if (newRange[0] < min)
    {
    newRange[0] = min;
    }
  if (newRange[1] > max)
    {
    newRange[1] = max;
    }
    
  // If the range is not locked by the user,
  // set the range to the whole rnage.
  if ( ! this->ScalarRangeLock)
    { // Expand the range with whole range.
    newRange[0] = min;
    newRange[1] = max;
    }
    
  this->SetScalarRangeInternal(newRange[0], newRange[1]);
}

//----------------------------------------------------------------------------
int vtkPVColorMap::GetNumberOfColors()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LookupTableProxy->GetProperty("NumberOfTableValues"));
  if (!ivp)
    {
    vtkErrorMacro("LookupTableProxy does not have property NumberOfTableValues");
    return 0;
    }
  return ivp->GetElement(0);
}

//----------------------------------------------------------------------------
const char* vtkPVColorMap::GetVectorComponentTitle(int idx)
{
  if (idx < 0 || idx > 2)
    {
    return NULL;
    }
  return this->VectorComponentTitles[idx];
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetNumberOfLabels(int num)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarProxy->GetProperty("NumberOfLabels"));
  if (!ivp)
    {
    vtkErrorMacro("ScalarBarProxy does not have property NumberOfLabels");
    return;
    }

  ivp->SetElement(0, num);
  this->ScalarBarProxy->UpdateVTKObjects();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetLowLookupTableValue(double color[3])
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLowLookupTableValue %f %f %f",
                                   this->GetTclName(),
                                   color[0], color[1], color[2]);
  this->LookupTableProxy->SetLowOutOfRangeColor(color);
  this->LookupTableProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetHighLookupTableValue(double color[3])
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetHighLookupTableValue %f %f %f",
                                   this->GetTclName(),
                                   color[0], color[1], color[2]);
  this->LookupTableProxy->SetHighOutOfRangeColor(color);
  this->LookupTableProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetUseLowOutOfRangeColor(int val)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetUseLowOutOfRangeColor %d",
                                   this->GetTclName(), val);
  this->LookupTableProxy->SetUseLowOutOfRangeColor(val);
  this->LookupTableProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
int vtkPVColorMap::GetUseLowOutOfRangeColor()
{
  return this->LookupTableProxy->GetUseLowOutOfRangeColor();
}

//----------------------------------------------------------------------------
double* vtkPVColorMap::GetLowLookupTableValue()
{
  return this->LookupTableProxy->GetLowOutOfRangeColor();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetUseHighOutOfRangeColor(int val)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SetUseHighOutOfRangeColor %d",
                                   this->GetTclName(), val);
  this->LookupTableProxy->SetUseHighOutOfRangeColor(val);
  this->LookupTableProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
int vtkPVColorMap::GetUseHighOutOfRangeColor()
{
  return this->LookupTableProxy->GetUseHighOutOfRangeColor();
}

//----------------------------------------------------------------------------
double* vtkPVColorMap::GetHighLookupTableValue()
{
  return this->LookupTableProxy->GetHighOutOfRangeColor();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "UseCount: " << this->UseCount << endl;

  os << indent << "ScalarBarProxy: " << this->ScalarBarProxy << endl;
  
  os << indent << "ScalarBarVisibility: " << this->ScalarBarVisibility << endl;
  os << indent << "VisitedFlag: " << this->VisitedFlag << endl;
  
  os << indent << "ScalarRange: " << this->ScalarRange[0] << ", " 
     << this->ScalarRange[1] << endl;
  os << indent << "WholeScalarRange: " << this->WholeScalarRange[0] << ", " 
     << this->WholeScalarRange[1] << endl;
  os << indent << "VectorComponent: " << this->VectorComponent << endl;
  os << indent << "ScalarBarTitle: " << ((this->ScalarBarTitle)?
    this->ScalarBarTitle : "NULL") << endl;
  os << indent << "VectorMagnitudeTitle: " << ((this->VectorMagnitudeTitle)?
    this->VectorMagnitudeTitle : "NULL") << endl;
  os << indent << "MapHeight: " << this->MapHeight << endl;
  os << indent << "MapWidth: " << this->MapWidth << endl;
  os << indent << "TitleTextProperty: " << this->TitleTextProperty << endl;
  os << indent << "LabelTextProperty: " << this->LabelTextProperty << endl;
  os << indent << "StartColor: " << this->StartColor[0] << " "
     << this->StartColor[1] << " " << this->StartColor[2] << endl;
  os << indent << "EndColor: " << this->EndColor[0] << " "
     << this->EndColor[1] << " " << this->EndColor[2] << endl;
  os << indent << "ScalarRangeLock: " << this->ScalarRangeLock << endl;
  os << indent << "Displayed: " << this->Displayed << endl;
}
