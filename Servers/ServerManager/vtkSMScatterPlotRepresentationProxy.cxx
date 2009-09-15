/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScatterPlotRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMScatterPlotRepresentationProxy.h"

//#include "vtkAbstractMapper.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMScatterPlotViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMInputProperty.h"
#include <vtksys/ios/sstream>
#include <vtkstd/vector>

inline void vtkSMScatterPlotRepresentationProxySetString(
  vtkSMProxy* proxy, const char* pname, const char* pval)
{
  vtkSMStringVectorProperty* ivp = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, pval);
    proxy->UpdateProperty(pname);
    }
}

inline void vtkSMScatterPlotRepresentationProxySetInt(
  vtkSMProxy* proxy, const char* pname, int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, val);
    proxy->UpdateProperty(pname);
    }
}

struct vtkSMScatterPlotRepresentationProxy::vtkInternal
{
  vtkstd::vector<vtkSMViewProxy*> Views;
};

vtkStandardNewMacro(vtkSMScatterPlotRepresentationProxy);
vtkCxxRevisionMacro(vtkSMScatterPlotRepresentationProxy, "1.11");
//-----------------------------------------------------------------------------
vtkSMScatterPlotRepresentationProxy::vtkSMScatterPlotRepresentationProxy()
{
  this->Internal = new vtkSMScatterPlotRepresentationProxy::vtkInternal;
  this->FlattenFilter = 0;
  this->Mapper = 0;
  this->LODMapper = 0;
  this->Prop3D = 0;
  this->Property = 0;
  this->CubeAxesActor = 0;
  this->CubeAxesProperty = 0;
  this->CubeAxesVisibility = 0;
  this->GlyphInput = 0;
  this->GlyphOutputPort = 0;
}

//-----------------------------------------------------------------------------
vtkSMScatterPlotRepresentationProxy::~vtkSMScatterPlotRepresentationProxy()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkSMScatterPlotRepresentationProxy::AddInput(unsigned int port,
                                             vtkSMSourceProxy* input,
                                             unsigned int outputPort,
                                             const char* method)
{
  switch (port)
    {
    case 0:
      this->Superclass::AddInput(port, input, outputPort, method);
      break;
    case 1:
      if (!input)
        {
        vtkErrorMacro("Representation cannot have NULL input.");
        return;
        }

      input->CreateOutputPorts();
      if (input->GetNumberOfOutputPorts() == 0)
        {
        vtkErrorMacro("Input has no output. Cannot create the representation.");
        return;
        }

      this->GlyphInput = input;
      this->GlyphOutputPort = outputPort;
      this->UnRegisterVTKObjects();
      this->CreateVTKObjects();
      break;
    default:
      break;
    }
}


//----------------------------------------------------------------------------
void vtkSMScatterPlotRepresentationProxy::SetViewInformation(
  vtkInformation* info)
{
  this->Superclass::SetViewInformation(info);
//   if (this->CubeAxesRepresentation)
//     {
//     this->CubeAxesRepresentation->SetViewInformation(info);
//     }
}

//-----------------------------------------------------------------------------
/*void vtkSMScatterPlotRepresentationProxy::AddInput(unsigned int inputPort,
                                                   vtkSMSourceProxy* input,
                                                   unsigned int outputPort,
                                                   const char* method)
{
  this->vtkSMDataRepresentationProxy::AddInput(inputPort,input,outputPort,method);
  this->UpdatePropertyInformation();
}
*/
//-----------------------------------------------------------------------------
bool vtkSMScatterPlotRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }
  //this->GeometryFilter = 
  //  vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("GeometryFilter"));
  this->FlattenFilter = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("FlattenFilter"));
  // Setup pointers to subproxies  for easy access and set server flags. 
  //this->ScatterPlot = vtkSMSourceProxy::SafeDownCast(
  //  this->GetSubProxy("ScatterPlot"));
  this->Mapper = this->GetSubProxy("Mapper");
  this->LODMapper = this->GetSubProxy("LODMapper");
  this->Prop3D = this->GetSubProxy("Prop3D");
  this->Property = this->GetSubProxy("Property");

//  this->CubeAxesRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
//    this->GetSubProxy("CubeAxesRepresentation"));
  this->CubeAxesActor = this->GetSubProxy("CubeAxesActor");
  this->CubeAxesProperty = this->GetSubProxy("CubeAxesProperty");
  
  this->FlattenFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->Mapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  
  if(this->LODMapper)
    {
    this->LODMapper->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    }
  this->Prop3D->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Property->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->CubeAxesActor->SetServers(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->CubeAxesProperty->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMScatterPlotRepresentationProxy::EndCreateVTKObjects()
{
  if (this->GlyphInput)
    {
    vtkstd::vector<vtkSMViewProxy*>::iterator iter;
    for (iter = this->Internal->Views.begin(); 
         iter != this->Internal->Views.end(); ++iter)
      {
      // We know the input data type: it has to be a vtkPolyData. Hence we can
      // simply ask the view for the correct strategy.
      vtkSmartPointer<vtkSMRepresentationStrategy> glyphStrategy;
      glyphStrategy.TakeReference((*iter)->NewStrategy(VTK_POLY_DATA));
      if (!glyphStrategy.GetPointer())
        {
        vtkErrorMacro("View could not provide a strategy to use."
                      "Cannot be rendered in this view of type: " << (*iter)->GetClassName());
        return false;
        }

      glyphStrategy->SetEnableLOD(false);
      glyphStrategy->SetConnectionID(this->ConnectionID);
  
      this->Connect(this->GlyphInput, glyphStrategy, 
                    "Input", this->GlyphOutputPort);
      this->Connect(glyphStrategy->GetOutput(), this->Mapper, "GlyphInput");

      // Creates the strategy objects.
      glyphStrategy->UpdateVTKObjects();

      this->AddStrategy(glyphStrategy);
      }
    }
  
  //this->Connect(this->GetInputProxy(), this->GeometryFilter, 
  //  "Input", this->OutputPort);
  this->Connect(this->GetInputProxy(), this->FlattenFilter, 
                "Input", this->OutputPort);
  
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
//  this->Connect(this->LODMapper, this->Prop3D, "LODMapper");
  this->Connect(this->Property, this->Prop3D, "Property");
  this->Connect(this->CubeAxesProperty, this->CubeAxesActor, "Property");
  
  this->SetCubeAxesVisibility(this->CubeAxesVisibility);

//   if (this->CubeAxesRepresentation)
//     {
//     this->Connect(this->Mapper, this->CubeAxesRepresentation);
//     vtkSMScatterPlotRepresentationProxySetInt(
//       this->CubeAxesRepresentation, 
//       "Visibility", 1);
//     }
  

  /*
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GeometryFilter->GetProperty("PassThroughIds"));
  ivp->SetElement(0, 1); 
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GeometryFilter->GetProperty("PassThroughPointIds"));
  ivp->SetElement(0, 1); 
  
  this->GeometryFilter->UpdateVTKObjects();*/
  //this->UpdatePropertyInformation();
  return this->Superclass::EndCreateVTKObjects();
}

//-----------------------------------------------------------------------------
bool vtkSMScatterPlotRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // We know the input data type: it has to be a vtkImageData. Hence we can
  // simply ask the view for the correct strategy.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  //strategy.TakeReference(view->NewStrategy(VTK_DATA_OBJECT));
  strategy.TakeReference(view->NewStrategy(VTK_POLY_DATA));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use."
      "Cannot be rendered in this view of type: " << view->GetClassName());
    return false;
    }

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMDataRepresentationProxy::AddToView()).

  // TODO: For now, I am not going to worry about the LOD pipeline.
  strategy->SetEnableLOD(false);
  
  strategy->SetConnectionID(this->ConnectionID);
  
  //this->Connect(this->GetInputProxy(), this->ScatterPlot,
  //              "Input", this->OutputPort);
  //this->Connect(this->ScatterPlot, strategy);

  //this->Connect(this->GetInputProxy(), strategy,
  //  "Input", this->OutputPort);
  
  //this->Connect(this->GetInputProxy(), strategy);
  //this->Connect(this->GeometryFilter, strategy);
  this->Connect(this->FlattenFilter, strategy);
  this->Connect(strategy->GetOutput(), this->Mapper);
  // this->Connect(strategy->GetLODOutput(), this->LODMapper);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  this->AddStrategy(strategy);
  /*
  if (this->GlyphInput)
    { 
    cout << "vtkSMScatterPlotRepresentationProxy: ADD GLYPHINPUT " << endl;
    // We know the input data type: it has to be a vtkPolyData. Hence we can
    // simply ask the view for the correct strategy.
    vtkSmartPointer<vtkSMRepresentationStrategy> glyphStrategy;
    glyphStrategy.TakeReference(view->NewStrategy(VTK_POLY_DATA));
    if (!glyphStrategy.GetPointer())
      {
      vtkErrorMacro("View could not provide a strategy to use."
                    "Cannot be rendered in this view of type: " << view->GetClassName());
      return false;
      }

    glyphStrategy->SetEnableLOD(false);
    glyphStrategy->SetConnectionID(this->ConnectionID);
  
    //this->Connect(this->GlyphInput, glyphStrategy, 
    //             "Input", this->GlyphOutputPort);
    //this->Connect(glyphStrategy->GetOutput(), this->Mapper);

    // Creates the strategy objects.
    glyphStrategy->UpdateVTKObjects();

    this->AddStrategy(glyphStrategy);
    }
  */

  return this->Superclass::InitializeStrategy(view);  
}

//----------------------------------------------------------------------------
bool vtkSMScatterPlotRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

//   if (this->CubeAxesRepresentation)
//     {
//     this->CubeAxesRepresentation->AddToView(view);
//     }

  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  this->Internal->Views.push_back(view);

  renderView->AddPropToRenderer(this->Prop3D);
  
  renderView->AddPropToRenderer(this->CubeAxesActor);


  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << renderView->GetRendererProxy()->GetID()
          << "GetActiveCamera"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesActor->GetID()
          << "SetCamera"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;

  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, 
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMScatterPlotRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->Prop3D);
  renderView->RemovePropFromRenderer(this->CubeAxesActor);

  vtkstd::vector<vtkSMViewProxy*>::iterator iter;
  for (iter = this->Internal->Views.begin(); 
       iter != this->Internal->Views.end(); ++iter)
    {
    if (*iter == view)
      {
      this->Internal->Views.erase(iter);
      break;
      }
    }

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesActor->GetID()
          << "SetCamera" << 0
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);

  //this->Strategy = 0;

//   if (this->CubeAxesRepresentation)
//     {    
//     this->CubeAxesRepresentation->RemoveFromView(view);
//     }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotRepresentationProxy::SetVisibility(int visible)
{
//   if (this->CubeAxesRepresentation)
//     {    
//     vtkSMScatterPlotRepresentationProxySetInt(
//       this->CubeAxesRepresentation, "Visibility",
//       visible && this->CubeAxesVisibility);
//     this->CubeAxesRepresentation->UpdateVTKObjects();
//     }
//   if (this->SelectionRepresentation && !visible)
//     {
//     vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
//       this->SelectionRepresentation->GetProperty("Visibility"));
//     ivp->SetElement(0, visible);
//     this->SelectionRepresentation->UpdateProperty("Visibility");
//     }
//  vtkSMProxy* prop3D = this->GetSubProxy("Prop3D");
  vtkSMProxy* prop2D = this->GetSubProxy("Prop2D");

  if (this->Prop3D)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->Prop3D->GetProperty("Visibility"));
    ivp->SetElement(0, visible);
    this->Prop3D->UpdateProperty("Visibility");
    }

  if (prop2D)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      prop2D->GetProperty("Visibility"));
    ivp->SetElement(0, visible);
    prop2D->UpdateProperty("Visibility");
    }

  if (this->CubeAxesActor)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->CubeAxesActor->GetProperty("Visibility"));
    ivp->SetElement(0, this->CubeAxesVisibility && visible);
    this->CubeAxesActor->UpdateProperty("Visibility");
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotRepresentationProxy::SetCubeAxesVisibility(int visible)
{
  if (this->CubeAxesActor)
    {
    this->CubeAxesVisibility = visible;
    vtkSMScatterPlotRepresentationProxySetInt(
      this->CubeAxesActor, "Visibility",
      visible && this->GetVisibility());
    this->CubeAxesActor->UpdateVTKObjects();
    }
}

/*
//-----------------------------------------------------------------------------
void vtkSMScatterPlotRepresentationProxy::SetColorAttributeType(int type)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ScalarMode"));
  switch (type)
    {
  case POINT_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA); 
    break;

  case CELL_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  default:
    vtkWarningMacro("Incorrect Color attribute type.");
    ivp->SetElement(0,  VTK_SCALAR_MODE_DEFAULT);
    }
  this->Mapper->UpdateVTKObjects();
  //this->LODMapper->UpdateVTKObjects();
  //this->UpdateShadingParameters();
}
*/

//-----------------------------------------------------------------------------
/*
void vtkSMScatterPlotRepresentationProxy::SetColorArrayName(const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ColorArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }

  this->Mapper->UpdateVTKObjects();
  //this->LODMapper->UpdateVTKObjects();
}
*/
//-----------------------------------------------------------------------------
bool vtkSMScatterPlotRepresentationProxy::GetBounds(double bounds[6])
{
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->Mapper->GetID()
          << "GetBounds"
          << vtkClientServerStream::End;
  
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
  const vtkClientServerStream& res = 
    vtkProcessModule::GetProcessModule()->GetLastResult(
      this->ConnectionID,
      vtkProcessModule::RENDER_SERVER);
  bool ret = static_cast<bool>(res.GetArgument(0, 0, bounds, 6));
  if(!ret)
    {
    ret = this->Superclass::GetBounds(bounds);
    }
  
//   cout << __FUNCTION__ << " bounds: " 
//        << bounds[0] << " " << bounds[1] << " " << bounds[2] << " " 
//        << bounds[3] << " " << bounds[4] << " " << bounds[5] << endl;
  
  return ret;
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotRepresentationProxy::Update(vtkSMViewProxy* view)
{
  bool update = this->UpdateRequired();
  if (this->CubeAxesActor)
    {    
    double bounds[6];
    this->GetBounds(bounds);
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->CubeAxesActor->GetProperty("Bounds"));
    dvp->SetElements(bounds);
    this->CubeAxesActor->UpdateVTKObjects();
    }

  this->Superclass::Update(view);
  if(update)
    {
    this->GetInputProxy()->UpdatePropertyInformation();
    this->UpdatePropertyInformation();
    }
}

void vtkSMScatterPlotRepresentationProxy::SetXAxisArrayName(const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("XCoordsArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  this->Mapper->UpdateVTKObjects();
}

void vtkSMScatterPlotRepresentationProxy::SetYAxisArrayName(const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("YCoordsArray"));
  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  this->Mapper->UpdateVTKObjects();
}

void vtkSMScatterPlotRepresentationProxy::SetZAxisArrayName(const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ZCoordsArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  this->Mapper->UpdateVTKObjects();
}

void vtkSMScatterPlotRepresentationProxy::SetColorArrayName(const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ColorizeArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  this->Mapper->UpdateVTKObjects();
}

void vtkSMScatterPlotRepresentationProxy::SetGlyphScalingArrayName(const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("GlyphXScalingArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("GlyphYScalingArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("GlyphZScalingArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  this->Mapper->UpdateVTKObjects();
}

void vtkSMScatterPlotRepresentationProxy::SetGlyphMultiSourceArrayName(const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("GlyphSourceArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  this->Mapper->UpdateVTKObjects();
}

void vtkSMScatterPlotRepresentationProxy::SetGlyphOrientationArrayName(const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("GlyphXOrientationArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("GlyphYOrientationArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("GlyphZOrientationArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }
  this->Mapper->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
int vtkSMScatterPlotRepresentationProxy::GetNumberOfSeries()
{
  int count = 0;
  vtkPVDataInformation* dataInformation = 
    //this->GetInputProxy()->GetDataInformation();
    this->FlattenFilter->GetDataInformation();
  if(!dataInformation)
    {
    return count;
    }
/*
  if(dataInformation->GetPointArrayInformation())
    {
    count += dataInformation->GetPointArrayInformation()->GetNumberOfComponents();
    }
  if(dataInformation->GetPointDataInformation())
    {                                                   
    int numberOfArrays = dataInformation->GetPointDataInformation()
      ->GetNumberOfArrays();
    for(int i = 0; i < numberOfArrays; ++i)
      {
      count += dataInformation->GetPointDataInformation()
        ->GetArrayInformation(i)->GetNumberOfComponents();
      }
    }
  if(dataInformation->GetCellDataInformation())
    {                                                   
    int numberOfArrays = dataInformation->GetCellDataInformation()
      ->GetNumberOfArrays();
    for(int i = 0; i < numberOfArrays; ++i)
      {
      count += dataInformation->GetCellDataInformation()
        ->GetArrayInformation(i)->GetNumberOfComponents();
      }
    }
*/
  if (dataInformation->GetPointArrayInformation())
    {
    //cout << "Number of : " << dataInformation->GetPointArrayInformation()->GetNumberOfComponents() << " and " << dataInformation->GetPointArrayInformation()->GetNumberOfTuples() << endl;
      }
  count = (dataInformation->GetPointArrayInformation()? 1 :0) + // coordinates
    (dataInformation->GetPointDataInformation() ? 
     dataInformation->GetPointDataInformation()->GetNumberOfArrays() : 0) +
    (dataInformation->GetCellDataInformation() ? 
     dataInformation->GetCellDataInformation()->GetNumberOfArrays() : 0);
  //todo handles other arrays
  return count;
}

//----------------------------------------------------------------------------
//const char* vtkSMScatterPlotRepresentationProxy::GetSeriesName(int series)
vtkStdString vtkSMScatterPlotRepresentationProxy::GetSeriesName(int series)
{
  vtkPVDataInformation* dataInformation = 
    //this->GetInputProxy()->GetDataInformation();
    //this->strategy->GetOutputProxy()->GetDataInformation();
    //activeStrategies.begin()->GetPointer()->GetOutput()->GetDataInformation();
    this->FlattenFilter->GetDataInformation();
  if(!dataInformation)
    {
    return NULL;
    }

  // COORDINATES
  if(dataInformation->GetPointArrayInformation())
    {
    /*
    int numberOfComponents = 
      dataInformation->GetPointArrayInformation()->GetNumberOfComponents();
    if(series < numberOfComponents)
      {
      vtksys_ios::stringstream str;
      str << dataInformation->GetPointArrayInformation()->GetName();
      if(numberOfComponents > 1)
        {
        str << "(" << series << ")";
        }
      return str.str();//.c_str();
      }
    series -= numberOfComponents;
    */
    if (series == 0)
      {
      return dataInformation->GetPointArrayInformation()->GetName();
      }
    else
      {
      --series;
      }
    }

  // POINT DATA
  if(dataInformation->GetPointDataInformation())
    {
    int numberOfArrays = 
      dataInformation->GetPointDataInformation()->GetNumberOfArrays();
    for(int i=0; i < numberOfArrays; ++i)
      {
      /*
      int numberOfComponents = dataInformation->GetPointDataInformation()
        ->GetArrayInformation(i)->GetNumberOfComponents();
      if(series < numberOfComponents)
        {
        vtksys_ios::stringstream str;
        str << dataInformation->GetPointDataInformation()
          ->GetArrayInformation(i)->GetName();
        if(numberOfComponents > 1)
          {
          str << "(" << series << ")";
          }
        return str.str();//.c_str();
        }
      else
        {
        series -= numberOfComponents;
        }
      */
      if( series == 0 )
        {
        return dataInformation->GetPointDataInformation()
          ->GetArrayInformation(i)->GetName();
        }
      else
        {
        --series;
        }
      }
    }

  // CELL DATA
  if(dataInformation->GetCellDataInformation())
    {                                                   
    int numberOfArrays= dataInformation->GetCellDataInformation()->GetNumberOfArrays();
    for(int i=0; i < numberOfArrays;++i)
      {
      /*
      int numberOfComponents = dataInformation->GetCellDataInformation()
        ->GetArrayInformation(i)->GetNumberOfComponents();
      if(series < numberOfComponents)
        {
        vtksys_ios::stringstream str;
        str << dataInformation->GetCellDataInformation()
          ->GetArrayInformation(i)->GetName();
        if(numberOfComponents > 1)
          {
          str << "(" << series << ")";
          }
        return str.str();//.c_str();
        }
      else
        {
        series -= numberOfComponents;
        }
      */
      if (series == 0)
        {
        return dataInformation->GetCellDataInformation()
          ->GetArrayInformation(i)->GetName();
        }
      else
        {
        --series;
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkSMScatterPlotRepresentationProxy::GetSeriesType(int series)
{
  //cout << "GetSeriesType: " << this->GetInputProxy()->GetVTKClassName() 
  //     << " " << this->GetInputProxy()->GetClientSideObject() 
  //     << " " << this->GetInputProxy()->GetClientSideObject()->GetClassName() << endl;
//  vtkSMRepresentationStrategyVector activeStrategies;
//  this->GetActiveStrategies(activeStrategies);

  vtkPVDataInformation* dataInformation = 
    //this->GetInputProxy()->GetDataInformation();
    //this->strategy->GetOutputProxy()->GetDataInformation();
    //activeStrategies.begin()->GetPointer()->GetOutput()->GetDataInformation();
    this->FlattenFilter->GetDataInformation();
  if(!dataInformation)
    {
    return vtkDataObject::NUMBER_OF_ASSOCIATIONS;
    }

  if(dataInformation->GetPointArrayInformation())
    {
    /*
    int numberOfComponents = 
      dataInformation->GetPointArrayInformation()->GetNumberOfComponents();
    if(series < numberOfComponents)
      {
      return vtkDataObject::NUMBER_OF_ASSOCIATIONS;
      }
    series -= numberOfComponents;
    */ 
    if (series == 0)
      {
      return vtkDataObject::NUMBER_OF_ASSOCIATIONS;
      }
    else
      {
      --series;
      }
    }
  if(dataInformation->GetPointDataInformation())
    {
    int numberOfArrays= dataInformation->GetPointDataInformation()->GetNumberOfArrays();
    /*
    for(int i=0; i < numberOfArrays;++i)
      {
      int numberOfComponents = dataInformation->GetPointDataInformation()
        ->GetArrayInformation(i)->GetNumberOfComponents();
      if(series < numberOfComponents)
        {
        return vtkDataObject::FIELD_ASSOCIATION_POINTS;
        }
      else
        {
        series -= numberOfComponents;
        }
      }
    */
    if (series < numberOfArrays)
        {
        return vtkDataObject::FIELD_ASSOCIATION_POINTS;
        }
      else
        {
        series -= numberOfArrays;
        }
    }
  if(dataInformation->GetCellDataInformation())
    {                                                   
    int numberOfArrays= dataInformation->GetCellDataInformation()->GetNumberOfArrays();
    /*
    for(int i=0; i < numberOfArrays;++i)
      {
      int numberOfComponents = dataInformation->GetCellDataInformation()
        ->GetArrayInformation(i)->GetNumberOfComponents();
      if(series < numberOfComponents)
        {
        return vtkDataObject::FIELD_ASSOCIATION_CELLS;
        }
      else
        {
        series -= numberOfComponents;
        }
      }
    */
    if (series < numberOfArrays)
      {
      return vtkDataObject::FIELD_ASSOCIATION_CELLS;
      }
    else
      {
      series -= numberOfArrays;
      }
    }
  return vtkDataObject::NUMBER_OF_ASSOCIATIONS;
}

//----------------------------------------------------------------------------
int vtkSMScatterPlotRepresentationProxy::GetSeriesNumberOfComponents(int series)
{
  vtkPVDataInformation* dataInformation = 
    //this->GetInputProxy()->GetDataInformation();
    //this->strategy->GetOutputProxy()->GetDataInformation();
    //activeStrategies.begin()->GetPointer()->GetOutput()->GetDataInformation();
    this->FlattenFilter->GetDataInformation();
  if(!dataInformation)
    {
    return 0;
    }

  // COORDINATES
  if(dataInformation->GetPointArrayInformation())
    {
    if (series == 0)
      {
      return dataInformation->GetPointArrayInformation()->GetNumberOfComponents();
      }
    else
      {
      --series;
      }
    }

  // POINT DATA
  if(dataInformation->GetPointDataInformation())
    {
    int numberOfArrays = 
      dataInformation->GetPointDataInformation()->GetNumberOfArrays();
    for(int i=0; i < numberOfArrays; ++i)
      {
      if( series == 0 )
        {
        return dataInformation->GetPointDataInformation()
          ->GetArrayInformation(i)->GetNumberOfComponents();
        }
      else
        {
        --series;
        }
      }
    }

  // CELL DATA
  if(dataInformation->GetCellDataInformation())
    {                                                   
    int numberOfArrays= dataInformation->GetCellDataInformation()->GetNumberOfArrays();
    for(int i=0; i < numberOfArrays;++i)
      {
      if (series == 0)
        {
        return dataInformation->GetCellDataInformation()
          ->GetArrayInformation(i)->GetNumberOfComponents();
        }
      else
        {
        --series;
        }
      }
    }
  return 0;
}
//-----------------------------------------------------------------------------
void vtkSMScatterPlotRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
