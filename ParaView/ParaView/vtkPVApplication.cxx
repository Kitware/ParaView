/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplication.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkKWDialog.h"
#include "vtkKWWindowCollection.h"

#include "vtkMultiProcessController.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkTclUtil.h"
#include "vtkPolyDataMapper.h"
#include "vtkKWResetViewButton.h"
#include "vtkKWFlyButton.h"
#include "vtkKWRotateViewButton.h"
#include "vtkKWTranslateViewButton.h"
#include "vtkKWPickCenterButton.h"
#include "vtkPVRenderView.h"
#include "vtkPVCalculatorButton.h"
#include "vtkPVThresholdButton.h"
#include "vtkPVContourButton.h"
#include "vtkPVProbeButton.h"
#include "vtkPVGlyphButton.h"
#include "vtkPV3DCursor.h"
#include "vtkPVCutButton.h"
#include "vtkPVClipButton.h"
#include "vtkPVExtractGridButton.h"
#include "vtkProbeFilter.h"
#include "vtkMapper.h"

#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include "vtkOutputWindow.h"
#include <sys/stat.h>

#ifdef _WIN32
#include "htmlhelp.h"
#include "vtkKWRegisteryUtilities.h"
#endif

#include "vtkKWLabeledFrame.h"

extern "C" int Vtktkrenderwidget_Init(Tcl_Interp *interp);
extern "C" int Vtkkwparaviewtcl_Init(Tcl_Interp *interp);


// initialze the class variables
int vtkPVApplication::GlobalLODFlag = 0;


Tcl_Interp *vtkPVApplication::InitializeTcl(int argc, char *argv[])
{

  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc,argv);
  
  //  if (Vtkparalleltcl_Init(interp) == TCL_ERROR) 
  //  {
   // cerr << "Init Parallel error\n";
   // }

  // Why is this here?  Doesn't the superclass initialize this?
  if (vtkKWApplication::GetWidgetVisibility())
    {
    Vtktkrenderwidget_Init(interp);
    }
   
  Vtkkwparaviewtcl_Init(interp);
  
  // Create the component loader procedure in Tcl.
  char* script = new char[strlen(vtkPVApplication::LoadComponentProc)+1];
  strcpy(script, vtkPVApplication::LoadComponentProc);
  if (Tcl_GlobalEval(interp, script) != TCL_OK)
    {
    // ????
    }  
  delete [] script;
  
  return interp;
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVApplication::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVApplication");
  if(ret)
    {
    return (vtkPVApplication*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVApplication;
}

int vtkPVApplicationCommand(ClientData cd, Tcl_Interp *interp,
			    int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVApplication::vtkPVApplication()
{
  this->RunningParaViewScript = 0;

  char name[128];
  this->CommandFunction = vtkPVApplicationCommand;
  this->MajorVersion = 0;
  this->MinorVersion = 5;
  this->SetApplicationName("ParaView");
  sprintf(name, "ParaView%d.%d", this->MajorVersion, this->MinorVersion);
  this->SetApplicationVersionName(name);
  this->SetApplicationReleaseName("development");

  this->Controller = NULL;

  struct stat fs;

  if (stat("ParaViewTrace1.pvs", &fs) == 0) 
    {
    rename("ParaViewTrace1.pvs", "ParaViewTrace2.pvs");
    }

  if (stat("ParaViewTrace.pvs", &fs) == 0) 
    {
    rename("ParaViewTrace.pvs", "ParaViewTrace1.pvs");
    }

  this->TraceFile = new ofstream("ParaViewTrace.pvs", ios::out);
  if (this->TraceFile && this->TraceFile->fail())
    {
    delete this->TraceFile;
    this->TraceFile = NULL;
    }

  vtkKWLabeledFrame::AllowShowHideOn();
}


//----------------------------------------------------------------------------
vtkPVWindow *vtkPVApplication::GetMainWindow()
{
  this->Windows->InitTraversal();
  return (vtkPVWindow*)(this->Windows->GetNextItemAsObject());
}


//----------------------------------------------------------------------------
void vtkPVApplication::SetController(vtkMultiProcessController *c)
{
  if (this->Controller == c)
    {
    return;
    }

  if (c)
    {
    c->Register(this);
    }
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    }

  this->Controller = c;
}

//----------------------------------------------------------------------------
vtkPVApplication::~vtkPVApplication()
{
  this->SetController(NULL);
}


//----------------------------------------------------------------------------
void vtkPVApplication::RemoteScript(int id, char *format, ...)
{
  char event[16000];
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  this->RemoteSimpleScript(id, event);
}
//----------------------------------------------------------------------------
void vtkPVApplication::RemoteSimpleScript(int remoteId, const char *str)
{
  int length;

  // send string to evaluate.
  length = strlen(str) + 1;
  if (length <= 1)
    {
    return;
    }

  if (this->Controller->GetLocalProcessId() == remoteId)
    {
    this->SimpleScript(str);
    return;
    }
  
  this->Controller->TriggerRMI(remoteId, const_cast<char*>(str), 
			       VTK_PV_SLAVE_SCRIPT_RMI_TAG);
}

//----------------------------------------------------------------------------
void vtkPVApplication::BroadcastScript(char *format, ...)
{
  char event[16000];
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  this->BroadcastSimpleScript(event);
}

//----------------------------------------------------------------------------
void vtkPVApplication::BroadcastSimpleScript(const char *str)
{
  int id, num;
  
  num = this->Controller->GetNumberOfProcesses();

  int len = strlen(str);
  if (!str || (len < 1))
    {
    return;
    }

  for (id = 1; id < num; ++id)
    {
    this->RemoteSimpleScript(id, str);
    }
  
  // Do reverse order, because 0 will block.
  this->SimpleScript(str);
}

//----------------------------------------------------------------------------
int vtkPVApplication::AcceptLicense()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::AcceptEvaluation()
{
  return 1;
}

//----------------------------------------------------------------------------
int VerifyKey(unsigned long vtkNotUsed(key), const char* vtkNotUsed(name), int vtkNotUsed(id))
{
 return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::PromptRegistration(char* vtkNotUsed(name), 
                                         char* vtkNotUsed(IDS))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::CheckRegistration()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::CheckForArgument(int argc, char* argv[], 
				       const char* arg, int& index)
{
  if (!arg)
    {
    return VTK_ERROR;
    }

  int i;
  for (i=0; i < argc; i++)
    {
    if (argv[i] && strcmp(arg, argv[i]) == 0)
      {
      index = i;
      return VTK_OK;
      }
    }
  return VTK_ERROR;
}

const char vtkPVApplication::ArgumentList[vtkPVApplication::NUM_ARGS][128] = 
{ "--start-empty" , "--enable-mangled-mesa", "" };

int vtkPVApplication::IsParaViewScriptFile(const char* arg)
{
  if (!arg || strlen(arg) < 4)
    {
    return 0;
    }
  if (strcmp(arg + strlen(arg) - 4,".pvs") == 0)
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVApplication::Start(int argc, char*argv[])
{
  vtkOutputWindow::GetInstance()->PromptUserOn();

  // set the font size to be small
#ifdef _WIN32
  this->Script("option add *font {{MS Sans Serif} 8}");
#else
  this->Script("option add *font -adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1");
//  this->Script("option add *font -adobe-helvetica-medium-r-normal--11-80-100-100-p-56-iso8859-1");
  this->Script("option add *highlightThickness 0");
  this->Script("option add *highlightBackground #ccc");
  this->Script("option add *activeBackground #eee");
  this->Script("option add *activeForeground #000");
  this->Script("option add *background #ccc");
  this->Script("option add *foreground #000");
  this->Script("option add *Entry.background #ffffff");
  this->Script("option add *Text.background #ffffff");
  this->Script("option add *Button.padX 6");
  this->Script("option add *Button.padY 3");
#endif


  int i;
  for (i=1; i < argc; i++)
    {
    if ( vtkPVApplication::IsParaViewScriptFile(argv[i]) )
      {
      this->RunningParaViewScript = 1;
      break;
      }
    }

  if (!this->RunningParaViewScript)
    {
    for (i=1; i < argc; i++)
      {
      int valid=0;
      if (argv[i])
	{
	int  j=0;
	const char* argument = ArgumentList[j];
	while (argument && argument[0])
	  {
	  if ( strcmp(argv[i], argument) == 0 )
	    {
	    valid = 1;
	    }
	  argument = ArgumentList[++j];
	  }
	}
      if (!valid)
	{
	vtkErrorMacro("Unrecognized argument " << argv[i] << ".");
	this->Exit();
	return;
	}
      }
    }

  int index;

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--enable-mangled-mesa",
					  index) 
       == VTK_OK )
    {
    this->BroadcastScript("vtkGraphicsFactory _graphics_fact\n"
			  "_graphics_fact SetUseMesaClasses 1\n"
			  "_graphics_fact Delete");
    this->BroadcastScript("vtkImagingFactory _imaging_fact\n"
			  "_imaging_fact SetUseMesaClasses 1\n"
			  "_imaging_fact Delete");
    }

  vtkPVWindow *ui = vtkPVWindow::New();
  this->Windows->AddItem(ui);

  this->CreateButtonPhotos();

  if ( vtkPVApplication::CheckForArgument(argc, argv, "--start-empty", index) 
       == VTK_OK )
    {
    ui->InitializeDefaultInterfacesOff();
    }
  ui->Create(this,"");

  // ui has ref. count of at least 1 because of AddItem() above
  ui->Delete();

  // If any of the argumens has a .pvs extension, load it as a script.
  for (i=1; i < argc; i++)
    {
    if (vtkPVApplication::IsParaViewScriptFile(argv[i]))
      {
      this->RunningParaViewScript = 1;
      ui->LoadScript(argv[i]);
      this->RunningParaViewScript = 0;
      }
    }


  this->vtkKWApplication::Start(argc,argv);
}


//----------------------------------------------------------------------------
void vtkPVApplication::Exit()
{
  int id, myId, num;
  
  // Send a break RMI to each of the slaves.
  num = this->Controller->GetNumberOfProcesses();
  myId = this->Controller->GetLocalProcessId();
  
  this->vtkKWApplication::Exit();

  for (id = 0; id < num; ++id)
    {
    if (id != myId)
      {
      this->Controller->TriggerRMI(id, 
				   vtkMultiProcessController::BREAK_RMI_TAG);
      }
    }

}


//----------------------------------------------------------------------------
void vtkPVApplication::SendDataBounds(vtkDataSet *data)
{
  float *bounds;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  bounds = data->GetBounds();
  this->Controller->Send(bounds, 6, 0, 1967);
}

//----------------------------------------------------------------------------
void vtkPVApplication::SendProbeData(vtkProbeFilter *source)
{
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  
  vtkDataSet *output = source->GetOutput();
  float bounds[6];
  
  source->GetSource()->GetBounds(bounds);
  
  vtkIdType numPoints = source->GetValidPoints()->GetMaxId() + 1;
  this->Controller->Send(&numPoints, 1, 0, 1970);
  if (numPoints > 0)
    {
    this->Controller->Send(source->GetValidPoints(), 0, 1971);
    this->Controller->Send(output, 0, 1972);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::SendDataNumberOfCells(vtkDataSet *data)
{
  int num;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  num = data->GetNumberOfCells();
  this->Controller->Send(&num, 1, 0, 1968);
}

//----------------------------------------------------------------------------
void vtkPVApplication::SendDataNumberOfPoints(vtkDataSet *data)
{
  int num;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  num = data->GetNumberOfPoints();
  this->Controller->Send(&num, 1, 0, 1969);
}

//----------------------------------------------------------------------------
void vtkPVApplication::GetMapperColorRange(float range[2],
                                           vtkPolyDataMapper *mapper)
{
  vtkDataSetAttributes *attr = NULL;
  vtkDataArray *array;
  
  if (mapper == NULL || mapper->GetInput() == NULL)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    return;
    }

  // Determine and get the array used to color the model.
  if (mapper->GetScalarMode() == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
    {
    attr = mapper->GetInput()->GetPointData();
    }
  if (mapper->GetScalarMode() == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
    attr = mapper->GetInput()->GetCellData();
    }

  // Sanity check.
  if (attr == NULL)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    return;
    }

  array = attr->GetArray(mapper->GetArrayName());
  if (array == NULL)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    return;
    }

  array->GetRange( range, mapper->GetArrayComponent());
}


//----------------------------------------------------------------------------
void vtkPVApplication::SendMapperColorRange(vtkPolyDataMapper *mapper)
{
  float range[2];
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }

  this->GetMapperColorRange(range, mapper);
  this->Controller->Send(range, 2, 0, 1969);
}

//----------------------------------------------------------------------------
void vtkPVApplication::SendDataArrayRange(vtkDataSet *data, char *arrayName)
{
  float range[2];
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  
  data->GetPointData()->GetArray(arrayName)->GetRange(range, 0);
  this->Controller->Send(range, 2, 0, 1976);
}

//----------------------------------------------------------------------------
void vtkPVApplication::CreateButtonPhotos()
{
  this->CreatePhoto("KWResetViewButton", KW_RESET_VIEW_BUTTON, 
              KW_RESET_VIEW_BUTTON_WIDTH, KW_RESET_VIEW_BUTTON_HEIGHT);
  this->CreatePhoto("KWTranslateViewButton", KW_TRANSLATE_VIEW_BUTTON, 
              KW_TRANSLATE_VIEW_BUTTON_WIDTH, KW_TRANSLATE_VIEW_BUTTON_HEIGHT);
  this->CreatePhoto("KWActiveTranslateViewButton", KW_ACTIVE_TRANSLATE_VIEW_BUTTON, 
              KW_ACTIVE_TRANSLATE_VIEW_BUTTON_WIDTH, KW_ACTIVE_TRANSLATE_VIEW_BUTTON_HEIGHT);

  this->CreatePhoto("KWFlyButton", KW_FLY_BUTTON, 
              KW_FLY_BUTTON_WIDTH, KW_FLY_BUTTON_HEIGHT);
  this->CreatePhoto("KWActiveFlyButton", KW_ACTIVE_FLY_BUTTON, 
              KW_ACTIVE_FLY_BUTTON_WIDTH, KW_ACTIVE_FLY_BUTTON_HEIGHT);
  this->CreatePhoto("KWRotateViewButton", KW_ROTATE_VIEW_BUTTON, 
              KW_ROTATE_VIEW_BUTTON_WIDTH, KW_ROTATE_VIEW_BUTTON_HEIGHT);
  this->CreatePhoto("KWActiveRotateViewButton", KW_ACTIVE_ROTATE_VIEW_BUTTON, 
              KW_ACTIVE_ROTATE_VIEW_BUTTON_WIDTH, KW_ACTIVE_ROTATE_VIEW_BUTTON_HEIGHT);
  this->CreatePhoto("KWPickCenterButton", KW_PICK_CENTER_BUTTON, 
              KW_PICK_CENTER_BUTTON_WIDTH, KW_PICK_CENTER_BUTTON_HEIGHT);
  
  this->CreatePhoto("PVCalculatorButton", PV_CALCULATOR_BUTTON,
                    PV_CALCULATOR_BUTTON_WIDTH, PV_CALCULATOR_BUTTON_HEIGHT);
  this->CreatePhoto("PVThresholdButton", PV_THRESHOLD_BUTTON,
                    PV_THRESHOLD_BUTTON_WIDTH, PV_THRESHOLD_BUTTON_HEIGHT);
  this->CreatePhoto("PVContourButton", PV_CONTOUR_BUTTON,
                    PV_CONTOUR_BUTTON_WIDTH, PV_CONTOUR_BUTTON_HEIGHT);
  this->CreatePhoto("PVProbeButton", PV_PROBE_BUTTON,
                    PV_PROBE_BUTTON_WIDTH, PV_PROBE_BUTTON_HEIGHT);
  this->CreatePhoto("PVGlyphButton", PV_GLYPH_BUTTON,
                    PV_GLYPH_BUTTON_WIDTH, PV_GLYPH_BUTTON_HEIGHT);
  this->CreatePhoto("PV3DCursorButton", PV_3D_CURSOR_BUTTON,
                    PV_3D_CURSOR_BUTTON_WIDTH, PV_3D_CURSOR_BUTTON_HEIGHT);
  this->CreatePhoto("PVActive3DCursorButton", PV_ACTIVE_3D_CURSOR_BUTTON,
                    PV_ACTIVE_3D_CURSOR_BUTTON_WIDTH, PV_ACTIVE_3D_CURSOR_BUTTON_HEIGHT);
  this->CreatePhoto("PVCutButton", PV_CUT_BUTTON,
                    PV_CUT_BUTTON_WIDTH, PV_CUT_BUTTON_HEIGHT);
  this->CreatePhoto("PVClipButton", PV_CLIP_BUTTON,
                    PV_CLIP_BUTTON_WIDTH, PV_CLIP_BUTTON_HEIGHT);
  this->CreatePhoto("PVExtractGridButton", PV_EXTRACT_GRID_BUTTON,
                    PV_EXTRACT_GRID_BUTTON_WIDTH, PV_EXTRACT_GRID_BUTTON_HEIGHT);
}

//----------------------------------------------------------------------------
void vtkPVApplication::CreatePhoto(char *name, unsigned char *data, 
                                    int width, int height)
{
  Tk_PhotoHandle photo;
  Tk_PhotoImageBlock block;

  this->Script("image create photo %s -height %d -width %d", 
               name, width, height);
  block.width = width;
  block.height = height;
  block.pixelSize = 3;
  block.pitch = block.width*block.pixelSize;
  block.offset[0] = 0;
  block.offset[1] = 1;
  block.offset[2] = 2;
  block.pixelPtr = data;

  photo = Tk_FindPhoto(this->GetMainInterp(), name);
  if (!photo)
    {
    vtkWarningMacro("error looking up color ramp image");
    return;
    }  
  Tk_PhotoPutBlock(photo, &block, 0, 0, block.width, block.height);

}

//----------------------------------------------------------------------------
void vtkPVApplication::StartRecordingScript(char *filename)
{
  if (this->TraceFile)
    {
    *this->TraceFile << "Application StartRecordingScript " << filename << endl;
    this->StopRecordingScript();
    }

  this->TraceFile = new ofstream(filename, ios::out);
  if (this->TraceFile && this->TraceFile->fail())
    {
    vtkErrorMacro("Could not open trace file " << filename);
    delete this->TraceFile;
    this->TraceFile = NULL;
    return;
    }

  // Initialize a couple of variables in the trace file.
  this->AddTraceEntry("set kw(%s) [Application GetMainWindow]",
                      this->GetMainWindow()->GetTclName());
  this->GetMainWindow()->SetTraceInitialized(1);
}

//----------------------------------------------------------------------------
void vtkPVApplication::StopRecordingScript()
{
  if (this->TraceFile)
    {
    this->TraceFile->close();
    delete this->TraceFile;
    this->TraceFile = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkPVApplication::CompleteArrays(vtkMapper *mapper, char *mapperTclName)
{
  int i, j;
  int numProcs;
  int nonEmptyFlag;
  int activeAttributes[5];

  if (mapper->GetInput() == NULL || this->Controller == NULL ||
      mapper->GetInput()->GetNumberOfPoints() > 0 ||
      mapper->GetInput()->GetNumberOfCells() > 0)
    {
    return;
    }

  // Find the first non empty data object on another processes.
  numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
    {
    this->RemoteScript(i, "Application SendCompleteArrays %s", mapperTclName);
    this->Controller->Receive(&nonEmptyFlag, 1, i, 987243);
    if (nonEmptyFlag)
      { // This process has data.  Receive all the arrays, type and component.
      int num;
      vtkDataArray *array = 0;
      char *name;
      int nameLength;
      int type;
      int numComps;
      
      // First Point data.
      this->Controller->Receive(&num, 1, i, 987244);
      for (j = 0; j < num; ++j)
        {
        this->Controller->Receive(&type, 1, i, 987245);
        switch (type)
          {
          case VTK_INT:
            array = vtkIntArray::New();
            break;
          case VTK_FLOAT:
            array = vtkFloatArray::New();
            break;
          case VTK_DOUBLE:
            array = vtkDoubleArray::New();
            break;
          case VTK_CHAR:
            array = vtkCharArray::New();
            break;
          case VTK_LONG:
            array = vtkLongArray::New();
            break;
          case VTK_SHORT:
            array = vtkShortArray::New();
            break;
          case VTK_UNSIGNED_CHAR:
            array = vtkUnsignedCharArray::New();
            break;
          case VTK_UNSIGNED_INT:
            array = vtkUnsignedIntArray::New();
            break;
          case VTK_UNSIGNED_LONG:
            array = vtkUnsignedLongArray::New();
            break;
          case VTK_UNSIGNED_SHORT:
            array = vtkUnsignedShortArray::New();
            break;
          }
        this->Controller->Receive(&numComps, 1, i, 987246);
        array->SetNumberOfComponents(numComps);
        this->Controller->Receive(&nameLength, 1, i, 987247);
        name = new char[nameLength];
        this->Controller->Receive(name, nameLength, i, 987248);
        array->SetName(name);
        delete [] name;
        mapper->GetInput()->GetPointData()->AddArray(array);
        array->Delete();
        } // end of loop over point arrays.
      // Which scalars, ... are active?
      this->Controller->Receive(activeAttributes, 5, i, 987258);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[0],0);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[1],1);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[2],2);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[3],3);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[4],4);
 
      // Next Cell data.
      this->Controller->Receive(&num, 1, i, 987244);
      for (j = 0; j < num; ++j)
        {
        this->Controller->Receive(&type, 1, i, 987245);
        switch (type)
          {
          case VTK_INT:
            array = vtkIntArray::New();
            break;
          case VTK_FLOAT:
            array = vtkFloatArray::New();
            break;
          case VTK_DOUBLE:
            array = vtkDoubleArray::New();
            break;
          case VTK_CHAR:
            array = vtkCharArray::New();
            break;
          case VTK_LONG:
            array = vtkLongArray::New();
            break;
          case VTK_SHORT:
            array = vtkShortArray::New();
            break;
          case VTK_UNSIGNED_CHAR:
            array = vtkUnsignedCharArray::New();
            break;
          case VTK_UNSIGNED_INT:
            array = vtkUnsignedIntArray::New();
            break;
          case VTK_UNSIGNED_LONG:
            array = vtkUnsignedLongArray::New();
            break;
          case VTK_UNSIGNED_SHORT:
            array = vtkUnsignedShortArray::New();
            break;
          }
        this->Controller->Receive(&numComps, 1, i, 987246);
        array->SetNumberOfComponents(numComps);
        this->Controller->Receive(&nameLength, 1, i, 987247);
        name = new char[nameLength];
        this->Controller->Receive(name, nameLength, i, 987248);
        array->SetName(name);
        delete [] name;
        mapper->GetInput()->GetCellData()->AddArray(array);
        array->Delete();
        } // end of loop over cell arrays.
      // Which scalars, ... are active?
      this->Controller->Receive(activeAttributes, 5, i, 987258);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[0],0);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[1],1);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[2],2);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[3],3);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[4],4);
      
      // We only need information from one.
      return;
      } // End of if-non-empty check.
    }// End of loop over processes.
}



//----------------------------------------------------------------------------
void vtkPVApplication::SendCompleteArrays(vtkMapper *mapper)
{
  int nonEmptyFlag;
  int num;
  int i;
  int type;
  int numComps;
  int nameLength;
  const char *name;
  vtkDataArray *array;
  int activeAttributes[5];

  if (mapper->GetInput() == NULL ||
      (mapper->GetInput()->GetNumberOfPoints() == 0 &&
       mapper->GetInput()->GetNumberOfCells() == 0))
    {
    nonEmptyFlag = 0;
    this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);
    return;
    }
  nonEmptyFlag = 1;
  this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);

  // First point data.
  num = mapper->GetInput()->GetPointData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, 0, 987244);
  for (i = 0; i < num; ++i)
    {
    array = mapper->GetInput()->GetPointData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, 0, 987245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, 0, 987246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = strlen(name)+1;
    this->Controller->Send(&nameLength, 1, 0, 987247);
    // I am pretty sure that Send does not modify the string.
    this->Controller->Send(const_cast<char*>(name), nameLength, 0, 987248);
    }
  mapper->GetInput()->GetPointData()->GetAttributeIndices(activeAttributes);
  this->Controller->Send(activeAttributes, 5, 0, 987258);

  // Next cell data.
  num = mapper->GetInput()->GetCellData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, 0, 987244);
  for (i = 0; i < num; ++i)
    {
    array = mapper->GetInput()->GetCellData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, 0, 987245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, 0, 987246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = strlen(name+1);
    this->Controller->Send(&nameLength, 1, 0, 987247);
    this->Controller->Send(const_cast<char*>(name), nameLength, 0, 987248);
    }
  mapper->GetInput()->GetCellData()->GetAttributeIndices(activeAttributes);
  this->Controller->Send(activeAttributes, 5, 0, 987258);
}


void vtkPVApplication::SetGlobalLODFlag(int val)
{
  vtkPVApplication::GlobalLODFlag = val;

  if (this->Controller->GetLocalProcessId() == 0)
    {
    int idx, num;
    num = this->Controller->GetNumberOfProcesses();
    for (idx = 1; idx < num; ++idx)
      {
      this->RemoteScript(idx, "Application SetGlobalLODFlag %d", val);
      }
    }
}

int vtkPVApplication::GetGlobalLODFlag()
{
  return vtkPVApplication::GlobalLODFlag;
}


//============================================================================
// Make instances of sources.
//============================================================================

//----------------------------------------------------------------------------
vtkObject *vtkPVApplication::MakeTclObject(const char *className,
                                           const char *tclName)
{
  vtkObject *o;
  int error;

  this->BroadcastScript("%s %s", className, tclName);
  o = (vtkObject *)(vtkTclGetPointerFromObject(tclName,
                                  "vtkObject", this->GetMainInterp(), error));
  
  if (o == NULL)
    {
    vtkErrorMacro("Could not get object from pointer.");
    }
  
  return o;
}

void vtkPVApplication::DisplayAbout(vtkKWWindow *master)
{
  ostrstream str;
  str << this->GetApplicationName() << " was developed by Kitware Inc." << endl
      << "http://public.kitware.com/ParaView" << endl
      << "http://www.kitware.com" << endl
      << "This is version " << this->MajorVersion << "." << this->MinorVersion
      << ", release " << this->GetApplicationReleaseName() << ends;

  char* msg = str.str();
  vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
  dlg->SetTitle("About ParaView");
  dlg->SetMasterWindow(master);
  dlg->Create(this,"");
  dlg->SetText(msg);
  dlg->Invoke();  
  dlg->Delete(); 
  delete[] msg;
}

void vtkPVApplication::DisplayHelp(vtkKWWindow* master)
{
#ifdef _WIN32
  char temp[1024];
  char loc[1024];
  vtkKWRegisteryUtilities *reg = this->GetRegistery();
  sprintf(temp, "%i", this->GetApplicationKey());
  reg->SetTopLevel(temp);
  if (reg->ReadValue("Inst", "Loc", loc))
    {
    sprintf(temp,"%s/%s.chm::/UsersGuide/index.html",
            loc,this->ApplicationName);
    }
  else
    {
    sprintf(temp,"%s.chm::/UsersGuide/index.html",this->ApplicationName);
    }
  if ( HtmlHelp(NULL, temp, HH_DISPLAY_TOPIC, 0) )
    {
    return;
    }
#endif
  vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
  dlg->SetTitle("ParaView Help");
  dlg->SetMasterWindow(master);
  dlg->Create(this,"");
  dlg->SetText(
    "HTML help is included in the Documentation/HTML subdirectory of\n"
    "this application. You can view this help using a standard web browser.");
  dlg->Invoke();  
  dlg->Delete();
}

//----------------------------------------------------------------------------
void vtkPVApplication::LogStartEvent(char* str)
{
  vtkTimerLog::MarkStartEvent(str);
}

//----------------------------------------------------------------------------
void vtkPVApplication::LogEndEvent(char* str)
{
  vtkTimerLog::MarkEndEvent(str);
}

//----------------------------------------------------------------------------
void vtkPVApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->GetController();
  os << indent << "MajorVersion: " << this->GetMajorVersion();
  os << indent << "MinorVersion: " << this->GetMinorVersion();
}

//----------------------------------------------------------------------------
const char* const vtkPVApplication::LoadComponentProc =
"namespace eval ::paraview {\n"
"    proc load_component {name} {\n"
"        \n"
"        global tcl_platform auto_path env\n"
"        \n"
"        # First dir is empty, to let Tcl try in the current dir\n"
"        \n"
"        set dirs {\"\"}\n"
"        set ext [info sharedlibextension]\n"
"        if {$tcl_platform(platform) == \"unix\"} {\n"
"            set prefix \"lib\"\n"
"            # Help Unix a bit by browsing into $auto_path and /usr/lib...\n"
"            set dirs [concat $dirs /usr/local/lib /usr/local/lib/vtk $auto_path]\n"
"            if {[info exists env(LD_LIBRARY_PATH)]} {\n"
"                set dirs [concat $dirs [split $env(LD_LIBRARY_PATH) \":\"]]\n"
"            }\n"
"            if {[info exists env(PATH)]} {\n"
"                set dirs [concat $dirs [split $env(PATH) \":\"]]\n"
"            }\n"
"        } else {\n"
"            set prefix \"\"\n"
"            if {$tcl_platform(platform) == \"windows\"} {\n"
"                if {[info exists env(PATH)]} {\n"
"                    set dirs [concat $dirs [split $env(PATH) \";\"]]\n"
"                }\n"
"            }\n"
"        }\n"
"        \n"
"        foreach dir $dirs {\n"
"            set libname [file join $dir ${prefix}${name}${ext}]\n"
"            if {[file exists $libname]} {\n"
"                if {![catch {load $libname} errormsg]} {\n"
"                    # WARNING: it HAS to be \"\" so that pkg_mkIndex work (since\n"
"                    # while evaluating a package ::paraview::load_component won't\n"
"                    # exist and will default to the unknown() proc that \n"
"                    # returns \"\"\n"
"                    return \"\"\n"
"                } else {\n"
"                    # If not loaded but file was found, oops\n"
"                    puts stderr $errormsg\n"
"                }\n"
"            }\n"
"        }\n"
"        \n"
"        puts stderr \"::paraview::load_component: $name could not be found.\"\n"
"        \n"
"        return 1\n"
"    }   \n"
"    namespace export load_component\n"
"}\n";
