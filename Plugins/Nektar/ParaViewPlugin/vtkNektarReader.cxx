#include "vtkNektarReader.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"
#include "vtkCleanUnstructuredGrid.h"
#include "vtkDataArraySelection.h"

#include "vtkFloatArray.h"
#include "vtkCellType.h"
#include "vtkPointData.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkNektarReader);

int  setup (FileList *f, Element_List **U, int *nftot, int Nsnapshots, bool mesh_only);
void ReadCopyField (FileList *f, Element_List **U);
void ReadAppendField (FileList *f, Element_List **U,  int start_field_index);
void Calc_Vort (FileList *f, Element_List **U, int nfields, int Snapshot_index);
void Calc_WSS(FileList *f, Element_List **E, Bndry *Ubc, int Snapshot_index, double** wss_vals=NULL);
int get_number_of_vertices_WSS(FileList *f, Element_List **E, Bndry *Ubc, int Snapshot_index);

//----------------------------------------------------------------------------

int vtkNektarReader::next_patch_id = 0;

bool vtkNektarReader::NEED_TO_MANAGER_INIT = true;

vtkNektarReader::vtkNektarReader()
{
  //this->DebugOn();
  vtkDebugMacro(<<"vtkNektarReader::vtkNektarReader(): ENTER");

  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);

  this->FileName = 0;
  this->DataFileName = 0;
  this->ElementResolution = 1;
  this->WSSResolution = 1;
  this->my_patch_id = vtkNektarReader::getNextPatchID();
  vtkDebugMacro(<< "vtkNektarReader::vtkNektarReader(): my_patch_id = " << this->my_patch_id);

  this->UGrid = NULL;
  this->WSS_UGrid = NULL;

  this->READ_GEOM_FLAG = true;
  this->READ_WSS_GEOM_FLAG = false;
  this->HAVE_WSS_GEOM_FLAG = false;
  this->CALC_GEOM_FLAG = true;
  this->CALC_WSS_GEOM_FLAG = false;
  this->IAM_INITIALLIZED = false;
  this->I_HAVE_DATA = false;
  this->FIRST_DATA = true;
  this->USE_MESH_ONLY = false;

  this->UseProjection=0;
  this->ActualTimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->NumberOfTimeSteps = 0;
  this->p_time_start = 0.0;
  this->p_time_inc = 0.0;
  this->displayed_step = -1;
  this->memory_step = -1;
  this->requested_step = -1;

  this->num_extra_vars = 0;

  this->nfields = 4;
  memset (&this->fl, '\0', sizeof(FileList));

  this->fl.in.fp    =  stdin;
  this->fl.out.fp   =  stdout;
  this->fl.mesh.fp  =  stdin;

  this->fl.rea.name = new char[BUFSIZ];
  this->fl.in.name = new char[BUFSIZ];

  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->PointDataArraySelection->AddArray("Velocity");
  this->PointDataArraySelection->AddArray("Pressure");

  this->DerivedVariableDataArraySelection = vtkDataArraySelection::New();
  this->DerivedVariableDataArraySelection->AddArray("Vorticity");
  this->DerivedVariableDataArraySelection->AddArray("lambda_2");
  this->DerivedVariableDataArraySelection->AddArray("Wall Shear Stress");
  this->DisableAllDerivedVariableArrays();

  this->master = (Element_List **) malloc((2)*this->nfields*sizeof(Element_List *));
  this->Ubc = NULL;
  this->WSS_all_vals = NULL;
  this->wss_mem_step = -1;

  this->myList = new nektarList();

  vtkDebugMacro(<<"vtkNektarReader::vtkNektarReader(): EXIT");
}

//----------------------------------------------------------------------------
vtkNektarReader::~vtkNektarReader()
{
  vtkDebugMacro(<<"vtkNektarReader::~vtkNektarReader(): ENTER");

  this->SetFileName(0);
  this->SetDataFileName(0);
  this->PointDataArraySelection->Delete();
  this->DerivedVariableDataArraySelection->Delete();

  if(this->UGrid)
    {
    vtkDebugMacro(<<"vtkNektarReader::~vtkNektarReader(): ugrid not null, delete it");
    this->UGrid->Delete();
    }
  if(this->WSS_UGrid)
    {
    this->WSS_UGrid->Delete();
    }
  if(this->WSS_all_vals)
    {
    delete this->WSS_all_vals;
    }

  if (this->myList)
    {
    myList->~nektarList();
    }

  vtkDebugMacro(<<"vtkNektarReader::~vtkNektarReader(): EXIT");
}

//----------------------------------------------------------------------------
void vtkNektarReader::setActive()
{
  iparam_set("IDpatch", this->my_patch_id);
}

//----------------------------------------------------------------------------
void vtkNektarReader::GetAllTimes(vtkInformationVector *outputVector)
{
  FILE* dfPtr = NULL;
  char dfName[265];
  char paramLine[256];
  int file_index;
  float test_time_val;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);

  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = this->NumberOfTimeSteps-1;

  vtkDebugMacro(<< "vtkNektarReader::GetAllTimes: this->NumberOfTimeSteps = "<<
                this->NumberOfTimeSteps);

  this->TimeSteps.resize(this->NumberOfTimeSteps);

  if(!this->USE_MESH_ONLY)
    {
    if(this->p_time_inc == 0.0)
      {
      for (int i=0; i<(this->NumberOfTimeSteps); i++)
        {
        file_index = this->p_rst_start + (this->p_rst_inc*i);
        sprintf(dfName, this->p_rst_format, file_index );
        dfPtr = fopen(dfName, "r");
        if(!dfPtr)
          {
          vtkErrorMacro(<< "Failed to open file: "<< dfName);
          }
        // skip the first 5 lines
        for(int j=0; j<5; j++)
          {
          fgets(paramLine, 256, dfPtr);
          }
        fgets(paramLine, 256, dfPtr);

        //fprintf(stderr, "Line: \'%s\'\n", paramLine);
        //sscanf(paramLine, "%f", &(this->TimeSteps[i]));
        sscanf(paramLine, "%f", &test_time_val);
        this->TimeSteps[i] = test_time_val;

        fclose(dfPtr);
        dfPtr = NULL;
        //fprintf(stderr, "File: %s : time: %f\n", dfName, this->TimeSteps[i]);
        }
      }
    else
      {
      double cur_time = this->p_time_start;
      for (int i=0; i<(this->NumberOfTimeSteps); i++)
        {
        this->TimeSteps[i] = cur_time;
        cur_time += this->p_time_inc;
        }

      }
    }  // from if(!this->USE_MESH_ONLY)
  else
    {
    this->TimeSteps[0] = 0.0;
    }
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
               &(this->TimeSteps[0]),
               this->NumberOfTimeSteps);

  outInfo1->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                &(this->TimeSteps[0]),
                this->NumberOfTimeSteps);

  double timeRange[2];
  timeRange[0] = this->TimeSteps[0];
  timeRange[1] = this->TimeSteps[this->NumberOfTimeSteps-1];

  vtkDebugMacro(<< "vtkNektarReader::GetAllTimes: timeRange[0] = "<<timeRange[0]<< ", timeRange[1] = "<< timeRange[1]);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
               timeRange, 2);
  outInfo1->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                timeRange, 2);
} // vtkNektarReader::GetAllTimes()

//----------------------------------------------------------------------------
unsigned long vtkNektarReader::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  time = this->PointDataArraySelection->GetMTime();
  mTime = ( time > mTime ? time : mTime );

  time = this->DerivedVariableDataArraySelection->GetMTime();
  mTime = ( time > mTime ? time : mTime );

  return mTime;
}

//----------------------------------------------------------------------------
void vtkNektarReader::SetElementResolution(int val)
{
  if(val >0 && val<11)
    {
    this->ElementResolution = val;
    //iparam_set("VIS_RES", ElementResolution);

    // *** is this still always true?  I'm thinking not, but it
    //     may not matter....
    this->CALC_GEOM_FLAG = true;
    this->Modified();
    vtkDebugMacro(<<"vtkNektarReader::SetElementResolution: CALC_GEOM_FLAG now true (need to calculate) , ElementResolution= "<< this->ElementResolution);
    }
}

//----------------------------------------------------------------------------
void vtkNektarReader::SetWSSResolution(int val)
{
  if(val >0 && val<11)
    {
    this->WSSResolution = val;
    //iparam_set("VIS_RES", ElementResolution);

    // *** is this still always true?  I'm thinking not, but it
    //     may not matter....
    this->CALC_WSS_GEOM_FLAG = true;
    this->Modified();
    vtkDebugMacro(<<"vtkNektarReader::SetWSSResolution: CALC_GEOM_FLAG now true (need to calculate) , WSSResolution= "<< this->WSSResolution);
    }
}

//----------------------------------------------------------------------------
void vtkNektarReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkNektarReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkNektarReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkNektarReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkNektarReader::SetPointArrayStatus(const char* name, int status)
{
  if(status)
    {
    this->PointDataArraySelection->EnableArray(name);
    }
  else
    {
    this->PointDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
void vtkNektarReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkNektarReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
int vtkNektarReader::GetNumberOfDerivedVariableArrays()
{
  return this->DerivedVariableDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkNektarReader::GetDerivedVariableArrayName(int index)
{
  return this->DerivedVariableDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkNektarReader::GetDerivedVariableArrayStatus(const char* name)
{
  return this->DerivedVariableDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkNektarReader::SetDerivedVariableArrayStatus(const char* name, int status)
{
  if(status)
    {
    this->DerivedVariableDataArraySelection->EnableArray(name);
    }
  else
    {
    this->DerivedVariableDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
void vtkNektarReader::EnableAllDerivedVariableArrays()
{
  this->DerivedVariableDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkNektarReader::DisableAllDerivedVariableArrays()
{
  this->DerivedVariableDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkNektarReader::GetVariableNamesFromData()
{
  FILE* dfPtr = NULL;
  char  dfName[265];
  char  paramLine[256];
  int   ind = 4;
  char  var_name[2];

  var_name[1] = '\0';
  int my_rank = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  if(my_rank == 0)
    {
    sprintf(dfName, this->p_rst_format, this->p_rst_start );
    dfPtr = fopen(dfName, "r");
    if(!dfPtr) vtkErrorMacro(<< "Failed to open file: "<< dfName);
    // skip the first 8 lines
    for(int j=0; j<8; j++)
      {
      fgets(paramLine, 256, dfPtr);
      }
    fgets(paramLine, 256, dfPtr);

    fclose(dfPtr);
    dfPtr = NULL;

    vtkDebugMacro(<< "vtkNektarReader::GetVariableNamesFromData:before bcast my_rank: "<<my_rank<< "  paramLine = "<< paramLine);
    }

  vtkMultiProcessController::GetGlobalController()->Broadcast(paramLine, 256, 0);
  vtkDebugMacro(<< "vtkNektarReader::GetVariableNamesFromData:after  bcast my_rank: "<<my_rank<< "  paramLine = "<< paramLine);

  char* space_index = index(paramLine, ' ');
  *space_index = '\0';
  int len = strlen(paramLine);
  vtkDebugMacro(<< "vtkNektarReader::GetVariableNamesFromData:after strlen my_rank: "<<my_rank<< "  paramLine = "<< paramLine<< "  len= "<< len);

  this->num_extra_vars = len - 4;
  if(this->num_extra_vars > 0)
    {
    int ii=0;
    this->extra_var_names =  (char**) malloc(this->num_extra_vars* sizeof(char*));
    while(ind<len)
      {
      var_name[0] = paramLine[ind++];
      this->extra_var_names[ii++] = strdup( var_name);
      this->PointDataArraySelection->AddArray(var_name);
      }
    }
  else
    {
    this->num_extra_vars = 0;
    this->extra_var_names = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkNektarReader::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkDebugMacro(<<"vtkNektarReader::RequestInformation(): ENTER");

  FILE* inPtr = NULL;
  char* scan_ret;
  char* p;
  char* p2;
  char param[32];
  char paramLine[256];
  double timer_diff;


  if(!this->IAM_INITIALLIZED)
    {
    //print the name of the file we're supposed to open
    vtkDebugMacro(<< "vtkNektarReader::RequestInformation: FileName: " << this->GetFileName());

    inPtr = fopen(this->GetFileName(), "r");
    if(inPtr == NULL)
      {
      vtkErrorMacro(<< "vtkNektarReader::RequestInformation: could not open file: "<< this->GetFileName());
      return(0);
      }

    this->p_rst_digits = 0;

    scan_ret = fgets(paramLine, 256, inPtr);

    while(scan_ret != NULL)
      {
      p=strchr(paramLine, ':');

      while(p != NULL && !this->USE_MESH_ONLY)
        {
        *p = '\0';
        sscanf(paramLine, "%s", param);
        p = p+2;

        p2=strchr(p, '\n');
        *p2= '\0';

        if(strcasecmp(param, "REA_FILE") == 0)
          {
          while (*p == ' ')
            {
            p++;
            }
          strcpy(this->p_rea_file, p);
          }
        else if(strcasecmp(param, "RST_DIR") == 0)
          {
          while (*p == ' ')
            {
            p++;
            }
          strcpy(this->p_rst_dir, p);
          if(strcasecmp(this->p_rst_dir, "NULL") == 0)
            {
            this->USE_MESH_ONLY = true;
            this->p_rst_num = 1;
            p= NULL;
            fprintf(stderr, "found RST_DIR: %s\n", this->p_rst_dir);
            fflush(stderr);
            break;
            }
          }
        else if(strcasecmp(param, "RST_BASE") == 0)
          {
          while (*p == ' ')
            {
            p++;
            }
          strcpy(this->p_rst_base, p);
          }
        else if(strcasecmp(param, "RST_EXT") == 0)
          {
          while (*p == ' ')
            {
            p++;
            }
          strcpy(this->p_rst_ext, p);
          }
        else if(strcasecmp(param, "RST_START") == 0)
          {
          sscanf(p, "%d", &this->p_rst_start);
          }
        else if(strcasecmp(param, "RST_INC") == 0)
          {
          sscanf(p, "%d", &this->p_rst_inc);
          }
        else if(strcasecmp(param, "RST_NUM") == 0)
          {
          sscanf(p, "%d", &this->p_rst_num);
          }
        else if(strcasecmp(param, "RST_DIGITS") == 0)
          {
          sscanf(p, "%d", &this->p_rst_digits);
          }
        else if(strcasecmp(param, "TIME_START") == 0)
          {
          sscanf(p, "%f", &this->p_time_start);
          }
        else if(strcasecmp(param, "TIME_INC") == 0)
          {
          sscanf(p, "%f", &this->p_time_inc);
          }
        scan_ret = fgets(paramLine, 256, inPtr);
        if(scan_ret)
          {
          p=strchr(paramLine, ':');
          }
        else
          {
          p= NULL;
          }
        } // while(p!=NULL && !this->USE_MESH_ONLY)

      scan_ret = fgets(paramLine, 256, inPtr);
      }// while(ret != NULL)

    //this->p_rst_current = this->p_rst_start;

    vtkDebugMacro(<<"REA_FILE:   \'"<<this->p_rea_file<<"\'");
    vtkDebugMacro(<<"RST_DIR:    \'"<<this->p_rst_dir<<"\'");
    if(!this->USE_MESH_ONLY)
      {
      vtkDebugMacro(<<"RST_BASE:   \'"<<this->p_rst_base<<"\'");
      vtkDebugMacro(<<"RST_EXT:    \'"<<this->p_rst_ext<<"\'");
      vtkDebugMacro(<<"RST_START:  "<<this->p_rst_start);
      vtkDebugMacro(<<"RST_INC:    "<<this->p_rst_inc);
      vtkDebugMacro(<<"RST_NUM:    "<<this->p_rst_num);
      sprintf(this->p_rst_format, "%s/%s%%%dd.%s",
              this->p_rst_dir,
              this->p_rst_base,
              this->p_rst_digits,
              this->p_rst_ext);
      vtkDebugMacro(<<"RST_FORMAT: \'"<<this->p_rst_format<<"\'");
      }
    this->NumberOfTimeSteps = this->p_rst_num;

    fclose(inPtr);

    vtkNew<vtkTimerLog> timer;
    timer->StartTimer();
    this->GetAllTimes(outputVector);
    timer->StopTimer();
    timer_diff = timer->GetElapsedTime();

    if (!this->USE_MESH_ONLY)
      {
      this->GetVariableNamesFromData();
      }
    vtkDebugMacro(<<"Rank: "<<vtkMultiProcessController::GetGlobalController()->GetLocalProcessId()
                  <<" :: GetAllTimeSteps time: "<< timer_diff);

//      sprintf(this->fl.rea.name,"%s", this->reaFile);
//      this->SetDataFileName(this->reaFile);
    sprintf(this->fl.rea.name,"%s", this->p_rea_file);
    this->SetDataFileName(this->p_rea_file);

    this->fl.rea.fp = fopen(this->fl.rea.name,"r");
    if (!this->fl.rea.fp)
      {
      error_msg(Restart: no REA file read);
      }

    if(!this->USE_MESH_ONLY)
      {
      sprintf(this->p_rst_file, this->p_rst_format, this->p_rst_start);
      sprintf(this->fl.in.name,"%s", this->p_rst_file);
      this->fl.in.fp = fopen(this->fl.in.name,"r");
      if (!this->fl.in.fp)
        {
        error_msg(Restart: no dumps read from restart file);
        }
      }

    vtkInformation *outInfo0 =
      outputVector->GetInformationObject(0);
    outInfo0->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

    vtkInformation *outInfo1 =
      outputVector->GetInformationObject(1);
    outInfo1->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

    this->IAM_INITIALLIZED = true;
    }// if(!this->IAM_INITIALLIZED)

  vtkDebugMacro(<<"vtkNektarReader::RequestInformation(): EXIT");

  return 1;
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int vtkNektarReader::RequestData(
  vtkInformation* request,
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  double timer_diff;
  double total_timer_diff;

  vtkNew<vtkTimerLog> total_timer;
  total_timer->StartTimer();
  // the default implimentation is to do what the old pipeline did find what
  // output is requesting the data, and pass that into ExecuteData

  // which output port did the request come from
  int outputPort =
    request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

  // if output port is negative then that means this filter is calling the
  // update directly, in that case just assume port 0
  if (outputPort == -1)
    {
    outputPort = 0;
    }

  // get the data object
  vtkInformation *outInfo =
    outputVector->GetInformationObject(0);     //(outputPort);

  vtkInformation *outInfo1 =
    outputVector->GetInformationObject(1);

  vtkInformation *requesterInfo =
    outputVector->GetInformationObject(outputPort);

  int tsLength =
    requesterInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  double* steps =
    requesterInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  vtkDebugMacro(<<"RequestData: tsLength= "<< tsLength);

  // Check if a particular time was requested.
  bool hasTimeValue = false;
  // Collect the time step requested
  vtkInformationDoubleKey* timeKey =
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP();

  if (outInfo->Has(timeKey))
    {
    this->TimeValue = outInfo->Get(timeKey);
    hasTimeValue = true;
    }
  if(hasTimeValue)
    {
    vtkDebugMacro(<<"RequestData: this->TimeValue= "<< this->TimeValue);

    //find the timestep with the closest value to the requested time value
    int closestStep=0;
    double minDist=-1;
    for (int cnt=0;cnt<tsLength;cnt++)
      {
      //fprintf(stderr, "RequestData: steps[%d]=%f\n", cnt, steps[cnt]);
      double tdist=(steps[cnt]-this->TimeValue>this->TimeValue-steps[cnt])?steps[cnt]-this->TimeValue:this->TimeValue-steps[cnt];
      if (minDist<0 || tdist<minDist)
        {
        minDist=tdist;
        closestStep=cnt;
        }
      }
    this->ActualTimeStep=closestStep;
    }

  vtkDebugMacro(<<"RequestData: this->ActualTimeStep= "<< this->ActualTimeStep);

  // Force TimeStep into the "known good" range. Although this
  if ( this->ActualTimeStep < this->TimeStepRange[0] )
    {
    this->ActualTimeStep = this->TimeStepRange[0];
    }
  else if ( this->ActualTimeStep > this->TimeStepRange[1] )
    {
    this->ActualTimeStep = this->TimeStepRange[1];
    }

  int my_rank = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  vtkDebugMacro(<<"RequestData: ENTER: rank: "<< my_rank << "  outputPort: "
                << outputPort << "  this->ActualTimeStep = "<< this->ActualTimeStep);

  // if the user has requested wss, and we have not read the wss geometry
  // before, set READ_WSS_GEOM_FLAG to true.  This will only happen once.
  if(this->GetDerivedVariableArrayStatus("Wall Shear Stress") &&
     !this->HAVE_WSS_GEOM_FLAG)
    {
    this->READ_WSS_GEOM_FLAG = true;
    }

  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* wss_ugrid = vtkUnstructuredGrid::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));

  // Save the time value in the output (ugrid) data information.
  if (steps)
    {
    ugrid->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),
                                 steps[this->ActualTimeStep]);
    wss_ugrid->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),
                                     steps[this->ActualTimeStep]);
    }

//  int new_rst_val = this->p_rst_start + (this->p_rst_inc* this->ActualTimeStep);
  this->requested_step = this->p_rst_start + (this->p_rst_inc* this->ActualTimeStep);

  //  if the step being displayed is different than the one requested
  if(this->displayed_step != this->requested_step)
    {
    // get the requested object from the list, if the ugrid in the object is NULL
    // then we have not loaded it yet
    this->curObj = this->myList->getObject(this->requested_step);

    // need to compare the request with the curObj
    // if the resolutions are not the same, or any variable in the request is true,
    // but false in the object, then we will need the data to be in memory
    if( (this->curObj->resolution != this->ElementResolution) ||
        (this->curObj->wss_resolution != this->WSSResolution) ||
        (this->curObj->use_projection != this->UseProjection) ||
        (this->GetPointArrayStatus("Pressure") && !this->curObj->pressure) ||
        (this->GetPointArrayStatus("Velocity") && !this->curObj->velocity) ||
        (this->GetDerivedVariableArrayStatus("Vorticity") && !this->curObj->vorticity) ||
        (this->GetDerivedVariableArrayStatus("lambda_2") && !this->curObj->lambda_2) ||
        (this->GetDerivedVariableArrayStatus("Wall Shear Stress") && !this->curObj->wss) )
      {
      // if the step in memory is different than the step requested
      if(this->requested_step != this->memory_step)
        {
        this->I_HAVE_DATA = false;
        }

      // if the resolutions are not the same, also need to calculate the continuum geometry
      if(this->curObj->resolution != this->ElementResolution)
        {
        this->CALC_GEOM_FLAG = true;
        }
      // if the resolutions are not the same, also need to calculate the wss geometry
      if(this->curObj->wss_resolution != this->WSSResolution)
        {
        this->CALC_WSS_GEOM_FLAG = true;
        }
      }

    //this->I_HAVE_DATA = false;
    }

  vtkDebugMacro(<< "vtkNektarReader::RequestData:  ElementResolution: " << ElementResolution );

  // if I have not yet read the geometry, this should only happen once
  if(this->READ_GEOM_FLAG)
    {
    if(true == vtkNektarReader::NEED_TO_MANAGER_INIT)
      {
      vtkDebugMacro( << "vtkNektarReader::RequestData: READ_GEOM_FLAG==true, call manager_init()" );

      manager_init();       /* initialize the symbol table manager */
      vtkDebugMacro(<< "vtkNektarReader::RequestData: manager_init complete" );
      vtkNektarReader::NEED_TO_MANAGER_INIT = false;
      }
    else
      {
      vtkDebugMacro( << "vtkNektarReader::RequestData: READ_GEOM_FLAG==true, NO manager_init(), already called" );
      }
    option_set("FEstorage",1);
    option_set("iterative",1);
    option_set("GSLEVEL", min(vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses(),8));
    iparam_set("NORDER.REQ", UNSET);
    iparam_set("VIS_RES", this->ElementResolution);

    //read mesh file and header of the  data file
    this->setActive();
    vtkNew<vtkTimerLog> timer;
    vtkDebugMacro( << "vtkNektarReader::RequestData: call setup with this->USE_MESH_ONLY= "<<this->USE_MESH_ONLY );
    this->nfields = setup (&this->fl, this->master, &this->nfields, 1, this->USE_MESH_ONLY);

    timer->StopTimer();
    timer_diff = timer->GetElapsedTime();

    this->READ_GEOM_FLAG = false;

    vtkDebugMacro(<<"vtkNektarReader::RequestData:: Rank: "<<my_rank
                  <<" ::Done calling setup (read mesh: "<< this->p_rea_file<<"): Setup Time: "<< timer_diff);
    } // if(this->READ_GEOM_FLAG)

  if(this->READ_WSS_GEOM_FLAG)
    {
    iparam_set("WSS_RES", this->WSSResolution);
    // begin: NEW WSS call
    this->setActive();
    vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<< my_rank<<" Active: "
                  << this->my_patch_id << " : About to call ReadBCs, this->Ubc = "<< this->Ubc);

    this->Ubc = ReadBCs(this->fl.rea.fp, this->master[0]->fhead);
//    this->Ubc = ReadMeshBCs(this->fl.rea.fp, this->master[0]);
    vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<< my_rank
                  <<" Done with call to ReadBCs, this->Ubc = "<< this->Ubc);

    for(Bndry* B=this->Ubc; B; B = B->next)
      {
      if(B->type == 'W')
        {
        B->elmt->Surface_geofac(B);
        }
      }

    this->READ_WSS_GEOM_FLAG = false;
    this->HAVE_WSS_GEOM_FLAG = true;
    } // if(this->READ_GEOM_FLAG)

  // if we need to read the data from disk..

  if(!this->I_HAVE_DATA && !this->USE_MESH_ONLY)
    {

    //sprintf(this->p_rst_file, this->p_rst_format, this->p_rst_current);
    sprintf(this->p_rst_file, this->p_rst_format, this->requested_step);
    sprintf(this->fl.in.name,"%s", this->p_rst_file);
    this->fl.in.fp = fopen(this->fl.in.name,"r");
    if (!this->fl.in.fp)
      {
      error_msg(Restart: no dumps read from restart file);
      }

    /* read the field data */
    if(FIRST_DATA)
      {
      vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<< my_rank<<" Now reading data from file: "<< this->p_rst_file);
      this->setActive();
      vtkNew<vtkTimerLog> timer;
      timer->StartTimer();
      ReadCopyField(&this->fl,this->master);

      timer->StopTimer();
      timer_diff = timer->GetElapsedTime();

      vtkDebugMacro(<<"vtkNektarReader::RequestData:: Rank: "<<my_rank<<" ::Done reading data from file: "<< this->p_rst_file<<":: Read  time: "<< timer_diff);
      this->curObj->setDataFilename(this->p_rst_file);
      FIRST_DATA = false;
      }
    else
      {
      this->setActive();
      vtkNew<vtkTimerLog> timer;
      timer->StartTimer();
      ReadAppendField(&this->fl,this->master, 0);
      timer->StopTimer();
      timer_diff = timer->GetElapsedTime();
      vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<<my_rank<<" ::Done reading (Append) data from file: "<< this->p_rst_file<<":: Read  time: "<< timer_diff);
      this->curObj->setDataFilename(this->p_rst_file);
      }

    /* transform field into physical space */
    vtkNew<vtkTimerLog> timer;
    for(int i = 0; i < this->nfields; ++i)
      {
      this->master[i]->Trans(this->master[i],J_to_Q);
      }
    timer->StopTimer();
    timer_diff = timer->GetElapsedTime();
    vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<<my_rank<<" :: Transform field into physical space time: "<< timer_diff);
    this->I_HAVE_DATA = true;
    this->memory_step = this->requested_step;

    iparam_set("VIS_RES", this->ElementResolution);
    iparam_set("WSS_RES", this->WSSResolution);
    } // if(!this->I_HAVE_DATA && !this->USE_MESH_ONLY)
  else
    {
    iparam_set("VIS_RES", this->ElementResolution);
    iparam_set("WSS_RES", this->WSSResolution);
    }

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  this->updateVtuData(ugrid, wss_ugrid); // , outputPort);
  timer->StopTimer();
  timer_diff = timer->GetElapsedTime();
  vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<<my_rank<<" :: updateVtuData time: "<<  timer_diff);

  if(!this->USE_MESH_ONLY)
    {
    this->SetDataFileName(this->curObj->dataFilename);
    }
  total_timer->StopTimer();
  total_timer_diff = total_timer->GetElapsedTime();

  vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<<my_rank<< "  outputPort: " << outputPort <<" EXIT :: Total time: "<< total_timer_diff);
  return 1;
}

void vtkNektarReader::updateVtuData(vtkUnstructuredGrid* pv_ugrid, vtkUnstructuredGrid* pv_wss_ugrid) //, int outputPort)
{
  register int i,j,k,n;
  int      qa,wss_qa,cnt;
  int      alloc_res;
  int      ntot = 0;
  double timer_diff;

  int my_rank = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  vtkDebugMacro(<<"vtkNektarReader::updateVtuData: ENTER: Rank: "<<my_rank); //<<" :: outputPort: "<< outputPort);
  // if the grid in the curObj is not NULL, we may have everything we need
  if(this->curObj->ugrid)
    {
    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": this->curObj->ugrid != NULL, see if it matches");
    // if the curObj matches the request, just shallow copy, and we're done
    if( (curObj->resolution == this->ElementResolution) &&
        (curObj->wss_resolution == this->WSSResolution) &&
        (curObj->use_projection  == this->UseProjection) &&
        (this->GetPointArrayStatus("Pressure") == this->curObj->pressure) &&
        (this->GetPointArrayStatus("Velocity") == this->curObj->velocity) &&
        (this->GetDerivedVariableArrayStatus("Vorticity")  == this->curObj->vorticity) &&
        (this->GetDerivedVariableArrayStatus("lambda_2") == this->curObj->lambda_2) &&
        (this->GetDerivedVariableArrayStatus("Wall Shear Stress") == this->curObj->wss ) )
      {
      pv_ugrid->ShallowCopy(this->curObj->ugrid);
      if(this->GetDerivedVariableArrayStatus("Wall Shear Stress"))
        {
        pv_wss_ugrid->ShallowCopy(this->curObj->wss_ugrid);
        }
      else
        {
        if( pv_wss_ugrid)
          {
          pv_wss_ugrid->Initialize();
          }
        }
      this->displayed_step = this->requested_step;
      vtkDebugMacro(<<"vtkNektarReader::updateVtuData: ugrid same, copy : Rank: "<<my_rank); //<<" :: outputPort: "<< outputPort);
      if(!this->USE_MESH_ONLY)
        {
        this->SetDataFileName(curObj->dataFilename);
        }
      return;
      }
    // else if the request is for less than what is in the curObj,
    // remove unwanted, update the curObj, shallow copy and we're done.
    else if( (this->curObj->resolution == this->ElementResolution) &&
             (this->curObj->wss_resolution == this->WSSResolution) &&
             (this->curObj->use_projection == this->UseProjection) &&
             (!this->GetPointArrayStatus("Pressure") ||
              (this->GetPointArrayStatus("Pressure") && this->curObj->pressure)) &&
             (!this->GetPointArrayStatus("Velocity") ||
              (this->GetPointArrayStatus("Velocity") && this->curObj->velocity)) &&
             (!this->GetDerivedVariableArrayStatus("Vorticity") ||
              (this->GetDerivedVariableArrayStatus("Vorticity") && this->curObj->vorticity)) &&
             (!this->GetDerivedVariableArrayStatus("lambda_2") ||
              (this->GetDerivedVariableArrayStatus("lambda_2") && this->curObj->lambda_2)) &&
             (!this->GetDerivedVariableArrayStatus("Wall Shear Stress") ||
              (this->GetDerivedVariableArrayStatus("Wall Shear Stress") && this->curObj->wss)) )
      {
      // Remove pressure if curObj has it, but not in request
      if(!this->GetPointArrayStatus("Pressure") && this->curObj->pressure)
        {
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("Pressure") != NULL)
          {
          pv_ugrid->GetPointData()->RemoveArray("Pressure");
          }
        // Do I already have this array?  If so, remove it.
        if (this->curObj->ugrid->GetPointData()->GetArray("Pressure") != NULL)
          {
          this->curObj->ugrid->GetPointData()->RemoveArray("Pressure");
          }
        this->curObj->pressure = false;
        }
      // Remove velocity if curObj has it, but not in request
      if(!this->GetPointArrayStatus("Velocity") && this->curObj->velocity)
        {
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("Velocity") != NULL)
          {
          pv_ugrid->GetPointData()->RemoveArray("Velocity");
          }
        // Do I already have this array?  If so, remove it.
        if (this->curObj->ugrid->GetPointData()->GetArray("Velocity") != NULL)
          {
          this->curObj->ugrid->GetPointData()->RemoveArray("Velocity");
          }
        this->curObj->velocity = false;
        }
      // Remove vorticity if curObj has it, but not in request
      if(!this->GetDerivedVariableArrayStatus("Vorticity") && this->curObj->vorticity)
        {
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("Vorticity") != NULL)
          {
          pv_ugrid->GetPointData()->RemoveArray("Vorticity");
          }
        // Do I already have this array?  If so, remove it.
        if (this->curObj->ugrid->GetPointData()->GetArray("Vorticity") != NULL)
          {
          this->curObj->ugrid->GetPointData()->RemoveArray("Vorticity");
          }
        this->curObj->vorticity = false;
        }
      // Remove lambda_2 if curObj has it, but not in request
      if(!this->GetDerivedVariableArrayStatus("lambda_2") && this->curObj->lambda_2)
        {
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("lambda_2") != NULL)
          {
          pv_ugrid->GetPointData()->RemoveArray("lambda_2");
          }
        // Do I already have this array?  If so, remove it.
        if (this->curObj->ugrid->GetPointData()->GetArray("lambda_2") != NULL)
          {
          this->curObj->ugrid->GetPointData()->RemoveArray("lambda_2");
          }
        this->curObj->lambda_2 = false;
        }

      pv_ugrid->ShallowCopy(this->curObj->ugrid);

      if(!this->GetDerivedVariableArrayStatus("Wall Shear Stress") && this->curObj->wss)
        {
        //pv_wss_ugrid->ShallowCopy(this->curObj->ugrid);
        this->curObj->wss_ugrid = vtkUnstructuredGrid::New();
        pv_wss_ugrid->ShallowCopy(this->curObj->wss_ugrid);
        this->curObj->wss = false;
        }
      else
        {
        pv_wss_ugrid->ShallowCopy(this->curObj->wss_ugrid);
        }
      this->displayed_step = this->requested_step;
      if(!this->USE_MESH_ONLY)
        {
        this->SetDataFileName(curObj->dataFilename);
        }
      return;
      }
    }

  // if the wss_ugrid is NULL, we'll need to calculate WSS (before interpolating).  this is likely a new time step.
  bool NEED_TO_CALC_WSS = false;
  if(this->curObj->wss_ugrid == NULL || ( this->wss_mem_step != this->requested_step))
    {
    vtkDebugMacro(<<"+++++++++++++++++++++++++++  need to recalc WSS");
    NEED_TO_CALC_WSS = true;
    }
  else
    {
    vtkDebugMacro(<<"---------------------------  NO need to recalc WSS");
    }

  // otherwise the grid in the curObj is NULL, and/or the resolution has changed,
  // and/or we need more data than is in curObj, we need to do everything

  vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: can't use existing ugrid, wss_ugrid, or both, Rank = "<<my_rank);
  // <<" :: outputPort: "<< outputPort<<": this->master[0]->nel = "<< this->master[0]->nel);

  int Nvert_total = 0;
  int Nelements_total = 0;
  int index = 0;

  int num_total_wss_elements = 0;
  int num_total_wss_verts = 0;

  //int vert_ID_array_length = 0;
  int Nel = this->master[0]->nel;
  vtkSmartPointer<vtkPoints> points;
  vtkSmartPointer<vtkPoints> wss_points;

  if(this->master[0]->fhead->dim() == 3)
    {
    double ***num;
    double *temp_array[this->nfields];

    qa = iparam("VIS_RES");
    wss_qa = iparam("WSS_RES");
    vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: rank = "<<my_rank<<" QGmax = "<<QGmax<<", qa = "<< qa << ", wss_qa = "<< wss_qa);
    alloc_res = (QGmax > qa) ? QGmax : qa;
    alloc_res = (alloc_res > wss_qa) ? alloc_res : wss_qa;
    vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: rank = "<<my_rank<<" alloc_res = "<<alloc_res<<", alloc^3-1= "<<(alloc_res*alloc_res*alloc_res-1));
    Element* F  = this->master[0]->flist[0];
    vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: rank = "<<my_rank<<" : F->qa = "<< F->qa);

    if(!this->USE_MESH_ONLY)
      {
      for(i=0; i<this->nfields; i++)
        {
        temp_array[i]= dvector(0,alloc_res*alloc_res*alloc_res-1);
        }
      vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: rank = "<<my_rank<<" : DONE allocate temp_array");
      }

    // calculate the number of elements and vertices in the continuum mesh
    for(k = 0,n=i=0; k < Nel; ++k){
    F  = this->master[0]->flist[k];
    //if(Check_range_sub_cyl(F))
      {
      //if(Check_range(F)){
      //qa = F->qa;
      i += qa*(qa+1)*(qa+2)/6;
      n += (qa-1)*(qa-1)*(qa-1);
      }
    }

    Nvert_total = i;
    Nelements_total = n;
    vtkDebugMacro(<<"updateVtuData: rank = "<<my_rank<<" :Nvert_total= "<<Nvert_total<<", Nelements_total= "<<Nelements_total<<", Nel= "<< Nel);

    // Calculate total number of vertices and elements for the WSS mesh
    for(Bndry* B=this->Ubc; B; B = B->next)
      {
      if(B->type == 'W')
        {
        // wss_qa is set to WSS_S
        num_total_wss_elements += (wss_qa-1)*(wss_qa-1);
        num_total_wss_verts += (wss_qa+1)*wss_qa/2;
        }
      }

    int element_vert_cnt = (wss_qa-1)*(wss_qa-1) *3;
    int wss_index[element_vert_cnt];

    // Generate the connectivity for one WSS element
    generateWSSconnectivity(wss_index, wss_qa);

    // declare and allocate memory for needed scalars/vectors
    // this will need to be made into variable length arrays to accommodate
    // arbitrary numbers of variables in the next gen code

    // these should each probably be wrapped in an if statement, checking whether user requested the data

    vtkDebugMacro(<<"updateVtuData:: rank = "<<my_rank<<" : num_total_wss_elements= "<<num_total_wss_elements<<", num_total_wss_verts= "<<num_total_wss_verts);
    vtkDebugMacro(<<"updateVtuData:: rank = "<<my_rank<<" : wss_qa= "<<wss_qa<<", (wss_qa+1)*wss_qa/2= "<<((wss_qa+1)*wss_qa/2));


    vtkSmartPointer<vtkFloatArray> vorticity;
    vtkSmartPointer<vtkFloatArray> lambda_2;
    vtkSmartPointer<vtkFloatArray> wall_shear_stress;

    if(!this->USE_MESH_ONLY)
      {
      if(this->GetDerivedVariableArrayStatus("Vorticity"))
        {
        vorticity = vtkSmartPointer<vtkFloatArray>::New();
        vorticity->SetNumberOfComponents(3);
        vorticity->SetNumberOfTuples(Nvert_total);
        vorticity->SetName("Vorticity");
        }

      if(this->GetDerivedVariableArrayStatus("lambda_2"))
        {
        lambda_2 = vtkSmartPointer<vtkFloatArray>::New();
        lambda_2->SetNumberOfComponents(1);
        lambda_2->SetNumberOfValues(Nvert_total);
        lambda_2->SetName("lambda_2");
        }

      if(this->GetDerivedVariableArrayStatus("Wall Shear Stress"))
        {
        wall_shear_stress = vtkSmartPointer<vtkFloatArray>::New();
        wall_shear_stress->SetNumberOfComponents(3);
        wall_shear_stress->SetNumberOfValues(num_total_wss_verts);
        wall_shear_stress->SetName("Wall Shear Stress");
        }
      } // if(!this->USE_MESH_ONLY)

    // if we need to calculate the geometry (first time, or it has changed)
    if (this->CALC_GEOM_FLAG)
      {
      // first for the full (continuum) mesh

      vtkNew<vtkTimerLog> timer;
      timer->StartTimer();
      if(this->UGrid)
        {
        this->UGrid->Delete();
        }
      this->UGrid = vtkUnstructuredGrid::New();
      this->UGrid->Allocate(Nelements_total);

      points = vtkSmartPointer<vtkPoints>::New();
      points->SetNumberOfPoints(Nvert_total);

      vtkDebugMacro(<<"updateVtuData : rank = "<<my_rank<<": Nelements_total = "<<Nelements_total<<" Nvert_total = "<< Nvert_total);

      /* fill XYZ  arrays */
      index = 0;
      interpolateAndCopyContinuumPoints(alloc_res, qa, points);

      //vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<" number of points added:"<< index);
      timer->StopTimer();
      timer_diff = timer->GetElapsedTime();
      vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": time to copy/convert xyz and uvw: "<< timer_diff);
      } // if (this->CALC_GEOM_FLAG)

    if (this->CALC_WSS_GEOM_FLAG)
      {
      // now for the WSS mesh (if needed)

      //if(this->GetDerivedVariableArrayStatus("Wall Shear Stress"))
        {
        vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": *** GetDerivedVariableArrayStatus(Wall Shear Stress)");

        if(this->WSS_UGrid)
          {
          this->WSS_UGrid->Delete();
          }
        this->WSS_UGrid = vtkUnstructuredGrid::New();

        this->WSS_UGrid->Allocate(num_total_wss_elements);

        wss_points = vtkSmartPointer<vtkPoints>::New();
        wss_points->SetNumberOfPoints(num_total_wss_verts);

        vtkDebugMacro(<<"updateVtuData:: my_rank= " << my_rank<<" : post SetNumberOfPoints("<<num_total_wss_verts<<")");

	interpolateAndCopyWSSPoints(alloc_res, wss_qa, wss_points);
        vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": post interpolateAndCopyWSSPoints()");
	
        } // if(this->GetDerivedVariableArrayStatus("Wall Shear Stress"))
      } // if (this->CALC_WSS_GEOM_FLAG)

    if(!this->USE_MESH_ONLY)
      {
      vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": call interpolateAndCopyContinuumData()");
      interpolateAndCopyContinuumData(pv_ugrid, temp_array, qa, Nvert_total);

      vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": finished with interpolateAndCopyContinuumData (pressure and velocity) , see if we want vorticity and/or lambda_2");

      if(this->GetDerivedVariableArrayStatus("Vorticity") || this->GetDerivedVariableArrayStatus("lambda_2"))
        {
        vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": Yes, we want vorticity and/or lambda_2");
        double *vel_array[4];
        vtkNew<vtkTimerLog> timer;
        timer->StartTimer();
        for (k=0; k < 4; k++)
          {
          vel_array[k]= dvector(0, master[k]->htot*sizeof(double));
          memcpy(vel_array[k], master[k]->base_h, master[k]->htot*sizeof(double));
          }
        timer->StopTimer();
        timer_diff = timer->GetElapsedTime();
        vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": time to backup master: "<< timer_diff);
        vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": call Calc_Vort()");
        timer->StartTimer();
        /// we may be able to do this only once and store the values... look at this later

        if(this->UseProjection)
          option_set("PROJECT", true);
        else
          option_set("PROJECT", false);
        Calc_Vort(&this->fl, this->master, this->nfields, 0);
        timer->StartTimer();
        timer_diff = timer->GetElapsedTime();
        vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": Calc_Vort() complete, this->nfields: "<<this->nfields<<" :: time: "<< timer_diff );
        index = 0;
        for(k = 0; k < Nel; ++k)
          {
          F  = this->master[0]->flist[k];
          //if(Check_range_sub_cyl(F))
            {

            //for(n = 0; n < this->nfields-1; ++n)
            for(n = 0; n < this->nfields; ++n)
              {
              F = this->master[n]->flist[k];
              ntot = Interp_symmpts(F,qa,F->h_3d[0][0],temp_array[n],'p');
              }

            for(i = 0; i < ntot; ++i)
              {
              if(this->GetDerivedVariableArrayStatus("Vorticity"))
                {
                vorticity->SetTuple3(
                  index,
                  temp_array[0][i],
                  temp_array[1][i],
                  temp_array[2][i]);
                }
              if(this->GetDerivedVariableArrayStatus("lambda_2"))
                {
                lambda_2->SetValue(index, temp_array[3][i]);
                }

              index ++;
              } // for(i = 0; i < ntot; ++i)
            }
          }// for(k = 0; k < Nel; ++k)

        if(this->GetDerivedVariableArrayStatus("Vorticity"))
          {
          this->UGrid->GetPointData()->AddArray(vorticity);
          }
        else
          {
          // Does PV already have this array?  If so, remove it.
          if (pv_ugrid->GetPointData()->GetArray("Vorticity") != NULL)
            {
            pv_ugrid->GetPointData()->RemoveArray("Vorticity");
            }
          // Do I already have this array?  If so, remove it.
          if (this->UGrid->GetPointData()->GetArray("Vorticity") != NULL)
            {
            this->UGrid->GetPointData()->RemoveArray("Vorticity");
            }
          }

        if(this->GetDerivedVariableArrayStatus("lambda_2"))
          {
          this->UGrid->GetPointData()->AddArray(lambda_2);
          }
        else  // user does not want this variable
          {
          // Does PV already have this array?  If so, remove it.
          if (pv_ugrid->GetPointData()->GetArray("lambda_2") != NULL)
            {
            pv_ugrid->GetPointData()->RemoveArray("lambda_2");
            }
          // Do I already have this array?  If so, remove it.
          if (this->UGrid->GetPointData()->GetArray("lambda_2") != NULL)
            {
            this->UGrid->GetPointData()->RemoveArray("lambda_2");
            }
          }

        timer->StartTimer();
        for (k=0; k < 4; k++)
          {
          vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": Now memcopy vel_array["<<k<<"] back to master["<<k<<"]");
          memcpy(master[k]->base_h, vel_array[k], master[k]->htot*sizeof(double));
          free(vel_array[k]);
          vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": vel_array["<<k<<"] has been freed");
          }
        timer->StopTimer();
        timer_diff = timer->GetElapsedTime();
        vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": time to restore master: "<<timer_diff);

        } // if(this->GetDerivedVariableArrayStatus("Vorticity")) || this->GetDerivedVariableArrayStatus("lambda_2")
      else  // user does not want either of these variables
        {
        //if(this->GetDerivedVariableArrayStatus("Vorticity"))
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("Vorticity") != NULL)
          {
          pv_ugrid->GetPointData()->RemoveArray("Vorticity");
          }
        // Do I already have this array?  If so, remove it.
        if (this->UGrid->GetPointData()->GetArray("Vorticity") != NULL)
          {
          this->UGrid->GetPointData()->RemoveArray("Vorticity");
          }
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("lambda_2") != NULL)
          {
          pv_ugrid->GetPointData()->RemoveArray("lambda_2");
          }
        // Do I already have this array?  If so, remove it.
        if (this->UGrid->GetPointData()->GetArray("lambda_2") != NULL)
          {
          this->UGrid->GetPointData()->RemoveArray("lambda_2");
          }
        }

      } // if(!this->USE_MESH_ONLY)

    // now see if they want the Wall Shear Stress (WSS)
    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<"Should we calc WSS: "<< this->GetDerivedVariableArrayStatus("Wall Shear Stress"));

    if(this->GetDerivedVariableArrayStatus("Wall Shear Stress"))
      {
      vtkIdType num_wss_cells = this->WSS_UGrid->GetNumberOfCells();
      if(num_wss_cells == 0)
        {
        addCellsToWSSMesh(wss_index, wss_qa);
        this->WSS_UGrid->SetPoints(wss_points);
        }

      if(!this->USE_MESH_ONLY)
        {
        if(this->Ubc == NULL)
          {
          vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": Want to call Calc_WSS(, but (this->Ubc == NULL");
          }
        vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": Lets call Calc_WSS()");
        // if WSS_all_vals is NULL, then we need to allocate it
        if(this->WSS_all_vals == NULL)
          {
          // figure out how many total values there are
          int some_number = get_number_of_vertices_WSS(&this->fl, this->master, this->Ubc, 0);
          // allocate enough memory for them
          this->WSS_all_vals= new double*[3];
          for(int w2=0; w2<3; w2++)
            {
            this->WSS_all_vals[w2] = (double*) malloc(some_number *sizeof(double));
            }
          }
        if(NEED_TO_CALC_WSS)
          {
          Calc_WSS(&this->fl, this->master, this->Ubc, 0, this->WSS_all_vals);
          this->wss_mem_step = this->requested_step;
          vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": Done calling Calc_WSS()");

          }

        vtkDebugMacro(<< "~~~~~updateVtuData: my_rank= " << my_rank<<":  this->WSS_UGrid->GetNumberOfCells() = "<< num_wss_cells);

        interpolateAndCopyWSSData(alloc_res, num_total_wss_verts , wss_qa);
        } // if(!this->USE_MESH_ONLY)

      }// if(this->GetDerivedVariableArrayStatus("Wall Shear Stress"))

    vtkNew<vtkTimerLog> timer;
    timer->StartTimer();
    if (this->CALC_GEOM_FLAG)
      {
      /* continuum numbering array */
      num = dtarray(0,alloc_res-1,0,alloc_res-1,0,alloc_res-1);
      for(cnt = 1, k = 0; k < qa; ++k)
        {
        for(j = 0; j < qa-k; ++j)
          {
          for(i = 0; i < qa-k-j; ++i, ++cnt)
            {
            num[i][j][k] = cnt;
            }
          }
        }
      vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<":: cnt = "<<cnt<<" : Nel = "<<Nel<<" : cnt*Nel = "<<(cnt*Nel)<< " : num= "<< num << " : num[0][0][0]= "<< num[0][0][0]);

      gsync();
      addCellsToContinuumMesh(qa, num);

      vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": index = "<< index);
      this->UGrid->SetPoints(points);
      free_dtarray(num,0,0,0);
      }//end "if (this->CALC_GEOM_FLAG)"

    timer->StopTimer();
    timer_diff = timer->GetElapsedTime();
    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": time of CALC_GEOM (the mesh): "<< timer_diff);

    timer->StartTimer();
    vtkNew<vtkCleanUnstructuredGrid> clean;
    vtkNew<vtkUnstructuredGrid> tmpGrid;
    tmpGrid->ShallowCopy(this->UGrid);
    clean->SetInputData(tmpGrid.GetPointer());
    clean->Update();
    timer->StopTimer();
    timer_diff = timer->GetElapsedTime();
    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": time to clean the grid: "<< timer_diff);

    timer->StartTimer();
    pv_ugrid->ShallowCopy(clean->GetOutput());

    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<":  completed ShallowCopy to pv_ugrid\n");
    if(this->curObj->ugrid)
      {
      this->curObj->ugrid->Delete();
      }
    this->curObj->ugrid = vtkUnstructuredGrid::New();
    this->curObj->ugrid->ShallowCopy(clean->GetOutput());

    if(this->curObj->wss_ugrid)
      {
      vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": Delete this->curObj->wss_ugrid it is not null\n");
      this->curObj->wss_ugrid->Delete();
      }

    this->curObj->wss_ugrid = vtkUnstructuredGrid::New();
    vtkNew<vtkCleanUnstructuredGrid> wss_clean;

    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": created new this->curObj->wss_ugrid, and wss_clean\n");

    if(this->GetDerivedVariableArrayStatus("Wall Shear Stress"))
      {
      vtkNew<vtkUnstructuredGrid> tmpGrid2;
      tmpGrid2->ShallowCopy(this->WSS_UGrid);
      wss_clean->SetInputData(tmpGrid2.GetPointer());
      wss_clean->Update();
      vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": they want WSS, do shallow copy\n");
      // this will need to be updated to be the real wss grid
      this->curObj->wss_ugrid->ShallowCopy(wss_clean->GetOutput());
      pv_wss_ugrid->ShallowCopy(wss_clean->GetOutput());
      }
    else
      {
      vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": they DO NOT want WSS, do nothing\n");
      pv_wss_ugrid->ShallowCopy(this->curObj->wss_ugrid );
      }

    //fprintf(stderr, "updateVtuData:: Rank: %d:  completed ShallowCopy to curObj->ugrid\n");
    timer->StopTimer();
    timer_diff = timer->GetElapsedTime();
    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank<<": time to shallow copy cleaned grid to pv_ugrid: "<<timer_diff);

    this->displayed_step = this->requested_step;
    this->curObj->pressure = this->GetPointArrayStatus("Pressure");
    this->curObj->velocity = this->GetPointArrayStatus("Velocity");
    this->curObj->vorticity = this->GetDerivedVariableArrayStatus("Vorticity");
    this->curObj->lambda_2 = this->GetDerivedVariableArrayStatus("lambda_2");
    this->curObj->wss = this->GetDerivedVariableArrayStatus("Wall Shear Stress");
    for(int kk=0; kk<this->num_extra_vars; kk++)
      {
      this->curObj->extra_vars[kk] = this->GetPointArrayStatus(this->extra_var_names[kk]);
      }
    this->curObj->resolution = this->ElementResolution;
    this->curObj->wss_resolution = this->WSSResolution;
    this->curObj->use_projection = this->UseProjection;

    if(!this->USE_MESH_ONLY)
      {
      for(i=0; i<this->nfields; i++)
        {
        free(temp_array[i]);
        }
      }
    this->CALC_GEOM_FLAG=false;
    }
  else
    {
    fprintf(stderr,"2D case is not implemented");
    }

} // vtkNektarReader::updateVtuData()

void vtkNektarReader::addCellsToContinuumMesh(int qa, double ***num)
{
  int my_rank = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  vtkDebugMacro(<< "addCellsToContinuumMesh(): my_rank= " << my_rank<<"  ENTER");
  vtkIdType pts[4];
  int index = 0;

  vtkDebugMacro(<< "addCellsToContinuumMesh(): my_rank= " << my_rank<< " : num= "<< num << " : num[0][0][0]= "<< num[0][0][0]);
  int n = 0;
  for(int e = 0; e < this->master[0]->nel; ++e)
    {
    Element* F  = this->master[0]->flist[e];
    //if(Check_range_sub_cyl(F))
      {
      //if(Check_range(F)){
      //qa = F->qa;
      /* dump connectivity */
      switch(F->identify())
        {
        case Nek_Tet:
          for(int k=0; k < qa-1; ++k)
            {
            for(int j = 0; j < qa-1-k; ++j)
              {
              for(int i = 0; i < qa-2-k-j; ++i)
                {

                pts[0] = n+(int) num[i][j][k] -1;
                pts[1] = n+(int) num[i+1][j][k] -1;
                pts[2] = n+(int) num[i][j+1][k] -1;
                pts[3] = n+(int) num[i][j][k+1] -1;
                this->UGrid->InsertNextCell(VTK_TETRA, 4, pts);
                index++;

                pts[0] = n+(int) num[i+1][j][k]-1;
                pts[1] = n+(int) num[i][j+1][k]-1;
                pts[2] = n+(int) num[i][j][k+1]-1;
                pts[3] = n+(int) num[i+1][j][k+1]-1;
                this->UGrid->InsertNextCell(VTK_TETRA, 4, pts);
                index++;

                pts[0] = n+(int) num[i+1][j][k+1]-1;
                pts[1] = n+(int) num[i][j][k+1]-1;
                pts[2] = n+(int) num[i][j+1][k+1]-1;
                pts[3] = n+(int) num[i][j+1][k]-1;
                this->UGrid->InsertNextCell(VTK_TETRA, 4, pts);
                index++;

                pts[0] = n+(int) num[i+1][j+1][k]-1;
                pts[1] = n+(int) num[i][j+1][k]-1;
                pts[2] = n+(int) num[i+1][j][k]-1;
                pts[3] = n+(int) num[i+1][j][k+1]-1;
                this->UGrid->InsertNextCell(VTK_TETRA, 4, pts);
                index++;

                pts[0] = n+(int) num[i+1][j+1][k]-1;
                pts[1] = n+(int) num[i][j+1][k]-1;
                pts[2] = n+(int) num[i+1][j][k+1]-1;
                pts[3] = n+(int) num[i][j+1][k+1]-1;
                this->UGrid->InsertNextCell(VTK_TETRA, 4, pts);
                index++;

                if(i < qa-3-k-j)
                  {
                  pts[0] = n+(int) num[i][j+1][k+1]-1;
                  pts[1] = n+(int) num[i+1][j+1][k+1]-1;
                  pts[2] = n+(int) num[i+1][j][k+1]-1;
                  pts[3] = n+(int) num[i+1][j+1][k]-1;
                  this->UGrid->InsertNextCell(VTK_TETRA, 4, pts);
                  index++;
                  }

                }
              pts[0] = n+(int) num[qa-2-k-j][j][k]-1;
              pts[1] = n+(int) num[qa-1-k-j][j][k]-1;
              pts[2] = n+(int) num[qa-2-k-j][j+1][k]-1;
              pts[3] = n+(int) num[qa-2-k-j][j][k+1]-1;
              this->UGrid->InsertNextCell(VTK_TETRA, 4, pts);
              index++;
              }
            }
          n += qa*(qa+1)*(qa+2)/6;
          break;
        default:
          fprintf(stderr,"WriteS is not set up for this element type \n");
          exit(1);
          break;
        } // switch(F->identify())
      }
    } // for(e = 0,n=0; e <   int Nel = this->master[0]->nel; ++e)
  vtkDebugMacro(<< "addCellsToContinuumMesh(): my_rank= " << my_rank<<"  EXIT");
}// addPointsToContinuumMesh()

void vtkNektarReader::generateWSSconnectivity(int * wss_index, int res)
{
  int cnt_local=0, k=0;

  for(int j = 0; j < res-1; ++j)
    {
    for(int i = 0; i < res-2-j; ++i)
      {
      wss_index[k++] = cnt_local + i + 1 - 1;
      wss_index[k++] = cnt_local + i + 2 - 1;
      wss_index[k++] = cnt_local + res - j + i + 1 - 1;
      wss_index[k++] = cnt_local + res - j + i + 2 - 1;
      wss_index[k++] = cnt_local + res - j + i + 1 - 1;
      wss_index[k++] = cnt_local + i + 2 - 1;
      }
    wss_index[k++] = cnt_local + res - 1 - j - 1;
    wss_index[k++] = cnt_local + res - j - 1;
    wss_index[k++] = cnt_local + 2*res - 2*j - 1 - 1;
    cnt_local += res - j;
    }// for(int j = 0; j < qa-1; ++j)
}// vtkNektarReader::generateWSSconnectivity()

void vtkNektarReader::addCellsToWSSMesh(int * wss_index, int wss_qa)
{
  int n=0;
  vtkIdType pts[4];

  for(Bndry* B=this->Ubc; B; B = B->next)
    {
    if(B->type == 'W')
      {

      int vert_index = 0;
      for(int j = 0; j < wss_qa-1; ++j)
        {
        //for(i = 0; i < QGmax-2-j; ++i)
        //for(i = 0; i < alloc_res-2-j; ++i)
        for(int i = 0; i < wss_qa-2-j; ++i)
          {
          //fprintf(out,"%d %d %d\n",cnt+i+1, cnt+i+2,cnt+wss_qa-j+i+1);
          pts[0] = n +  wss_index[vert_index++];
          pts[1] = n +  wss_index[vert_index++];
          pts[2] = n +  wss_index[vert_index++];
          this->WSS_UGrid->InsertNextCell(VTK_TRIANGLE, 3, pts);
          //fprintf(out,"%d %d %d\n",cnt+wss_qa-j+i+2,cnt+wss_qa-j+i+1,cnt+i+2);
          pts[0] = n +  wss_index[vert_index++];
          pts[1] = n +  wss_index[vert_index++];
          pts[2] = n +  wss_index[vert_index++];
          this->WSS_UGrid->InsertNextCell(VTK_TRIANGLE, 3, pts);
          }
        //fprintf(out,"%d %d %d\n",cnt+wss_qa-1-j,cnt+wss_qa-j,cnt+2*wss_qa-2*j-1);

        pts[0] = n +  wss_index[vert_index++];
        pts[1] = n +  wss_index[vert_index++];
        pts[2] = n +  wss_index[vert_index++];
        this->WSS_UGrid->InsertNextCell(VTK_TRIANGLE, 3, pts);
        }// for(cnt = 0,j = 0; j < alloc_res-1; ++j)

      n+= (wss_qa+1)*wss_qa/2;

      } // if(B->type == 'W')
    }// for(B=this->Ubc; B; B = B->next)
}// addCellsToWSSMesh

void vtkNektarReader::interpolateAndCopyContinuumPoints(int alloc_res, int interp_res, vtkPoints* points)
{
  /* fill XYZ  arrays */
  Coord    X;
  int index = 0;

  X.x = dvector(0,alloc_res*alloc_res*alloc_res-1);
  X.y = dvector(0,alloc_res*alloc_res*alloc_res-1);
  X.z = dvector(0,alloc_res*alloc_res*alloc_res-1);

  // for each spectral element in the continuum mesh
  for(int k = 0; k < this->master[0]->nel; ++k)
    {
    Element* F  = this->master[0]->flist[k];

    //if(Check_range_sub_cyl(F))
      {
      //if(Check_range(F)){
      //qa = F->qa;

      // Get the coordinates for this element of the continuum mesh
      F->coord(&X);

      // interpolate to the requested resolution (qa == VIS_RES)
      int ntot = Interp_symmpts(F,interp_res,X.x,X.x,'p');
      ntot = Interp_symmpts(F,interp_res,X.y,X.y,'p');
      ntot = Interp_symmpts(F,interp_res,X.z,X.z,'p');

      // for every point in this spectral element
      for(int i = 0; i < ntot; ++i)
        {
        points->InsertPoint(index, X.x[i], X.y[i], X.z[i]);
        index ++;
        }// for(i = 0; i < ntot; ++i)
      }
    }//for(k = 0; k < this->master[0]->nel; ++k)

  free(X.x); free(X.y); free(X.z);
  int my_rank = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  vtkDebugMacro(<< "interpolateContinuumCoordinates: my_rank= " << my_rank<<" number of points added:"<< index);
}// vtkNektarReader::interpolateContinuumCoordinates()

void vtkNektarReader::interpolateAndCopyContinuumData(vtkUnstructuredGrid* pv_ugrid, double **data_array, int interp_res, int num_verts)
{
  int my_rank = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  vtkDebugMacro(<< "interpolateAndCopyContinuumData: my_rank= " << my_rank<<"  ENTER");

  int index = 0;
  vtkFloatArray* pressure;
  pressure = vtkFloatArray::New();
  pressure->SetNumberOfComponents(1);
  pressure->SetNumberOfValues(num_verts);
  pressure->SetName("Pressure");

  vtkFloatArray** scalars;
  scalars = (vtkFloatArray**) malloc(this->num_extra_vars * sizeof(vtkFloatArray*));
  for(int jj=0; jj<this->num_extra_vars; jj++)
    {
    scalars[jj] = vtkFloatArray::New();
    scalars[jj]->SetNumberOfComponents(1);
    scalars[jj]->SetNumberOfValues(num_verts);
    scalars[jj]->SetName(this->extra_var_names[jj]);
    }

  vtkFloatArray* vectors;
  vectors = vtkFloatArray::New();
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(num_verts);
  vectors->SetName("Velocity");

  // for each spectral element in the continuum mesh
  for(int k = 0; k < this->master[0]->nel; ++k)
    {
    Element* F  = this->master[0]->flist[k];
      {
      // interpolate the field data on the resuested resolution, storing result in data_array
      int ntot = 0;
      for(int n = 0; n < this->nfields; ++n)
        {
        F = this->master[n]->flist[k];
        ntot = Interp_symmpts(F,interp_res,F->h_3d[0][0],data_array[n],'p');
        }

      // for every point in this spectral element
      for(int i = 0; i < ntot; ++i)
        {
        if(this->GetPointArrayStatus("Velocity"))
          {
          vectors->SetTuple3(
            index,
            data_array[0][i],
            data_array[1][i],
            data_array[2][i]);
          }
        if(this->GetPointArrayStatus("Pressure"))
          {
          pressure->SetValue(index, data_array[3][i]);
          }

        for(int kk=0; kk<this->num_extra_vars; kk++)
          {
          // var_num is the index into the data_array, which is assumed to have
          // uvwp (4) + extra vars.
          int var_num = kk+4;
          if(this->GetPointArrayStatus(this->extra_var_names[kk]))
            {
            scalars[kk]->SetValue(index, data_array[var_num][i]);
            }
          }
        index ++;
        }// for(i = 0; i < ntot; ++i)
      }
    }//for(k = 0; k < this->master[0]->nel; ++k)

  if(this->GetPointArrayStatus("Pressure"))
    {
    this->UGrid->GetPointData()->SetScalars(pressure);
    }
  else  // user does not want this variable
    {
    // Does PV already have this array?  If so, remove it.
    if (pv_ugrid->GetPointData()->GetArray("Pressure") != NULL)
      {
      pv_ugrid->GetPointData()->RemoveArray("Pressure");
      }
    // Do I already have this array?  If so, remove it.
    if (this->UGrid->GetPointData()->GetArray("Pressure") != NULL)
      {
      this->UGrid->GetPointData()->RemoveArray("Pressure");
      }
    }
  pressure->Delete();

  for(int kk=0; kk<this->num_extra_vars; kk++)
    {
    if(this->GetPointArrayStatus(this->extra_var_names[kk]))
      {
      this->UGrid->GetPointData()->AddArray(scalars[kk]);
      }
    else  // user does not want this variable
      {
      // Does PV already have this array?  If so, remove it.
      if (pv_ugrid->GetPointData()->GetArray(this->extra_var_names[kk]) != NULL)
        {
        pv_ugrid->GetPointData()->RemoveArray(this->extra_var_names[kk]);
        }
      // Do I already have this array?  If so, remove it.
      if (this->UGrid->GetPointData()->GetArray(this->extra_var_names[kk]) != NULL)
        {
        this->UGrid->GetPointData()->RemoveArray(this->extra_var_names[kk]);
        }
      }
    scalars[kk]->Delete();
    }

  if(this->GetPointArrayStatus("Velocity"))
    {
    this->UGrid->GetPointData()->AddArray(vectors);
    }
  else  // user does not want this variable
    {
    // Does PV already have this array?  If so, remove it.
    if (pv_ugrid->GetPointData()->GetArray("Velocity") != NULL)
      {
      pv_ugrid->GetPointData()->RemoveArray("Velocity");
      }
    // Do I already have this array?  If so, remove it.
    if (this->UGrid->GetPointData()->GetArray("Velocity") != NULL)
      {
      this->UGrid->GetPointData()->RemoveArray("Velocity");
      }
    }

  vectors->Delete();

  vtkDebugMacro(<< "interpolateAndCopyContinuumData: my_rank= " << my_rank<<"  EXIT");

}// vtkNektarReader::interpolateAndCopyContinuumData()

void vtkNektarReader::interpolateAndCopyWSSPoints(int alloc_res, int interp_res, vtkPoints* wss_points)
{
  int my_rank = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  vtkDebugMacro(<< "interpolateAndCopyWSSPoints: my_rank= " << my_rank<<": ENTER: alloc_res= "<< alloc_res<< " interp_res: " << interp_res);
  int index=0;
  double *wk = dvector(0,alloc_res*alloc_res);
  Coord    wss_X;

  wss_X.x = dvector(0,alloc_res*alloc_res*alloc_res-1);
  wss_X.y = dvector(0,alloc_res*alloc_res*alloc_res-1);
  wss_X.z = dvector(0,alloc_res*alloc_res*alloc_res-1);

  Tri tri;
  vtkDebugMacro(<< "vtkNektarReader::vtkNektarReader():  tri->Nverts= "<< tri.Nverts);
  vtkDebugMacro(<< "vtkNektarReader::vtkNektarReader():  tri->dim()= "<< tri.dim());

  for(Bndry* B=this->Ubc; B; B = B->next)
    {
    // if it is a wall element
    if(B->type == 'W')
      {
      Element* F  = B->elmt;
      if(F->Nfverts(B->face) == 3)
        {
        tri.qa = F->qa;
        tri.qb = F->qc; /* fix for prisms */
        }
      else
        {
        tri.qa = F->qa;
        tri.qb = F->qb;
        }

vtkDebugMacro(<<  "interpolateAndCopyWSSPoints: my_rank= " << my_rank<<": Wall eleme: (B->face)==3: post assign tri.qb");
      // get the coordinates for this element
      F->coord(&wss_X);

      F->GetFace(wss_X.x,B->face,wk);
      F->InterpToFace1(B->face,wk,wss_X.x);
      Interp_symmpts(&tri,interp_res,wss_X.x,wss_X.x,'n'); //wk has coordinates of "x"
      F->GetFace(wss_X.y,B->face,wk);
      F->InterpToFace1(B->face,wk,wss_X.y);
      Interp_symmpts(&tri,interp_res,wss_X.y,wss_X.y,'n'); //wk has coordinates of "y"
      F->GetFace(wss_X.z,B->face,wk);
      F->InterpToFace1(B->face,wk,wss_X.z);
      int ntot = Interp_symmpts(&tri,interp_res,wss_X.z,wss_X.z,'n'); //wk has coordinates of "z"

      // put all of the points for this wss element into the list of wss points
      for(int i = 0; i < ntot; ++i)
        {
        wss_points->InsertPoint(index, wss_X.x[i], wss_X.y[i], wss_X.z[i]);
        index ++;
        }
      }// if(B->type == 'W')
    }// for(B=this->Ubc; B; B = B->next)
  
  vtkDebugMacro(<< "interpolateAndCopyWSSPoints: my_rank= " << my_rank<<": num added= "<< index);


  free(wss_X.x); 
  free(wss_X.y); 
  free(wss_X.z);
  free(wk);


  vtkDebugMacro(<< "interpolateAndCopyWSSPoints: my_rank= " << my_rank<<": EXIT ");

} // interpolateAndCopyWSSPoints()

void vtkNektarReader::interpolateAndCopyWSSData(int alloc_res, int num_verts, int interp_res)
{
  int my_rank = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  vtkDebugMacro(<< "interpolateAndCopyWSSData(): my_rank= " << my_rank<<"  ENTER");

  int ntot =3;
  int index=0;

  vtkDebugMacro(<< "interpolateAndCopyWSSData(): my_rank= " << my_rank<<" :: num_vert= "<< num_verts<<" : interp_res= " <<interp_res << "  : (interp_res+1)*interp_res/2= " << ((interp_res+1)*interp_res/2));

  double * wss_valX = dvector(0, alloc_res*alloc_res*alloc_res-1);
  double * wss_valY = dvector(0, alloc_res*alloc_res*alloc_res-1);
  double * wss_valZ = dvector(0, alloc_res*alloc_res*alloc_res-1);

  int wss_val_offset =0;

  vtkFloatArray* wall_shear_stress;
  Tri tri;
 
  if(this->GetDerivedVariableArrayStatus("Wall Shear Stress"))
    {
    wall_shear_stress = vtkFloatArray::New();
    wall_shear_stress->SetNumberOfComponents(3);
    wall_shear_stress->SetNumberOfTuples(num_verts);
    wall_shear_stress->SetName("Wall Shear Stress");

    // for each element in the boundary
    for(Bndry* B=this->Ubc; B; B = B->next)
      {
      // if it is a wall element
      if(B->type == 'W')
        {
        Element* F  = B->elmt;
        //fprintf(stderr, "F->Nfverts(B->face): %d\n", F->Nfverts(B->face));

        if(F->Nfverts(B->face) == 3)
          {
          tri.qa = F->qa;
          tri.qb = F->qc; /* fix for prisms */
          }
        else
          {
          tri.qa = F->qa;
          tri.qb = F->qb;
          }

        ntot = Interp_symmpts(&tri,interp_res,this->WSS_all_vals[0]+wss_val_offset, wss_valX, 'n');
        ntot = Interp_symmpts(&tri,interp_res,this->WSS_all_vals[1]+wss_val_offset, wss_valY, 'n');
        ntot = Interp_symmpts(&tri,interp_res,this->WSS_all_vals[2]+wss_val_offset, wss_valZ, 'n');

        // put all of the points for this wss element into the list of wss points
        for(int i = 0; i < ntot; ++i)
          {
          //if(this->GetDerivedVariableArrayStatus("Vorticity"))
            {
            wall_shear_stress->SetTuple3(index, -wss_valX[i], -wss_valY[i], -wss_valZ[i]);
            }

            index ++;
          }

        wss_val_offset += tri.qa*tri.qb;
        }// if(B->type == 'W')

      }// for(B=this->Ubc; B; B = B->next)

    if(this->WSS_UGrid)
      {
      this->WSS_UGrid->GetPointData()->AddArray(wall_shear_stress);
      }
    else
      {
      vtkDebugMacro(<< "interpolateAndCopyWSSData(): my_rank= " << my_rank<<" wss_ugrid is NULL");
      }

    wall_shear_stress->Delete();

    vtkDebugMacro(<< "~~~~~~~~~~~~~~~  interpolateAndCopyWSSData: my_rank= " << my_rank<<": num_verts= "<<num_verts<<"  index= "<<index);
    vtkDebugMacro(<< "~~~~~~~~~~~~~~~  interpolateAndCopyWSSData: my_rank= " << my_rank<<": ntot= "<<ntot<<"  (interp_res+1)*interp_res/2= "<<((interp_res+1)*interp_res/2));
    }// if(this->GetDerivedVariableArrayStatus("Wall Shear Stress"))

  free(wss_valX);
  free(wss_valY);
  free(wss_valZ);
  vtkDebugMacro(<< "interpolateAndCopyWSSData(): my_rank= " << my_rank<<"  EXIT");

} // interpolateAndCopyWSSData()
