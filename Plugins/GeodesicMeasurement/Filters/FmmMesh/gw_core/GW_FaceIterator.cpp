#include "stdafx.h"
#include "GW_FaceIterator.h"
#include "GW_Face.h"

using namespace GW;


/* progression : \todo take in account NULL pointer */
void GW_FaceIterator::operator++()
{
    /*!!
    if( nNbrIncrement_>100 )
    {
        GW_ASSERT( GW_False );
        (*this) = GW_FaceIterator(NULL,NULL,NULL);
        return;
    }*/

    if( pFace_!=NULL && pDirection_!=NULL && pOrigin_!=NULL )
    {
        GW_Face* pNextFace = pFace_->GetFaceNeighbor( *pDirection_ );
        /* check for end() */
        if(  pNextFace==pOrigin_->GetFace() )
        {
            (*this) = GW_FaceIterator(NULL,NULL,NULL);
        }
        else
        {
            if( pNextFace==NULL )
            {
                /* we are on a border face : Rewind on the first face */
                GW_Face* pPrevFace = pFace_;
                pDirection_ = pFace_->GetVertex( *pDirection_, *pOrigin_ );    // get rewind direction
                GW_ASSERT( pDirection_!=NULL );

                GW_U32 nIter = 0;
                do
                {
                    pFace_ = pPrevFace;
                    pPrevFace = pPrevFace->GetFaceNeighbor( *pDirection_ );
                    pDirection_ = pFace_->GetVertex( *pOrigin_, *pDirection_ ); // next direction
                    nIter++;
                    GW_ASSERT( nIter<20 );
                    if( nIter>=20 )
                    {
                        // this is on non-manifold ...
                        (*this) = GW_FaceIterator(NULL,NULL,NULL);
                        return;
                    }

                }
                while( pPrevFace!=NULL );

                if( pFace_==pOrigin_->GetFace() )
                {
                    // we are on End.
                    (*this) = GW_FaceIterator(NULL,NULL,NULL);
                }
                else
                {
                    GW_ASSERT( pDirection_!=NULL );
                    (*this) = GW_FaceIterator( pFace_, pOrigin_, pDirection_, nNbrIncrement_+1 );
                }
                return;
            }
            GW_Vertex* pNextDirection = pFace_->GetVertex( *pOrigin_, *pDirection_ );
            GW_ASSERT( pNextDirection!=NULL );
            (*this) = GW_FaceIterator( pNextFace, pOrigin_, pNextDirection, nNbrIncrement_+1 );
        }
    }
    else
    {
        (*this) = GW_FaceIterator(NULL,NULL,NULL);
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_FaceIterator::GetLeftVertex
/**
 *  \return [GW_Vertex*] The vertex.
 *  Get the vertex on the left.
 */
/*------------------------------------------------------------------------------*/
GW_Vertex* GW_FaceIterator::GetLeftVertex()
{
    return pDirection_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_FaceIterator::GetRightVertex
/**
*  \return [GW_Vertex*] The vertex.
*  Get the vertex on the right.
*/
/*------------------------------------------------------------------------------*/
GW_Vertex* GW_FaceIterator::GetRightVertex()
{
    if( pFace_==NULL )
        return NULL;
    return pFace_->GetVertex( *pDirection_, *pOrigin_ );
}
