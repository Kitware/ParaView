/*=========================================================================

   Program: ParaView
   Module:    vtkSMClientDeliverySource.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkSMClientDeliverySource - generate source data object via a user-specified function
// .SECTION Description
// vtkSMClientDeliverySource is a source object that is programmable by
// the user. The output of the filter is a data object (vtkDataObject) which
// represents data via an instance of field data. To use this object, you
// must specify a function that creates the output.  
//
// Example use of this filter includes reading tabular data and encoding it
// as vtkFieldData. You can then use filters like vtkDataObjectToDataSetFilter
// to convert the data object to a dataset and then visualize it.  Another
// important use of this class is that it allows users of interpreters (e.g.,
// Tcl or Java) the ability to write source objects without having to
// recompile C++ code or generate new libraries.
// 
// .SECTION See Also
// vtkProgrammableFilter vtkProgrammableAttributeDataFilter
// vtkProgrammableSource vtkDataObjectToDataSetFilter

#ifndef __vtkSMClientDeliverySource_h
#define __vtkSMClientDeliverySource_h

#include "OverViewCoreExport.h"

#include <vtkDataObjectAlgorithm.h>
#include <vtkSMClientDeliveryRepresentationProxy.h>

/// "Adapts" vtkSMClientDeliveryRepresentationProxy so it can be used as a pipeline source
class OVERVIEW_CORE_EXPORT vtkSMClientDeliverySource :
  public vtkDataObjectAlgorithm
{
public:
  static vtkSMClientDeliverySource *New();
  vtkTypeMacro(vtkSMClientDeliverySource, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetObjectMacro(DeliveryProxy, vtkSMClientDeliveryRepresentationProxy);

private:
  vtkSMClientDeliverySource(const vtkSMClientDeliverySource&);  // Not implemented.
  void operator=(const vtkSMClientDeliverySource&);  // Not implemented.

  vtkSMClientDeliverySource();
  ~vtkSMClientDeliverySource();

  vtkSMClientDeliveryRepresentationProxy* DeliveryProxy;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
};

#endif

