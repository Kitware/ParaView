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

#include "vtkClientServerStream.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSMPlotMatrixViewProxy);

//---------------------------------------------------------------------------
vtkSMPlotMatrixViewProxy::vtkSMPlotMatrixViewProxy()
{
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
void vtkSMPlotMatrixViewProxy::ReceiveDouble3Vector(const char *func, 
                                                         int plotType, 
                                                         double *data)
{
  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this) << func  << plotType 
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  vtkClientServerStream values = this->GetLastResult();

  if (values.GetNumberOfArguments(0) < 0)
    {
    vtkErrorMacro("Error getting array from server.");
    return;
    }
  
  vtkTypeUInt32 length;
  values.GetArgumentLength(0, 0, &length);
  if (length != 3)
    {
    vtkErrorMacro("Error getting array from server - incorrect size returned.");
    return;
    }
  int status = values.GetArgument(0, 0, data, 3);
  if (!status)
    {
    vtkErrorMacro("Error getting double vector 3.");
    }
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
