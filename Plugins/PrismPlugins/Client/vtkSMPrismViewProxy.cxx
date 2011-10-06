/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPrismViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkSMPrismViewProxy.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPrismViewProxy - view setup for vtkPrism
// .SECTION Description


#include "vtkSMPrismViewProxy.h"
#include "vtkSMPrismSourceProxy.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"

#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

#include "vtkSmartPointer.h" // xxx
#include "vtkCollection.h" // xxx
#include "vtkCollectionIterator.h" // xxx

#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMRepresentationProxy.h"

#include "vtkSMEnumerationDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkPVXMLElement.h"


#define DEBUGPRINT_VIEW(arg) arg;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPrismViewProxy);


//-----------------------------------------------------------------------------
vtkSMPrismViewProxy::vtkSMPrismViewProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMPrismViewProxy::~vtkSMPrismViewProxy()
{
}



//-----------------------------------------------------------------------------
void vtkSMPrismViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMPrismViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
    sproxy->UpdatePipeline(view_time);
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "PrismCompositeRepresentation");

  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool usg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (usg)
    {
    vtkSMProxy *repProxy = 
      pxm->NewProxy("representations", "PrismCompositeRepresentation");
    vtkSMPrismSourceProxy *pspProxy =
      vtkSMPrismSourceProxy::SafeDownCast(source);
    if ( opport == 0 && pspProxy)
      {
      //we don't want to be able to select anything on the zero output port
      //on prism surfaces and filters which are vtkSMPrismSourceProxy instead
      //of vtkSMSourceProxies.
      vtkSMPropertyHelper helper(repProxy,"Pickable");
      helper.Set(0);
      }
    return vtkSMRepresentationProxy::SafeDownCast(repProxy);
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "UniformGridRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "UniformGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "GeometryRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool g = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (g)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "GeometryRepresentation"));
    }

  vtkPVXMLElement* hints = source->GetHints();
  if (hints)
    {
    // If the source has an hint as follows, then it's a text producer and must
    // be is display-able.
    //  <Hints>
    //    <OutputPort name="..." index="..." type="text" />
    //  </Hints>

    unsigned int numElems = hints->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < numElems; cc++)
      {
      int index;
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      if (child->GetName() &&
        strcmp(child->GetName(), "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) &&
        index == opport &&
        child->GetAttribute("type") &&
        strcmp(child->GetAttribute("type"), "text") == 0)
        {
        return vtkSMRepresentationProxy::SafeDownCast(
          pxm->NewProxy("representations", "TextSourceRepresentation"));
        }
      }
    }

  return 0;
}

//
////-----------------------------------------------------------------------------
//vtkSMRepresentationProxy* vtkSMPrismViewProxy::CreateDefaultRepresentation(
//  vtkSMProxy* source, int opport)
//{
// /* if (!source)
//    {
//    return 0;
//    }
//
//  DEBUGPRINT_VIEW(
//    cerr << "SV(" << this << ") CreateDefaultRepresentation" << endl;
//    );
//
//  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
//
//  // Update with time to avoid domains updating without time later.
//  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
//  if (sproxy)
//    {
//    sproxy->UpdatePipeline(this->GetViewUpdateTime());
//    }
//
//  // Choose which type of representation proxy to create.
//  vtkSMProxy* prototype;
//  prototype = pxm->GetPrototypeProxy("representations",
//    "PrismRepresentation");
//  vtkSMInputProperty *pp = vtkSMInputProperty::SafeDownCast(
//    prototype->GetProperty("Input"));
//  pp->RemoveAllUncheckedProxies();
//  pp->AddUncheckedInputConnection(source, opport);
//  bool g = (pp->IsInDomains()>0);
//  pp->RemoveAllUncheckedProxies();
//  if (g)
//    {
//    DEBUGPRINT_VIEW(
//      cerr << "SV(" << this << ") Created PrismRepresentation" << endl;
//      );
//    return vtkSMRepresentationProxy::SafeDownCast(
//      pxm->NewProxy("representations", "PrismRepresentation"));
//    }
//*/
//  return 0;
//}
