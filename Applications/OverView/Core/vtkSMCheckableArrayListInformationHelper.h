/*=========================================================================

   Program: ParaView
   Module:    vtkSMCheckableArrayListInformationHelper.h

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
// .NAME vtkSMCheckableArrayListInformationHelper - populates
// vtkSMStringVectorProperty using a vtkSMArrayListDomain on the same property.
// .SECTION Description
// Unlike other informaition helpers, vtkSMCheckableArrayListInformationHelper does not 
// use server side objects directly. Instead it uses a vtkSMArrayListDomain
// on the information property itself to propulate the property. Look
// at XYPlotDisplay2 proxy for how to use this information helper.

#ifndef __vtkSMCheckableArrayListInformationHelper_h
#define __vtkSMCheckableArrayListInformationHelper_h

#include "OverViewCoreExport.h"
#include <vtkSMInformationHelper.h>

class OVERVIEW_CORE_EXPORT vtkSMCheckableArrayListInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMCheckableArrayListInformationHelper* New();
  vtkTypeMacro(vtkSMCheckableArrayListInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using value obtained for server. It creates
  // an instance of the server helper class vtkPVServerArraySelection
  // and passes the objectId (which the helper class gets as a pointer)
  // and populates the property using the values returned.
  // Each array is represented by two components:
  // name, state (on/off)  
  virtual void UpdateProperty(
    vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX
  
  // Description:
  // Get/Set the name for the vtkSMArrayListDomain. If none is set,
  // the first vtkSMArrayListDomain in the property, if any, is used.
  // Can be set via xml using the "list_domain_name" attribute.
  vtkSetStringMacro(ListDomainName);
  vtkGetStringMacro(ListDomainName);

protected:
  vtkSMCheckableArrayListInformationHelper();
  ~vtkSMCheckableArrayListInformationHelper();

  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  char* ListDomainName;
private:
  vtkSMCheckableArrayListInformationHelper(const vtkSMCheckableArrayListInformationHelper&); // Not implemented.
  void operator=(const vtkSMCheckableArrayListInformationHelper&); // Not implemented.
};

#endif

