/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPart.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPart.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVDataInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPart);
vtkCxxRevisionMacro(vtkSMPart, "1.1");


//----------------------------------------------------------------------------
vtkSMPart::vtkSMPart()
{
  this->Name = NULL;

  this->DataInformation = vtkPVDataInformation::New();
  this->DataInformationValid = 0;
  this->VTKDataID.ID = 0;

  this->ClassNameInformation = vtkPVClassNameInformation::New();
}

//----------------------------------------------------------------------------
vtkSMPart::~vtkSMPart()
{  
//   // Get rid of the circular reference created by the extent translator.
//   // We have a problem with ExtractPolyDataPiece also.
//   if (this->VTKDataID.ID != 0)
//     {
//     vtkClientServerStream& stream = pm->GetStream();
//     stream << vtkClientServerStream::Invoke 
//            << this->VTKDataID << "SetExtentTranslator" << 0 
//            << vtkClientServerStream::End;
//     pm->SendStreamToServer();
//     }

  this->SetName(NULL);

  this->DataInformation->Delete();
  this->DataInformation = NULL;

  this->ClassNameInformation->Delete();
  this->ClassNameInformation = NULL;
}


//----------------------------------------------------------------------------
// void vtkSMPart::CreateParallelTclObjects(vtkPVApplication *pvApp)
// {
//   if ( !pvApp )
//     {
//     return;
//     }
//   this->vtkKWObject::SetApplication(pvApp);
//   vtkPVProcessModule* pm = pvApp->GetProcessModule();
//   vtkClientServerStream& stream = pm->GetStream();
//   this->GeometryID = pm->NewStreamObject("vtkPVGeometryFilter");
//   stream << vtkClientServerStream::Invoke << this->GeometryID << "SetUseStrips"
//          << pvApp->GetMainView()->GetTriangleStripsCheck()->GetState()
//          << vtkClientServerStream::End;
//   pm->SendStreamToClientAndServer();
//   // Keep track of how long each geometry filter takes to execute.
//   vtkClientServerStream start;
//   start << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//         << "LogStartEvent" << "Execute Geometry" 
//         << vtkClientServerStream::End;
//   vtkClientServerStream end;
//   end << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//       << "LogEndEvent" << "Execute Geometry" 
//       << vtkClientServerStream::End;
//   pm->GetStream() << vtkClientServerStream::Invoke << this->GeometryID 
//                   << "AddObserver"
//                   << "StartEvent"
//                   << start
//                   << vtkClientServerStream::End;
//   pm->GetStream() << vtkClientServerStream::Invoke << this->GeometryID 
//                   << "AddObserver"
//                   << "EndEvent"
//                   << end
//                   << vtkClientServerStream::End;
//   pm->SendStreamToClientAndServer();
// }

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMPart::GetDataInformation()
{
  if (this->DataInformationValid == 0)
    {
    this->GatherDataInformation();
    }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
void vtkSMPart::InvalidateDataInformation()
{
  this->DataInformationValid = 0;
}

//----------------------------------------------------------------------------
void vtkSMPart::GatherDataInformation()
{
//   vtkPVApplication *pvApp = this->GetPVApplication();
//   if ( !pvApp )
//     {
//     return;
//     }
//   vtkPVProcessModule *pm = pvApp->GetProcessModule();

//   // This does nothing if the geometry is already up to date.
//   if (this->PartDisplay)
//     {
//     this->PartDisplay->Update();
//     }

//   pm->GatherInformation(this->DataInformation, this->GetVTKDataID());

//   this->DataInformationValid = 1;

//   // Recompute total visibile memory size. !!!!!!!
//   // This should really be in vtkSMPartDisplay when it gathers its informantion.
//   pvApp->GetRenderModule()->SetTotalVisibleMemorySizeValid(0);

//   // Look for a name defined in Field data.
//   this->SetName(this->DataInformation->GetName());

//   // I would like to have all sources generate names and all filters
//   // Pass the field data, but until all is working well, this is a default.
//   if (this->Name == NULL || this->Name[0] == '\0')
//     {
//     char str[100];
//     if (this->DataInformation->GetDataSetType() == VTK_POLY_DATA)
//       {
//       long nc = this->DataInformation->GetNumberOfCells();
//       sprintf(str, "Polygonal: %ld cells", nc);
//       }
//     else if (this->DataInformation->GetDataSetType() == VTK_UNSTRUCTURED_GRID)
//       {
//       long nc = this->DataInformation->GetNumberOfCells();
//       sprintf(str, "Unstructured Grid: %ld cells", nc);
//       }
//     else if (this->DataInformation->GetDataSetType() == VTK_IMAGE_DATA)
//       {
//       int *ext = this->DataInformation->GetExtent();
//       sprintf(str, "Uniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
//               ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
//       }
//     else if (this->DataInformation->GetDataSetType() == VTK_RECTILINEAR_GRID)
//       {
//       int *ext = this->DataInformation->GetExtent();
//       sprintf(str, "Nonuniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
//               ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
//       }
//     else if (this->DataInformation->GetDataSetType() == VTK_STRUCTURED_GRID)
//       {
//       int *ext = this->DataInformation->GetExtent();
//       sprintf(str, "Curvilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
//               ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
//       }
//     else
//       {
//       sprintf(str, "Part of unknown type");
//       }    
//     this->SetName(str);
//     }
}


//----------------------------------------------------------------------------
void vtkSMPart::SetVTKDataID(vtkClientServerID id)
{
  this->VTKDataID = id;
}

//----------------------------------------------------------------------------
// Create the extent translator (sources with no inputs only).
// Needs to be before "ExtractPieces" because translator propagates.
void vtkSMPart::CreateTranslatorIfNecessary()
{
//   vtkPVApplication *pvApp = this->GetPVApplication();
//   vtkPVProcessModule* pm = pvApp->GetProcessModule();
  
//   // We are going to create the piece filter with a dummy tcl name,
//   // setup the pipeline, and remove tcl's reference to the objects.
//   // The vtkData object will be moved to the output of the piece filter.
//   pm->GatherInformation(this->ClassNameInformation, this->VTKDataID);
//   char *className = this->ClassNameInformation->GetVTKClassName();
//   if (strcmp(className, "vtkImageData") == 0 ||
//       strcmp(className, "vtkStructuredPoints") == 0 ||
//       strcmp(className, "vtkStructuredGrid") == 0 ||
//       strcmp(className, "vtkRectilinearGrid") == 0 )
//     {
//     // Do not overwrite custom extent translators.
//     // PVExtent translator should really be the default,
//     // Then we would not need to do this.
//     pm->GetStream() << vtkClientServerStream::Invoke
//                     << this->VTKDataID << "GetExtentTranslator"
//                     << vtkClientServerStream::End
//                     << vtkClientServerStream::Invoke
//                     << vtkClientServerStream::LastResult
//                     << "GetClassName"
//                     << vtkClientServerStream::End;
//     pm->SendStreamToServerRoot();
//     char* classname = 0;
//     if(!pm->GetLastServerResult().GetArgument(0,0,&classname))
//       {
//       vtkErrorMacro(<< "Faild to get server result.");
//       }
//     if(classname && strcmp(classname, "vtkExtentTranslator") == 0)
//       {
//       vtkClientServerID translatorID =
//         pm->NewStreamObject("vtkPVExtentTranslator");
//       pm->GetStream() << vtkClientServerStream::Invoke
//                       << this->VTKDataID << "SetExtentTranslator"
//                       << translatorID
//                       << vtkClientServerStream::End;
//       // Translator has to be set on source because it is propagated.
//       pm->GetStream() << vtkClientServerStream::Invoke
//                       << translatorID << "SetOriginalSource"
//                       << this->VTKDataID
//                       << vtkClientServerStream::End;
//       pm->DeleteStreamObject(translatorID);
//       pm->SendStreamToServer();
//       }
//    }

}


//----------------------------------------------------------------------------
void vtkSMPart::InsertExtractPiecesIfNecessary()
{
//   vtkPVApplication *pvApp = this->GetPVApplication();
//   vtkPVProcessModule* pm = pvApp->GetProcessModule();
  
//   // We are going to create the piece filter with a dummy tcl name,
//   // setup the pipeline, and remove tcl's reference to the objects.
//   // The vtkData object will be moved to the output of the piece filter.
//   pm->GatherInformation(this->ClassNameInformation, this->VTKDataID);
//   char *className = this->ClassNameInformation->GetVTKClassName();
//   vtkClientServerID tempDataPiece = {0};
//   if (!strcmp(className, "vtkPolyData"))
//     {
//     // Don't add anything if we are only using one processes.
//     // Image can still benifit from its cache so this check
//     // is specific for unstructured data.
//     if (pm->GetNumberOfPartitions() == 1)
//       {
//       return;
//       }  
//     pm->GetStream() << vtkClientServerStream::Invoke 
//                     << this->VTKDataID << "UpdateInformation"
//                     << vtkClientServerStream::End;
//     pm->SendStreamToServer();
//     pm->GetStream() << vtkClientServerStream::Invoke 
//                     << this->VTKDataID << "GetMaximumNumberOfPieces"
//                     << vtkClientServerStream::End;
//     pm->SendStreamToServerRoot();
//     int num =0;
//     pm->GetLastServerResult().GetArgument(0,0,&num);
//     if (num != 1)
//       { // The source can already produce pieces.
//       return;
//       }
//     // Transmit is more efficient, but has the possiblity of hanging.
//     // It will hang if all procs do not  call execute.
//     if (getenv("PV_LOCK_SAFE") != NULL)
//       {
//       tempDataPiece = pm->NewStreamObject("vtkExtractPolyDataPiece");
//       }
//     else
//       {
//       tempDataPiece = pm->NewStreamObject("vtkTransmitPolyDataPiece");
//       vtkClientServerStream start;
//       start << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//             << "LogStartEvent" << "Execute TransmitPData" 
//             << vtkClientServerStream::End;
//       pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
//                       << "AddObserver"
//                       << "StartEvent"
//                       << start
//                       << vtkClientServerStream::End;
//       vtkClientServerStream end;
//       end << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//           << "LogEndEvent" << "Execute TransmitPData" 
//           << vtkClientServerStream::End;
//       pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
//                       << "AddObserver"
//                       << "EndEvent"
//                       << end
//                       << vtkClientServerStream::End;
//       }
//     }
//   else if (!strcmp(className, "vtkUnstructuredGrid"))
//     {
//     // Don't add anything if we are only using one processes.
//     // Image can still benifit from its cache so this check
//     // is specific for unstructured data.
//     if (pm->GetNumberOfPartitions() == 1)
//       {
//       return;
//       }
//     pm->GetStream() << vtkClientServerStream::Invoke 
//                     << this->VTKDataID << "UpdateInformation"
//                     << vtkClientServerStream::End;
//     pm->SendStreamToServer();
//     pm->GetStream() << vtkClientServerStream::Invoke 
//                     << this->VTKDataID << "GetMaximumNumberOfPieces"
//                     << vtkClientServerStream::End;
//     pm->SendStreamToServerRoot();
//     int num =0;
//     pm->GetLastServerResult().GetArgument(0,0,&num);
//     if (num != 1)
//       { // The source can already produce pieces.
//       return;
//       }

//     // Transmit is more efficient, but has the possiblity of hanging.
//     // It will hang if all procs do not  call execute.
//     if (getenv("PV_LOCK_SAFE") != NULL)
//       { 
//       tempDataPiece = pm->NewStreamObject("vtkExtractUnstructuredGridPiece");
//       }
//     else
//       {
//       tempDataPiece = pm->NewStreamObject("vtkTransmitUnstructuredGridPiece");
//       vtkClientServerStream start;
//       start << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//             << "LogStartEvent" << "Execute TransmitPData" 
//             << vtkClientServerStream::End;
//       pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
//                       << "AddObserver"
//                       << "StartEvent"
//                       << start
//                       << vtkClientServerStream::End;
//       vtkClientServerStream end;
//       end << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//           << "LogEndEvent" << "Execute TransmitPData" 
//           << vtkClientServerStream::End;
//       pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
//                       << "AddObserver"
//                       << "EndEvent"
//                       << end
//                       << vtkClientServerStream::End;
//       }
//     }
//   else if (!strcmp(className, "vtkImageData"))
//     {
//     if (getenv("PV_LOCK_SAFE") == NULL)
//       { 
//       tempDataPiece = pm->NewStreamObject("vtkStructuredCacheFilter");
//       }
//     }
//   else if (!strcmp(className, "vtkStructuredGrid"))
//     {
//     if (getenv("PV_LOCK_SAFE") == NULL)
//       {
//       tempDataPiece = pm->NewStreamObject("vtkStructuredCacheFilter");
//       }
//     }
//   else if (!strcmp(className, "vtkRectilinearGrid"))
//     {
//     if (getenv("PV_LOCK_SAFE") == NULL)
//       {
//       tempDataPiece = pm->NewStreamObject("vtkStructuredCacheFilter");
//       }
//     }
//   else
//     {
//     return;
//     }
//   // Connect the filter to the pipeline.  
//   pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
//                   << "SetInput"
//                   << this->VTKDataID
//                   << vtkClientServerStream::End;
//   pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
//                   << "GetOutput"
//                   << vtkClientServerStream::End;
//   this->VTKDataID = pm->GetUniqueID();
//   pm->GetStream() << vtkClientServerStream::Assign << this->VTKDataID
//                   << vtkClientServerStream::LastResult
//                   << vtkClientServerStream::End;
//   pm->DeleteStreamObject(tempDataPiece);
//   pm->SendStreamToServer();
//   this->SetVTKDataID(this->VTKDataID);
}


//----------------------------------------------------------------------------
void vtkSMPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Name: " << (this->Name?this->Name:"none") << endl;
  os << indent << "ClassNameInformation: " << this->ClassNameInformation << endl;
}
