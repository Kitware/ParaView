#include "stdafx.h"
#include "GW_Face.h"
#include "GW_Mesh.h"

#ifndef GW_USE_INLINE
    #include "GW_Face.inl"
#endif

using namespace GW;

/*------------------------------------------------------------------------------*/
// Name : GW_Face::operator=
/*------------------------------------------------------------------------------*/
GW_Face& GW_Face::operator=(const GW_Face& Face)
{
    this->nID_ = Face.nID_;
    return *this;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetArea
/*------------------------------------------------------------------------------*/
GW_Float GW_Face::GetArea()
{
    if( Vertex_[0]==NULL ) return 0;
    if( Vertex_[1]==NULL ) return 0;
    if( Vertex_[2]==NULL ) return 0;
    GW_Vector3D e1 = this->GetVertex(1)->GetPosition() - this->GetVertex(0)->GetPosition();
    GW_Vector3D e2 = this->GetVertex(2)->GetPosition() - this->GetVertex(0)->GetPosition();
    return 0.5*GW_ABS( ~(e1^e2) );
}
