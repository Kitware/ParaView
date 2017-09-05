/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeometryCell.inl
 *  \brief  Inlined methods for \c GW_GeometryCell
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 */
/*------------------------------------------------------------------------------*/

#include "GW_GeometryCell.h"

namespace GW {


/*------------------------------------------------------------------------------*/
// Name : GW_GeometryCell::GetInterpolatedPosition
/**
*  \param  ParamMesh [GW_Mesh&] Parameter mesh.
*  \param  RealMesh [GW_Mesh&] Surface.
*  \param  v [GW_Vector3D&] Parameter position.
*  \param  seed [GW_Face*] A seed for the search. Will be modified !
*  \return [GW_Vector3D] The position in 3D space.
*  \author Gabriel Peyré
*  \date   2-4-2004
*
*  Interpolate a position on the surface.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vector3D GW_GeometryCell::GetInterpolatedPosition( GW_Mesh& ParamMesh, GW_Mesh& RealMesh, GW_Vector3D& v, GW_Face* &seed, GW_Vector3D& Normal )
{
    GW_Vector3D lambda(0,0,1);

#if 1
    /* here perform an exhaustive search */
    GW_Face* sel_face;
    GW_Float min_dist = GW_INFINITE;
    GW_Vector3D min_lambda;
    for( GW_U32 i=0; i<ParamMesh.GetNbrFace(); ++i )
    {
        seed = ParamMesh.GetFace(i);
        /* find the barycentric coords according to the cur face */
        GW_Vector3D v1 = seed->GetVertex(0)->GetPosition();
        GW_Vector3D v2 = seed->GetVertex(1)->GetPosition();
        GW_Vector3D v3 = seed->GetVertex(2)->GetPosition();
        GW_Vector2D a = GW_Vector3D(v1 - v3).ToVector2D();
        GW_Vector2D b = GW_Vector3D(v2 - v3).ToVector2D();
        GW_Vector2D c = GW_Vector3D(v - v3).ToVector2D();
        GW_Matrix2x2 M(a,b);    // M map canonical basis of R^3 onto v
        M.AutoInvert();
        GW_Vector2D mu = M*c;
        lambda[0] = mu[0];
        lambda[1] = mu[1];
        lambda[2] = 1-mu[0]-mu[1];
        GW_Float rDist = 0;
        if( lambda[0]<0 )
            rDist -= lambda[0];
        if( lambda[1]<0 )
            rDist -= lambda[1];
        if( lambda[2]<0 )
            rDist -= lambda[2];
        if(  rDist<min_dist)
        {
            min_dist = rDist;
            sel_face = seed;
            min_lambda = lambda;
        }
        if( rDist==0 )        // that's it, we have found the face
            break;
    }

    seed = sel_face;
    lambda = min_lambda;
    // crop
    GW_CLAMP_01( lambda[0] );
    GW_CLAMP_01( lambda[1] );
    GW_CLAMP_01( lambda[2] );
    lambda = lambda/(lambda[0]+lambda[1]+lambda[2]);

#else
    if( seed==NULL )
    {
        GW_Vertex* pVert = ParamMesh.GetRandomVertex();    GW_ASSERT( pVert!=NULL );
        seed = pVert->GetFace();
    }
    while( GW_True )
    {
        /* find the barycentric coords according to the cur face */
        GW_Vector3D v1 = seed->GetVertex(0)->GetPosition();
        GW_Vector3D v2 = seed->GetVertex(1)->GetPosition();
        GW_Vector3D v3 = seed->GetVertex(2)->GetPosition();
        GW_Vector2D a = GW_Vector3D(v1 - v3).ToVector2D();
        GW_Vector2D b = GW_Vector3D(v2 - v3).ToVector2D();
        GW_Vector2D c = GW_Vector3D(v - v3).ToVector2D();
        GW_Matrix2x2 M(a,b);    // M map canonical basis of R^3 onto v
        M.AutoInvert();
        GW_Vector2D mu = M*c;
        lambda[0] = mu[0];
        lambda[1] = mu[1];
        lambda[2] = 1-mu[0]-mu[1];
        if( lambda[0]>=0 && lambda[1]>=0 && lambda[2]>=0 )        // that's it, we have found the face
            break;
        GW_Face* pNextSeed = NULL;
        for( GW_U32 i=0; i<3; ++i )
        {
            if( lambda[i]<0 )
                pNextSeed = seed->GetFaceNeighbor(i);
        }
        if( pNextSeed==NULL )
        {
            // we are on a boundary
            GW_CLAMP_01( lambda[0] );
            GW_CLAMP_01( lambda[1] );
            GW_CLAMP_01( lambda[2] );
            lambda = lambda/(lambda[0]+lambda[1]+lambda[2]);
            break;
        }
        seed = pNextSeed;
    };
#endif

    /* interpolate linearly */
    GW_U32 nID = seed->GetID();
    GW_Face* pRealFace = RealMesh.GetFace(nID);    GW_ASSERT( pRealFace );
    GW_Vector3D pos =    pRealFace->GetVertex(0)->GetPosition()*lambda[0] +
        pRealFace->GetVertex(1)->GetPosition()*lambda[1] +
        pRealFace->GetVertex(2)->GetPosition()*lambda[2];
    Normal = pRealFace->GetVertex(0)->GetNormal()*lambda[0] +
        pRealFace->GetVertex(1)->GetNormal()*lambda[1] +
        pRealFace->GetVertex(2)->GetNormal()*lambda[2];
    Normal.Normalize();
    return pos;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeometryCell::InterpolateAllPositions
/**
 *  \param  ParamMesh [GW_Mesh&] Parameter space.
 *  \param  RealMesh [GW_Mesh&] Real position of 3D meshes.
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 *
 *  Interpolate the position of each point.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeometryCell::InterpolateAllPositions( GW_Mesh& ParamMesh, GW_Mesh& RealMesh  )
{
    GW_Face* seed = NULL;
    for( GW_U32 i=0; i<nSize_[0]; ++i )
    for( GW_U32 j=0; j<nSize_[1]; ++j )
    {
        GW_Vector3D& v = this->GetData(i,j);
        GW_Vector3D Normal;
        GW_Vector3D pos = GW_GeometryCell::GetInterpolatedPosition( ParamMesh, RealMesh, v, seed, Normal );
        this->SetData(i,j, pos);
        this->SetNormal(i,j,Normal);
    }
}


} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
