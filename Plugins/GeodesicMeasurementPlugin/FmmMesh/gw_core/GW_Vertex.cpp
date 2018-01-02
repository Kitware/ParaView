/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Vertex.cpp
 *  \brief  Definition of class \c GW_Vertex
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_Vertex.cpp(c) Gabriel Peyré2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_Vertex.h"
#include "GW_Face.h"
#include "GW_FaceIterator.h"
#include "GW_VertexIterator.h"
#include "GW_Mesh.h"

#ifndef GW_USE_INLINE
    #include "GW_Vertex.inl"
#endif

using namespace GW;

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::operator=
/**
 *  \param  v [GW_Vertex&] Vertex
 *  \return [GW_Vertex&] *this
 *  \author Gabriel Peyré
 *  \date   1-13-2004
 *
 *  Copy operator
 */
/*------------------------------------------------------------------------------*/
GW_Vertex& GW_Vertex::operator=(const GW_Vertex& Vert)
{
    this->Position_ = Vert.Position_;
    this->Normal_ = Vert.Normal_;
    this->CurvDirMin_ = Vert.CurvDirMin_;
    this->CurvDirMax_ = Vert.CurvDirMax_;
    this->rMinCurv_ = Vert.rMinCurv_;
    this->rMaxCurv_ = Vert.rMaxCurv_;
    this->TexCoords_[0] = Vert.TexCoords_[0];
    this->TexCoords_[1] = Vert.TexCoords_[1];
    this->pFace_ = NULL;    // to be set later
    this->nID_ = Vert.nID_;
    return *this;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::SetFace
/**
 *  \param  Face [GW_Face&] The parent face we are managed by.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Set the parent face.
 */
/*------------------------------------------------------------------------------*/
void GW_Vertex::SetFace( GW_Face& Face )
{
    pFace_ = &Face;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetFace
/**
 *  \return [GW_Face*] The face.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Set the face we are managed by.
 */
/*------------------------------------------------------------------------------*/
GW_Face* GW_Vertex::GetFace()
{
    return pFace_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetFace
/**
*  \return [GW_Face*] The face.
*  \author Gabriel Peyré
*  \date   2-15-2003
*
*  Set the face we are managed by.
*/
/*------------------------------------------------------------------------------*/
const GW_Face* GW_Vertex::GetFace() const
{
    return pFace_;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::BeginFaceIterator
/**
 *  \return [GW_FaceIterator] The iterator.
 *  \author Gabriel Peyré
 *  \date   4-1-2003
 *
 *  Begin iterator on the surrounding of the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_FaceIterator GW_Vertex::BeginFaceIterator()
{
    if( this->GetFace()==NULL )
    {
        return this->EndFaceIterator();
    }
    else
    {
        return GW_FaceIterator( this->GetFace(), this, this->GetFace()->GetNextVertex(*this) );
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::EndFaceIterator
/**
 *  \return [GW_FaceIterator] The iterator.
 *  \author Gabriel Peyré
 *  \date   4-1-2003
 *
 *  End iterator for the surrounding of the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_FaceIterator GW_Vertex::EndFaceIterator()
{
    return GW_FaceIterator(NULL,NULL,NULL);
}


/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::BeginVertexIterator
/**
*  \return [GW_VertexIterator] The iterator.
*  \author Gabriel Peyré
*  \date   4-1-2003
*
*  Begin iterator on the surrounding of the vertex.
*/
/*------------------------------------------------------------------------------*/
GW_VertexIterator GW_Vertex::BeginVertexIterator()
{
    if( this->GetFace()==NULL )
    {
        return this->EndVertexIterator();
    }
    else
    {
        return GW_VertexIterator( this->GetFace(), this, this->GetFace()->GetNextVertex(*this), NULL );
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::EndVertexIterator
/**
*  \return [GW_VertexIterator] The iterator.
*  \author Gabriel Peyré
*  \date   4-1-2003
*
*  End iterator for the surrounding of the vertex.
*/
/*------------------------------------------------------------------------------*/
GW_VertexIterator GW_Vertex::EndVertexIterator()
{
    return GW_VertexIterator(NULL,NULL,NULL,NULL);
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetFaces
/**
 *  \param  Vert [GW_Vertex&] The other vertex.
 *  \param  pFace1 [GW_Face*&] First face.
 *  \param  pFace2 [GW_Face*&] Second face.
 *  \author Gabriel Peyré
 *  \date   4-19-2003
 *
 *  Get the face around this vertex and another one.
 */
/*------------------------------------------------------------------------------*/
void GW_Vertex::GetFaces( const GW_Vertex& Vert, GW_Face*& pFace1, GW_Face*& pFace2 )
{
    pFace1 = NULL;
    pFace2 = NULL;
    for( GW_VertexIterator it=this->BeginVertexIterator(); it!=this->EndVertexIterator(); ++it )
    {
        GW_Vertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        if( pVert==&Vert )
        {
            pFace1 = it.GetLeftFace();
            pFace2 = it.GetRightFace();
            return;
        }
    }
}



/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::BuildRawNormal
/**
*  \author Gabriel Peyré
*  \date   4-2-2003
*
*  Compute the normal at the vertex using a very simple scheme.
*/
/*------------------------------------------------------------------------------*/
void GW_Vertex::BuildRawNormal()
{
    GW_Vector3D FaceNormal;

    Normal_.SetZero();
    GW_U32 nIter = 0;
    for( GW_FaceIterator it = this->BeginFaceIterator(); it!=this->EndFaceIterator(); ++it )
    {
        GW_Face* pFace = *it;
        GW_ASSERT( pFace!=NULL );
        FaceNormal =    (pFace->GetVertex(0)->GetPosition()-pFace->GetVertex(1)->GetPosition()) ^
            (pFace->GetVertex(0)->GetPosition()-pFace->GetVertex(2)->GetPosition());
        FaceNormal.Normalize();
        Normal_ += FaceNormal;
        nIter++;
        if( nIter>20 )
            break;
    }
    Normal_.Normalize();
}

GW_Float GW_Vertex::rTotalArea_ = 0;
/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::BuildCurvatureData
/**
*  \author Gabriel Peyré
*  \date   4-2-2003
*
*  Compute all curvature data using comlex schemes. This includes
*  normal, curvatures and curvature directions.
*/
/*------------------------------------------------------------------------------*/
void GW_Vertex::BuildCurvatureData()
{
    if( this->GetFace()==NULL )
    {
        Normal_        = GW_Vector3D(0,0,1);
        CurvDirMin_    = GW_Vector3D(1,0,0);
        CurvDirMax_    = GW_Vector3D(0,1,0);
        rMinCurv_ = rMaxCurv_ = 0;
        return;
    }

    GW_Float rArea;
    this->ComputeNormalAndCurvature( rArea );
    this->ComputeCurvatureDirections( rArea );
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::ComputeNormalAndCurvature
/**
 *  \author Gabriel Peyré
 *  \date   4-14-2003
 *
 *  Compute the normal via local averaging. Compute also the normal.
 */
/*------------------------------------------------------------------------------*/
void GW_Vertex::ComputeNormalAndCurvature( GW_Float& rArea )
{
    GW_Vector3D CurEdge;
    GW_Vector3D CurEdgeNormalized;
    GW_Float rCurEdgeLength;
    GW_Float rCotan;
    GW_Vector3D TempEdge1, TempEdge2;
    GW_Float rTempEdge1Length, rTempEdge2Length;
    GW_Float rAngle, rInnerAngle;
    GW_Vertex* pTempVert = NULL;
    GW_Float rDotP;

    Normal_.SetZero();
    rArea = 0;
    GW_Float rGaussianCurv = 0;

    for( GW_VertexIterator it = this->BeginVertexIterator(); it!=this->EndVertexIterator(); ++it )
    {
        GW_Vertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        CurEdge = pVert->GetPosition() - this->GetPosition();
        rCurEdgeLength = CurEdge.Norm();
        CurEdgeNormalized = CurEdge/rCurEdgeLength;

        /* here we store the value of the sum of the cotan */
        rCotan = 0;

        /* compute left angle */
        pTempVert = it.GetLeftVertex();
        if( pTempVert!=NULL )
        {
            TempEdge1 = this->GetPosition() -  pTempVert->GetPosition();
            TempEdge2 = pVert->GetPosition() - pTempVert->GetPosition();

            TempEdge1.Normalize();
            TempEdge2.Normalize();
            /* we use tan(acos(x))=sqrt(1-x^2)/x */
            rDotP = TempEdge1*TempEdge2;
            if( rDotP!=1 && rDotP!=-1 )
                rCotan += (GW_Float) rDotP/((GW_Float) sqrt(1-rDotP*rDotP));
        }

        /* compute right angle AND Gaussian contribution */
        pTempVert = it.GetRightVertex();
        if( pTempVert!=NULL )
        {
            TempEdge1 = this->GetPosition() -  pTempVert->GetPosition();
            TempEdge2 = pVert->GetPosition() - pTempVert->GetPosition();
            rTempEdge1Length = TempEdge1.Norm();
            rTempEdge2Length = TempEdge2.Norm();
            TempEdge1 /= rTempEdge1Length;
            TempEdge2 /= rTempEdge2Length;
            rAngle      = (GW_Float) acos( -(TempEdge1 * CurEdgeNormalized) );
            /* we use tan(acos(x))=sqrt(1-x^2)/x */
            rDotP = TempEdge1*TempEdge2;
            rInnerAngle = (GW_Float) acos( rDotP  );
            if( rDotP!=1 && rDotP!=-1 )
                rCotan += rDotP/((GW_Float) sqrt(1-rDotP*rDotP));

            rGaussianCurv += rAngle;

            /* compute the contribution to area, testing for special obtuse angle */
            if(       rAngle<GW_HALFPI                            // condition on 1st angle
                && rInnerAngle<GW_HALFPI                    // condition on 2nd angle
                && (GW_PI-rAngle-rInnerAngle)<GW_HALFPI )    // condition on 3rd angle
            {
                /* non-obtuse : 1/8*( |PR|²cot(Q)+|PQ|²cot(R) ) where P=this, Q=pVert, R=pTempVert   */
                rArea += ( rCurEdgeLength*rCurEdgeLength*rDotP/sqrt(1-rDotP*rDotP)
                    + rTempEdge1Length*rTempEdge1Length/tan(GW_PI-rAngle-rInnerAngle) )*0.125;
            }
            else if( rAngle>=GW_HALFPI )
            {
                /* obtuse at the central vertex : 0.5*area(T) */
                rArea += 0.25*rCurEdgeLength*rTempEdge1Length* ~(CurEdgeNormalized^TempEdge1);
            }
            else
            {
                /* obtuse at one of side vertex */
                rArea += 0.125*rCurEdgeLength*rTempEdge1Length* ~(CurEdgeNormalized^TempEdge1);
            }
        }
        GW_CHECK_MATHSBIT();

        /* add the contribution to Normal */
        Normal_ -= CurEdge*rCotan;
    }
    GW_CHECK_MATHSBIT();

    GW_ASSERT( rArea!=0 );    // remove this !

    /* the Gaussian curv */
    rGaussianCurv = (GW_TWOPI - rGaussianCurv)/rArea;
    /* compute Normal and mean curv */
    Normal_ /= 4.0*rArea;
    GW_Float rMeanCurv = Normal_.Norm();
    if( GW_ABS(rMeanCurv)>GW_EPSILON )
    {
        GW_Vector3D Normal = Normal_/rMeanCurv;
        /* see if we need to flip the normal */
        this->BuildRawNormal();
        if( Normal*Normal_<0 )
            Normal_ = -Normal;
        else
            Normal_ = Normal;
    }
    else
    {
        /* we must use another method to compute normal */
        this->BuildRawNormal();
    }

    GW_Vertex::rTotalArea_ += rArea;

    /* compute the two curv values */
    GW_Float rDelta = rMeanCurv*rMeanCurv - rGaussianCurv;
    if( rDelta<0 )
        rDelta = 0;
    rDelta = sqrt(rDelta);
    rMinCurv_ = rMeanCurv - rDelta;
    rMaxCurv_ = rMeanCurv + rDelta;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::ComputeCurvatureDirections
/**
 *  \author Gabriel Peyré
 *  \date   4-14-2003
 *
 *  Compute the principal curvature directions via least square
 *  minimization. The normal should be computed BEFORE.
 */
/*------------------------------------------------------------------------------*/
void GW_Vertex::ComputeCurvatureDirections( GW_Float rArea )
{
    GW_Vector3D CurEdge;
    GW_Vector3D CurEdgeNormalized;
    GW_Float rCurEdgeLength;
    GW_Float rCotan;
    GW_Vector3D TempEdge1, TempEdge2;
    GW_Vertex* pTempVert = NULL;
    GW_Float rDotP;

    /***********************************************************************************/
    /* compute the two curvature directions */
    /* (v1,v2) form a basis of the tangent plante */
    GW_Vector3D v1 = Normal_ ^ GW_Vector3D(0,0,1);
    GW_Float rNorm = v1.Norm();
    if( rNorm<GW_EPSILON )
    {
        /* orthogonalize using another direction */
        v1 = Normal_ ^ GW_Vector3D(0,1,0);
        rNorm = v1.Norm();
        GW_ASSERT( rNorm>GW_EPSILON );
    }
    v1 /= rNorm;
    GW_Vector3D v2 = Normal_ ^ v1;

    /* now we must find the curvature matrix entry by minimising a mean square problem
    the 3 entry of the symmetric curvature matrix in (v1,v2) basis are (a,b,c), stored in vector x.
    IMPORTANT : we must ensure a<c, so that eigenvalues are in correct order. */
    GW_Float a = 0, b = 0, c = 0;            // the vector (a,b,c) we are searching. We use a+c=2*MeanCurv so we don't take care of c.
    GW_Float D[2] = {0,0};                    // the right side of the equation.
    GW_Float M00 = 0, M11 = 0, M01 = 0;        // the positive-definite matrix entries of the mean-square problem.
    GW_Float d1, d2;    // decomposition of current edge on (v1,v2) basis
    GW_Float w, k;        // current weight and current approximated directional curvature.

    /* now build the matrix by iterating around the vertex */
    for( GW_VertexIterator it = this->BeginVertexIterator(); it!=this->EndVertexIterator(); ++it )
    {
        GW_Vertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        CurEdge = pVert->GetPosition() - this->GetPosition();
        rCurEdgeLength = CurEdge.Norm();
        CurEdgeNormalized = CurEdge/rCurEdgeLength;

        /* here we store the value of the sum of the cotan */
        rCotan = 0;

        /* compute projection onto v1,v2 basis */
        d1 = v1*CurEdge;
        d2 = v2*CurEdge;
        rNorm = sqrt(d1*d1 + d2*d2);
        if( rNorm>0 )
        {
            d1 /= rNorm;
            d2 /= rNorm;
        }

        /* compute left angle */
        pTempVert = it.GetLeftVertex();
        if( pTempVert!=NULL )
        {
            TempEdge1 = this->GetPosition() -  pTempVert->GetPosition();
            TempEdge2 = pVert->GetPosition() - pTempVert->GetPosition();
            TempEdge1.Normalize();
            TempEdge2.Normalize();
            /* we use tan(acos(x))=sqrt(1-x^2)/x */
            rDotP = TempEdge1*TempEdge2;
            if( rDotP!=1 && rDotP!=-1 )
                rCotan += rDotP/sqrt(1-rDotP*rDotP);
        }

        /* compute right angle AND Gaussian contribution */
        pTempVert = it.GetRightVertex();
        if( pTempVert!=NULL )
        {
            TempEdge1 = this->GetPosition() -  pTempVert->GetPosition();
            TempEdge2 = pVert->GetPosition() - pTempVert->GetPosition();
            TempEdge1.Normalize();
            TempEdge2.Normalize();
            /* we use tan(acos(x))=sqrt(1-x^2)/x */
            rDotP = TempEdge1*TempEdge2;
            if( rDotP!=1 && rDotP!=-1 )
                rCotan += (GW_Float) rDotP/sqrt(1-rDotP*rDotP);

        }
        GW_CHECK_MATHSBIT();

        /*compute weight */
        w = 0.125/rArea*rCotan*rCurEdgeLength*rCurEdgeLength;
        /* compute directional curvature */
        k = -2*(CurEdge*Normal_)/(rCurEdgeLength*rCurEdgeLength);
        k = k-(rMinCurv_+rMaxCurv_);    // modified by the fact that we use a+c=2*MeanCurv
        /* add contribution to M matrix and D vector*/
        M00        +=   w*(d1*d1-d2*d2)*(d1*d1-d2*d2);
        M11        += 4*w*d1*d1*d2*d2;
        M01        += 2*w*(d1*d1-d2*d2)*d1*d2;
        D[0]    +=   w*k*( d1*d1-d2*d2);
        D[1]    += 2*w*k*d1*d2;
    }
    GW_CHECK_MATHSBIT();

    /* solve the system */
    GW_Float rDet = M00*M11 - M01*M01;
    if( rDet!=0 )
    {
        /*    The inverse matrix is :      | M11 -M01|
                                1/rDet * |-M01  M00| */
        a = 1/rDet * ( M11*D[0] - M01*D[1] );
        b = 1/rDet * (-M01*D[0] + M00*D[1] );
    }

    c = (rMinCurv_+rMaxCurv_) - a;
    // GW_ORDER(a,c);

    /* compute the direction via Givens rotations */
    GW_Float rTheta;
    if( GW_ABS(c-a) < GW_EPSILON )
    {
        if( b==0 )
            rTheta = 0;
        else
            rTheta = GW_HALFPI;
    }
    else
    {
        rTheta = (GW_Float) 2.0f*b/(c-a);
        rTheta = (GW_Float) 0.5f*atan(rTheta);
    }

    GW_CHECK_MATHSBIT();

    CurvDirMin_ = v1*cos(rTheta) - v2*sin(rTheta);
    CurvDirMax_ = v1*sin(rTheta) + v2*cos(rTheta);

    GW_Float vp1 = 0, vp2 = 0;
    if( rTheta!=0 )
    {
        GW_Float r1 = a*cos(rTheta) - b*sin(rTheta);
        GW_Float r2 = b*cos(rTheta) - c*sin(rTheta);
        r1 =(GW_Float)  r1/cos(rTheta);
        r2 = (GW_Float) -r2/sin(rTheta);
        vp1 = r1;
//        GW_ASSERT( GW_ABS(r1-r2)<0.001*GW_ABS(r1) );

        r1 = (GW_Float) a*sin(rTheta) + b*cos(rTheta);
        r2 = (GW_Float) b*sin(rTheta) + c*cos(rTheta);
        r1 = (GW_Float) r1/sin(rTheta);
        r2 = (GW_Float) r2/cos(rTheta);
        vp2 = r2;
//        GW_ASSERT( GW_ABS(r1-r2)<0.001*GW_ABS(r1) );
    }

    if( vp1>vp2 )
    {
        GW_Vector3D vtemp = CurvDirMin_;
        CurvDirMin_ = CurvDirMax_;
        CurvDirMax_ = vtemp;
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetNumberNeighbor
/**
 *  \return [GW_U32] Answer.
 *  \author Gabriel Peyré
 *  \date   6-6-2003
 *
 *  Get the number of neighbors around this vertex.
 */
/*------------------------------------------------------------------------------*/
GW_U32 GW_Vertex::GetNumberNeighbor()
{
    GW_U32 nNum = 0;
    for( GW_VertexIterator it = this->BeginVertexIterator(); it!=this->EndVertexIterator(); ++it )
        nNum++;
    return nNum;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::IsBoundaryVertex
/**
 *  \return [GW_U32] Answer.
 *  \author Gabriel Peyré
 *  \date   6-6-2003
 *
 *  Test if the vertex is a boundary one.
 */
/*------------------------------------------------------------------------------*/
GW_Bool GW_Vertex::IsBoundaryVertex()
{
    for( GW_VertexIterator it = this->BeginVertexIterator(); it!=this->EndVertexIterator(); ++it )
    {
        if( it.GetLeftFace()==NULL || it.GetRightFace()==NULL )
            return GW_True;
    }
    return GW_False;
}


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
