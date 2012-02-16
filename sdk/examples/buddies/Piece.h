/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SIFTEO_BUDDIES_PIECE_H_
#define SIFTEO_BUDDIES_PIECE_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

struct Piece
{
    enum Attribute
    {
        ATTR_NONE = 0,
        ATTR_FIXED,
        ATTR_HIDDEN,
        
        NUM_ATTRIBUTES
    };
    
    Piece()
        : mBuddy(0)
        , mPart(0)
        , mMustSolve(true)
        , mAttribute(ATTR_NONE)
    {
    }
    
    Piece(
        int buddy,
        int part,
        bool must_solve,
        Attribute attribute = ATTR_NONE)
        : mBuddy(buddy)
        , mPart(part)
        , mMustSolve(must_solve)
        , mAttribute(attribute)
    {
    }
    
    int mBuddy;
    int mPart;
    bool mMustSolve;
    Attribute mAttribute;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Compare(const Piece &lhs, const Piece &rhs)
{
    return lhs.mBuddy == rhs.mBuddy && lhs.mPart == rhs.mPart;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
