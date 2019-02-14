/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_TriangularInterpolation_Cubic.cpp
 *  \brief  Definition of class \c GW_TriangularInterpolation_Cubic
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_TriangularInterpolation_Cubic.cpp(c) Gabriel Peyré2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_TriangularInterpolation_Cubic.h"
#include "GW_GeodesicFace.h"

#ifndef GW_USE_INLINE
    #include "GW_TriangularInterpolation_Cubic.inl"
#endif

using namespace GW;


/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Cubic::SetLocalGradient
/**
*  \param  grad [GW_Vector3D] The gradient.
*  \author Gabriel Peyré
*  \date   5-6-2003
*
*  Set the value of the gradient in local coords.
*/
/*------------------------------------------------------------------------------*/
void GW_TriangularInterpolation_Cubic::SetLocalGradient( GW_Vector3D& grad, GW_GeodesicFace& Face, GW_GeodesicVertex& Vert )
{
    this->ComputeLocalBasis( Face );
    GW_I32 nNum = Face.GetEdgeNumber( Vert );
    GW_ASSERT( nNum>=0 );
    LocalGradient_[nNum][0] = grad*u;
    LocalGradient_[nNum][1] = grad*v;
}


/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Cubic constructor
/**
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_TriangularInterpolation_Cubic::GW_TriangularInterpolation_Cubic()
:    bIsLocalBasisComputed_( GW_False )
{
    /* NOTHING */
}


/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Cubic destructor
/**
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_TriangularInterpolation_Cubic::~GW_TriangularInterpolation_Cubic()
{
    /* NOTHING */
}



/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Cubic::SetUpTriangularInterpolation
/**
 *  \param  Face [GW_GeodesicFace&] The face.
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 *
 *  Compute the coefficients of the interpolation.
 */
/*------------------------------------------------------------------------------*/
void GW_TriangularInterpolation_Cubic::SetUpTriangularInterpolation( GW_GeodesicFace& Face )
{
    /* retrieve vertex and length */
    GW_GeodesicVertex* pV0 = (GW_GeodesicVertex*) Face.GetVertex(0);    GW_ASSERT( pV0!=NULL );
    GW_GeodesicVertex* pV1 = (GW_GeodesicVertex*) Face.GetVertex(1);    GW_ASSERT( pV1!=NULL );
    GW_GeodesicVertex* pV2 = (GW_GeodesicVertex*) Face.GetVertex(2);    GW_ASSERT( pV2!=NULL );

    GW_Face* pFace0 = Face.GetFaceNeighbor(0);
    GW_Face* pFace1 = Face.GetFaceNeighbor(1);
    GW_Face* pFace2 = Face.GetFaceNeighbor(2);
    GW_GeodesicVertex *pW0 = NULL, *pW1 = NULL, *pW2 = NULL;

    if( pFace0!=NULL )
        pW0 = (GW_GeodesicVertex*) pFace0->GetVertex( *pV1, *pV2 );
    if( pFace1!=NULL )
        pW1 = (GW_GeodesicVertex*) pFace1->GetVertex( *pV0, *pV2 );
    if( pFace2!=NULL )
        pW2 = (GW_GeodesicVertex*) pFace2->GetVertex( *pV0, *pV1 );

    GW_Vector3D& V0 = pV0->GetPosition();
    GW_Vector3D& V1 = pV1->GetPosition();
    GW_Vector3D& V2 = pV2->GetPosition();
    GW_Vector3D W0, W1, W2;

    if( pW0!=NULL )
        W0 = pW0->GetPosition();
    else
        W0 = (V1+V2)*0.5;
    if( pW1!=NULL )
        W1 = pW1->GetPosition();
    else
        W1 = (V0+V1)*0.5;
    if( pW2!=NULL )
        W2 = pW2->GetPosition();
    else
        W2 = (V0+V1)*0.5;

    /* edge of the main triangle */
    GW_Vector3D e0 = V0-V2;
    GW_Vector3D e1 = V1-V2;
    GW_Vector3D e2 = V1-V0;
    /* edge of side triangles */
    GW_Vector3D s0 = W0 - V2;
    GW_Vector3D s1 = W1 - V2;
    GW_Vector3D s2 = W2 - V0;

    GW_Float l0 = ~e0;
    GW_Float l1 = ~e1;
    GW_Float l2 = ~e2;
    GW_Float m0 = ~s0;
    GW_Float m1 = ~s1;
    GW_Float m2 = ~s2;


    /* compute the orthonormal basis in which the interpolation is performed */
    u = e0/l0;
    v = ( (u^e1)^u );
    v.Normalize();
    w = V2;    // origin

    /* now compute angles */
    GW_Float a = acos( (e0*e1)/(l0*l1) );
    GW_Float b = acos( (e1*s0)/(l1*m0) );
    GW_Float c = acos( (e0*s1)/(l0*m1) );
    GW_Float d = acos(-(e0*e2)/(l0*l2) );
    GW_Float e = acos( (e2*s2)/(l2*m2) );

    /* compute 2D position of points.
    Point 0,1,2 are V0,V1,V2    Points 3,4,5 are W0,W1,W2 */
    GW_Float Points[6][2];

    Points[0][0] = l0;
    Points[0][1] = 0;

    Points[1][0] = l1*cos(a);
    Points[1][1] = l1*sin(a);

    Points[2][0] = 0;
    Points[2][1] = 0;


    Points[3][0] =  m0*cos(a+b);
    Points[3][1] =  m0*sin(a+b);

    Points[4][0] =  m1*cos(c);
    Points[4][1] = -m1*sin(c);

    Points[5][0] =  l0 - m2*cos(d+e);
    Points[5][1] =  m2*sin(d+e);

    /* compute values */
    GW_Float Values[6];
    Values[0] = pV0->GetDistance();
    Values[1] = pV1->GetDistance();
    Values[2] = pV2->GetDistance();

    if( pW0!=NULL )
        Values[3] = pW0->GetDistance();
    else
        Values[3] = (Values[1]+Values[2])*0.5;

    if( pW1!=NULL )
        Values[4] = pW1->GetDistance();
    else
        Values[4] = (Values[0]+Values[2])*0.5;

    if( pW2!=NULL )
        Values[5] = pW2->GetDistance();
    else
        Values[5] = (Values[0]+Values[1])*0.5;

    /* compute the coefficients, given in that order : 0->cst, 1->X, 2->Y, 3->XY, 4->X^2, 5->Y^2. */
    GW_Maths::Fit2ndOrderPolynomial2D( Points, Values, Coeffs );
}



/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Cubic::ComputeGradient
/**
 *  \param  v0 [GW_GeodesicVertex&] 1st vertex of local frame.
 *  \param  v1 [GW_GeodesicVertex&] 2nd vertex.
 *  \param  v2 [GW_GeodesicVertex&] 3rd vertex.
 *  \param  x [GW_Float] x local coord.
 *  \param  y [GW_Float] y local coord.
 *  \param  dx [GW_Float&] x coord of the gradient in local coord.
 *  \param  dy [GW_Float&] y coord of the gradient in local coord.
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 *
 *  Compute the gradient at given point in local frame.
 */
/*------------------------------------------------------------------------------*/
void GW_TriangularInterpolation_Cubic::ComputeGradient( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2, GW_Float x, GW_Float y, GW_Float& dx, GW_Float& dy )
{
    /* compute local basis */
    GW_Vector3D& e = v2.GetPosition();            // origin
    GW_Vector3D e0 = v0.GetPosition() - e;
    GW_Vector3D e1 = v1.GetPosition() - e;

    /* compute (s,t) the parameter of the point M in basis (w;u,v). We use equality :
        M = w + s u + t v = e + x e0 + y e1     so :
        s = x <e0,u> + y <e1,u> + <e-w,u>
        t = x <e0,v> + y <e1,v> + <e-w,v>
    i.e. :
        |s|   |<e0,u> <e1,u>| |x|   |<e-w,u>|
        |t| = |<e0,v> <e1,v>|*|y| + |<e-w,v>|
    */
    GW_Vector3D trans = e-w;
    /* compute passage matrix */
    GW_Float p00 = e0*u;
    GW_Float p01 = e1*u;
    GW_Float p10 = e0*v;
    GW_Float p11 = e1*v;
    GW_Float s = x*p00 + y*p01 + (trans*u);
    GW_Float t = x*p10 + y*p11 + (trans*v);

    /* now we can compute the gradient in orthogonal basis (w;u,v) */
    GW_Float gu = Coeffs[1] + Coeffs[3]*t + Coeffs[4]*2*s;
    GW_Float gv = Coeffs[2] + Coeffs[3]*s + Coeffs[5]*2*t;

    /* convert from orthogonal basis to local basis :
        |<e0,u> <e1,u>| |dx|   |gu|
        |<e0,v> <e1,v>|*|dy| = |gv|
    ie :
        |dx|           | p11 -p01| |gu|
        |dy| = 1/det * |-p10  p00|*|gv|    */
    GW_Float rDet = p00*p11 - p01*p10;
    GW_ASSERT( rDet!=0 );
    dx = 1/rDet * ( p11*gu - p01*gv ) * ~e0;
    dy = 1/rDet * (-p10*gu + p00*gv ) * ~e1;
}


/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Cubic::ComputeValue
/**
 *  \param  v0 [GW_GeodesicVertex&] 1st vertex of local frame.
 *  \param  v1 [GW_GeodesicVertex&] 2nd vertex.
 *  \param  v2 [GW_GeodesicVertex&] 3rd vertex.
 *  \param  x [GW_Float] x local coord.
 *  \param  y [GW_Float] y local coord.
 *  \return  value of the distance function.
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 *
 *  Compute the value at a given location in the triangle.
 */
/*------------------------------------------------------------------------------*/
GW_Float GW_TriangularInterpolation_Cubic::ComputeValue( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2, GW_Float x, GW_Float y )
{
    /* compute local basis */
    GW_Vector3D& e = v2.GetPosition();            // origin
    GW_Vector3D e0 = v0.GetPosition() - e;
    GW_Vector3D e1 = v1.GetPosition() - e;

    /* compute (s,t) the parameter of the point M in basis (w;u,v). We use equality :
        M = w + s u + t v = e + x e0 + y e1     so :
        s = x <e0,u> + y <e1,u> + <e-w,u>
        t = x <e0,v> + y <e1,v> + <e-w,v>
    i.e. :
        |s|   |<e0,u> <e1,u>| |x|   |<e-w,u>|
        |t| = |<e0,v> <e1,v>|*|y| + |<e-w,v>|
    */
    GW_Vector3D trans = e-w;
    /* compute passage matrix */
    GW_Float p00 = e0*u;
    GW_Float p01 = e1*u;
    GW_Float p10 = e0*v;
    GW_Float p11 = e1*v;
    GW_Float s = x*p00 + y*p01 + (trans*u);
    GW_Float t = x*p10 + y*p11 + (trans*v);

    /* now we can compute the value in orthogonal basis (w;u,v) */
    return Coeffs[0] + Coeffs[1]*s + Coeffs[2]*t + Coeffs[3]*s*t + Coeffs[4]*s*s + Coeffs[5]*t*t;
}


/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Cubic::ComputeLocalBasis
/**
 *  \param  Face [GW_GeodesicFace&] The face.
 *  \author Gabriel Peyré
 *  \date   5-6-2003
 *
 *  Compute the local basis of the face.
 */
/*------------------------------------------------------------------------------*/
void GW_TriangularInterpolation_Cubic::ComputeLocalBasis( GW_GeodesicFace& Face )
{
    if( !bIsLocalBasisComputed_ )
    {
        GW_GeodesicVertex* pV0 = (GW_GeodesicVertex*) Face.GetVertex(0);    GW_ASSERT( pV0!=NULL );
        GW_GeodesicVertex* pV1 = (GW_GeodesicVertex*) Face.GetVertex(1);    GW_ASSERT( pV1!=NULL );
        GW_GeodesicVertex* pV2 = (GW_GeodesicVertex*) Face.GetVertex(2);    GW_ASSERT( pV2!=NULL );
        GW_Vector3D& V0 = pV0->GetPosition();
        GW_Vector3D& V1 = pV1->GetPosition();
        GW_Vector3D& V2 = pV2->GetPosition();
        GW_Vector3D e0 = V0-V2;
        GW_Vector3D e1 = V1-V2;
        GW_Vector3D e2 = V1-V0;
        u = e0/e0.Norm();
        v = ( (u^e1)^u );
        v.Normalize();
        w = V2;
    }
    bIsLocalBasisComputed_ = GW_True;
}



/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Cubic::ComputeLocalGradient
/**
 *  \param  Vert [GW_GeodesicVertex&] The vertex.
 *  \author Gabriel Peyré
 *  \date   5-6-2003
 *
 *  Compute the gradient using a least square approximation on the edges around.
 */
/*------------------------------------------------------------------------------*/
void GW_TriangularInterpolation_Cubic::ComputeLocalGradient(  GW_GeodesicVertex& Vert )
{
    /* compute the total angle */
    GW_Vector3D PrevEdge;
    GW_Float rTotalAngle = 0;
    for( GW_VertexIterator it=Vert.BeginVertexIterator(); it!=Vert.EndVertexIterator(); ++it )
    {
        GW_Vertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        if( it==Vert.BeginVertexIterator() )
        {
            PrevEdge = pVert->GetPosition() - Vert.GetPosition();
            PrevEdge.Normalize();
        }
        else
        {
            GW_Vector3D NextEdge = pVert->GetPosition() - Vert.GetPosition();
            NextEdge.Normalize();
            rTotalAngle += acos( NextEdge*PrevEdge );
            PrevEdge = NextEdge;
        }
    }

    /* matrix and RHS for least square minimusation */
    GW_Float M[2][2] = {{0,0},{0,0}};
    GW_Float b[2] = {0,0};

    GW_Float rCurAngle = 0;
    PrevEdge.SetZero();
    for( GW_VertexIterator it=Vert.BeginVertexIterator(); it!=Vert.EndVertexIterator(); ++it )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) *it;
        GW_ASSERT( pVert!=NULL );

        GW_Vector3D Edge = pVert->GetPosition() - Vert.GetPosition();
        GW_Float a = Edge.Norm();
        Edge /= a;

        if( it!=Vert.BeginVertexIterator() )
        {
            /* update the angle */
            rCurAngle += acos( Edge*PrevEdge );
        }

        /* directional gradient estimation */
        GW_Float delta = (pVert->GetDistance() - Vert.GetDistance())/a;
        /* coordinate of the edge on (u,v) [flattened coords] */
        GW_Float eu = a*cos( rCurAngle/rTotalAngle );
        GW_Float ev = a*sin( rCurAngle/rTotalAngle );
        /* update the matrix */
        M[0][0] += eu*eu;
        M[0][1] += eu*ev;
        M[1][0] += eu*ev;
        M[1][1] += ev*ev;
        b[0] += delta*eu;
        b[1] += delta*ev;

        PrevEdge = Edge;
    }

    /* invert the system */
    GW_Float det = M[0][0]*M[1][1] - M[0][1]*M[1][0];
    GW_ASSERT( det!=0 );
    GW_Float gu = 1/det * ( M[1][1]*b[0] - M[0][1]*b[1] );
    GW_Float gv = 1/det * (-M[1][0]*b[0] + M[0][0]*b[1] );

    /* set the gradient in local coords for each surrounding face */
    rCurAngle = 0;
    for( GW_FaceIterator it = Vert.BeginFaceIterator(); it!=Vert.EndFaceIterator(); ++it )
    {
        GW_GeodesicFace* pFace = (GW_GeodesicFace*) *it;
        GW_ASSERT( pFace!=NULL );
        GW_Vertex* pVert1 = it.GetLeftVertex();        GW_ASSERT( pVert1!=NULL );
        GW_Vertex* pVert2 = it.GetRightVertex();    GW_ASSERT( pVert1!=NULL );
        GW_Vector3D e1 = pVert1->GetPosition() - Vert.GetPosition();
        GW_Vector3D e2 = pVert2->GetPosition() - Vert.GetPosition();
        GW_Float a1 = e1.Norm();
        GW_Float a2 = e2.Norm();
        e1 /= a1;
        e2 /= a2;
        GW_Float rInnerAngle = acos( e1*e2 );
        /* flattened position of the two vertex */
        GW_Float p1[2], p2[2];
        p1[0] = cos( rCurAngle );
        p1[1] = sin( rCurAngle );
        p2[0] = cos( rCurAngle+rInnerAngle );
        p2[1] = sin( rCurAngle+rInnerAngle );

        /* we have                    grad = gu*u + gv*v
           we are searching for        grad = g1*p1 + g2*p2, so:
                gu = g1*<p1,u> + g2*<p2,u>
                gv = g1*<p1,v> + g2*<p2,v>
            i.e.
                |p1[0] p2[0]| |g1|   |gu|
                |p1[1] p2[1]|*|g2| = |gv|
        */
        det = p1[0]*p2[1]-p1[1]*p2[0];
        GW_ASSERT( det!=0 );
        GW_Float g1 = 1/det * ( p2[1]*gu - p2[0]*gv );
        GW_Float g2 = 1/det * (-p1[1]*gu + p1[0]*gv );

        /* now compute the gradient in world coords */
        GW_Vector3D LocGrad = e1*g1 + e2*g2;

        GW_TriangularInterpolation_ABC* pInterp = pFace->GetTriangularInterpolation();
        if( pInterp==NULL )
        {
            pInterp = new GW_TriangularInterpolation_Cubic;
            pFace->SetTriangularInterpolation( *pInterp );
        }
        GW_ASSERT( pInterp->GetType()==kCubicTriangulationInterpolation );

        ((GW_TriangularInterpolation_Cubic*) pInterp)->SetLocalGradient( LocGrad, *pFace, Vert );

        rCurAngle += rInnerAngle;
    }

}





///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
