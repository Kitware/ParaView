/*=========================================================================

   Program: ParaView
   Module:    vtkSMCheckableArrayListInformationHelper.cxx

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
#include "vtkSMCheckableArrayListInformationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMDomainIterator.h"

vtkStandardNewMacro(vtkSMCheckableArrayListInformationHelper);
//-----------------------------------------------------------------------------
vtkSMCheckableArrayListInformationHelper::vtkSMCheckableArrayListInformationHelper()
{
  this->ListDomainName = 0;
}

//-----------------------------------------------------------------------------
vtkSMCheckableArrayListInformationHelper::~vtkSMCheckableArrayListInformationHelper()
{
  this->SetListDomainName(0);
}

//-----------------------------------------------------------------------------
void vtkSMCheckableArrayListInformationHelper::UpdateProperty(
  vtkIdType vtkNotUsed(connectionId),  int vtkNotUsed(serverIds), 
  vtkClientServerID vtkNotUsed(objectId), 
  vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
    {
    vtkErrorMacro("A null property or a property of a different type was "
                  "passed when vtkSMStringVectorProperty was needed.");
    return;
    }

  vtkSMArrayListDomain* ild = 0;
  if (this->ListDomainName)
    {
    ild = vtkSMArrayListDomain::SafeDownCast(
      prop->GetDomain(this->ListDomainName));
    }
  else
    {
    vtkSMDomainIterator* di = prop->NewDomainIterator();
    di->Begin();
    while (!di->IsAtEnd())
      {
      ild = vtkSMArrayListDomain::SafeDownCast(di->GetDomain());
      if (ild)
        {
        break;
        }
      di->Next();
      }
    di->Delete();
    }

  if (ild)
    {
    unsigned int num_string = ild->GetNumberOfStrings();

    svp->SetNumberOfElementsPerCommand(2);
    svp->SetElementType(0, vtkSMStringVectorProperty::STRING);
    svp->SetElementType(1, vtkSMStringVectorProperty::INT);
    svp->SetNumberOfElements(num_string*2);
    for(unsigned int i=0; i < num_string; ++i)
      {
      svp->SetElement(2*i, ild->GetString(i));
      svp->SetElement(2*i+1, "1");
      }
    }
}

//-----------------------------------------------------------------------------
int vtkSMCheckableArrayListInformationHelper::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
    {
    return 0;
    }
  const char* list_domain_name = element->GetAttribute("list_domain_name");
  if (list_domain_name)
    {
    this->SetListDomainName(list_domain_name);
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMCheckableArrayListInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ListDomainName: " << 
    (this->ListDomainName? this->ListDomainName : "(none)") << endl;
}
