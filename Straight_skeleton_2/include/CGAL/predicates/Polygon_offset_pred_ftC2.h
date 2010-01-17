// Copyright (c) 2006 Fernando Luis Cacciola Carballal. All rights reserved.
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
#ifndef CGAL_POLYGON_OFFSET_PRED_FTC2_H
#define CGAL_POLYGON_OFFSET_PRED_FTC2_H 1

#include <CGAL/constructions/Straight_skeleton_cons_ftC2.h>

CGAL_BEGIN_NAMESPACE

namespace CGAL_SS_i
{

// Given a triple of oriented straight line segments: (e0,e1,e2) such that
// there exists a distance 'et' for which the offsets lines at 'et' (e0',e1',e2') intersect in a single point;
// returns the relative order of 't' w.r.t 'et'.
// PRECONDITION: There exist a positive distance et for which the offset triple intersect at a single point.
template<class FT, class Trisegment_2>
Uncertain<Comparison_result> compare_offset_against_isec_timeC2 ( FT const& t, intrusive_ptr< Trisegment_2 > const& tri )
{
  typedef Rational<FT> Rational ;
  typedef Quotient<FT> Quotient ;
  
  typedef optional<Rational> Optional_rational ;
 
  Uncertain<Comparison_result> rResult = Uncertain<Comparison_result>::indeterminate();

  Optional_rational et_ = compute_offset_lines_isec_timeC2(tri);
  if ( et_ )
  {
    Quotient et = et_->to_quotient();

    CGAL_assertion ( CGAL_NTS certified_is_positive(et) ) ;

    rResult = CGAL_NTS certified_compare( Quotient(t), et);
  }

  return rResult ;

}

} // namespace CGAL_SS_i

CGAL_END_NAMESPACE

#endif // CGAL_POLYGON_OFFSET_PRED_FTC2_H //
// EOF //

