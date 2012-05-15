/*=========================================================================



=========================================================================*/
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
#include "vtkPointData.h"

#include <mpi.h>

#include "vtksys/SystemTools.hxx"

vtkStandardNewMacro(vtkNektarReader);


int  setup (FileList *f, Element_List **U, int *nftot, int Nsnapshots);
void ReadCopyField (FileList *f, Element_List **U);
void ReadAppendField (FileList *f, Element_List **U,  int start_field_index);
void Calc_Vort (FileList *f, Element_List **U, int nfields, int Snapshot_index);



//----------------------------------------------------------------------------

int vtkNektarReader::next_patch_id = 0;

vtkNektarReader::vtkNektarReader()
{
  this->DebugOn();

  vtkDebugMacro(<<"vtkNektarReader::vtkNektarReader(): ENTER");

  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->FileName = 0;
  this->DataFileName = 0;
  this->ElementResolution = 1;
  this->my_patch_id = vtkNektarReader::getNextPatchID();
  vtkDebugMacro(<< "vtkNektarReader::vtkNektarReader(): my_patch_id = " << this->my_patch_id);

  this->points = NULL;
  this->ugrid = NULL;

  this->READ_GEOM_FLAG = true;
  this->CALC_GEOM_FLAG = true;
  this->IAM_INITIALLIZED = false;
  this->I_HAVE_DATA = false;
  this->FIRST_DATA = true;

  this->TimeStep = 0;
  this->ActualTimeStep = 0;
  this->TimeSteps = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->NumberOfTimeSteps = 0;
  this->p_time_start = 0.0;
  this->p_time_inc = 0.0;
  this->displayed_step = -1;
  this->memory_step = -1;
  this->requested_step = -1;


  MPI_Comm_rank(MPI_COMM_WORLD, &this->my_rank);
  vtkDebugMacro(<< "vtkNektarReader::vtkNektarReader(): my_rank= " << this->my_rank);


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
  this->DisableAllDerivedVariableArrays();

  this->master = (Element_List **) malloc((2)*this->nfields*sizeof(Element_List *));

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

  if (this->TimeSteps)
  {
    delete [] this->TimeSteps;
    this->TimeSteps = 0;
    this->NumberOfTimeSteps = 0;
  }

  if(this->points)
  {
    vtkDebugMacro(<<"vtkNektarReader::~vtkNektarReader(): points not null, delete it");
    this->points->Delete();
  }
  if(this->ugrid)
  {
    vtkDebugMacro(<<"vtkNektarReader::~vtkNektarReader(): ugrid not null, delete it");
    this->ugrid->Delete();
  }


  vtkDebugMacro(<<"vtkNektarReader::~vtkNektarReader(): EXIT");
  if (myList)
  {
      myList->~nektarList();
  }
}

//----------------------------------------------------------------------------

void vtkNektarReader::setActive()
{
    //fprintf(stderr, "vtkNektarReader::setActive: %d ENTER\n", this->my_patch_id);
    iparam_set("IDpatch", this->my_patch_id);
    //fprintf(stderr, "vtkNektarReader::setActive: %d EXIT\n", this->my_patch_id);

}

//----------------------------------------------------------------------------


void vtkNektarReader::GetAllTimes(vtkInformationVector *outputVector)
{
    FILE* dfPtr = NULL;
    char dfName[265];
    char* scan_ret;
    char* p;
    char* p2;
    char param[32];
    char paramLine[256];
    int file_index;
    float test_time_val;

    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    this->TimeStepRange[0] = 0;
    this->TimeStepRange[1] = this->NumberOfTimeSteps-1;

    vtkDebugMacro(<< "vtkNektarReader::GetAllTimes: this->NumberOfTimeSteps = "<<
      this->NumberOfTimeSteps);


    if (this->TimeSteps)
    {
    delete [] this->TimeSteps;
    }
    this->TimeSteps = new double[this->NumberOfTimeSteps];


    if(this->p_time_inc == 0.0)
    {
  for (int i=0; i<(this->NumberOfTimeSteps); i++)
  {
      file_index = this->p_rst_start + (this->p_rst_inc*i);
      sprintf(dfName, this->p_rst_format, file_index );
      dfPtr = fopen(dfName, "r");
      if(!dfPtr) vtkErrorMacro(<< "Failed to open file: "<< dfName);
      // skip the first 5 lines
      for(int j=0; j<5; j++)
    scan_ret = fgets(paramLine, 256, dfPtr);

      scan_ret = fgets(paramLine, 256, dfPtr);

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

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 this->TimeSteps,
                 this->NumberOfTimeSteps);
    double timeRange[2];
    timeRange[0] = this->TimeSteps[0];
    timeRange[1] = this->TimeSteps[this->NumberOfTimeSteps-1];

    vtkDebugMacro(<< "vtkNektarReader::GetAllTimes: timeRange[0] = "<<timeRange[0]<< ", timeRange[1] = "<< timeRange[1]);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 timeRange, 2);
}

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
      ElementResolution = val;
      //iparam_set("VIS_RES", ElementResolution);

      // *** is this still always true?  I'm thinking not, but it
      //     may not matter....
      this->CALC_GEOM_FLAG = true;
      this->Modified();
      vtkDebugMacro(<<"vtkNektarReader::SetElementResolution: CALC_GEOM_FLAG now true (need to calculate) , ElementResolution= "<< ElementResolution);
  }
}
//----------------------------------------------------------------------------

int vtkNektarReader::GetElementResolution()
{
  return(ElementResolution);
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


//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkNektarReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkNektarReader::GetOutput(int port)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkNektarReader::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkNektarReader::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkNektarReader::GetUnstructuredGridInput(int port)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
int vtkNektarReader::ProcessRequest(vtkInformation* request,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkNektarReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkNektarReader::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkNektarReader::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
   vtkDebugMacro(<<"vtkNektarReader::RequestInformation(): ENTER");

  int numArrays;
  int nprocs;
  int mytid;
  FILE* inPtr = NULL;
  char* scan_ret;
  char* p;
  char* p2;
  char param[32];
  char paramLine[256];
  double timer_start;
  double timer_stop;
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

  while(p != NULL)
  {
      *p = '\0';
      sscanf(paramLine, "%s", param);
      p = p+2;

      p2=strchr(p, '\n');
       *p2= '\0';

      if(strcasecmp(param, "REA_FILE") == 0)
      {
    while (*p == ' ')
        p++;
          strcpy(this->p_rea_file, p);
      }
      else if(strcasecmp(param, "RST_DIR") == 0)
      {
    while (*p == ' ')
        p++;
          strcpy(this->p_rst_dir, p);
      }
      else if(strcasecmp(param, "RST_BASE") == 0)
      {
    while (*p == ' ')
        p++;
    strcpy(this->p_rst_base, p);
      }
      else if(strcasecmp(param, "RST_EXT") == 0)
      {
    while (*p == ' ')
        p++;
    strcpy(this->p_rst_ext, p);
      }
      else if(strcasecmp(param, "RST_START") == 0)
          sscanf(p, "%d", &this->p_rst_start);
      else if(strcasecmp(param, "RST_INC") == 0)
          sscanf(p, "%d", &this->p_rst_inc);
      else if(strcasecmp(param, "RST_NUM") == 0)
          sscanf(p, "%d", &this->p_rst_num);
      else if(strcasecmp(param, "RST_DIGITS") == 0)
          sscanf(p, "%d", &this->p_rst_digits);
      else if(strcasecmp(param, "TIME_START") == 0)
          sscanf(p, "%f", &this->p_time_start);
      else if(strcasecmp(param, "TIME_INC") == 0)
          sscanf(p, "%f", &this->p_time_inc);

      scan_ret = fgets(paramLine, 256, inPtr);
      if(scan_ret)
      {
          p=strchr(paramLine, ':');
      }
      else
      {
                p= NULL;
      }
  } // while(p!=NULL)

  scan_ret = fgets(paramLine, 256, inPtr);
      }// while(ret != NULL)

      //this->p_rst_current = this->p_rst_start;

      vtkDebugMacro(<<"REA_FILE:   \'"<<this->p_rea_file<<"\'");
      vtkDebugMacro(<<"RST_DIR:    \'"<<this->p_rst_dir<<"\'");
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

      this->NumberOfTimeSteps = this->p_rst_num;

      fclose(inPtr);

      timer_start = MPI_Wtime();
      this->GetAllTimes(outputVector);
      timer_stop = MPI_Wtime();
      timer_diff = timer_stop - timer_start;

      vtkDebugMacro(<<"Rank: "<<this->my_rank<<" :: GetAllTimeSteps time: "<< timer_diff);



//      sprintf(this->fl.rea.name,"%s", this->reaFile);
//      this->SetDataFileName(this->reaFile);
      sprintf(this->fl.rea.name,"%s", this->p_rea_file);
      this->SetDataFileName(this->p_rea_file);

      this->fl.rea.fp = fopen(this->fl.rea.name,"r");
      if (!this->fl.rea.fp) error_msg(Restart: no REA file read);


      sprintf(this->p_rst_file, this->p_rst_format, this->p_rst_start);
      sprintf(this->fl.in.name,"%s", this->p_rst_file);
      this->fl.in.fp = fopen(this->fl.in.name,"r");
      if (!this->fl.in.fp) error_msg(Restart: no dumps read from restart file);

      vtkInformation *outInfo =
    outputVector->GetInformationObject(0);

      outInfo->Set(vtkStreamingDemandDrivenPipeline::
       MAXIMUM_NUMBER_OF_PIECES(), -1);

      this->IAM_INITIALLIZED = true;
  }// if(!this->IAM_INITIALLIZED)


  vtkDebugMacro(<<"vtkNektarReader::RequestInformation(): EXIT");

  return 1;
}

//----------------------------------------------------------------------------
int vtkNektarReader::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkDebugMacro(<<"vtkNektarReader::RequestUpdateExtent(): ENTER");
  int numInputPorts = this->GetNumberOfInputPorts();
  for (int i=0; i<numInputPorts; i++)
    {
    int numInputConnections = this->GetNumberOfInputConnections(i);
    for (int j=0; j<numInputConnections; j++)
      {
      vtkInformation* inputInfo = inputVector[i]->GetInformationObject(j);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
      }
    }
  vtkDebugMacro(<<"vtkNektarReader::RequestUpdateExtent(): EXIT");
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
  double timer_start;
  double timer_stop;
  double timer_diff;

  double total_timer_start;
  double total_timer_stop;
  double total_timer_diff;

  total_timer_start = MPI_Wtime();
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
    outputVector->GetInformationObject(outputPort);

  vtkDebugMacro(<<"RequestData: enter: this->TimeStep = "<< this->TimeStep);

  this->ActualTimeStep = this->TimeStep;
  int tsLength =
    outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  vtkDebugMacro(<<"RequestData: tsLength= "<< tsLength);

  // Check if a particular time was requested.
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // Get the requested time step. We only supprt requests of a single time
    // step in this reader right now
    double requestedTimeStep =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    this->TimeValue = requestedTimeStep;

    vtkDebugMacro(<<"RequestData: this->TimeValue= "<< this->TimeValue);

    //find the timestep with the closest value to the requested time
    //value
    int cnt = 0;
    int closestStep=0;
    double minDist=-1;
    for (cnt=0;cnt<tsLength;cnt++)
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


  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Save the time value in the output (ugrid) data information.
  if (steps)
  {
    ugrid->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),
          steps[this->ActualTimeStep]);
  }

//  int new_rst_val = this->p_rst_start + (this->p_rst_inc* this->ActualTimeStep);
  this->requested_step = this->p_rst_start + (this->p_rst_inc* this->ActualTimeStep);

//  if(this->p_rst_current != new_rst_val)
  if(this->displayed_step != this->requested_step)
  {
      //this->p_rst_current = new_rst_val;

      // get the requested object from the list, if the ugrid in the object is NULL
      // then we have not loaded it yet
      this->curObj = this->myList->getObject(this->requested_step);

      // need to compare the request with the curObj
      // if the resolutions are not the same, or any variable in the request is true,
      // but false in the object, then we will need the data to be in memory
      if( (this->curObj->resolution != this->ElementResolution) ||
    (this->GetPointArrayStatus("Pressure") && !this->curObj->pressure) ||
    (this->GetPointArrayStatus("Velocity") && !this->curObj->velocity) ||
    (this->GetDerivedVariableArrayStatus("Vorticity") && !this->curObj->vorticity) ||
    (this->GetDerivedVariableArrayStatus("lambda_2") && !this->curObj->lambda_2) )
      {
    // if the step in memory is different than the step requested
    if(this->requested_step != this->memory_step)
    {
        this->I_HAVE_DATA = false;
    }

    // if the resolutions are not the same, also need to calculate the geometry
    if(this->curObj->resolution != this->ElementResolution)
    {
        this->CALC_GEOM_FLAG = true;
    }
      }

      //this->I_HAVE_DATA = false;
  }

   vtkDebugMacro(<< "vtkNektarReader::RequestData:  ElementResolution: " << ElementResolution );

  // if I have not yet read the geometry, this should only happen once
  if(this->READ_GEOM_FLAG)
  {

    vtkDebugMacro( << "vtkNektarReader::RequestData: READ_GEOM_FLAG==true, call manager_init()" );

    manager_init();       /* initialize the symbol table manager */
    //vtkDebugMacro(<< "vtkNektarReader::RequestData: manager_init complete" );
    option_set("FEstorage",1);
    option_set("iterative",1);
    iparam_set("NORDER.REQ", UNSET);
    iparam_set("VIS_RES", ElementResolution);


    //read mesh file and header of the  data file
    this->setActive();
    timer_start = MPI_Wtime();
    this->nfields = setup (&this->fl, this->master, &this->nfields, 1);

    timer_stop = MPI_Wtime();
    timer_diff = timer_stop - timer_start;

    //fprintf(stdout, "vtkNektarReader::RequestData:: Rank: %d ::Read Mesh  time: %f\n", this->my_rank, timer_diff);
    //fprintf(stderr, "vtkNektarReader::RequestData: JUST GOT PAST setup(), this->nfields = %d\n", this->nfields );

    this->READ_GEOM_FLAG = false;
  } // if(this->READ_GEOM_FLAG)

  // if we need to read the data from disk..
  if(!this->I_HAVE_DATA)
  {

      //sprintf(this->p_rst_file, this->p_rst_format, this->p_rst_current);
    sprintf(this->p_rst_file, this->p_rst_format, this->requested_step);
    sprintf(this->fl.in.name,"%s", this->p_rst_file);
    this->fl.in.fp = fopen(this->fl.in.name,"r");
    if (!this->fl.in.fp) error_msg(Restart: no dumps read from restart file);


    /* read the field data */
    if(FIRST_DATA)
    {
      vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<< this->my_rank<<" Now reading data from file: "<< this->p_rst_file);
      this->setActive();
      timer_start = MPI_Wtime();
      ReadCopyField(&this->fl,this->master);
      timer_stop = MPI_Wtime();
      timer_diff = timer_stop - timer_start;

      vtkDebugMacro(<<"vtkNektarReader::RequestData:: Rank: "<<this->my_rank<<" ::Done reading data from file: "<< this->p_rst_file<<":: Read  time: "<< timer_diff);
      FIRST_DATA = false;
    }
    else
    {
      this->setActive();
      timer_start = MPI_Wtime();
      ReadAppendField(&this->fl,this->master, 0);
      timer_stop = MPI_Wtime();
      timer_diff = timer_stop - timer_start;
      vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<<this->my_rank<<" ::Done reading (Append) data from file: "<< this->p_rst_file<<":: Read  time: "<< timer_diff);
    }


    /* transform field into physical space */
    timer_start = MPI_Wtime();
    for(int i = 0; i < this->nfields; ++i)
    {
       this->master[i]->Trans(this->master[i],J_to_Q);
    }
    timer_stop = MPI_Wtime();
    timer_diff = timer_stop - timer_start;
    vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<<this->my_rank<<" :: Transform field into physical space time: "<< timer_diff);
    this->I_HAVE_DATA = true;
    this->memory_step = this->requested_step;

    iparam_set("VIS_RES", ElementResolution);

  } // if(!this->I_HAVE_DATA)
  else
  {
      iparam_set("VIS_RES", ElementResolution);
  }

  timer_start = MPI_Wtime();
  this->updateVtuData(ugrid);
  timer_stop = MPI_Wtime();
  timer_diff = timer_stop - timer_start;
  vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<<this->my_rank<<" :: updateVtuData time: "<<  timer_diff);

  total_timer_stop = MPI_Wtime();
  total_timer_diff = total_timer_stop - total_timer_start;

  vtkDebugMacro(<<"vtkNektarReader::RequestData: Rank: "<<this->my_rank<<" :: Total time: "<< total_timer_diff);
  return 1;
}

//----------------------------------------------------------------------------
// Assume that any source that implements ExecuteData
// can handle an empty extent.
void vtkNektarReader::ExecuteData(vtkDataObject *output)
{
  // I want to find out if the requested extent is empty.
  if (output && this->UpdateExtentIsEmpty(output))
    {
    output->Initialize();
    return;
    }

  this->Execute();
}

//----------------------------------------------------------------------------
void vtkNektarReader::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass and you should really use the ExecuteData(vtkInformation *request,...) signature instead");
}

//----------------------------------------------------------------------------
void vtkNektarReader::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkNektarReader::SetInput(int index, vtkDataObject* input)
{
  if(input)
  {
    this->SetInputConnection(index, input->GetProducerPort());
  }
  else
  {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(index, 0);
  }
}

//----------------------------------------------------------------------------
void vtkNektarReader::AddInput(vtkDataObject* input)
{
  this->AddInput(0, input);
}

//----------------------------------------------------------------------------
void vtkNektarReader::AddInput(int index, vtkDataObject* input)
{
  if(input)
  {
    this->AddInputConnection(index, input->GetProducerPort());
  }
}

void vtkNektarReader::updateVtuData(vtkUnstructuredGrid* pv_ugrid)
{
  register int i,j,k,n,e,nelmts;
  int      qa,cnt, *interior;
  int      alloc_res;
  const int    nel = this->master[0]->nel;
  int      dim = this->master[0]->fhead->dim(),ntot;
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;
  //int My_rank = 0;
  double timer_start;
  double timer_stop;
  double timer_diff;

//  int FLAG_GEOM = 1;

//#ifdef PARALLEL
#if 1
//  My_rank = mynode();
#endif


  // if the grid in the curObj is not NULL, we may have everything we need
  if(this->curObj->ugrid)
  {
      // if the curObj matches the request, just shallow copy, and we're done
      if( (curObj->resolution == this->ElementResolution) &&
    (this->GetPointArrayStatus("Pressure") == this->curObj->pressure) &&
    (this->GetPointArrayStatus("Velocity") == this->curObj->velocity) &&
    (this->GetDerivedVariableArrayStatus("Vorticity")  == this->curObj->vorticity) &&
    (this->GetDerivedVariableArrayStatus("lambda_2") == this->curObj->lambda_2) )
      {
    pv_ugrid->ShallowCopy(this->curObj->ugrid);
    this->displayed_step = this->requested_step;
    return;
      }
      // else if the request is for less than what is in the curObj,
      // remove unwanted, update the curObj, shallow copy and we're done.
      else if( (this->curObj->resolution == this->ElementResolution) &&
         (!this->GetPointArrayStatus("Pressure") ||
    (this->GetPointArrayStatus("Pressure") && this->curObj->pressure)) &&
         (!this->GetPointArrayStatus("Velocity") ||
    (this->GetPointArrayStatus("Velocity") && this->curObj->velocity)) &&
         (!this->GetDerivedVariableArrayStatus("Vorticity") ||
    (this->GetDerivedVariableArrayStatus("Vorticity") && this->curObj->lambda_2)) &&
         (!this->GetDerivedVariableArrayStatus("lambda_2") ||
    (this->GetDerivedVariableArrayStatus("lambda_2") && this->curObj->lambda_2)) )
      {
    // Remove pressure if curObj has it, but not in request
    if(!this->GetPointArrayStatus("Pressure") && this->curObj->pressure)
    {
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("Pressure") != NULL)
      pv_ugrid->GetPointData()->RemoveArray("Pressure");
        // Do I already have this array?  If so, remove it.
        if (this->curObj->ugrid->GetPointData()->GetArray("Pressure") != NULL)
      this->curObj->ugrid->GetPointData()->RemoveArray("Pressure");
        this->curObj->pressure = false;
    }
          // Remove velocity if curObj has it, but not in request
    if(!this->GetPointArrayStatus("Velocity") && this->curObj->velocity)
    {
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("Velocity") != NULL)
      pv_ugrid->GetPointData()->RemoveArray("Velocity");
        // Do I already have this array?  If so, remove it.
        if (this->curObj->ugrid->GetPointData()->GetArray("Velocity") != NULL)
      this->curObj->ugrid->GetPointData()->RemoveArray("Velocity");
        this->curObj->velocity = false;
    }
          // Remove vorticity if curObj has it, but not in request
    if(!this->GetDerivedVariableArrayStatus("Vorticity") && this->curObj->vorticity)
    {
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("Vorticity") != NULL)
      pv_ugrid->GetPointData()->RemoveArray("Vorticity");
        // Do I already have this array?  If so, remove it.
        if (this->curObj->ugrid->GetPointData()->GetArray("Vorticity") != NULL)
      this->curObj->ugrid->GetPointData()->RemoveArray("Vorticity");
        this->curObj->vorticity = false;
    }
    // Remove lambda_2 if curObj has it, but not in request
    if(!this->GetDerivedVariableArrayStatus("lambda_2") && this->curObj->lambda_2)
    {
        // Does PV already have this array?  If so, remove it.
        if (pv_ugrid->GetPointData()->GetArray("lambda_2") != NULL)
      pv_ugrid->GetPointData()->RemoveArray("lambda_2");
        // Do I already have this array?  If so, remove it.
        if (this->curObj->ugrid->GetPointData()->GetArray("lambda_2") != NULL)
      this->curObj->ugrid->GetPointData()->RemoveArray("lambda_2");
        this->curObj->lambda_2 = false;
    }

    pv_ugrid->ShallowCopy(this->curObj->ugrid);
    this->displayed_step = this->requested_step;
    return;
      }
  }

  // otherwise the grid in the curObj is NULL, and/or the resolution has changed,
  // and/or we need more data than is in curObj, we need to do everything

  vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: rank = "<<this->my_rank<<": this->master[0]->nel = "<< this->master[0]->nel);

  interior = ivector(0,this->master[0]->nel-1);


  int Nvert_total = 0;
  int Nelements_total = 0;
  int index = 0;
  //int vert_ID_array_length = 0;
  int Nel = this->master[0]->nel;

  if(this->master[0]->fhead->dim() == 3)
  {
    double ***num;
    double *temp_array[this->nfields];

    qa = iparam("VIS_RES");
    vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: rank = "<<this->my_rank<<" QGmax = "<<QGmax<<", qa = "<< qa);
    alloc_res = (QGmax > qa) ? QGmax : qa;
    vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: rank = "<<this->my_rank<<" alloc_res = "<<alloc_res<<", alloc^3-1= "<<(alloc_res*alloc_res*alloc_res-1));
    F  = this->master[0]->flist[0];
    vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: rank = "<<this->my_rank<<" : F->qa = "<< F->qa);


    for(i=0; i<this->nfields; i++)
    {
  temp_array[i]= dvector(0,alloc_res*alloc_res*alloc_res-1);
    }
    vtkDebugMacro(<<"vtkNektarReader::updateVtuData:: rank = "<<this->my_rank<<" : DONE allocate temp_array");



    X.x = dvector(0,alloc_res*alloc_res*alloc_res-1);
    X.y = dvector(0,alloc_res*alloc_res*alloc_res-1);
    X.z = dvector(0,alloc_res*alloc_res*alloc_res-1);

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
    vtkDebugMacro(<<"updateVtuData: Nvert_total= "<<Nvert_total<<", Nelements_total= "<<Nelements_total<<", Nel= "<< Nel);


    vtkFloatArray* scalars;
    scalars = vtkFloatArray::New();
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfValues(Nvert_total);
    scalars->SetName("Pressure");

    vtkFloatArray* vectors;
    vectors = vtkFloatArray::New();
    vectors->SetNumberOfComponents(3);
    vectors->SetNumberOfTuples(Nvert_total);
    vectors->SetName("Velocity");

    vtkFloatArray* vorticity;
    if(this->GetDerivedVariableArrayStatus("Vorticity"))
    {
        vorticity = vtkFloatArray::New();
  vorticity->SetNumberOfComponents(3);
  vorticity->SetNumberOfTuples(Nvert_total);
  vorticity->SetName("Vorticity");
    }

    vtkFloatArray* lambda_2;
    if(this->GetDerivedVariableArrayStatus("lambda_2"))
    {
  lambda_2 = vtkFloatArray::New();
  lambda_2->SetNumberOfComponents(1);
  lambda_2->SetNumberOfValues(Nvert_total);
  lambda_2->SetName("lambda_2");
    }

    if (this->CALC_GEOM_FLAG)
    {
      timer_start = MPI_Wtime();

      if(this->ugrid)
        this->ugrid->Delete();

      this->ugrid = vtkUnstructuredGrid::New();

      this->ugrid->Allocate(Nelements_total);

      if(this->points)
        this->points->Delete();

      this->points = vtkPoints::New();
      this->points->SetNumberOfPoints(Nvert_total);

      vtkDebugMacro(<<"updateVtuData : rank = "<<this->my_rank<<": Nelements_total = "<<Nelements_total<<" Nvert_total = "<< Nvert_total);

      //XYZ = new float[Nvert_total*3];

      //fprintf(stderr, "rank = %d: k loop, Nel= %d\n", this->my_rank, Nel);

      /* fill XYZ and UVWP arrays */
      index = 0;
      for(k = 0; k < Nel; ++k)
      {
//    fprintf(stderr, "k= %d, Enter loop\n", k);
        F  = this->master[0]->flist[k];
//  fprintf(stderr, "rank = %d:  k= %d, got F= master\n", this->my_rank, k);
        //if(Check_range_sub_cyl(F))
  {
          //if(Check_range(F)){
          //qa = F->qa;
          F->coord(&X);
          ntot = Interp_symmpts(F,qa,X.x,X.x,'p');
          ntot = Interp_symmpts(F,qa,X.y,X.y,'p');
          ntot = Interp_symmpts(F,qa,X.z,X.z,'p');

    int loopMax=0;
    int loop;

          for(n = 0; n < this->nfields; ++n)
    {
            F = this->master[n]->flist[k];

      loopMax = Interp_symmpts(F,qa,F->h_3d[0][0],temp_array[n],'p');
          }

          for(i = 0; i < ntot; ++i)
    {

      this->points->InsertPoint(index, X.x[i], X.y[i], X.z[i]);

      if(this->GetPointArrayStatus("Velocity"))
      {
          vectors->SetTuple3(
        index,
        temp_array[0][i],
        temp_array[1][i],
        temp_array[2][i]);
      }
      if(this->GetPointArrayStatus("Pressure"))
      {
          scalars->SetValue(index, temp_array[3][i]);
      }
            index ++;
          }// for(i = 0; i < ntot; ++i)
        }
      }//for(k = 0; k < Nel; ++k)
      vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<" number of points added:"<< index);
      timer_stop = MPI_Wtime();
      timer_diff = timer_stop - timer_start;
      vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": time to copy/convert xyz and uvw: "<< timer_diff);

    } // if (this->CALC_GEOM_FLAG)
    else
    {
  vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": CALC_GEOM_FLAG == false, don't need to regenerate geom\n");
  timer_start = MPI_Wtime();
     /* fill  UVWP arrays */
      index = 0;
      for(k = 0; k < Nel; ++k)
      {
        F  = this->master[0]->flist[k];
  {
          for(n = 0; n < this->nfields; ++n){
            F = this->master[n]->flist[k];
      ntot = Interp_symmpts(F,qa,F->h_3d[0][0],temp_array[n],'p');
          }
         for(i = 0; i < ntot; ++i)
   {
      if(this->GetPointArrayStatus("Velocity"))
      {
          vectors->SetTuple3(
        index,
        temp_array[0][i],
        temp_array[1][i],
        temp_array[2][i]);
      }

      if(this->GetPointArrayStatus("Pressure"))
      {
    scalars->SetValue(index, temp_array[3][i]);
      }
            index ++;
   } // for(i = 0; i < ntot; ++i)
        }
      }// for(k = 0; k < Nel; ++k)

      timer_stop = MPI_Wtime();
      timer_diff = timer_stop - timer_start;
      vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": time to copy/convert uvw: "<<timer_diff);
    }// else (from if(this->GEOM_FLAG) )

    //if(this->PointDataArraySelection->ArrayIsEnabled("Pressure"))
    if(this->GetPointArrayStatus("Pressure"))
    {
        this->ugrid->GetPointData()->SetScalars(scalars);
  //this->ugrid->GetPointData()->AddArray(scalars);
    }
    else  // user does not want this variable
    {
        // Does PV already have this array?  If so, remove it.
  if (pv_ugrid->GetPointData()->GetArray("Pressure") != NULL)
      pv_ugrid->GetPointData()->RemoveArray("Pressure");
        // Do I already have this array?  If so, remove it.
  if (this->ugrid->GetPointData()->GetArray("Pressure") != NULL)
      this->ugrid->GetPointData()->RemoveArray("Pressure");

    }
    scalars->Delete();

    //if(this->PointDataArraySelection->ArrayIsEnabled("Velocity"))
    if(this->GetPointArrayStatus("Velocity"))
    {
        //this->ugrid->GetPointData()->SetVectors(vectors);
  this->ugrid->GetPointData()->AddArray(vectors);
    }
    else  // user does not want this variable
    {
        // Does PV already have this array?  If so, remove it.
  if (pv_ugrid->GetPointData()->GetArray("Velocity") != NULL)
      pv_ugrid->GetPointData()->RemoveArray("Velocity");
        // Do I already have this array?  If so, remove it.
  if (this->ugrid->GetPointData()->GetArray("Velocity") != NULL)
      this->ugrid->GetPointData()->RemoveArray("Velocity");
    }
    vectors->Delete();

    vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": finished with pressure and velocity, see if we want vorticity and/or lambda_2");
    if(this->GetDerivedVariableArrayStatus("Vorticity") || this->GetDerivedVariableArrayStatus("lambda_2"))
    {
  vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": Yes, we want vorticity and/or lambda_2");
  double *vel_array[4];
  timer_start = MPI_Wtime();
  for (k=0; k < 4; k++)
  {
      vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": Allocate vel_array["<<k<<"] (Nvert_total: "<<Nvert_total<<" master["<<k<<"]->htot: "<<master[k]->htot<<")");
      vel_array[k]= dvector(0, master[k]->htot*sizeof(double));
      vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": Now memcopy master["<<k<<"] to vel_array["<<k<<"]");
      memcpy(vel_array[k], master[k]->base_h, master[k]->htot*sizeof(double));
  }
  timer_stop = MPI_Wtime();
  timer_diff = timer_stop - timer_start;
  vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": time to backup master: "<< timer_diff);
  vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": call Calc_Vort()");
  timer_start = MPI_Wtime();
  Calc_Vort(&this->fl, this->master, this->nfields, 0);
  timer_stop = MPI_Wtime();
  timer_diff = timer_stop - timer_start;
  vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": Calc_Vort() complete, this->nfields: "<<this->nfields<<" :: time: "<< timer_diff );
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
      this->ugrid->GetPointData()->AddArray(vorticity);
      vorticity->Delete();
  }
  else
  {
      // Does PV already have this array?  If so, remove it.
      if (pv_ugrid->GetPointData()->GetArray("Vorticity") != NULL)
    pv_ugrid->GetPointData()->RemoveArray("Vorticity");
      // Do I already have this array?  If so, remove it.
      if (this->ugrid->GetPointData()->GetArray("Vorticity") != NULL)
    this->ugrid->GetPointData()->RemoveArray("Vorticity");
  }
  //vorticity->Delete();

  if(this->GetDerivedVariableArrayStatus("lambda_2"))
  {
      this->ugrid->GetPointData()->AddArray(lambda_2);
      lambda_2->Delete();
  }
  else  // user does not want this variable
  {
      // Does PV already have this array?  If so, remove it.
      if (pv_ugrid->GetPointData()->GetArray("lambda_2") != NULL)
    pv_ugrid->GetPointData()->RemoveArray("lambda_2");
      // Do I already have this array?  If so, remove it.
      if (this->ugrid->GetPointData()->GetArray("lambda_2") != NULL)
    this->ugrid->GetPointData()->RemoveArray("lambda_2");

  }
  //lambda_2->Delete();


  timer_start = MPI_Wtime();
  for (k=0; k < 4; k++)
  {
      vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": Now memcopy vel_array["<<k<<"] back to master["<<k<<"]");
      memcpy(master[k]->base_h, vel_array[k], master[k]->htot*sizeof(double));
      free(vel_array[k]);
      vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": vel_array["<<k<<"] has been freed");
  }
  timer_stop = MPI_Wtime();
  timer_diff = timer_stop - timer_start;
  vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": time to restore master: "<<timer_diff);

    } // if(this->GetDerivedVariableArrayStatus("Vorticity"))
    else  // user does not want this variable
    {
  if(this->GetDerivedVariableArrayStatus("Vorticity"))
        // Does PV already have this array?  If so, remove it.
  if (pv_ugrid->GetPointData()->GetArray("Vorticity") != NULL)
      pv_ugrid->GetPointData()->RemoveArray("Vorticity");
        // Do I already have this array?  If so, remove it.
  if (this->ugrid->GetPointData()->GetArray("Vorticity") != NULL)
      this->ugrid->GetPointData()->RemoveArray("Vorticity");

  // Does PV already have this array?  If so, remove it.
  if (pv_ugrid->GetPointData()->GetArray("lambda_2") != NULL)
      pv_ugrid->GetPointData()->RemoveArray("lambda_2");
  // Do I already have this array?  If so, remove it.
  if (this->ugrid->GetPointData()->GetArray("lambda_2") != NULL)
      this->ugrid->GetPointData()->RemoveArray("lambda_2");
    }

    timer_start = MPI_Wtime();
    if (this->CALC_GEOM_FLAG)
    {
      /* numbering array */
      for(e = 0,n=0; e < Nel; ++e)
      {
        F  = this->master[0]->flist[e];
        //if(Check_range_sub_cyl(F))
  {
          //if(Check_range(F)){
          //qa = F->qa;
          /* dump connectivity */
          switch(F->identify()){
          case Nek_Tet:
            n += qa*(qa+1)*(qa+2)/6;
            break;
          default:
            fprintf(stderr,"WriteS is not set up for this element type \n");
            exit(1);
            break;
          }
        }
      }

//      num = dtarray(0,QGmax-1,0,QGmax-1,0,QGmax-1);
      num = dtarray(0,alloc_res-1,0,alloc_res-1,0,alloc_res-1);
      for(cnt = 1, k = 0; k < qa; ++k)
        for(j = 0; j < qa-k; ++j)
          for(i = 0; i < qa-k-j; ++i, ++cnt)
            num[i][j][k] = cnt;

      vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<":: cnt = "<<cnt<<" : Nel = "<<Nel<<" : cnt*Nel = "<<(cnt*Nel));

//#ifdef PARALLEL
#if 1
      gsync();
#endif


      //vtkIdType *pts;
      vtkIdType pts[4];
      index = 0;
      for(e = 0,n=0; e < Nel; ++e)
      {
        F  = this->master[0]->flist[e];
        //if(Check_range_sub_cyl(F))
  {
          //if(Check_range(F)){
          //qa = F->qa;
          /* dump connectivity */
          switch(F->identify())
    {
          case Nek_Tet:
            for(k=0; k < qa-1; ++k)
              for(j = 0; j < qa-1-k; ++j){
                for(i = 0; i < qa-2-k-j; ++i){

      pts[0] = n+(int) num[i][j][k] -1;
                  pts[1] = n+(int) num[i+1][j][k] -1;
                  pts[2] = n+(int) num[i][j+1][k] -1;
                  pts[3] = n+(int) num[i][j][k+1] -1;
      this->ugrid->InsertNextCell(VTK_TETRA, 4, pts);
      index++;

                  pts[0] = n+(int) num[i+1][j][k]-1;
                  pts[1] = n+(int) num[i][j+1][k]-1;
                  pts[2] = n+(int) num[i][j][k+1]-1;
                  pts[3] = n+(int) num[i+1][j][k+1]-1;
      this->ugrid->InsertNextCell(VTK_TETRA, 4, pts);
      index++;

                  pts[0] = n+(int) num[i+1][j][k+1]-1;
                  pts[1] = n+(int) num[i][j][k+1]-1;
                  pts[2] = n+(int) num[i][j+1][k+1]-1;
                  pts[3] = n+(int) num[i][j+1][k]-1;
      this->ugrid->InsertNextCell(VTK_TETRA, 4, pts);
      index++;

                  pts[0] = n+(int) num[i+1][j+1][k]-1;
                  pts[1] = n+(int) num[i][j+1][k]-1;
                  pts[2] = n+(int) num[i+1][j][k]-1;
                  pts[3] = n+(int) num[i+1][j][k+1]-1;
      this->ugrid->InsertNextCell(VTK_TETRA, 4, pts);
      index++;

                  pts[0] = n+(int) num[i+1][j+1][k]-1;
                  pts[1] = n+(int) num[i][j+1][k]-1;
                  pts[2] = n+(int) num[i+1][j][k+1]-1;
                  pts[3] = n+(int) num[i][j+1][k+1]-1;
      this->ugrid->InsertNextCell(VTK_TETRA, 4, pts);
      index++;

                  if(i < qa-3-k-j)
      {
                    pts[0] = n+(int) num[i][j+1][k+1]-1;
                    pts[1] = n+(int) num[i+1][j+1][k+1]-1;
                    pts[2] = n+(int) num[i+1][j][k+1]-1;
                    pts[3] = n+(int) num[i+1][j+1][k]-1;
        this->ugrid->InsertNextCell(VTK_TETRA, 4, pts);
        index++;
                  }

                }
                pts[0] = n+(int) num[qa-2-k-j][j][k]-1;
                pts[1] = n+(int) num[qa-1-k-j][j][k]-1;
                pts[2] = n+(int) num[qa-2-k-j][j+1][k]-1;
                pts[3] = n+(int) num[qa-2-k-j][j][k+1]-1;
    this->ugrid->InsertNextCell(VTK_TETRA, 4, pts);
    index++;
              }
            n += qa*(qa+1)*(qa+2)/6;
            break;
          default:
            fprintf(stderr,"WriteS is not set up for this element type \n");
            exit(1);
            break;
          } // switch(F->identify())
        }
      } // for(e = 0,n=0; e < Nel; ++e)

      vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": index = "<< index);

      this->ugrid->SetPoints(points);

      free_dtarray(num,0,0,0);

    }//end "if (this->CALC_GEOM_FLAG)"
    timer_stop = MPI_Wtime();
    timer_diff = timer_stop - timer_start;
    vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": time of CALC_GEOM (the mesh): "<< timer_diff);

    this->ugrid->Update();

    timer_start = MPI_Wtime();
    vtkCleanUnstructuredGrid *clean = vtkCleanUnstructuredGrid::New();
    //clean->SetInputConnection(this->ugrid);
    clean->SetInput(this->ugrid);
    clean->Update();
    timer_stop = MPI_Wtime();
    timer_diff = timer_stop - timer_start;
    vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": time to clean the grid: "<< timer_diff);


    timer_start = MPI_Wtime();
    //pv_ugrid->ShallowCopy(this->ugrid);
    pv_ugrid->ShallowCopy(clean->GetOutput());
    vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<":  completed ShallowCopy to pv_ugrid\n");
    if(this->curObj->ugrid)
    {
  this->curObj->ugrid->Delete();
    }
    this->curObj->ugrid = vtkUnstructuredGrid::New();
    //this->curObj->ugrid->ShallowCopy(this->ugrid);
    this->curObj->ugrid->ShallowCopy(clean->GetOutput());
    //fprintf(stderr, "updateVtuData:: Rank: %d:  completed ShallowCopy to curObj->ugrid\n");
    timer_stop = MPI_Wtime();
    timer_diff = timer_stop - timer_start;
    vtkDebugMacro(<< "updateVtuData: my_rank= " << this->my_rank<<": time to shallow copy cleaned grid to pv_ugrid: "<<timer_diff);
    clean->Delete();

    this->displayed_step = this->requested_step;
    this->curObj->pressure = this->GetPointArrayStatus("Pressure");
    this->curObj->velocity = this->GetPointArrayStatus("Velocity");
    this->curObj->vorticity = this->GetDerivedVariableArrayStatus("Vorticity");
    this->curObj->lambda_2 = this->GetDerivedVariableArrayStatus("lambda_2");
    this->curObj->resolution = this->ElementResolution;

    free(X.x); free(X.y); free(X.z);
    for(i=0; i<this->nfields; i++)
    {
  free(temp_array[i]);
    }

    this->CALC_GEOM_FLAG=false;
//     if (this->GEOM_FLAG)
//     {
//   this->GEOM_FLAG = 0;
//     }



  }
  else{
      fprintf(stderr,"2D case is not implemented");
     ;
  }
}
