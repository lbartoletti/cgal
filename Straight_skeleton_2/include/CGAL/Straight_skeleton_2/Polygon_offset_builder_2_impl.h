// Copyright (c) 2005, 2006 Fernando Luis Cacciola Carballal. All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
// Author(s)     : Fernando Cacciola <fernando_cacciola@ciudad.com.ar>
//
#ifndef CGAL_POLYGON_OFFSET_BUILDER_2_IMPL_H
#define CGAL_POLYGON_OFFSET_BUILDER_2_IMPL_H 1

CGAL_BEGIN_NAMESPACE


template<class Ss, class Gt, class Cont, class Visitor>
Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::Polygon_offset_builder_2( Ss const& aSs, Traits const& aTraits, Visitor const& aVisitor )
  :
   mTraits (aTraits)
  ,mVisitor(aVisitor)
{

  int lMaxID = -1 ;

  for ( Halfedge_const_handle lHE = aSs.halfedges_begin() ; lHE != aSs.halfedges_end() ; ++ lHE )
  {
    if ( lHE->id() > lMaxID )
      lMaxID = lHE->id() ;

    if ( !lHE->is_bisector() && handle_assigned(lHE->face()) )
      mBorders.push_back(lHE);
  }

  CGAL_POLYOFFSET_TRACE(2, "Border count: " << mBorders.size() ) ;

  CGAL_POLYOFFSET_TRACE(2, "Highest Bisector ID: " << lMaxID ) ;

  mBisectorData.resize(lMaxID+1);

  ResetBisectorData();
}


template<class Ss, class Gt, class Cont, class Visitor>
typename Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::Halfedge_const_handle
Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::LocateHook( FT                    aTime
                                                        , Halfedge_const_handle aBisector 
                                                        , bool                  aIncludeLastBisector
                                                        , Hook_position&        rPos
                                                        )
{
  CGAL_POLYOFFSET_TRACE(2,"Searching for hook at " << aTime ) ;

  Halfedge_const_handle rHook ;

  while ( aBisector->is_bisector() && ( aIncludeLastBisector ? true : aBisector->prev()->is_bisector() ) )
  {
    Halfedge_const_handle lPrev = aBisector->prev();
    Halfedge_const_handle lNext = aBisector->next();
    
    CGAL_POLYOFFSET_TRACE(2,"Testing hook on " << e2str(*aBisector) ) ;
                          
    CGAL_POLYOFFSET_TRACE(4, "Next: " << e2str(*lNext) << " - Prev: " << e2str(*lPrev) ) ;
                          
    if ( !IsVisited(aBisector) )
    {
      if ( aBisector->slope() != ZERO )
      {
        // A hook is found here if 'aTime' is within the bisector time interval.
        //
        // Depending on the bisector slope, src-time might be smaller or larger than tgt-time,
        // so the test is:
        //
        //  (src-time <= time <= tgt-time ) || ( tgt-time <= time <= src-time )
        //
        
        Comparison_result lTimeWrtSrcTime = lPrev->is_bisector() ? Compare_offset_against_event_time(aTime,lPrev    ->vertex()) : LARGER ;
        Comparison_result lTimeWrtTgtTime = lNext->is_bisector() ? Compare_offset_against_event_time(aTime,aBisector->vertex()) : LARGER ;
        CGAL_POLYOFFSET_TRACE(3,"  TimeWrtSrcTime: " << lTimeWrtSrcTime << " TimeWrtTgtTime: " << lTimeWrtTgtTime ) ;
        
        //
        // The above test expressed in terms of comparisons of src/tgt time against aTime is:
        //
        // (     ( time-wrt-src-time == ZERO || time-wrt-src-time == SMALLER )
        //    && ( time-wrt-tgt-time == ZERO || time-wrt-tgt-time == LARGER  )
        // )
        //
        // || -the same with src/tgt inverted-
        //
        // (      ( time-wrt-tgt-time == ZERO || time-wrt-tgt-time == SMALLER )
        //     && ( time-wrt-src-time == ZERO || time-wrt-src-time == LARGER  )
        // )
        //
        // But since bisectors of slope zero are skipped, both comparisons cannot be zero, thus, the test above is really: 
        //
        //    ( ( time-wrt-src-time == ZERO || time-wrt-src-time == SMALLER )  && (                              time-wrt-tgt-time == LARGER ) )
        // || ( (                              time-wrt-src-time == SMALLER )  && ( time-wrt-tgt-time == ZERO || time-wrt-tgt-time == LARGER ) )
        // || ( ( time-wrt-tgt-time == ZERO || time-wrt-tgt-time == SMALLER )  && (                              time-wrt-src-time == LARGER ) )
        // || ( (                              time-wrt-tgt-time == SMALLER )  && ( time-wrt-src-time == ZERO || time-wrt-src-time == LARGER ) )
        // 
        // Which actually boils down to this:
        //
        if ( lTimeWrtSrcTime != lTimeWrtTgtTime )
        {
          CGAL_stskel_intrinsic_test_assertion( !CGAL_SS_i::is_time_clearly_not_within_possibly_inexact_bisector_time_interval(aTime,aBisector) ) ;
          
          bool lLocalPeak = false ;
          
          if ( aBisector->slope() == POSITIVE && lTimeWrtSrcTime == EQUAL )
          {
            Halfedge_const_handle lPrev = aBisector->prev();
            while ( lPrev->is_bisector() && ( lPrev->slope() == ZERO ) )
             lPrev = lPrev->prev();
             
            lLocalPeak = ( lPrev->slope() == NEGATIVE ) ;
          }   
        
          if ( !lLocalPeak )
          {
            rPos = ( lTimeWrtTgtTime == EQUAL ? TARGET : lTimeWrtSrcTime == EQUAL ? SOURCE : INSIDE ) ; 
            
            rHook = aBisector ;
            
            CGAL_POLYOFFSET_TRACE(2, "  Hook found here at " << Hook_position2Str(rPos) ) ;
            
            break ;
          }
          else
          {
            CGAL_POLYOFFSET_TRACE(2, "  Hook found here local peak. Ignored." ) ;
          }
        }
        else
        {
          CGAL_stskel_intrinsic_test_assertion( !CGAL_SS_i::is_time_clearly_within_possibly_inexact_bisector_time_interval(aTime,aBisector) ) ;
          
          CGAL_POLYOFFSET_TRACE(2, "  Hook not found here.") ;
        }
      }  
      else
      {
        CGAL_POLYOFFSET_TRACE(2,"Bisector is a roof peak.");  
      }
    }
    else
    {
      CGAL_POLYOFFSET_TRACE(2,"Bisector already visited");  
    }
    aBisector = lPrev ;
  }

  return rHook;
}

template<class Ss, class Gt, class Cont, class Visitor>
typename Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::Halfedge_const_handle
Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::LocateSeed( FT aTime, Halfedge_const_handle aBorder )
{
  CGAL_POLYOFFSET_TRACE(2,"\nLocating seed for face " << e2str(*aBorder) ) ;
    
  Hook_position lPos ;
  Halfedge_const_handle rSeed = LocateHook(aTime,aBorder->prev(),false,lPos);
  if ( handle_assigned(rSeed) )
  {
    if ( !IsUsedSeed(rSeed) )
    {
      SetIsUsedSeed(rSeed);
      
      CGAL_postcondition( rSeed->prev()->is_bisector() ) ;
      
      // If a seed hook is found right at a bisector source, 
      // the next hook will be found right at the prev bisector's target, which would be a mistake,
      // so we ajust the seed as the (target) of the prev 
      if ( lPos == SOURCE )
        rSeed = rSeed->prev() ;
    }
    else
    {
      CGAL_POLYOFFSET_TRACE(2,"Seed already used. Discarded");
      rSeed = Halfedge_const_handle();
    }
  }
  return rSeed ;
}


template<class Ss, class Gt, class Cont, class Visitor>
typename Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::Halfedge_const_handle
Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::LocateSeed( FT aTime )
{
  CGAL_POLYOFFSET_TRACE(2,"Searching for seed at " << aTime ) ;

  Halfedge_const_handle rSeed ;

  for ( typename Halfedge_vector::const_iterator f = mBorders.begin()
       ; f != mBorders.end() && !handle_assigned(rSeed)
       ; ++ f
      )
    rSeed = LocateSeed(aTime,*f);
  
  CGAL_POLYOFFSET_TRACE(2,"Seed:" << eh2str(rSeed) ) ;
  
  return rSeed;
}


template<class Ss, class Gt, class Cont, class Visitor>
void Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::AddOffsetVertex( FT                    aTime
                                                                  , Halfedge_const_handle aHook
                                                                  , ContainerPtr          aPoly 
                                                                  )
{

  OptionalPoint_2_twotuple lPts = Construct_offset_point(aTime,aHook);

  if ( ! lPts )
  {
    OptionalPoint_2 lP = mVisitor.on_offset_point_overflowed(aHook) ;
    if ( lP )
      lPts = boost::make_tuple(*lP,*lP);
  }
    
  CGAL_postcondition(lPts);
  
  Point_2 lP1, lP2 ;
  boost::tie(lP1,lP2) = *lPts ;

  CGAL_POLYOFFSET_TRACE(1,"Found offset point p=" << p2str(lP1) << " at offset " << aTime << " along bisector " << e2str(*aHook) << " reaching " << v2str(*aHook->vertex()) ) ;
  
  mVisitor.on_offset_point(lP1);
  
  if ( lP1 != mLastPoint )
  {
    aPoly->push_back(lP1);
    mLastPoint = lP1 ;
  }
  else
  {
    CGAL_POLYOFFSET_TRACE(1,"Duplicate point. Ignored");
  }
  
  if ( lP1 != lP2 )
  {
    CGAL_POLYOFFSET_TRACE(1,"TWIN degenerate offset point p=" << p2str(lP2) << " also found" ) ;
  
    mVisitor.on_offset_point(lP2);
  
    aPoly->push_back(lP2);
    
    mLastPoint = lP2 ;
  }

  CGAL_POLYOFFSET_DEBUG_CODE( ++ mStepID ) ;
}

template<class Ss, class Gt, class Cont, class Visitor>
template<class OutputIterator>
OutputIterator Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::TraceOffsetPolygon( FT aTime, Halfedge_const_handle aSeed, bool aIsOpen, OutputIterator aOut )
{
  CGAL_POLYOFFSET_TRACE(1,"\nTracing new offset polygon" ) ;

  ContainerPtr lPoly( new Container() ) ;

  mVisitor.on_offset_contour_started();
  
  if ( aIsOpen)
  {
    AddOffsetVertex(aTime,aSeed->opposite(), lPoly);
 }
  
  Halfedge_const_handle lHook = aSeed ;

  do
  {
    CGAL_POLYOFFSET_TRACE(1,"STEP " << mStepID ) ;
    
    Halfedge_const_handle lLastHook = lHook ;
    Hook_position lPos ;
    lHook = LocateHook(aTime,lHook->prev(),true,lPos) ;
    Visit(lLastHook);
    
    if ( handle_assigned(lHook) ) 
    {
      CGAL_POLYOFFSET_TRACE(1,"B" << lLastHook->id() << " and B" << lHook->id() << " visited." ) ;
      
      if ( !aIsOpen || lHook->opposite() != aSeed )
      {
        AddOffsetVertex(aTime,lHook, lPoly);
        Visit(lHook);
      }  

      lHook = lHook->opposite();
    }
    
  }
  while ( handle_assigned(lHook) && lHook != aSeed  && !IsVisited(lHook)) ;

  bool lComplete = aIsOpen || ( !aIsOpen && ( lHook == aSeed ) ) ;
  
  CGAL_POLYOFFSET_TRACE(1,"Offset polygon of " << lPoly->size() << " vertices traced." << ( lComplete ? "COMPLETE" : "INCOMPLETE" ) ) ;
  
  CGAL_assertion ( !lComplete || ( lComplete && lPoly->size() >= 2 ) ) ;
  
  mVisitor.on_offset_contour_finished( lComplete );
  
  if ( lComplete )
    *aOut++ = lPoly ;

  return aOut ;
}

template<class Ss, class Gt, class Cont, class Visitor>
void Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::ResetBisectorData()
{
  std::fill(mBisectorData.begin(),mBisectorData.end(), Bisector_data() );
}

template<class Ss, class Gt, class Cont, class Visitor>
template<class OutputIterator>
OutputIterator Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::construct_offset_contours( FT aTime, OutputIterator aOut )
{
  CGAL_precondition( aTime > static_cast<FT>(0.0) ) ;

  CGAL_POLYOFFSET_DEBUG_CODE( mStepID = 0 ) ;

  mVisitor.on_construction_started(aTime);
  
  mLastPoint = boost::none ;
  
  ResetBisectorData();

  CGAL_POLYOFFSET_TRACE(1,"Constructing offset polygons for offset: " << aTime ) ;
  for ( Halfedge_const_handle lSeed = LocateSeed(aTime); handle_assigned(lSeed); lSeed = LocateSeed(aTime) )
  {
    bool lIsOpen = IsSeedLeftTerminal(lSeed) ;
    aOut = TraceOffsetPolygon(aTime,lSeed,lIsOpen,aOut);
  }
  mVisitor.on_construction_finished();
  
  return aOut ;
}

template<class Ss, class Gt, class Cont, class Visitor>
typename Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::Trisegment_2_ptr
Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::CreateTrisegment ( Vertex_const_handle aNode ) const
{
  CGAL_precondition(handle_assigned(aNode));
  
  Trisegment_2_ptr r ;
  
  CGAL_POLYOFFSET_TRACE(3,"Creating Trisegment for " << v2str(*aNode) ) ;
  
  if ( aNode->is_skeleton() )
  {
    Triedge const& lEventTriedge =  aNode->event_triedge() ;
    
    r = CreateTrisegment(lEventTriedge) ;
    
    CGAL_stskel_intrinsic_test_assertion
    (
      !CGAL_SS_i::is_possibly_inexact_distance_clearly_not_equal_to( Construct_ss_event_time_and_point_2(mTraits)(r)->get<0>()
                                                                   , aNode->time()
                                                                   )  
    ) ;
    
    CGAL_POLYOFFSET_TRACE(3,"Event triedge=" << lEventTriedge ) ;
    
    if ( r->degenerate_seed_id() == Trisegment_2::LEFT )
    {
     CGAL_POLYOFFSET_TRACE(3,"Left seed is degenerate." ) ;
      
      Vertex_const_handle lLeftSeed = GetSeedVertex(aNode
                                                   ,aNode->primary_bisector()->prev()->opposite()
                                                   ,lEventTriedge.e0()
                                                   ,lEventTriedge.e1()
                                                   ) ; 
      if ( handle_assigned(lLeftSeed) ) 
        r->set_child_l( CreateTrisegment(lLeftSeed) ) ; // Recursive call
    }
    else if ( ! aNode->is_split() && r->degenerate_seed_id() == Trisegment_2::RIGHT )
    {
      CGAL_POLYOFFSET_TRACE(3,"Right seed is degenerate." ) ;
      
      Vertex_const_handle lRightSeed = GetSeedVertex(aNode
                                                    ,aNode->primary_bisector()->opposite()->next()
                                                    ,lEventTriedge.e1()
                                                    ,lEventTriedge.e2()
                                                    ) ; 
      if ( handle_assigned(lRightSeed) ) 
        r->set_child_r( CreateTrisegment(lRightSeed) ) ; // Recursive call
    }
  }
    
  return r ;
}

template<class Ss, class Gt, class Cont, class Visitor>
typename Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::Vertex_const_handle
Polygon_offset_builder_2<Ss,Gt,Cont,Visitor>::GetSeedVertex ( Vertex_const_handle   aNode
                                                            , Halfedge_const_handle aBisector
                                                            , Halfedge_const_handle aEa
                                                            , Halfedge_const_handle aEb 
                                                            ) const
{
  Vertex_const_handle rSeed ;
    
  if ( Is_bisector_defined_by(aBisector,aEa,aEb) )
  {
    rSeed = aBisector->vertex();
    
    CGAL_POLYOFFSET_TRACE(3,"Seed of N" << aNode->id() << " for vertex (E" << hid(aEa) << ",E" << hid(aEb) << ") directly found: V" << rSeed->id() ) ;
  }
  else 
  {
    typedef typename Vertex::Halfedge_around_vertex_const_circulator Halfedge_around_vertex_const_circulator ;
    
    Halfedge_around_vertex_const_circulator cb = aNode->halfedge_around_vertex_begin() ;
    Halfedge_around_vertex_const_circulator c  = cb ;
    do
    {
      Halfedge_const_handle lBisector = *c ;
      if ( Is_bisector_defined_by(lBisector,aEa,aEb) )
      {
        rSeed = lBisector->opposite()->vertex();
        CGAL_POLYOFFSET_TRACE(3,"Seed of N" << aNode->id() << " for vertex (E" << hid(aEa) << ",E" << hid(aEb) << ") indirectly found: V" << rSeed->id() ) ;
      }
    }
    while ( !handle_assigned(rSeed) && ++ c != cb ) ;
  }
  
  return rSeed ;
}

CGAL_END_NAMESPACE

#endif // CGAL_POLYGON_OFFSET_BUILDER_2_IMPL_H //
// EOF //
