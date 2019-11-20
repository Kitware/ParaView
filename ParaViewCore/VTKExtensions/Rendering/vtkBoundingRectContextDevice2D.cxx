#include "vtkBoundingRectContextDevice2D.h"

#include "vtkAbstractContextItem.h"
#include "vtkContext2D.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkStdString.h"
#include "vtkUnicodeString.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBoundingRectContextDevice2D)

  //-----------------------------------------------------------------------------
  vtkBoundingRectContextDevice2D::vtkBoundingRectContextDevice2D()
  : Initialized(false)
  , DelegateDevice(NULL)
{
  this->Reset();
}

//-----------------------------------------------------------------------------
vtkBoundingRectContextDevice2D::~vtkBoundingRectContextDevice2D()
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->Delete();
    this->DelegateDevice = NULL;
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::Reset()
{
  this->Initialized = false;
  this->BoundingRect = vtkRectf(0, 0, 0, 0);
}

//-----------------------------------------------------------------------------
vtkRectf vtkBoundingRectContextDevice2D::GetBoundingRect(
  vtkAbstractContextItem* item, vtkViewport* viewport)
{
  if (!item || !viewport)
  {
    return vtkRectf();
  }

  vtkNew<vtkContextDevice2D> contextDevice;
  vtkNew<vtkBoundingRectContextDevice2D> bbDevice;
  bbDevice->SetDelegateDevice(contextDevice.Get());
  bbDevice->Begin(viewport);
  vtkNew<vtkContext2D> context;
  context->Begin(bbDevice.Get());
  item->Paint(context.Get());
  context->End();
  bbDevice->End();

  return bbDevice->GetBoundingRect();
}

//-----------------------------------------------------------------------------
vtkRectf vtkBoundingRectContextDevice2D::GetBoundingRect()
{
  return this->BoundingRect;
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawString(float* point, const vtkStdString& string)
{
  this->DrawString(point, vtkUnicodeString::from_utf8(string));
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawString(float* point, const vtkUnicodeString& string)
{
  if (!this->DelegateDevice)
  {
    vtkWarningMacro(<< "No DelegateDevice defined");
    return;
  }

  float bounds[4];
  this->DelegateDevice->ComputeJustifiedStringBounds(string.utf8_str(), bounds);

  this->AddPoint(point[0] + bounds[0], point[1] + bounds[1]);
  this->AddPoint(point[0] + bounds[0] + bounds[2], point[1] + bounds[1] + bounds[3]);
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawMathTextString(float* point, const vtkStdString& string)
{
  if (!this->DelegateDevice)
  {
    vtkWarningMacro(<< "No DelegateDevice defined");
    return;
  }

  // Not sure if this will work for math text
  float bounds[4];
  this->DelegateDevice->ComputeJustifiedStringBounds(string.c_str(), bounds);

  this->AddPoint(point[0] + bounds[0], point[1] + bounds[1]);
  this->AddPoint(point[0] + bounds[0] + bounds[2], point[1] + bounds[1] + bounds[3]);
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawImage(float p[2], float scale, vtkImageData* image)
{
  this->AddPoint(p);
  int* extent = image->GetExtent();
  this->AddPoint(p[0] + scale * extent[1], p[1] + scale * extent[3]);
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawImage(const vtkRectf& pos, vtkImageData* vtkNotUsed(image))
{
  this->AddPoint(pos.GetX(), pos.GetY());
  this->AddPoint(pos.GetX() + pos.GetWidth(), pos.GetY() + pos.GetHeight());
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::SetColor4(unsigned char color[4])
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->SetColor4(color);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::SetTexture(vtkImageData* image, int properties)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->SetTexture(image, properties);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::SetPointSize(float size)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->SetPointSize(size);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::SetLineWidth(float width)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->SetLineWidth(width);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::SetLineType(int type)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->SetLineType(type);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::SetMatrix(vtkMatrix3x3* m)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->SetMatrix(m);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::GetMatrix(vtkMatrix3x3* m)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->GetMatrix(m);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::MultiplyMatrix(vtkMatrix3x3* m)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->MultiplyMatrix(m);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::PushMatrix()
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->PushMatrix();
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::PopMatrix()
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->PopMatrix();
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::EnableClipping(bool enable)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->EnableClipping(enable);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::SetClipping(int* x)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->SetClipping(x);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::ApplyPen(vtkPen* pen)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->ApplyPen(pen);
  }
}

//-----------------------------------------------------------------------------
vtkPen* vtkBoundingRectContextDevice2D::GetPen()
{
  if (this->DelegateDevice)
  {
    return this->DelegateDevice->GetPen();
  }

  return NULL;
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::ApplyBrush(vtkBrush* brush)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->ApplyBrush(brush);
  }
}

//-----------------------------------------------------------------------------
vtkBrush* vtkBoundingRectContextDevice2D::GetBrush()
{
  if (this->DelegateDevice)
  {
    return this->DelegateDevice->GetBrush();
  }

  return NULL;
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::ApplyTextProp(vtkTextProperty* prop)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->ApplyTextProp(prop);
  }
}

//-----------------------------------------------------------------------------
vtkTextProperty* vtkBoundingRectContextDevice2D::GetTextProp()
{
  if (this->DelegateDevice)
  {
    return this->DelegateDevice->GetTextProp();
  }

  return NULL;
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawPoly(
  float* points, int n, unsigned char* colors, int nc_comps)
{
  this->DrawLines(points, n, colors, nc_comps);
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawLines(
  float* f, int n, unsigned char* vtkNotUsed(colors), int vtkNotUsed(nc_comps))
{
  if (f == NULL)
  {
    return;
  }

  for (int i = 0; i < n; ++i)
  {
    this->AddPoint(f + 2 * i);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawPoints(
  float* points, int n, unsigned char* vtkNotUsed(colors), int vtkNotUsed(nc_comps))
{
  if (points == NULL)
  {
    return;
  }

  for (int i = 0; i < n; ++i)
  {
    this->AddPoint(points + 2 * i);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawPointSprites(vtkImageData* vtkNotUsed(sprite),
  float* points, int n, unsigned char* vtkNotUsed(colors), int vtkNotUsed(nc_comps))
{
  if (points == NULL || this->DelegateDevice)
  {
    return;
  }

  // Point sprites are squares whose sides are the current pen's width
  float penWidth = this->DelegateDevice->GetPen()->GetWidth();
  float halfWidth = 0.5 * penWidth;
  for (int i = 0; i < n; ++i)
  {
    float x = points[2 * i + 0];
    float y = points[2 * i + 1];
    this->AddPoint(x - halfWidth, y - halfWidth);
    this->AddPoint(x + halfWidth, y - halfWidth);
    this->AddPoint(x - halfWidth, y + halfWidth);
    this->AddPoint(x + halfWidth, y + halfWidth);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawMarkers(int vtkNotUsed(shape), bool vtkNotUsed(highlight),
  float* points, int n, unsigned char* colors, int nc_comps)
{
  if (points == NULL)
  {
    return;
  }

  this->DrawPointSprites(NULL, points, n, colors, nc_comps);
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawEllipseWedge(float x, float y, float outRx, float outRy,
  float inRx, float inRy, float startAngle, float stopAngle)
{
  if (outRy == 0.0f && outRx == 0.0f)
  {
    // we make sure maxRadius will never be null.
    return;
  }

  // Add one point per degree of rotation, which is probably good enough.
  while (startAngle <= stopAngle)
  {
    double a = vtkMath::RadiansFromDegrees(startAngle);
    this->AddPoint(outRx * cos(a) + x, outRy * sin(a) + y);
    this->AddPoint(inRx * cos(a) + x, inRy * sin(a) + y);
    startAngle += 1.0;
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::DrawEllipticArc(
  float x, float y, float rX, float rY, float startAngle, float stopAngle)
{
  // Add one point per degree of rotation, which is probably good enough.
  while (startAngle <= stopAngle)
  {
    double a = vtkMath::RadiansFromDegrees(startAngle);
    this->AddPoint(rX * cos(a) + x, rY * sin(a) + y);
    startAngle += 1.0;
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::ComputeStringBounds(
  const vtkStdString& string, float bounds[4])
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->ComputeStringBounds(string, bounds);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::ComputeStringBounds(
  const vtkUnicodeString& string, float bounds[4])
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->ComputeStringBounds(string, bounds);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::ComputeJustifiedStringBounds(
  const char* string, float bounds[4])
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->ComputeJustifiedStringBounds(string, bounds);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::Begin(vtkViewport* viewport)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->Begin(viewport);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::End()
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->End();
  }
}

//-----------------------------------------------------------------------------
bool vtkBoundingRectContextDevice2D::GetBufferIdMode() const
{
  if (this->DelegateDevice)
  {
    return this->DelegateDevice->GetBufferIdMode();
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::BufferIdModeBegin(vtkAbstractContextBufferId* bufferId)
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->BufferIdModeBegin(bufferId);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::BufferIdModeEnd()
{
  if (this->DelegateDevice)
  {
    this->DelegateDevice->BufferIdModeEnd();
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::AddPoint(float x, float y)
{
  float point[2] = { x, y };
  this->AddPoint(point);
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::AddPoint(float pt[2])
{
  if (this->Initialized)
  {
    this->BoundingRect.AddPoint(pt);
  }
  else
  {
    this->Initialized = true;
    this->BoundingRect = vtkRectf(pt[0], pt[1], 0.0f, 0.0f);
  }
}

//-----------------------------------------------------------------------------
void vtkBoundingRectContextDevice2D::AddRect(const vtkRectf& rect)
{
  if (this->Initialized)
  {
    this->BoundingRect.AddRect(rect);
  }
  else
  {
    this->Initialized = true;
    this->BoundingRect = rect;
  }
}
