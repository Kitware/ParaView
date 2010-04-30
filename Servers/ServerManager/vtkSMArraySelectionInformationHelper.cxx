/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArraySelectionInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArraySelectionInformationHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMStringVectorProperty.h"

#include <string.h>
#include <algorithm>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMArraySelectionInformationHelper);

//---------------------------------------------------------------------------
struct  vtkSMArraySelectionInformationHelperSortArray
{
  int          arrayIndx;
  const char * arrayName;
};

bool    vtkSMArraySelectionInformationHelperAlphabeticSorting
( const vtkSMArraySelectionInformationHelperSortArray & thisArray,
  const vtkSMArraySelectionInformationHelperSortArray & thatArray )
{
#if defined(_WIN32)
  return  (  stricmp( thisArray.arrayName, thatArray.arrayName )  <=  0  )
          ?  true  :  false;
#else
  return  (  strcasecmp( thisArray.arrayName, thatArray.arrayName )  <=  0  )
          ?  true  :  false;
#endif
}
//---------------------------------------------------------------------------
vtkSMArraySelectionInformationHelper::vtkSMArraySelectionInformationHelper()
{
  this->AttributeName = 0;
}

//---------------------------------------------------------------------------
vtkSMArraySelectionInformationHelper::~vtkSMArraySelectionInformationHelper()
{
  this->SetAttributeName(0);
}

//---------------------------------------------------------------------------
void vtkSMArraySelectionInformationHelper::UpdateProperty(
  vtkIdType connectionId,  int serverIds, vtkClientServerID objectId, 
  vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
    {
    vtkErrorMacro("A null property or a property of a different type was "
                  "passed when vtkSMStringVectorProperty was needed.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // Create server-side helper if necessary.
  vtkClientServerStream str;

  vtkClientServerID serverSideID = 
    pm->NewStreamObject("vtkPVServerArraySelection", str);

  str << vtkClientServerStream::Invoke
      << serverSideID << "SetProcessModule" << pm->GetProcessModuleID()
      << vtkClientServerStream::End;
  
  // Get the parameters from the server.
  str << vtkClientServerStream::Invoke
      << serverSideID << "GetArraySettings" << objectId << this->AttributeName
      << vtkClientServerStream::End;
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str, 1);

  vtkClientServerStream arrays;
  int retVal = 
    pm->GetLastResult(connectionId, 
      vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &arrays);

  pm->DeleteStreamObject(serverSideID, str);
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str, 0);

  if(!retVal)
    {
    vtkErrorMacro("Error getting array settings from server.");
    return;
    }

  int numArrays = arrays.GetNumberOfArguments(0)/2;

  // sort the arrays alphabetically
  int   arrayIndx = 0;
  vtkstd::vector < vtkSMArraySelectionInformationHelperSortArray > sortArays;
  sortArays.clear();

  if ( numArrays > 0 )
    {
    sortArays.resize( numArrays );
    for ( int j = 0; j < numArrays; j ++ )
      {
      sortArays[j].arrayIndx = j;
      if (   !arrays.GetArgument
              (  0,  j * 2,  &( sortArays[j].arrayName )  )   )
        {
        vtkErrorMacro( "Error getting array name from reader." );
        break;
        }
      }

    vtkstd::sort( sortArays.begin(), sortArays.end(),
                  vtkSMArraySelectionInformationHelperAlphabeticSorting );
    }

  svp->SetNumberOfElementsPerCommand(2);
  svp->SetElementType(0, vtkSMStringVectorProperty::STRING);
  svp->SetElementType(1, vtkSMStringVectorProperty::INT);
  svp->SetNumberOfElements(numArrays*2);
  for(int i=0; i < numArrays; ++i)
    {
    // Get the array status.
    int   status;
    if(  !arrays.GetArgument
          ( 0, sortArays[i].arrayIndx * 2 + 1, &status )  )
      {
      vtkErrorMacro( "Error getting array status from reader." );
      break;
      }

    // Set the selection to match the reader.
    svp->SetElement( 2 * i, sortArays[i].arrayName );
    if( status )
      {
      svp->SetElement( 2 * i + 1, "1" );
      }
    else
      {
      svp->SetElement( 2 * i + 1, "0" );
      }
    }

}

//---------------------------------------------------------------------------
int vtkSMArraySelectionInformationHelper::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
    {
    return 0;
    }

  const char* attribute_name = element->GetAttribute("attribute_name");
  if(attribute_name)
    {
    this->SetAttributeName(attribute_name);
    }
  else
    {
    vtkErrorMacro("No attribute_name specified.");
    return 0;
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMArraySelectionInformationHelper::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
