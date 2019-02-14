/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_VertexIterator.cpp
 *  \brief  Definition of class \c GW_VertexIterator
 *  \author Gabriel Peyré
 *  \date   4-2-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_VertexIterator.cpp(c) Gabriel Peyré2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_VertexIterator.h"
#include "GW_Face.h"

using namespace GW;

GW_VertexIterator::GW_VertexIterator(  GW_Face* pFace, GW_Vertex* pOrigin, GW_Vertex* pDirection, GW_Face* pPrevFace, GW_U32 nNbrIncrement )
:    pFace_            ( pFace),
    pOrigin_        ( pOrigin  ),
    pDirection_        ( pDirection ),
    pPrevFace_        ( pPrevFace ),
    nNbrIncrement_    ( nNbrIncrement )
{ }



/* progression : \todo take in account NULL pointer */
void GW_VertexIterator::operator++()
{
    /*
    if( this->nNbrIncrement_>100 )
    {
        GW_ASSERT( GW_False );
        (*this) = GW_VertexIterator(NULL,NULL,NULL,NULL);
        return;
    }*/

    if( pFace_==NULL && pOrigin_!=NULL )
    {
        GW_ASSERT(pDirection_!=NULL);
        /* we are on a border face : Rewind on the first face */
        while( pPrevFace_!=NULL )
        {
            pFace_ = pPrevFace_;
            pPrevFace_  = pPrevFace_->GetFaceNeighbor( *pDirection_ );
            pDirection_ = pFace_->GetVertex( *pOrigin_, *pDirection_ );
        }
        if( pFace_==pOrigin_->GetFace() )
        {
            // we are on End.
            (*this) = GW_VertexIterator(NULL,NULL,NULL,NULL);
        }
        else
        {
            (*this) = GW_VertexIterator( pFace_, pOrigin_, pDirection_, pPrevFace_, nNbrIncrement_+1 );
        }
        return;
    }

    if( pFace_!=NULL && pDirection_!=NULL && pOrigin_!=NULL )
    {
        GW_Face* pNextFace = pFace_->GetFaceNeighbor( *pDirection_ );
        /* check for end() */
        if(  pNextFace==pOrigin_->GetFace() )
        {
            (*this) = GW_VertexIterator(NULL,NULL,NULL,NULL);
        }
        else
        {
            GW_Vertex* pNextDirection = pFace_->GetVertex( *pOrigin_, *pDirection_ );
            GW_ASSERT( pNextDirection!=NULL );
            /* RMK : pNextFace can be NULL in case of a border edge */
            (*this) = GW_VertexIterator( pNextFace, pOrigin_, pNextDirection, pFace_, nNbrIncrement_+1 );
        }
    }
    else
    {
        (*this) = GW_VertexIterator(NULL,NULL,NULL,NULL);
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_VertexIterator::GetLeftFace
/**
 *  \return [GW_Face*] Can be NULL.
 *  \author Gabriel Peyré
 *  \date   4-3-2003
 *
 *  Get the face in the left of the current edge.
 */
/*------------------------------------------------------------------------------*/
GW_Face* GW_VertexIterator::GetLeftFace()
{
    if( pDirection_==NULL )
        return NULL;
    if( pPrevFace_!=NULL )
    {
        /* we must use prev-face because we can be on a border vertex. */
        return pPrevFace_;;
    }
    else
    {
        GW_ASSERT( pFace_!=NULL );
        GW_ASSERT( pOrigin_!=NULL );
        return pFace_->GetFaceNeighbor( *pDirection_,*pOrigin_ );
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_VertexIterator::GetRightFace
/**
*  \return [GW_Face*] Can be NULL.
*  \author Gabriel Peyré
*  \date   4-3-2003
*
*  Get the face in the right of the current edge.
*/
/*------------------------------------------------------------------------------*/
GW_Face* GW_VertexIterator::GetRightFace()
{
    return pFace_;
}


/*------------------------------------------------------------------------------*/
// Name : GW_VertexIterator::GetRightVertex
/**
 *  \return [GW_Vertex*] Can be NULL
 *  \author Gabriel Peyré
 *  \date   4-3-2003
 *
 *  Get the vertex just after.
 */
/*------------------------------------------------------------------------------*/
GW_Vertex* GW_VertexIterator::GetRightVertex()
{
    if( pDirection_==NULL )
        return NULL;
    if( pFace_==NULL )
    {
        return NULL;
    }
    else
    {
        GW_ASSERT( pOrigin_!=NULL );
        return pFace_->GetVertex( *pDirection_,*pOrigin_ );
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_VertexIterator::GetLeftVertex
/**
*  \return [GW_Vertex*] Can be NULL
*  \author Gabriel Peyré
*  \date   4-3-2003
*
*  Get the vertex just before.
*/
/*------------------------------------------------------------------------------*/
GW_Vertex* GW_VertexIterator::GetLeftVertex()
{
    if( pDirection_==NULL )
        return NULL;
    if( pPrevFace_!=NULL )
    {
        /* we must use prev-face because we can be on a border vertex. */
        GW_ASSERT( pOrigin_!=NULL );
        return pPrevFace_->GetVertex( *pDirection_,*pOrigin_ );
    }
    else
    {
        GW_ASSERT( pFace_!=NULL );
        pPrevFace_ = pFace_->GetFaceNeighbor( *pDirection_,*pOrigin_ );
        if( pPrevFace_!=NULL )
            return pPrevFace_->GetVertex( *pDirection_,*pOrigin_ );
        else
            return NULL;
    }
}



///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
