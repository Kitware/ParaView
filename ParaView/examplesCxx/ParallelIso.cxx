
// This program tests the ports by setting up a simple pipeline.

#include "vtkImageMandelbrotSource.h"
#include "vtkImageShrink3D.h"
#include "vtkImageAppend.h"
#include "vtkStructuredPointsWriter.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkAppendPolyData.h"
#include "vtkOutputPort.h"
#include "vtkInputPort.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkTimerLog.h"
#include "vtkMath.h"

#include "vtkWindowToImageFilter.h"
#include "vtkTIFFWriter.h"
#include "vtkOutputWindow.h"


const float ISO_START=40.0;
const float ISO_STEP=0.0;
const int ISO_NUM=1;

class vtkNodeInfo
{
public:
  vtkMesaRenderer* Ren;
  vtkMesaRenderWindow* RenWindow;
  vtkMultiProcessController* Controller;
};

// A structure to communicate renderer info.
struct vtkPVRenderInfo 
{
  float CameraPosition[3];
  float CameraFocalPoint[3];
  float CameraViewUp[3];
  float CameraClippingRange[2];
  float LightPosition[3];
  float LightFocalPoint[3];
  int WindowSize[2];
};

//-------------------------------------------------------------------------
// Jim's composite stuff
//-------------------------------------------------------------------------
// Results are put in the local data.
void vtkCompositeImagePair(float *localZdata, float *localPdata, 
			   float *remoteZdata, float *remotePdata, 
			   int total_pixels, int flag) 
{
  int i,j;
  int pixel_data_size;
  float *pEnd;

  if (flag) 
    {
    pixel_data_size = 4;
    for (i = 0; i < total_pixels; i++) 
      {
      if (remoteZdata[i] < localZdata[i]) 
	{
	localZdata[i] = remoteZdata[i];
	for (j = 0; j < pixel_data_size; j++) 
	  {
	  localPdata[i*pixel_data_size+j] = remotePdata[i*pixel_data_size+j];
	  }
	}
      }
    } 
  else 
    {
    pEnd = remoteZdata + total_pixels;
    while(remoteZdata != pEnd) 
      {
      if (*remoteZdata < *localZdata) 
	{
	*localZdata++ = *remoteZdata++;
	*localPdata++ = *remotePdata++;
	}
      else
	{
	++localZdata;
	++remoteZdata;
	++localPdata;
	++remotePdata;
	}
      }
    }
}


#define vtkTCPow2(j) (1 << (j))


//----------------------------------------------------------------------------

void vtkTreeComposite(vtkMesaRenderWindow *renWin, 
		      vtkMultiProcessController *controller,
		      int flag, float *remoteZdata, 
		      float *remotePdata) 
{
  float *localZdata, *localPdata;
  int *windowSize;
  int total_pixels;
  int pdata_size, zdata_size;
  int myId, numProcs;
  int i, id;
  

  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  windowSize = renWin->GetSize();
  total_pixels = windowSize[0] * windowSize[1];

  // Get the z buffer.
  localZdata = renWin->GetZbufferData(0,0,windowSize[0]-1, windowSize[1]-1);
  zdata_size = total_pixels;

  // Get the pixel data.
  if (flag) 
    { 
    localPdata = renWin->GetRGBAPixelData(0,0,windowSize[0]-1, \
					  windowSize[1]-1,1);
    pdata_size = 4*total_pixels;
    } 
  else 
    {
    // Condition is here until we fix the resize bug in vtkMesarenderWindow.
    localPdata = (float*)renWin->GetRGBACharPixelData(0, 0, windowSize[0]-1,
						      windowSize[1]-1, 1);    
    pdata_size = total_pixels;
    }
  
  double doubleLogProcs = log((double)numProcs)/log((double)2);
  int logProcs = (int)doubleLogProcs;

  // not a power of 2 -- need an additional level
  if (doubleLogProcs != (double)logProcs) 
    {
    logProcs++;
    }

  for (i = 0; i < logProcs; i++) 
    {
    if ((myId % (int)vtkTCPow2(i)) == 0) 
      { // Find participants
      if ((myId % (int)vtkTCPow2(i+1)) < vtkTCPow2(i)) 
        {
	// receivers
	id = myId+vtkTCPow2(i);
	
	// only send or receive if sender or receiver id is valid
	// (handles non-power of 2 cases)
	if (id < numProcs) 
          {
	  controller->Receive(remoteZdata, zdata_size, id, 99);
	  controller->Receive(remotePdata, pdata_size, id, 99);
	  
	  // notice the result is stored as the local data
	  vtkCompositeImagePair(localZdata, localPdata, remoteZdata,
				remotePdata, total_pixels, flag);
	  }
	}
      else 
	{
	id = myId-vtkTCPow2(i);
	if (id < numProcs) 
	  {
	  controller->Send(localZdata, zdata_size, id, 99);
	  controller->Send(localPdata, pdata_size, id, 99);
	  }
	}
      }
    }

  if (myId ==0) 
    {
    if (flag) 
      {
      renWin->SetRGBAPixelData(0,0,windowSize[0]-1, 
			       windowSize[1]-1,localPdata,1);
      } 
    else 
      {
      renWin->SetRGBACharPixelData(0,0, windowSize[0]-1, \
			     windowSize[1]-1,(unsigned char*)localPdata,1);
      }
    }
}

//-------------------------------------------------------------------------


void start_render(vtkMesaRenderer *ren, vtkMesaRenderWindow *renWin,
		  vtkMultiProcessController *controller)
{
  struct vtkPVRenderInfo info;
  int id, num;
  int *windowSize;

  // Get a global (across all processes) clipping range.
  // ren->ResetCameraClippingRange();
  
  // Make sure the satellite renderers have the same camera I do.
  vtkCamera *cam = ren->GetActiveCamera();
  vtkLightCollection *lc = ren->GetLights();
  lc->InitTraversal();
  vtkLight *light = lc->GetNextItem();
  cerr << light << endl;
  cam->GetPosition(info.CameraPosition);
  cam->GetFocalPoint(info.CameraFocalPoint);
  cam->GetViewUp(info.CameraViewUp);
  cam->GetClippingRange(info.CameraClippingRange);
  light->GetPosition(info.LightPosition);
  light->GetFocalPoint(info.LightFocalPoint);
  // Make sure the render slave size matches our size
  windowSize = renWin->GetSize();
  info.WindowSize[0] = windowSize[0];
  info.WindowSize[1] = windowSize[1];
  num = controller->GetNumberOfProcesses();
  
  for (id = 1; id < num; ++id)
    {
    controller->Send((char*)(&info), 
		     sizeof(struct vtkPVRenderInfo), id, 133);
    }
  
  // Turn swap buffers off before the render so the end render method has a
  // chance to add to the back buffer.
  //renWin->SwapBuffersOff();
}

void end_render(vtkMesaRenderer *ren, vtkMesaRenderWindow *renWin,
		vtkMultiProcessController *controller)
{
  int *windowSize;
  int numPixels;
  int numProcs;
  float *pdata, *zdata;    
  
  windowSize = renWin->GetSize();
  numProcs = controller->GetNumberOfProcesses();
  numPixels = (windowSize[0] * windowSize[1]);

  cout << "In end_render, window size is: " << numPixels << endl;
  if (numProcs > 1)
    {
    pdata = new float[numPixels];
    zdata = new float[numPixels];
    cerr << "0: Starting Tree Composite\n";
    vtkTreeComposite(renWin, controller, 0, zdata, pdata);
    cerr << "0: Out of Tree Composite\n";
    
    delete [] zdata;
    delete [] pdata;    
    }
  
  // Force swap buffers here.
  //renWin->SwapBuffersOn();  
  //renWin->Frame();
}

void render_hack(vtkMesaRenderer *ren, vtkMesaRenderWindow *renWin,
		 vtkMultiProcessController *controller)
{
  int *window_size;
  int length, numPixels;
  int myId, numProcs;
  vtkPVRenderInfo info;
  
  cerr << "in render_hack\n";
  
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
  
  // Makes an assumption about how the tasks are setup (UI id is 0).
  // Receive the camera information.
  controller->Receive((char*)(&info), sizeof(struct vtkPVRenderInfo), 0, 133);
  vtkCamera *cam = ren->GetActiveCamera();
  vtkLightCollection *lc = ren->GetLights();
  lc->InitTraversal();
  vtkLight *light = lc->GetNextItem();
  
  cam->SetPosition(info.CameraPosition);
  cam->SetFocalPoint(info.CameraFocalPoint);
  cam->SetViewUp(info.CameraViewUp);
  cam->SetClippingRange(info.CameraClippingRange);
  if (light)
    {
    light->SetPosition(info.LightPosition);
    light->SetFocalPoint(info.LightFocalPoint);
    }
  
  renWin->SetSize(info.WindowSize);
  
  renWin->Render();

  window_size = renWin->GetSize();
  
  numPixels = (window_size[0] * window_size[1]);
  
  float *pdata, *zdata;
  pdata = new float[numPixels];
  zdata = new float[numPixels];
  cerr << "1: Starting Tree Composite\n";
  vtkTreeComposite(renWin, controller, 0, zdata, pdata);
  cerr << "1: Out of Tree Composite\n";
  
  delete [] zdata;
  delete [] pdata;
}

// call back to set the iso surface value.
void set_iso_val_rmi(void *localArg, void *remoteArg, 
		     int remoteArgLen, int id)
{ 
  float val;

  vtkSynchronizedTemplates3D *iso;
  iso = (vtkSynchronizedTemplates3D *)localArg;
  val = iso->GetValue(0);
  iso->SetValue(0, val + ISO_STEP);
}

void generate_image(vtkImageData *image)
{
  static int imageCount = 0;
  int tag = 5;  // completely arbitrary number
  vtkSynchronizedTemplates3D *iso;
  vtkElevationFilter *elev;
  vtkMultiProcessController *controller = vtkMultiProcessController::GetGlobalController();
  int myid = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();
  int i, j;
  float val;
  char imageName[20];
  vtkImageShrink3D *shrink = vtkImageShrink3D::New();
  vtkImageAppend *append;
  vtkImageData *remoteImage;
  FILE *cameraFile;
  char line[256];
  float pos[3], focalPt[3], viewUp[3], angle, clipRange[2];
  char tempString[20];
  
  shrink->SetInput(image);
  shrink->SetShrinkFactors(4, 4, 4);
  
  if (myid == 0)
    {
    append = vtkImageAppend::New();
    append->PreserveExtentsOn();
    append->AddInput(shrink->GetOutput());
    for (i = 1; i < numProcs; i++)
      {
      remoteImage = vtkImageData::New();
      controller->Receive(remoteImage, i, tag);
      append->AddInput(remoteImage);
      remoteImage->Delete();
      remoteImage = NULL;
      }
    vtkStructuredPointsWriter *writer = vtkStructuredPointsWriter::New();
    writer->SetFileName("standin.vtk");
    writer->SetInput(append->GetOutput());
    writer->SetFileTypeToBinary();
    writer->Write();
    writer->Delete();
    append->Delete();
    }
  else
    {
    shrink->Update();
    controller->Send(shrink->GetOutput(), i, tag);
    }
  
  iso = vtkSynchronizedTemplates3D::New();
  iso->SetInput(image);
  iso->SetValue(0, ISO_START);
  iso->ComputeScalarsOff();
  iso->ComputeGradientsOff();
  // This should be automatically determined by controller.
  iso->SetNumberOfThreads(1);
  
  // Compute a different color for each process.
  elev = vtkElevationFilter::New();
  elev->SetInput(iso->GetOutput());
  vtkMath::RandomSeed(myid * 100);
  val = vtkMath::Random();
  elev->SetScalarRange(val, val+0.001);
  
  vtkMesaRenderer *ren = vtkMesaRenderer::New();
  vtkMesaRenderWindow *renWindow = vtkMesaRenderWindow::New();
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  vtkActor *actor = vtkActor::New();
  vtkTimerLog *timer = vtkTimerLog::New();
  vtkCamera *cam = vtkCamera::New();

  renWindow->SetOffScreenRendering(1);
  vtkNodeInfo* nodeInfo = new vtkNodeInfo();
  nodeInfo->Ren = ren;
  nodeInfo->RenWindow = renWindow;
  nodeInfo->Controller = controller;
 
  renWindow->AddRenderer(ren);

  vtkLight* light = vtkLight::New();
  // ren->AddLight(light);
  
  ren->SetBackground(0.9, 0.9, 0.9);
  renWindow->SetSize( 400, 400);
  
  mapper->SetInput(elev->GetPolyDataOutput());
  actor->SetMapper(mapper);
  
  // assign our actor to the renderer
  ren->AddActor(actor);
  
  // want to read camera info in from file produced by ParaView
  if ((cameraFile = fopen("../../../Views/ParaView/camera.pv", "r")) == NULL)
    {
    cerr << "camera parameter file not found" << endl;
    cam->SetFocalPoint(100, 100, 65);
    cam->SetPosition(100, 450, 65);
    cam->SetViewUp(0, 0, -1);
    cam->SetViewAngle(30);
    cam->SetClippingRange(177.0, 536.0);
    }
  else
    {
    cerr << "camera parameter file found" << endl;
    fscanf(cameraFile, "position %f %f %f\n", &pos[0], &pos[1], &pos[2]);
    cerr << "pos: " << pos[0] << " " << pos[1] << " " << pos[2] << endl;
    fscanf(cameraFile, "focal_point %f %f %f\n", &focalPt[0], &focalPt[1], &focalPt[2]);
    cerr << "fPt: " << focalPt[0] << " " << focalPt[1] << " " << focalPt[2] << endl;
    fscanf(cameraFile, "view_up %f %f %f\n", &viewUp[0], &viewUp[1], &viewUp[2]);
    cerr << "vUp: " << viewUp[0] << " " << viewUp[1] << " " << viewUp[2] << endl;
    fscanf(cameraFile, "view_angle %f\n", &angle);
    cerr << "angle: " << angle << endl;
    fscanf(cameraFile, "clipping_range %f %f\n", &clipRange[0], &clipRange[1]);
    cerr << "clipRange: " << clipRange[0] << " " << clipRange[1] << endl;
    fclose(cameraFile);
    
    cam->SetPosition(pos);
    cam->SetFocalPoint(focalPt);
    cam->SetViewUp(viewUp);
    cam->SetViewAngle(angle);
    cam->SetClippingRange(clipRange);
    }
  
  ren->SetActiveCamera(cam);
  ren->CreateLight();  

  if (myid == 0)
    {
    start_render(ren, renWindow, controller);
    renWindow->Render();
    end_render(ren, renWindow, controller);
    }
  else
    {
    render_hack(ren, renWindow, controller);
    }
  
  
  if ( myid == 0 )
    {
    vtkWindowToImageFilter *w2if = vtkWindowToImageFilter::New();
    vtkTIFFWriter *rttiffw = vtkTIFFWriter::New();
    w2if->SetInput(renWindow);
    rttiffw->SetInput(w2if->GetOutput());
    sprintf(imageName, "runtime%03d.tif", imageCount);
    imageCount++;
    rttiffw->SetFileName(imageName); 
    rttiffw->Write();
    w2if->Delete();
    rttiffw->Delete();
    }
  
  iso->Delete();
  elev->Delete();
  ren->Delete();
  renWindow->Delete();
  mapper->Delete();
  actor->Delete();
  timer->Delete();
  cam->Delete();
  delete nodeInfo;
}

void process(vtkMultiProcessController *controller, void *arg )
{
  vtkImageMandelbrotSource *mandelbrot = vtkImageMandelbrotSource::New();
  vtkImageData *image = vtkImageData::New();
  int myid, numProcs;
  float val, i;
  char *save_filename = (char*)arg;
    
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
    
  mandelbrot->SetWholeExtent(-75, 75, -75, 75, -75, 75);
  mandelbrot->SetSampleCX(0.002, 0.002, 0.002, 0.002);
  mandelbrot->SetProjectionAxes(0, 1, 2);
  
  for (i = 0.1; i < 0.2; i += 0.025)
    {
    mandelbrot->SetOriginCX(-0.62, 0.42, 0.0, i);
    mandelbrot->GetOutput()->SetUpdateExtent(myid, numProcs);
    mandelbrot->Update();
    image->ShallowCopy(mandelbrot->GetOutput());
    generate_image(image);
    }
  image->Delete();
  mandelbrot->Delete();
}


class StdOut : public vtkOutputWindow
{
public:
  virtual void DisplayText(const char* o)
    { 
      cout << o << endl;
    }
};

void main( int argc, char *argv[] )
{
  vtkOutputWindow::SetInstance(new StdOut);
  vtkMultiProcessController *controller;
  char save_filename[100]="\0";
    
  controller = vtkMultiProcessController::New();
  
  controller->Initialize(argc, argv);
  // Needed for threaded controller.
  // controller->SetNumberOfProcesses(2);
  controller->SetSingleMethod(process, save_filename);
  if (controller->IsA("vtkThreadedController"))
    {
    controller->SetNumberOfProcesses(8);
    } 
  controller->SingleMethodExecute();

  controller->Delete();
}
