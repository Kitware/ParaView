#include "vtkCornerAnnotation.h"
#include "vtkImageData.h"
#include "vtkImageViewer2.h"
#include "vtkKWApplication.h"
#include "vtkKWCornerAnnotationEditor.h"
#include "vtkKWFrame.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLImageDataReader.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWCornerAnnotationEditorItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *win);

  vtkKWCornerAnnotationEditorItem();
  virtual ~vtkKWCornerAnnotationEditorItem();

protected:
  vtkKWRenderWidget *cae_renderwidget;
  vtkXMLImageDataReader *cae_reader;
  vtkImageViewer2 *cae_viewer;
  vtkKWCornerAnnotationEditor *cae_anno_editor;
};

void vtkKWCornerAnnotationEditorItem::Create(vtkKWWidget *parent, vtkKWWindow *win)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a render widget
  // Set the corner annotation visibility

  this->cae_renderwidget = vtkKWRenderWidget::New();
  this->cae_renderwidget->SetParent(parent);
  this->cae_renderwidget->Create();
  this->cae_renderwidget->CornerAnnotationVisibilityOn();

  app->Script("pack %s -side right -fill both -expand y -padx 0 -pady 0", 
              this->cae_renderwidget->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a volume reader

  this->cae_reader = vtkXMLImageDataReader::New();
  this->cae_reader->SetFileName(
    vtkKWWidgetsTourExample::GetPathToExampleData(app, "head100x100x47.vti"));

  // Create an image viewer
  // Use the render window and renderer of the renderwidget

  this->cae_viewer = vtkImageViewer2::New();
  this->cae_viewer->SetRenderWindow(this->cae_renderwidget->GetRenderWindow());
  this->cae_viewer->SetRenderer(this->cae_renderwidget->GetRenderer());
  this->cae_viewer->SetInput(this->cae_reader->GetOutput());
  this->cae_viewer->SetupInteractor(
    this->cae_renderwidget->GetRenderWindow()->GetInteractor());

  // Reset the window/level and the camera

  this->cae_reader->Update();
  double *range = this->cae_reader->GetOutput()->GetScalarRange();
  this->cae_viewer->SetColorWindow(range[1] - range[0]);
  this->cae_viewer->SetColorLevel(0.5 * (range[1] + range[0]));

  this->cae_renderwidget->ResetCamera();

  // -----------------------------------------------------------------------

  // The corner annotation has the ability to parse "tags" and fill
  // them with information gathered from other objects.
  // For example, let's display the slice and window/level in one corner
  // by connecting the corner annotation to our image actor and
  // image mapper

  vtkCornerAnnotation *ca = this->cae_renderwidget->GetCornerAnnotation();
  ca->SetImageActor(this->cae_viewer->GetImageActor());
  ca->SetWindowLevel(this->cae_viewer->GetWindowLevel());
  ca->SetText(2, "<slice>");
  ca->SetText(3, "<window>\n<level>");
  ca->SetText(1, "Hello, World!");

  // -----------------------------------------------------------------------

  // Create a corner annotation editor
  // Connect it to the render widget
  
  this->cae_anno_editor = vtkKWCornerAnnotationEditor::New();
  this->cae_anno_editor->SetParent(parent);
  this->cae_anno_editor->Create();
  this->cae_anno_editor->SetRenderWidget(this->cae_renderwidget);

  app->Script("pack %s -side left -anchor nw -expand n -padx 2 -pady 2", 
              this->cae_anno_editor->GetWidgetName());
}

int vtkKWCornerAnnotationEditorItem::GetType()
{
  return KWWidgetsTourItem::TypeVTK;
}

KWWidgetsTourItem* vtkKWCornerAnnotationEditorEntryPoint()
{
  return new vtkKWCornerAnnotationEditorItem();
}

vtkKWCornerAnnotationEditorItem::vtkKWCornerAnnotationEditorItem()
{
  this->cae_anno_editor = NULL;
  this->cae_reader = NULL;
  this->cae_renderwidget = NULL;
  this->cae_viewer = NULL;
}

vtkKWCornerAnnotationEditorItem::~vtkKWCornerAnnotationEditorItem()
{
  if (this->cae_anno_editor)
    {
    this->cae_anno_editor->Delete();
    }
  if (this->cae_reader)
    {
    this->cae_reader->Delete();
    }
  if (this->cae_renderwidget)
    {
    this->cae_renderwidget->Delete();
    }
  if (this->cae_viewer)
    {
    this->cae_viewer->Delete();
    }
}
