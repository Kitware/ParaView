/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkImageNetCDFPOPReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageNetCDFPOPReader.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkExtentTranslator.h"
#include "vtkFloatArray.h"
#include "vtkGridSampler1.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMetaInfoDatabase.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtk_netcdf.h"
#include <string>
#include <vector>

#include "vtkExtentTranslator.h"

#define DEBUGPRINT(arg) ;

#define DEBUGPRINT_RESOLUTION(arg) ;

#define DEBUGPRINT_METAINFORMATION(arg) ;

vtkStandardNewMacro(vtkImageNetCDFPOPReader);

//============================================================================
#define CALL_NETCDF(call) \
{ \
  int errorcode = call; \
  if (errorcode != NC_NOERR) \
  { \
    vtkErrorMacro(<< "netCDF Error: " << nc_strerror(errorcode)); \
    return 0; \
  } \
}
//============================================================================

class vtkImageNetCDFPOPReader::Internal
{
public:
  vtkSmartPointer<vtkDataArraySelection> VariableArraySelection;
  // a mapping from the list of all variables to the list of available
  // point-based variables
  std::vector<int> VariableMap;
  Internal()
  {
    this->VariableArraySelection =
      vtkSmartPointer<vtkDataArraySelection>::New();

    this->GridSampler = vtkGridSampler1::New();
    this->RangeKeeper = vtkMetaInfoDatabase::New();
    this->Resolution = 1.0;
    this->SI = 1;
    this->SJ = 1;
    this->SK = 1;
    this->WholeExtent[0] =
      this->WholeExtent[2] =
      this->WholeExtent[4] = 1;
    this->WholeExtent[1] =
      this->WholeExtent[3] =
      this->WholeExtent[5] = -1;
  }
  ~Internal()
  {
    this->GridSampler->Delete();
    this->RangeKeeper->Delete();
  }
  vtkGridSampler1 *GridSampler;
  vtkMetaInfoDatabase *RangeKeeper;
  double Resolution;
  int SI, SJ, SK;
  int WholeExtent[6];
};

//----------------------------------------------------------------------------
//set default values
vtkImageNetCDFPOPReader::vtkImageNetCDFPOPReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->FileName = NULL;
  this->NCDFFD = 0;

  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback
    (&vtkImageNetCDFPOPReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);

  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;

  this->Internals = new vtkImageNetCDFPOPReader::Internal;
  this->Internals->VariableArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this->SelectionObserver);
}

//----------------------------------------------------------------------------
//delete filename and netcdf file descriptor
vtkImageNetCDFPOPReader::~vtkImageNetCDFPOPReader()
{
  this->SetFileName(0);
  nc_close(this->NCDFFD);
  if(this->SelectionObserver)
    {
    this->SelectionObserver->Delete();
    this->SelectionObserver = NULL;
    }
  if(this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkImageNetCDFPOPReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(NULL)") << endl;
  os << indent << "NCDFFD: " << this->NCDFFD << endl;

  this->Internals->VariableArraySelection->PrintSelf
    (os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
// RequestInformation supplies global meta information
// This should return the reality of what the reader is going to supply.
// This retrieve the extents for the image grid
// NC_MAX_VAR_DIMS comes from the nc library
int vtkImageNetCDFPOPReader::RequestInformation(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  if(this->FileName == NULL)
    {
    vtkErrorMacro("FileName not set.");
    return 0;
    }

  int retval;
  if (!this->NCDFFD)
    {
    retval = nc_open(this->FileName, NC_NOWRITE, &this->NCDFFD);
    if (retval != NC_NOERR)
      {
      vtkErrorMacro(<< "Can't read file " << nc_strerror(retval));
      return 0;
      }
    }

  retval =
    this->Superclass::RequestInformation(request, inputVector, outputVector);
  if (retval != VTK_OK)
    {
    return retval;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::ORIGIN(),this->Origin,3);
  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);

  // get number of variables from file
  int numberOfVariables;
  nc_inq_nvars(this->NCDFFD, &numberOfVariables);
  int dimidsp[NC_MAX_VAR_DIMS];
  int dataDimension;
  size_t dimensions[4]; //dimension value
  this->Internals->VariableMap.resize(numberOfVariables);
  char variableName[NC_MAX_NAME+1];
  int actualVariableCounter = 0;
  // For every variable in the file
  for(int i=0;i<numberOfVariables;i++)
    {
    this->Internals->VariableMap[i] = -1;
    //get number of dimensions
    CALL_NETCDF(nc_inq_varndims(this->NCDFFD, i, &dataDimension));
    //Variable Dimension ID's containing x,y,z coords for the
    //grid spacing
    CALL_NETCDF(nc_inq_vardimid(this->NCDFFD, i, dimidsp));
    if(dataDimension == 3)
      {
      this->Internals->VariableMap[i] = actualVariableCounter++;
      //get variable name
      CALL_NETCDF(nc_inq_varname(this->NCDFFD, i, variableName));
      this->Internals->VariableArraySelection->AddArray(variableName);
      for(int m=0;m<dataDimension;m++)
        {
        CALL_NETCDF(nc_inq_dimlen(this->NCDFFD, dimidsp[m], dimensions+m));
        //acquire variable dimensions
        }
      this->Internals->WholeExtent[0] =
        this->Internals->WholeExtent[2] =
        this->Internals->WholeExtent[4] =0; //set extent
      this->Internals->WholeExtent[1] = static_cast<int>((dimensions[2] -1));
      this->Internals->WholeExtent[3] = static_cast<int>((dimensions[1] -1));
      this->Internals->WholeExtent[5] = static_cast<int>((dimensions[0] -1));
      }
    }

  int sWholeExtent[6];
  sWholeExtent[0] = this->Internals->WholeExtent[0];
  sWholeExtent[1] = this->Internals->WholeExtent[1];
  sWholeExtent[2] = this->Internals->WholeExtent[2];
  sWholeExtent[3] = this->Internals->WholeExtent[3];
  sWholeExtent[4] = this->Internals->WholeExtent[4];
  sWholeExtent[5] = this->Internals->WholeExtent[5];
  outInfo->Set
    (vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), sWholeExtent, 6);

  double sSpacing[3];
  sSpacing[0] = this->Spacing[0];
  sSpacing[1] = this->Spacing[1];
  sSpacing[2] = this->Spacing[2];

  this->Internals->Resolution = 1.0;

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    double rRes =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    int strides[3];
    double aRes;
    int pathLen;
    int *splitPath;

    this->Internals->GridSampler->SetWholeExtent(sWholeExtent);
    vtkIntArray *ia = this->Internals->GridSampler->GetSplitPath();
    pathLen = ia->GetNumberOfTuples();
    splitPath = ia->GetPointer(0);
    DEBUGPRINT_RESOLUTION
      (
       cerr << "pathlen = " << pathLen << endl;
       cerr << "SP = " << splitPath << endl;
       for (int i = 0; i <40 && i < pathLen; i++)
         {
         cerr << splitPath[i] << " ";
         }
       cerr << endl;
       );
    //save split path in translator
    vtkImageData *outData = vtkImageData::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkExtentTranslator *et = outData->GetExtentTranslator();
    et->SetSplitPath(pathLen, splitPath);

    this->Internals->GridSampler->SetSpacing(sSpacing);
    this->Internals->GridSampler->ComputeAtResolution(rRes);

    this->Internals->GridSampler->GetStridedExtent(sWholeExtent);
    this->Internals->GridSampler->GetStridedSpacing(sSpacing);
    this->Internals->GridSampler->GetStrides(strides);
    aRes = this->Internals->GridSampler->GetStridedResolution();

    DEBUGPRINT_RESOLUTION
      (
       cerr << "PST GRID\t";
       {for (int i = 0; i < 3; i++) cerr << sSpacing[i] << " ";}
       {for (int i = 0; i < 6; i++) cerr << sWholeExtent[i] << " ";}
       cerr << endl;
       );

    outInfo->Set
      (vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), sWholeExtent, 6);
    outInfo->Set(vtkDataObject::SPACING(), sSpacing, 3);

    this->Internals->Resolution = aRes;
    this->Internals->SI = strides[0];
    this->Internals->SJ = strides[1];
    this->Internals->SK = strides[2];
    DEBUGPRINT_RESOLUTION
      (
       cerr << "RI SET STRIDE ";
       {for (int i = 0; i < 3; i++) cerr << strides[i] << " ";}
       cerr << endl;
       );
    }

  outInfo->Get(vtkDataObject::ORIGIN(), this->Origin);

  double bounds[6];
  bounds[0] = this->Origin[0] + sSpacing[0] * sWholeExtent[0];
  bounds[1] = this->Origin[0] + sSpacing[0] * sWholeExtent[1];
  bounds[2] = this->Origin[1] + sSpacing[1] * sWholeExtent[2];
  bounds[3] = this->Origin[1] + sSpacing[1] * sWholeExtent[3];
  bounds[4] = this->Origin[2] + sSpacing[2] * sWholeExtent[4];
  bounds[5] = this->Origin[2] + sSpacing[2] * sWholeExtent[5];
  DEBUGPRINT_RESOLUTION
    (
     cerr << "RI SET BOUNDS ";
     {for (int i = 0; i < 6; i++) cerr << bounds[i] << " ";}
     cerr << endl;
     );
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
               bounds, 6);

  return 1;
}

//----------------------------------------------------------------------------
// Setting extents of the rectilinear grid
int vtkImageNetCDFPOPReader::RequestData(vtkInformation* request,
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector  )
{
  this->UpdateProgress(0);
  // the default implimentation is to do what the old pipeline did find what
  // output is requesting the data, and pass that into ExecuteData
  // which output port did the request come from
  int outputPort = request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());
  // if output port is negative then that means this filter is calling the
  // update directly, in that case just assume port 0
  if (outputPort == -1)
    {
    outputPort = 0;
    }
  // get the data object
  vtkInformation *outInfo = outputVector->GetInformationObject(outputPort);

  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  int subext[6];
  //vtkInformation * outInfo = output->GetInformationObject(0);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),subext);
  vtkImageData *imageData = vtkImageData::SafeDownCast(output);
  imageData->SetExtent(subext);

  //setup extents for netcdf library to read the netcdf data file
  size_t start[]= {subext[4]*this->Internals->SK,
                   subext[2]*this->Internals->SJ,
                   subext[0]*this->Internals->SI};

  size_t count[]= {subext[5]-subext[4]+1,
                   subext[3]-subext[2]+1,
                   subext[1]-subext[0]+1};

  ptrdiff_t rStride[3] = { (ptrdiff_t)this->Internals->SK,
                           (ptrdiff_t)this->Internals->SJ,
                           (ptrdiff_t)this->Internals->SI };

  double sSpacing[3];
  outInfo->Get(vtkDataObject::SPACING(), sSpacing);

  double range[2];
  int P = outInfo->Get
    (vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get
    (vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  //initialize memory (raw data space, x y z axis space) and rectilinear grid
  for(size_t i=0;i<this->Internals->VariableMap.size();i++)
    {
    if(this->Internals->VariableMap[i] != -1 &&
       this->Internals->VariableArraySelection->GetArraySetting(
         this->Internals->VariableMap[i]) != 0)
      {
      // varidp is probably i in which case nc_inq_varid isn't needed
      int varidp;
      nc_inq_varid(this->NCDFFD,
                   this->Internals->VariableArraySelection->GetArrayName(
                     this->Internals->VariableMap[i]), &varidp);
      imageData->SetSpacing(sSpacing[0], sSpacing[1], sSpacing[2]);
      //create vtkFloatArray and get the scalars into it
      vtkFloatArray *scalars = vtkFloatArray::New();
      vtkIdType numberOfTuples = (count[0])*(count[1])*(count[2]);
      scalars->SetNumberOfComponents(1);
      scalars->SetNumberOfTuples(numberOfTuples);
      float* data = new float[numberOfTuples];
      nc_get_vars_float(this->NCDFFD, varidp, start, count, rStride,
                        data);
      scalars->SetArray(data, numberOfTuples, 0, 1);

      //set list of variables to display data on rectilinear grid
      const char *name = this->Internals->VariableArraySelection->GetArrayName
        (this->Internals->VariableMap[i]);
      scalars->SetName(name);
      imageData->GetPointData()->AddArray(scalars);

      scalars->GetRange(range);
      this->Internals->RangeKeeper->Insert(P, NP, subext,
                                           this->Internals->Resolution,
                                           0, name, 0,
                                           range);
      DEBUGPRINT_METAINFORMATION
        (
         cerr << "SIP(" << this << ") Calculated range "
         << range[0] << ".." << range[1]
         << " for " << name << " "
         << P << "/" << NP << "&" << this->Internals->Resolution
         << endl;
         );

      scalars->Delete();
      }
    this->UpdateProgress((i+1.0)/this->Internals->VariableMap.size());
    }
  return 1;
}

//----------------------------------------------------------------------------
//following 5 functions are used for paraview user interface
void vtkImageNetCDFPOPReader::SelectionModifiedCallback
  (vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkImageNetCDFPOPReader*>(clientdata)->Modified();
}

//-----------------------------------------------------------------------------
int vtkImageNetCDFPOPReader::GetNumberOfVariableArrays()
{
  return this->Internals->VariableArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkImageNetCDFPOPReader::GetVariableArrayName(int index)
{
  if(index < 0 || index >= this->GetNumberOfVariableArrays())
    {
    return NULL;
    }
  return this->Internals->VariableArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkImageNetCDFPOPReader::GetVariableArrayStatus(const char* name)
{
  return this->Internals->VariableArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkImageNetCDFPOPReader::SetVariableArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(this->Internals->VariableArraySelection->ArrayExists(name) == 0)
    {
    vtkErrorMacro(<< name << " is not available in the file.");
    return;
    }
  int enabled = this->Internals->VariableArraySelection->ArrayIsEnabled(name);
  if(status != 0 && enabled == 0)
    {
    this->Internals->VariableArraySelection->EnableArray(name);
    this->Modified();
    }
  else if(status == 0 && enabled != 0)
    {
    this->Internals->VariableArraySelection->DisableArray(name);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkImageNetCDFPOPReader::ProcessRequest(vtkInformation *request,
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  DEBUGPRINT
    (
     vtkInformation *outInfo = outputVector->GetInformationObject(0);
     int P = outInfo->Get
     (vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
     int NP = outInfo->Get
     (vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
     double res = outInfo->Get
     (vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
     cerr << "SIP(" << this << ") PR " << P << "/" << NP << "@" << res << "->"
     << this->Internals->SI << " "
     << this->Internals->SJ << " "
     << this->Internals->SK << endl;
     );

  if(request->Has
     (vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT_INFORMATION()))
    {
    DEBUGPRINT_METAINFORMATION
      (
       cerr << "SIP(" << this << ") RUEI ==============================="<<endl;
       );
    //create meta information for this piece
    double *origin;
    double *spacing;
    int *ext;
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    origin = outInfo->Get(vtkDataObject::ORIGIN());
    spacing = outInfo->Get(vtkDataObject::SPACING());
    ext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    int P = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    int NP = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    double bounds[6];
    bounds[0] = origin[0] + spacing[0] * ext[0];
    bounds[1] = origin[0] + spacing[0] * ext[1];
    bounds[2] = origin[1] + spacing[1] * ext[2];
    bounds[3] = origin[1] + spacing[1] * ext[3];
    bounds[4] = origin[2] + spacing[2] * ext[4];
    bounds[5] = origin[2] + spacing[2] * ext[5];
    outInfo->Set
      (vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(), bounds, 6);

    int ic = (ext[1]-ext[0]);
    if (ic < 1)
      {
      ic = 1;
      }
    int jc = (ext[3]-ext[2]);
    if (jc < 1)
      {
      jc = 1;
      }
    int kc = (ext[5]-ext[4]);
    if (kc < 1)
      {
      kc = 1;
      }
    outInfo->Set
      (vtkStreamingDemandDrivenPipeline::ORIGINAL_NUMBER_OF_CELLS(), ic*jc*kc);

    double range[2];
    vtkInformationVector *miv =
      outInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
    int cnt = 0;
    for(size_t i=0;i<this->Internals->VariableMap.size();i++)
      {
      if(this->Internals->VariableMap[i] != -1 &&
         this->Internals->VariableArraySelection->GetArraySetting
         (this->Internals->VariableMap[i]) != 0)
        {
        const char *name = this->Internals->VariableArraySelection->GetArrayName
          (this->Internals->VariableMap[i]);
        vtkInformation *fInfo = miv->GetInformationObject(cnt);
        if (!fInfo)
          {
          fInfo = vtkInformation::New();
          miv->SetInformationObject(cnt, fInfo);
          fInfo->Delete();
          }
        cnt++;
        range[0] = 0;
        range[1] = -1;
        if (this->Internals->RangeKeeper->Search(P, NP, ext,
                                                 0, name, 0,
                                                 range))
          {
          DEBUGPRINT_METAINFORMATION
            (
             cerr << "Found range for " << name << " "
             << P << "/" << NP << " "
             << ext[0] << "," << ext[1] << ","
             << ext[2] << "," << ext[3] << ","
             << ext[4] << "," << ext[5] << " is "
             << range[0] << " .. " << range[1] << endl;
             );
          fInfo->Set(vtkDataObject::FIELD_ARRAY_NAME(), name);
          fInfo->Set(vtkDataObject::PIECE_FIELD_RANGE(), range, 2);
          }
        else
          {
          DEBUGPRINT_METAINFORMATION
            (
             cerr << "No range for "
             << ext[0] << "," << ext[1] << ","
             << ext[2] << "," << ext[3] << ","
             << ext[4] << "," << ext[5] << " yet" << endl;
             );
          fInfo->Remove(vtkDataObject::FIELD_ARRAY_NAME());
          fInfo->Remove(vtkDataObject::PIECE_FIELD_RANGE());
          }
        }
      }
    }

  //This is overridden just to intercept requests for debugging purposes.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    DEBUGPRINT
      (cerr << "SIP(" << this << ") RD =============================" << endl;);

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    int updateExtent[6];
    int wholeExtent[6];
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      updateExtent);
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
      wholeExtent);
    double res = this->Internals->Resolution;

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
      {
      res = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
      }
    bool match = true;
    for (int i = 0; i< 6; i++)
      {
      if (updateExtent[i] != wholeExtent[i])
        {
        match = false;
        }
      }
    if (match && (res == 1.0))
      {
      vtkErrorMacro("Whole extent requested, streaming is not working.");
      }
    }

  int rc = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  return rc;
}
