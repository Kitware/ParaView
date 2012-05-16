/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPlotMatrixViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPlotMatrixViewProxy.h"
#include "vtkScatterPlotMatrix.h"

#include "vtkClientServerStream.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPVPlotMatrixView.h"
#include "vtkPVXMLElement.h"
#include "vtkCommand.h"
#include "vtkVector.h"

#include "vtkSMSession.h"

vtkStandardNewMacro(vtkSMPlotMatrixViewProxy);

//---------------------------------------------------------------------------
vtkSMPlotMatrixViewProxy::vtkSMPlotMatrixViewProxy()
{
  this->ActiveChanged = false;
}

//---------------------------------------------------------------------------
vtkSMPlotMatrixViewProxy::~vtkSMPlotMatrixViewProxy()
{
  this->SetVTKClassName(0);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    return;
  this->Superclass::CreateVTKObjects();

  vtkPVPlotMatrixView *matrix =
      vtkPVPlotMatrixView::SafeDownCast(this->GetClientSideObject());
  if (matrix)
    {
//    matrix->AddObserver(vtkCommand::AnnotationChangedEvent, this,
//                        &vtkSMPlotMatrixViewProxy::ActivePlotChanged);
    matrix->AddObserver(vtkCommand::CreateTimerEvent, this,
                        &vtkSMPlotMatrixViewProxy::SendAnimationPath);
    matrix->AddObserver(vtkCommand::AnimationCueTickEvent, this,
                        &vtkSMPlotMatrixViewProxy::AnimationTickEvent);
    }
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::ActivePlotChanged()
{
  this->ActiveChanged = true;
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::PostRender(bool interactive)
{
  this->Superclass::PostRender(interactive);

  if (this->ActiveChanged)
    {
    vtkPVPlotMatrixView *matrix =
        vtkPVPlotMatrixView::SafeDownCast(this->GetClientSideObject());
    if (matrix)
      {
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke
             << VTKOBJECT(this) << "SetActivePlot"
             << matrix->GetActiveRow() << matrix->GetActiveColumn()
             << vtkClientServerStream::End;
      this->ExecuteStream(stream);
      this->ActiveChanged = false;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::AnimationTickEvent()
{
  if (this->Session->GetProcessRoles() != vtkPVSession::CLIENT_AND_SERVERS)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this) << "AdvanceAnimationPath"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SendAnimationPath()
{
  // Only send this to the render server(s) if we are not in builtin mode.
  if (this->Session->GetProcessRoles() != vtkPVSession::CLIENT_AND_SERVERS)
    {
    vtkPVPlotMatrixView *matrix =
        vtkPVPlotMatrixView::SafeDownCast(this->GetClientSideObject());
    if (!matrix)
      {
      return;
      }
    vtkScatterPlotMatrix *plotMatrix =
        vtkScatterPlotMatrix::SafeDownCast(matrix->GetContextItem());
    if (!plotMatrix || plotMatrix->GetNumberOfAnimationPathElements() == 0)
      {
      return;
      }

    vtkIdType n = plotMatrix->GetNumberOfAnimationPathElements();

    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this) << "ClearAnimationPath"
           << vtkClientServerStream::End;

    for (vtkIdType i = 0; i < n; ++i)
      {
      vtkVector2i element = plotMatrix->GetAnimationPathElement(i);
      stream << vtkClientServerStream::Invoke
             << VTKOBJECT(this) << "AddAnimationPath"
             << element[0] << element[1]
             << vtkClientServerStream::End;
      }

    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this) << "StartAnimationPath"
           << vtkClientServerStream::End;

    this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER);
    this->MarkModified(this);
    }
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SendDouble3Vector(const char *func, 
                                                     int plotType, 
                                                     double *data)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << func
         << plotType
         << data[0] << data[1] << data[2]
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}
//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SendDouble4Vector(const char *func, 
                                                 int plotType, 
                                                 double *data)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << func
    << plotType
    << data[0] << data[1] << data[2] << data[3]
    << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}
//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SendIntValue(const char *func, 
                                                     int plotType, 
                                                     int val)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << func
         << plotType
         << val
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}
//----------------------------------------------------------------------------
int vtkSMPlotMatrixViewProxy::ReceiveIntValue(const char *func, 
                                               int plotType)
{
  vtkClientServerStream values = this->InvokeTypeServerMethod(func, plotType);
  int val;
  values.GetArgument(0, 0, &val);
  return val;
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::ReceiveTypeDouble4Vector(const char *func, 
                                                         int plotType, 
                                                         double *data)
{
  vtkClientServerStream values = this->InvokeTypeServerMethod(func, plotType);
  values.GetArgument(0, 0, data, 4);
}
//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::ReceiveDouble4Vector(const char *func, 
                                                         double *data)
{
  vtkClientServerStream values = this->InvokeServerMethod(func);
  values.GetArgument(0, 0, data, 4);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetGutter(float x, float y)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << "SetGutter"
         << x << y
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}
//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetBorders(int left, int bottom, int right, int top)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << "SetBorders"
         << left << bottom << right << top
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetScatterPlotTitle(const char* title)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << "SetScatterPlotTitle"
         << title
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetScatterPlotTitleFont(const char* family, 
  int pointSize, bool bold, bool italic)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << "SetScatterPlotTitleFont"
         << family << pointSize << (bold ? 1 : 0) << (italic ? 1 : 0)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetScatterPlotTitleColor(
  double red, double green, double blue)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << "SetScatterPlotTitleColor"
         << red << green << blue
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetScatterPlotTitleAlignment(int alignment)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << "SetScatterPlotTitleAlignment"
         << alignment
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetGridVisibility(int plotType, bool visible)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << "SetGridVisibility"
         << plotType
         << (visible ? 1 : 0)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetBackgroundColor(int plotType, double red, double green,
                                         double blue, double alpha)
{
  double rgba[4]={red, green, blue, alpha};
  this->SendDouble4Vector("SetBackgroundColor", plotType, rgba);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetAxisColor(int plotType, double red, double green,
                                         double blue)
{
  double rgb[3]={red, green, blue};
  this->SendDouble3Vector("SetAxisColor", plotType, rgb);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetGridColor(int plotType, double red, double green,
                                         double blue)
{
  double rgb[3]={red, green, blue};
  this->SendDouble3Vector("SetGridColor", plotType, rgb);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetAxisLabelVisibility(int plotType, bool visible)
{
  this->SendIntValue("SetAxisLabelVisibility", plotType, visible);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetAxisLabelFont(int plotType, const char* family,
                                             int pointSize, bool bold,
                                             bool italic)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << "SetAxisLabelFont"
         << plotType
         << family << pointSize << (bold ? 1 : 0) << (italic ? 1 : 0)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetAxisLabelColor(
  int plotType, double red, double green, double blue)
{
  double rgb[3]={red, green, blue};
  this->SendDouble3Vector("SetAxisLabelColor", plotType, rgb);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetAxisLabelNotation(int plotType, int notation)
{
  this->SendIntValue("SetAxisLabelNotation", plotType, notation);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetAxisLabelPrecision(int plotType, int precision)
{
  this->SendIntValue("SetAxisLabelPrecision", plotType, precision);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetTooltipNotation(int plotType, int notation)
{
  this->SendIntValue("SetTooltipNotation", plotType, notation);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetTooltipPrecision(int plotType, int precision)
{
  this->SendIntValue("SetTooltipPrecision", plotType, precision);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetScatterPlotSelectedRowColumnColor(
  double red, double green, double blue, double alpha)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << "SetScatterPlotSelectedRowColumnColor"
    << red << green << blue << alpha
    << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SetScatterPlotSelectedActiveColor(
  double red, double green, double blue, double alpha)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << "SetScatterPlotSelectedActiveColor"
    << red << green << blue << alpha
    << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  stream.Reset();
  this->MarkModified(this);
}
//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::UpdateSettings()
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << "UpdateSettings"
    << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->MarkModified(this);
}
//----------------------------------------------------------------------------
const char* vtkSMPlotMatrixViewProxy::GetScatterPlotTitleFontFamily()
{
  vtkClientServerStream values = this->InvokeServerMethod(
    "GetScatterPlotTitleFontFamily");
  const char* fontfamily=NULL;  
  values.GetArgument(0, 0, &fontfamily);
  return fontfamily;
}
int vtkSMPlotMatrixViewProxy::GetScatterPlotTitleFontSize()
{
  vtkClientServerStream values = this->InvokeServerMethod(
    "GetScatterPlotTitleFontSize");
  int fsize;
  values.GetArgument(0, 0, &fsize);
  return fsize;
}
bool vtkSMPlotMatrixViewProxy::GetScatterPlotTitleFontBold()
{
  vtkClientServerStream values = this->InvokeServerMethod(
    "GetScatterPlotTitleFontBold");
  int bold=0;
  values.GetArgument(0, 0, &bold);
  return bold >0 ? true : false ;
}
bool vtkSMPlotMatrixViewProxy::GetScatterPlotTitleFontItalic()
{
  vtkClientServerStream values = this->InvokeServerMethod(
    "GetScatterPlotTitleFontItalic");
  int italic=0;
  values.GetArgument(0, 0, &italic);
  return italic >0 ? true : false ;
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::GetScatterPlotTitleColor(double* rgba)
{
  this->ReceiveDouble4Vector(
    "GetScatterPlotTitleColor", rgba);
}

//----------------------------------------------------------------------------
const char* vtkSMPlotMatrixViewProxy::GetScatterPlotTitle()
{
  vtkClientServerStream values = this->InvokeServerMethod(
    "GetScatterPlotTitle");
  const char* title=NULL;  
  values.GetArgument(0, 0, &title);
  return title;
}

//----------------------------------------------------------------------------
int vtkSMPlotMatrixViewProxy::GetScatterPlotTitleAlignment()
{
  vtkClientServerStream values = this->InvokeServerMethod(
    "GetScatterPlotTitleAlignment");
  int val;  
  values.GetArgument(0, 0, &val);
  return val;
}

//----------------------------------------------------------------------------
bool vtkSMPlotMatrixViewProxy::GetGridVisibility(int plotType)
{
  return this->ReceiveIntValue("GetGridVisibility", plotType)>0 ? true : false;
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::GetBackgroundColor(int plotType, double* rgba)
{
  this->ReceiveTypeDouble4Vector("GetBackgroundColor", plotType, rgba);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::GetAxisColor(int plotType, double* rgba)
{
  this->ReceiveTypeDouble4Vector("GetBackgroundColor", plotType, rgba);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::GetGridColor(int plotType, double* rgba)
{
  this->ReceiveTypeDouble4Vector("GetGridColor", plotType, rgba);
}

//----------------------------------------------------------------------------
bool vtkSMPlotMatrixViewProxy::GetAxisLabelVisibility(int plotType)
{
  return this->ReceiveIntValue("GetAxisLabelVisibility", plotType)>0 ? true : false;
}

//----------------------------------------------------------------------------
const char* vtkSMPlotMatrixViewProxy::GetAxisLabelFontFamily(int plotType)
{
  vtkClientServerStream values = this->InvokeTypeServerMethod(
    "GetAxisLabelFontFamily", plotType);
  const char* fontfamily=NULL;  
  values.GetArgument(0, 0, &fontfamily);
  return fontfamily;
}
int vtkSMPlotMatrixViewProxy::GetAxisLabelFontSize(int plotType)
{
  return this->ReceiveIntValue("GetAxisLabelFontSize", plotType);
}
bool vtkSMPlotMatrixViewProxy::GetAxisLabelFontBold(int plotType)
{
  return this->ReceiveIntValue("GetAxisLabelFontBold", plotType)>0 ? true : false;
}
bool vtkSMPlotMatrixViewProxy::GetAxisLabelFontItalic(int plotType)
{
  return this->ReceiveIntValue("GetAxisLabelFontItalic", plotType)>0 ? true : false;
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::GetAxisLabelColor(int plotType, double* rgba)
{
  this->ReceiveTypeDouble4Vector("GetAxisLabelColor", plotType, rgba);
}

//----------------------------------------------------------------------------
int vtkSMPlotMatrixViewProxy::GetAxisLabelNotation(int plotType)
{
  return this->ReceiveIntValue("GetAxisLabelNotation", plotType);
}

//----------------------------------------------------------------------------
int vtkSMPlotMatrixViewProxy::GetAxisLabelPrecision(int plotType)
{
  return this->ReceiveIntValue("GetAxisLabelPrecision", plotType);
}

//----------------------------------------------------------------------------
int vtkSMPlotMatrixViewProxy::GetTooltipNotation(int plotType)
{
  return this->ReceiveIntValue("GetTooltipNotation", plotType);
}
int vtkSMPlotMatrixViewProxy::GetTooltipPrecision(int plotType)
{
  return this->ReceiveIntValue("GetTooltipPrecision", plotType);
}
//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::GetGutter(float* vtkNotUsed(xy))
{
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::GetBorders(int* vtkNotUsed(borders))
{
}
//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::GetScatterPlotSelectedRowColumnColor(double* rgba)
{
  return this->ReceiveDouble4Vector("GetScatterPlotSelectedRowColumnColor",rgba);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::GetScatterPlotSelectedActiveColor(double* rgba)
{
  return this->ReceiveDouble4Vector("GetScatterPlotSelectedActiveColor",rgba);
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkSMPlotMatrixViewProxy::InvokeServerMethod(
  const char* method)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << method
    << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  return this->GetLastResult();
}
//----------------------------------------------------------------------------
const vtkClientServerStream& vtkSMPlotMatrixViewProxy::InvokeTypeServerMethod(
  const char* method,  int chartType)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << method << chartType
    << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  return this->GetLastResult();
}

//----------------------------------------------------------------------------
vtkAbstractContextItem* vtkSMPlotMatrixViewProxy::GetContextItem()
{
  vtkPVPlotMatrixView* pvview = vtkPVPlotMatrixView::SafeDownCast(
    this->GetClientSideObject());
  return pvview? pvview->GetContextItem() : NULL;
}
