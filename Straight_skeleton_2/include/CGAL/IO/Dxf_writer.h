// Copyright (c) 2007  GeometryFactory (France).  All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 2.1 of the License.
// See the file LICENSE.LGPL distributed with CGAL.
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
//
// Author(s)     : Fernando Cacciola
//
// Descriptions of the file format can be found at
// http://www.autodesk.com/techpubs/autocad/acad2000/dxf/
 

#ifndef CGAL_IO_DXF_WRITER_H
#define CGAL_IO_DXF_WRITER_H

#include <CGAL/basic.h>
#include <CGAL/algorithm.h>
#include <iostream>
#include <string>
#include <list>

CGAL_BEGIN_NAMESPACE

class Dxf_writer
{
  typedef std::list<std::string> Lines ;
  typedef Lines::iterator        Line_iterator ;
  
  typedef std::set<std::string> Layers ;
  typedef Layers::iterator      Layer_iterator ;

public:

  Dxf_writer ( std::ostream& out ) : mOut(out), mHandle(32)
  {
    mPos = mLines.end();
    add_header(); 
  }

  ~Dxf_writer()
  {
    add_footer(); 
    dump();
  } 

  template<class XYZ>
  void add_segment ( XYZ const&  aSrc
                   , XYZ const&  aTgt
                   , std::string aLayer = ""
                   , int         aColor = 255
                   )
  {
    add_entity ( "LINE" ,  aLayer ) ;
    add_code ( 62 , to_str ( aColor ) ) ;
    add_code ( 10 , to_str ( to_double(aSrc.x()) ) ) ;
    add_code ( 20 , to_str ( to_double(aSrc.y()) ) ) ;
    add_code ( 30 , to_str ( 0.0                 ) ) ;
    add_code ( 11 , to_str ( to_double(aTgt.x()) ) ) ;
    add_code ( 21 , to_str ( to_double(aTgt.y()) ) ) ;
    add_code ( 31 , to_str ( 0.0                 ) ) ;
  }
  
  
  template<class XYZ_Iterator>
  void add_polyline ( XYZ_Iterator aVerticesBegin
                    , XYZ_Iterator aVerticesEnd
                    , bool         aIsClosed
                    , std::string  aLayer = ""
                    , int          aColor = 255
                    )
  {
    if ( aVerticesBegin < aVerticesEnd )
    {
      add_entity ( "POLYLINE" , aLayer) ;
      add_code ( 62 , to_str ( aColor ) ) ;
      add_code ( 66 , to_str ( 1   ) ) ;
      add_code ( 10 , to_str ( 0.0 ) ) ;
      add_code ( 20 , to_str ( 0.0 ) ) ;
      add_code ( 30 , to_str ( 0.0 ) ) ;
      add_code ( 70 , to_str ( aIsClosed ? 1 : 0  ) ) ;
  
      while ( aVerticesBegin != aVerticesEnd )
      {
        add_entity ( "VERTEX" , aLayer) ;
        add_code ( 10 , to_str ( to_double( aVerticesBegin->x() ) ) ) ;
        add_code ( 20 , to_str ( to_double( aVerticesBegin->y() ) ) ) ;
        add_code ( 30 , to_str ( 0.0                              ) ) ;
        ++ aVerticesBegin ;
      }
      
      add_entity ( "SEQEND" , aLayer) ;
    }
  }
  
  template<class XYZ_Iterator>
  void add_segments ( XYZ_Iterator aVerticesBegin
                    , XYZ_Iterator aVerticesEnd
                    , bool         aIsClosed
                    , std::string  aLayer = ""
                    , int          aColor = 255
                    )
  {
    if ( aVerticesBegin < aVerticesEnd )
    {
      XYZ_Iterator lFirstVertex = aVerticesBegin ;
      XYZ_Iterator lLastVertex  = aVerticesEnd ; -- lLastVertex ;
      
      if ( lFirstVertex != lLastVertex )
      {
        XYZ_Iterator lCurrVertex  = aVerticesBegin ;
        
        while ( lCurrVertex != lLastVertex )
        {
          XYZ_Iterator lNextVertex = CGAL::successor(lCurrVertex);
          
          add_segment ( *lCurrVertex, *lNextVertex, aLayer, aColor ) ;
          
          ++ lCurrVertex ;
        }
        
        if ( aIsClosed  )
          add_segment ( *lLastVertex, *lFirstVertex, aLayer, aColor ) ;
      }
    }
    
  }
  
private:

  std::string get_entity_handle()
  {
    char lBuff[64];
    sprintf(lBuff,"%5x",mHandle++);
    return std::string(lBuff);
  }
  
  std::string to_str ( int aN )
  {
    char lBuff[64];
    sprintf(lBuff,"%6d",aN);
    return std::string(lBuff);
  }
  
  
  std::string to_str ( double aN )
  {
    char lBuff[64];
    sprintf(lBuff,"%6.6f",aN);
    return std::string(lBuff);
  }
  
  void insert_line ( Line_iterator aPos, std::string aLine )
  {
    mLines.insert(aPos,aLine);
  }

  void add_line ( std::string aLine )
  {
    insert_line(mPos,aLine);
  }  

  void add_code ( int aCode, std::string aValue )
  {
    add_line( to_str(aCode) ) ;
    add_line( aValue ) ;
  }

  void add_group_begin ( std::string aGroup, std::string aName )
  {
    add_code ( 0 , aGroup ) ;
    add_code ( 2 , aName  ) ;
  }
  
  void add_group_end ( std::string aGroup )
  {
    add_code ( 0 , aGroup ) ;
  }  

  void add_entity ( std::string aName, std::string aLayer )
  {
    add_code ( 0 , aName  ) ;
    add_code ( 5 , get_entity_handle() ) ;
    
    if ( !aLayer.empty() && aLayer != "0" )
    {
      mLayers.insert(aLayer);
      add_code ( 8 , aLayer ) ;
    }
  }
  
  void add_header()
  { 
    add_group_begin ( "SECTION" , "HEADER" ) ;
    add_group_end   ( "ENDSEC" ) ;
    
    add_group_begin ( "SECTION" , "TABLES" ) ;
      add_group_begin ( "TABLE" , "LTYPE" ) ;
        add_code ( 70 , to_str ( 1 ) ) ;
        add_code ( 0  , "LTYPE" ) ;
        add_code ( 2  , "CONTINUOUS" ) ;
        add_code ( 70 , to_str ( 0 ) ) ;
        add_code ( 3  , "Solid line" ) ;
        add_code ( 72 , to_str ( 65  ) ) ;
        add_code ( 73 , to_str ( 0   ) ) ;
        add_code ( 40 , to_str ( 0.0 ) ) ;
      add_group_end   ( "ENDTAB" ) ;
      add_group_begin ( "TABLE" , "APPID" ) ;
        add_code ( 70 , to_str ( 1 ) ) ;
        add_code ( 0  , "APPID" ) ;
        add_code ( 2  , "ACAD"  ) ;
        add_code ( 70 , to_str ( 0 ) ) ;
      add_group_end   ( "ENDTAB" ) ;
      
      mLayersTablePos = mPos ; -- mLayersTablePos ;
      
    add_group_end   ( "ENDSEC" ) ;

    add_group_begin ( "SECTION" , "ENTITIES" ) ;
  }

  void add_footer()
  {
    add_group_end( "ENDSEC" ) ;
    add_group_end( "EOF" ) ;

    insert_layers();
  }
  
  void insert_layers()
  {
    if ( mLayers.size() > 0 )
    {
      mPos = mLayersTablePos ; ++ mPos ;
      
      add_group_begin ( "TABLE" , "LAYER" ) ;
      add_code ( 70 , to_str ( int(mLayers.size() + 1) ) ) ;
      add_code ( 0  , "LAYER" ) ;
      add_code ( 2  , "0"     ) ;
      add_code ( 70 , to_str ( 0 ) ) ;
      add_code ( 62 , to_str ( 7 ) ) ;
      add_code ( 6  , "CONTINUOUS" ) ;
     
      for ( Layer_iterator lit = mLayers.begin() ; lit != mLayers.end() ; ++ lit )
      {
          add_code ( 0  , "LAYER" ) ;
          add_code ( 2  , *lit    ) ;
          add_code ( 70 , to_str ( 0 ) ) ;
          add_code ( 62 , to_str ( 0 ) ) ;
          add_code ( 6  , "CONTINUOUS" ) ;
      }
      add_group_end ( "ENDTAB" ) ;
    }
  }

  void dump()
  {
    std::copy(mLines.begin(),mLines.end(), std::ostream_iterator<std::string>(mOut,"\n"));
  }
  

  std::ostream& mOut ;
  Lines         mLines ;
  Line_iterator mPos ;
  Line_iterator mLayersTablePos ;
  Layers        mLayers ;
  int           mHandle ;

} ;

CGAL_END_NAMESPACE

#endif // CGAL_IO_DXF_WRITER_H
