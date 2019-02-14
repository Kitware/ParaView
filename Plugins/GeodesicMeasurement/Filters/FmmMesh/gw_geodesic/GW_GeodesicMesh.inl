/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicMesh.inl
 *  \brief  Inlined methods for \c GW_GeodesicMesh
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_GeodesicMesh.h"

namespace GW {

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh constructor
/**
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicMesh::GW_GeodesicMesh()
:    GW_Mesh(),
    WeightCallback_        ( GW_GeodesicMesh::BasicWeightCallback ),
    ForceStopCallback_            ( NULL ),
    NewDeadVertexCallback_        ( NULL ),
    VertexInsersionCallback_    ( NULL ),
    HeuristicToGoalCallbackFunction_    ( NULL ),
    bIsMarchingBegin_            ( GW_False ),
    bIsMarchingEnd_                ( GW_False ),
    CallbackData_ (NULL)
{
    /* NOTHING */
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh destructor
/**
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicMesh::~GW_GeodesicMesh()
{
    /* NOTHING */
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::CreateNewVertex
/**
*  \return [GW_Vertex&] The newly created vertex.
*  \author Gabriel Peyré
*  \date   4-9-2003
*
*  Allocate memory for a new vertex. You should overload this
*  method
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vertex& GW_GeodesicMesh::CreateNewVertex()
{
    return *(new GW_GeodesicVertex);
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::CreateNewFace
/**
*  \return [GW_Face&] The newly created face.
*  \author Gabriel Peyré
*  \date   4-9-2003
*
*  Allocate memory for a new face.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Face& GW_GeodesicMesh::CreateNewFace()
{
    return *(new GW_GeodesicFace);
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::AddStartVertex
/**
 *  \param  StartVert [GW_GeodesicVertex&] The new starting point.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Add a new vertex as a starting point for the next fire.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicMesh::AddStartVertex( GW_GeodesicVertex& StartVert )
{
    StartVert.SetFront( &StartVert );
    StartVert.SetDistance(0);
    StartVert.SetState( GW_GeodesicVertex::kAlive );

    NarrowBand::value_type v(0.0f,&StartVert);
    StartVert.ptr = map.insert(v);
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::BasicWeightCallback
/**
 *  \param  Vert [GW_GeodesicVertex&] Current vertex.
 *  \return [GW_Float] 1
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Just the constant function = 1.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_GeodesicMesh::BasicWeightCallback(GW_GeodesicVertex& /*Vert*/, void *)
{
    return 1;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::RegisterForceStopCallbackFunction
/**
 *  \param  pFunc [T_FastMarchingCallbackFunction] The function.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Set the function used to test if we should end the fast marching or not.
 *    The function return GW_False if the algorithm should be stopped.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicMesh::RegisterForceStopCallbackFunction( T_FastMarchingCallbackFunction pFunc )
{
    ForceStopCallback_ = pFunc;
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::RegisterWeightCallbackFunction
/**
*  \param  pFunc [T_WeightCallbackFunction] The function.
*  \author Gabriel Peyré
*  \date   4-10-2003
*
*  Set the function used to define the metric on the mesh.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicMesh::RegisterWeightCallbackFunction( T_WeightCallbackFunction pFunc )
{
    GW_ASSERT( pFunc!=NULL );
    WeightCallback_ = pFunc;
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::RegisterHeuristicToGoalCallbackFunction
/**
 *  \param  pFunc [T_HeuristicToGoalCallbackFunction] Callback function.
 *  \author Gabriel Peyré
 *  \date   3-14-2004
 *
 *  Turn the propagation into an A* like.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicMesh::RegisterHeuristicToGoalCallbackFunction( T_HeuristicToGoalCallbackFunction pFunc )
{
    HeuristicToGoalCallbackFunction_ = pFunc;
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::PerformFastMarchingOneStep
/**
 *  \return [GW_Bool] Is the marching process finished ?
 *  \author Gabriel Peyré
 *  \date   4-13-2003
 *
 *  Just one update step of the marching algorithm.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Bool GW_GeodesicMesh::PerformFastMarchingOneStep()
{
    if (map.empty()) return GW_True;
    GW_ASSERT( bIsMarchingBegin_ );

    NarrowBand::iterator it=map.begin();
    GW_GeodesicVertex* pCurVert = it->second;
    map.erase(it);                    //erase point from 'map', since we'll make it alive
    pCurVert->SetState( GW_GeodesicVertex::kDead );

    if( NewDeadVertexCallback_!=NULL ) NewDeadVertexCallback_( *pCurVert );

    for( GW_VertexIterator VertIt = pCurVert->BeginVertexIterator(); VertIt!=pCurVert->EndVertexIterator(); ++VertIt )
    {
        GW_GeodesicVertex* pNewVert = (GW_GeodesicVertex*) *VertIt;
        GW_ASSERT( pNewVert!=NULL );
        /* compute it's new value */

        if( pCurVert->GetIsStoppingVertex() && !pNewVert->GetIsStoppingVertex() && pNewVert->GetState()==GW_GeodesicVertex::kFar )
        {
            // this vertex is not allowed to add alive vertex that are not stopping.
        }
        else
        {
            /* compute it's new distance using neighborhood information */
            GW_Float rNewDistance = GW_INFINITE;
            for( GW_FaceIterator FaceIt=pNewVert->BeginFaceIterator(); FaceIt!=pNewVert->EndFaceIterator(); ++FaceIt )
            {
                GW_GeodesicFace* pFace = (GW_GeodesicFace*) *FaceIt;
                GW_GeodesicVertex* pVert1 = (GW_GeodesicVertex*) pFace->GetNextVertex( *pNewVert );
                GW_GeodesicVertex* pVert2 = (GW_GeodesicVertex*) pFace->GetNextVertex( *pVert1 );

                if( pVert1->GetDistance()> pVert2->GetDistance() )
                {
                    GW_GeodesicVertex* pTempVert = pVert1;
                    pVert1 = pVert2;
                    pVert2 = pTempVert;
                }
                rNewDistance = GW_MIN( rNewDistance, this->ComputeVertexDistance( *pFace, *pNewVert, *pVert1, *pVert2, *pCurVert->GetFront() ) );
            }
            switch( pNewVert->GetState() ) {
            case GW_GeodesicVertex::kFar:
                /* ask to the callback if we should update this vertex and add it to the path */
                if(VertexInsersionCallback_==NULL || VertexInsersionCallback_( *pNewVert,rNewDistance,CallbackData_ ))
                {
                    pNewVert->SetDistance( rNewDistance );
                    /* add the vertex to the heap */
                    NarrowBand::value_type v((NarrowBand::key_type)rNewDistance,pNewVert);
                    pNewVert->ptr = map.insert(v);

                    /* this one can be added to the heap */
                    pNewVert->SetState( GW_GeodesicVertex::kAlive );
                    pNewVert->SetFront( pCurVert->GetFront() );
                }
                break;
            case GW_GeodesicVertex::kAlive:
                /* just update it's value */
                if( rNewDistance<=pNewVert->GetDistance() )
                {
                    float diff = rNewDistance<pNewVert->GetDistance();
                    /* possible overlap with old value */
                    if( pCurVert->GetFront()!=pNewVert->GetFront() )
                        pNewVert->GetFrontOverlapInfo().RecordOverlap( *pNewVert->GetFront(), pNewVert->GetDistance() );
                    pNewVert->SetDistance( rNewDistance );
                    pNewVert->SetFront( pCurVert->GetFront() );

                    if (diff)
                    {
                      map.erase(pNewVert->ptr);
                      NarrowBand::value_type v((NarrowBand::key_type)rNewDistance,pNewVert);
                      pNewVert->ptr = map.insert(v);        //...and insert it back since its field-value changed
                    }
                }
                else
                {
                    /* possible overlap with new value */
                    if( pCurVert->GetFront()!=pNewVert->GetFront() )
                        pNewVert->GetFrontOverlapInfo().RecordOverlap( *pCurVert->GetFront(), rNewDistance );
                }
                break;
            case GW_GeodesicVertex::kDead:
                /* inform the user if there is an overlap */
                if( pCurVert->GetFront()!=pNewVert->GetFront() )
                    pNewVert->GetFrontOverlapInfo().RecordOverlap( *pCurVert->GetFront(), rNewDistance );
                break;
            default:
                ;
                // Commented out to avoid warnings
                // GW_ASSERT( GW_False );
            }
        }
    }

    /* have we finished ? */
    bIsMarchingEnd_ = map.empty();
    /* the user can force ending of the algorithm */
    if( ForceStopCallback_!=NULL && bIsMarchingEnd_==GW_False )
        bIsMarchingEnd_ = ForceStopCallback_(*pCurVert, CallbackData_);

    return bIsMarchingEnd_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::ComputeVertexDistance
/**
*  \param  CurrentVertex [GW_GeodesicVertex&] The vertex to update.
*  \param  Vert1 [GW_GeodesicVertex&] It's 1st neighbor.
*  \param  Vert2 [GW_GeodesicVertex&] 2nd vertex.
*  \return The value of the distance according to this triangle contribution.
*  \author Gabriel Peyré
*  \date   4-12-2003
*
*  Compute the update of a vertex from inside of a triangle.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_GeodesicMesh::ComputeVertexDistance( GW_GeodesicFace& CurrentFace, GW_GeodesicVertex& CurrentVertex,
                                                 GW_GeodesicVertex& Vert1, GW_GeodesicVertex& Vert2, GW_GeodesicVertex& CurrentFront )
{
    GW_Float F = this->WeightCallback_( CurrentVertex, CallbackData_ );

    GW::GW_GeodesicVertex::T_GeodesicVertexState s1 = Vert1.GetState();
    GW::GW_GeodesicVertex::T_GeodesicVertexState s2 = Vert2.GetState();

    if(s1!=GW_GeodesicVertex::kFar || s2!=GW_GeodesicVertex::kFar )
    {
        GW_Vector3D Edge1 = Vert1.GetPosition() - CurrentVertex.GetPosition();
        GW_Float b = Edge1.Norm();
        Edge1 /= b;
        GW_Vector3D Edge2 = Vert2.GetPosition() - CurrentVertex.GetPosition();
        GW_Float a = Edge2.Norm();
        Edge2 /= a;

        GW_Float d1 = Vert1.GetDistance();
        GW_Float d2 = Vert2.GetDistance();

        /*    Set it if you want only to take in account dead vertex
            during the update step. */
        #define USING_ONLY_DEAD

#ifndef USING_ONLY_DEAD
        GW_Bool bVert1Usable = s1!=GW_GeodesicVertex::kFar && Vert1.GetFront()==&CurrentFront;
        GW_Bool bVert2Usable = s2!=GW_GeodesicVertex::kFar && Vert2.GetFront()==&CurrentFront;
        if( !bVert1Usable && bVert2Usable )
        {
            /* only one point is a contributor */
            return d2 + a * F;
        }
        if( bVert1Usable && !bVert2Usable )
        {
            /* only one point is a contributor */
            return d1 + b * F;
        }
        if( bVert1Usable && bVert2Usable )
        {
#else
        GW_Bool bVert1Usable = s1==GW_GeodesicVertex::kDead && Vert1.GetFront()==&CurrentFront;
        GW_Bool bVert2Usable = s2==GW_GeodesicVertex::kDead && Vert2.GetFront()==&CurrentFront;
        if( !bVert1Usable && bVert2Usable )
        {
            /* only one point is a contributor */
            return d2 + a * F;
        }
        if( bVert1Usable && !bVert2Usable )
        {
            /* only one point is a contributor */
            return d1 + b * F;
        }
        if( bVert1Usable && bVert2Usable )
        {
#endif    // USING_ONLY_DEAD
            GW_Float dot = Edge1*Edge2;

            /*    you can choose whether to use Sethian or my own derivation of the equation.
                Basically, it gives the same answer up to normalization constants */
            #define USE_SETHIAN

            /* first special case for obtuse angles */
            if( dot<0 && bUseUnfolding_ )
            {
                GW_Float c, dot1, dot2;
                GW_GeodesicVertex* pVert = GW_GeodesicMesh::UnfoldTriangle( CurrentFace, CurrentVertex, Vert1, Vert2, c, dot1, dot2 );
                if( pVert!=NULL && pVert->GetState()!=GW_GeodesicVertex::kFar )
                {
                    GW_Float d3 = pVert->GetDistance();
                    GW_Float t;        // newly computed value
                    /* use the unfolded value */
#ifdef USE_SETHIAN
                    t = GW_GeodesicMesh::ComputeUpdate_SethianMethod( d1, d3, c, b, dot1, F );
                    t = GW_MIN( t, GW_GeodesicMesh::ComputeUpdate_SethianMethod( d3, d2, a, c, dot2, F ) );
#else
                    t = GW_GeodesicMesh::ComputeUpdate_MatrixMethod( d1, d3, c, b, dot1, F );
                    t = GW_MIN( t, GW_GeodesicMesh::ComputeUpdate_MatrixMethod( d3, d2, a, c, dot2, F ) );
#endif
                    return t;
                }
            }

#ifdef USE_SETHIAN
                return GW_GeodesicMesh::ComputeUpdate_SethianMethod( d1, d2, a, b, dot, F );
#else
                return GW_GeodesicMesh::ComputeUpdate_MatrixMethod( d1, d2, a, b, dot, F );
#endif

        }
    }

    return GW_INFINITE;
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::ComputeUpdate_SethianMethod
/**
 *  \param  d1 [GW_Float] Distance value at 1st vertex.
 *  \param  d2 [GW_Float] Distance value at 2nd vertex.
 *  \param  a [GW_Float] Length of the 1st edge.
 *  \param  b [GW_Float] Length of the 2nd edge.
 *  \param  dot [GW_Float] Value of the dot product between the 2 edges.
 *  \return [GW_Float] The update value.
 *  \author Gabriel Peyré
 *  \date   5-26-2003
 *
 *  Compute the update value using Sethian's method.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_GeodesicMesh::ComputeUpdate_SethianMethod( GW_Float d1, GW_Float d2, GW_Float a, GW_Float b, GW_Float dot, GW_Float F )
{
    GW_Float t = GW_INFINITE;

    GW_Float rCosAngle = dot;
    GW_Float rSinAngle = sqrt( 1-dot*dot );

    /* Sethian method */
    GW_Float u = d2-d1;        // T(B)-T(A)
//    GW_ASSERT( u>=0 );
    GW_Float f2 = a*a+b*b-2*a*b*rCosAngle;
    GW_Float f1 = b*u*(a*rCosAngle-b);
    GW_Float f0 = b*b*(u*u-F*F*a*a*rSinAngle*rSinAngle);

    /* discriminant of the quartic equation */
    GW_Float delta = f1*f1 - f0*f2;

    if( delta>=0 )
    {
        if( GW_ABS(f2)>GW_EPSILON )
        {
            /* there is a solution */
            t = (-f1 - sqrt(delta) )/f2;
            /* test if we must must choose the other solution */
            if( t<u ||
                b*(t-u)/t < a*rCosAngle ||
                a/rCosAngle < b*(t-u)/t )
            {
                t = (-f1 + sqrt(delta) )/f2;
            }
        }
        else
        {
            /* this is a 1st degree polynom */
            if( f1!=0 )
                t = - f0/f1;
            else
                t = -GW_INFINITE;
        }
    }
    else
        t = -GW_INFINITE;

    /* choose the update from the 2 vertex only if upwind criterion is met */
    if( u<t &&
        a*rCosAngle < b*(t-u)/t &&
        b*(t-u)/t < a/rCosAngle )
    {
        return t+d1;
    }
    else
    {
        return GW_MIN(b*F+d1,a*F+d2);
    }
}




/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::ComputeUpdate_MatrixMethod
/**
*  \param  d1 [GW_Float] Distance value at 1st vertex.
*  \param  d2 [GW_Float] Distance value at 2nd vertex.
*  \param  a [GW_Float] Length of the 1st edge.
*  \param  b [GW_Float] Length of the 2nd edge.
*  \param  dot [GW_Float] Value of the dot product between the 2 edges.
*  \return [GW_Float] The update value.
*  \author Gabriel Peyré
*  \date   5-26-2003
*
*  Compute the update value using a change of basis method.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_GeodesicMesh::ComputeUpdate_MatrixMethod( GW_Float d1, GW_Float d2, GW_Float a, GW_Float b, GW_Float dot, GW_Float F )
{
    GW_Float t;

    /* the directional derivative is D-t*L */
    GW_Vector2D D = GW_Vector2D( d1/b, d2/a);
    GW_Vector2D L = GW_Vector2D( 1/b,  1/a );

    GW_Vector2D QL;    //Q*L
    GW_Vector2D QD;    //Q*L

    GW_Float det = 1-dot*dot;        // 1/det(Q) where Q=(P*P^T)^-1

    QD[0] = 1/det * (      D[0] - dot*D[1] );
    QD[1] = 1/det * (- dot*D[0] +     D[1] );
    QL[0] = 1/det * (      L[0] - dot*L[1] );
    QL[1] = 1/det * (- dot*L[0] +     L[1] );

    /* compute the equation 'e2*t² + 2*e1*t + e0 = 0' */
    GW_Float e2 = QL[0]*L[0] + QL[1]*L[1];            // <L,Q*L>
    GW_Float e1 = -( QD[0]*L[0] + QD[1]*L[1] );        // -<L,Q*D>
    GW_Float e0 = QD[0]*D[0] + QD[1]*D[1] - F*F;    // <D,Q*D> - F²

    GW_Float delta = e1*e1 - e0*e2;

    if( delta>=0 )
    {
        if( GW_ABS(e2)>GW_EPSILON )
        {
            /* there is a solution */
            t = (-e1 - sqrt(delta) )/e2;
            /* upwind criterion : Q*(D-t*l)<=0, i.e. QD<=t*QL */
            if( t<GW_MAX(d1,d2) || QD[0]>t*QL[0] || QD[1]>t*QL[1] )
                t = (-e1 + sqrt(delta) )/e2;    // criterion not respected: choose bigger root.
        }
        else
        {
            if( e1!=0 )
                t = -e0/e1;
            else
                t = -GW_INFINITE;
        }
    }
    else
        t = -GW_INFINITE;
    /* choose the update from the 2 vertex only if upwind criterion is met */
    if( t>=GW_MAX(d1,d2) && QD[0]<=t*QL[0] && QD[1]<=t*QL[1] )
        return t;
    else
        return GW_MIN(b*F+d1,a*F+d2);
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::UnfoldTriangle
/**
 *  \param  CurFace [GW_GeodesicFace&] Vertex to update.
 *  \param  vert [GW_GeodesicVertex&] Current face.
 *  \param  vert1 [GW_GeodesicVertex&] 1st neighbor.
 *  \param  vert2 [GW_GeodesicVertex&] 2nd neighbor.
 *  \return [GW_GeodesicVertex*] The vertex.
 *  \author Gabriel Peyré
 *  \date   5-26-2003
 *
 *  Find a correct vertex to update \c v.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicVertex* GW_GeodesicMesh::UnfoldTriangle( GW_GeodesicFace& CurFace, GW_GeodesicVertex& vert, GW_GeodesicVertex& vert1, GW_GeodesicVertex& vert2,
                                                    GW_Float& dist, GW_Float& dot1, GW_Float& dot2 )
{
    GW_Vector3D& v  = vert.GetPosition();
    GW_Vector3D& v1 = vert1.GetPosition();
    GW_Vector3D& v2 = vert2.GetPosition();

    GW_Vector3D e1 = v1-v;
    GW_Float rNorm1 = ~e1;
    e1 /= rNorm1;
    GW_Vector3D e2 = v2-v;
    GW_Float rNorm2 = ~e2;
    e2 /= rNorm2;

    GW_Float dot = e1*e2;
    GW_ASSERT( dot<0 );

    /* the equation of the lines defining the unfolding region [e.g. line 1 : {x ; <x,eq1>=0} ]*/
    GW_Vector2D eq1 = GW_Vector2D( dot, sqrt(1-dot*dot) );
    GW_Vector2D eq2 = GW_Vector2D(1,0);

    /* position of the 2 points on the unfolding plane */
    GW_Vector2D x1(rNorm1, 0 );
    GW_Vector2D x2 = eq1*rNorm2;

    /* keep track of the starting point */
    GW_Vector2D xstart1 = x1;
    GW_Vector2D xstart2 = x2;

    GW_GeodesicVertex* pV1 = &vert1;
    GW_GeodesicVertex* pV2 = &vert2;
    GW_GeodesicFace* pCurFace = (GW_GeodesicFace*) CurFace.GetFaceNeighbor( vert );


    GW_U32 nNum = 0;
    while( nNum<50 && pCurFace!=NULL )
    {
        GW_GeodesicVertex* pV = (GW_GeodesicVertex*) pCurFace->GetVertex( *pV1, *pV2 );
        GW_ASSERT( pV!=NULL );

        e1 = pV2->GetPosition() - pV1->GetPosition();
        rNorm1 = ~e1;
        e1 /= rNorm1;
        e2 = pV->GetPosition() - pV1->GetPosition();
        rNorm2 = ~e2;
        e2 /= rNorm2;
        /* compute the position of the new point x on the unfolding plane (via a rotation of -alpha on (x2-x1)/rNorm1 )
                | cos(alpha) sin(alpha)|
            x = |-sin(alpha) cos(alpha)| * [x2-x1]*rNorm2/rNorm1 + x1   where cos(alpha)=dot
        */
        GW_Vector2D vv = (x2 - x1)*rNorm2/rNorm1;
        dot = e1*e2;
        GW_Vector2D x = vv.Rotate( -acos(dot) ) + x1;


        /* compute the intersection points.
           We look for x=x1+lambda*(x-x1) or x=x2+lambda*(x-x2) with <x,eqi>=0, so */
        GW_Float lambda11 = - (x1*eq1) / ( (x-x1)*eq1 );    // left most
        GW_Float lambda12 = - (x1*eq2) / ( (x-x1)*eq2 );    // right most
        GW_Float lambda21 = - (x2*eq1) / ( (x-x2)*eq1 );    // left most
        GW_Float lambda22 = - (x2*eq2) / ( (x-x2)*eq2 );    // right most
        GW_Bool bIntersect11 = (lambda11>=0) && (lambda11<=1);
        GW_Bool bIntersect12 = (lambda12>=0) && (lambda12<=1);
        GW_Bool bIntersect21 = (lambda21>=0) && (lambda21<=1);
        GW_Bool bIntersect22 = (lambda22>=0) && (lambda22<=1);
        if( bIntersect11 && bIntersect12 )
        {
//            GW_ASSERT( !bIntersect21 && !bIntersect22 );
            /* we should unfold on edge [x x1] */
            pCurFace = (GW_GeodesicFace*) pCurFace->GetFaceNeighbor( *pV2 );
            pV2 = pV;
            x2 = x;
        }
        else if( bIntersect21 && bIntersect22 )
        {
//            GW_ASSERT( !bIntersect11 && !bIntersect12 );
            /* we should unfold on edge [x x2] */
            pCurFace = (GW_GeodesicFace*) pCurFace->GetFaceNeighbor( *pV1 );
            pV1 = pV;
            x1 = x;
        }
        else
        {
            GW_ASSERT( bIntersect11 && !bIntersect12 &&
                       !bIntersect21 && bIntersect22 );
            /* that's it, we have found the point */
            dist = ~x;
            dot1 = x*xstart1 / (dist * ~xstart1);
            dot2 = x*xstart2 / (dist * ~xstart2);
            return pV;
        }
        nNum++;
    }

    return NULL;
}



/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::RegisterVertexInsersionCallbackFunction
/**
 *  \param  pFunc [T_VertexInsersionCallbackFunction] New function.
 *  \author Gabriel Peyré
 *  \date   5-13-2003
 *
 *  Set the function we use when trying to insert a new vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicMesh::RegisterVertexInsersionCallbackFunction( T_VertexInsersionCallbackFunction pFunc )
{
    VertexInsersionCallback_ = pFunc;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::RegisterNewDeadVertexCallbackFunction
/**
*  \param  pFunc [T_NewDeadVertexCallbackFunction] New function.
*  \author Gabriel Peyré
*  \date   5-13-2003
*
*  Set the function we use when a new dead vertex is set.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicMesh::RegisterNewDeadVertexCallbackFunction( T_NewDeadVertexCallbackFunction pFunc )
{
    NewDeadVertexCallback_ = pFunc;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::SetUseUnfolding
/**
 *  \param  bUseUnfolding [GW_Bool] Use it or not ?
 *  \author Gabriel Peyré
 *  \date   5-26-2003
 *
 *  Set whether to use or not the special handling of obtuse angles
 *  via unfolding.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicMesh::SetUseUnfolding( GW_Bool bUseUnfolding )
{
    bUseUnfolding_ = bUseUnfolding;
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicMesh::GetUseUnfolding
/**
 *  \return [GW_Bool] Answer.
 *  \author Gabriel Peyré
 *  \date   5-27-2003
 *
 *  Does the fast marching computations use unfolding of the obtuse angles ?
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Bool GW_GeodesicMesh::GetUseUnfolding()
{
    return bUseUnfolding_;
}



} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
