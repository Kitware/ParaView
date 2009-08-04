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

#include "vtkAbstractMapper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkClientServerStream.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkDataObject.h"
#include "vtkScatterPlotMapper.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMScatterPlotViewProxy.h"
#include "vtkSMOutputPort.h"
#include <vtksys/ios/sstream>

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

vtkStandardNewMacro(vtkSMScatterPlotRepresentationProxy);
vtkCxxRevisionMacro(vtkSMScatterPlotRepresentationProxy, "1.7");
//-----------------------------------------------------------------------------
vtkSMScatterPlotRepresentationProxy::vtkSMScatterPlotRepresentationProxy()
{
//  this->ScatterPlot = 0;
  //this->GeometryFilter = 0;
  this->FlattenFilter = 0;
  this->Mapper = 0;
  this->LODMapper = 0;
  this->Prop3D = 0;
  this->Property = 0;
  this->ScatterPlotView = 0;
}

//-----------------------------------------------------------------------------
vtkSMScatterPlotRepresentationProxy::~vtkSMScatterPlotRepresentationProxy()
{
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

  //this->GeometryFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->FlattenFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->Mapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  /*
  this->Mapper->SetArrayIndex(vtkScatterPlotMapper::X_COORDS, 
                              vtkDataSetAttributes::SCALARS, 0);
  this->Mapper->SetArrayIndex(vtkScatterPlotMapper::Y_COORDS, 
                              vtkDataSetAttributes::SCALARS, 1);
  */
  if(this->LODMapper)
    {
    this->LODMapper->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    }
  this->Prop3D->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Property->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMScatterPlotRepresentationProxy::EndCreateVTKObjects()
{  
  //this->Connect(this->GetInputProxy(), this->GeometryFilter, 
  //  "Input", this->OutputPort);
  this->Connect(this->GetInputProxy(), this->FlattenFilter, 
                "Input", this->OutputPort);
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
//  this->Connect(this->LODMapper, this->Prop3D, "LODMapper");
  this->Connect(this->Property, this->Prop3D, "Property");
  
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

  if (!this->Superclass::AddToView(view))
    {
    return false;
    }
  this->ScatterPlotView = vtkSMScatterPlotViewProxy::SafeDownCast(view);
  renderView->AddPropToRenderer(this->Prop3D);

/*
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
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);
*/
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
  this->ScatterPlotView = NULL;
/*
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesActor->GetID()
          << "SetCamera" << 0
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);
*/
  //this->Strategy = 0;
  return this->Superclass::RemoveFromView(view);
}


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
  /*
  if (!this->Superclass::GetBounds(bounds))
    {
    return false;
    }

  return true;
  */
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
  if(!res.GetArgument(0, 0, bounds, 6))
    {
    return this->Superclass::GetBounds(bounds);
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotRepresentationProxy::Update(vtkSMViewProxy* view)
{
  bool update = this->UpdateRequired();

  this->Superclass::Update(view);
//  this->VTKRepresentation->SetInputConnection(
//    this->GetOutput()->GetProducerPort());
//  this->VTKRepresentation->Update(); 
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
