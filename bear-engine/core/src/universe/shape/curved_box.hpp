/*
  Copyright (C) 2012 Stuffomatic Ltd. <contact@stuff-o-matic.com>

  All rights reserved.
*/
/**
 * \file
 * \brief A rectangle with a curved top.
 * \author Julien Jorge
 */
#ifndef __UNIVERSE_CURVED_BOX_HPP__
#define __UNIVERSE_CURVED_BOX_HPP__

#include "universe/shape/shape_base.hpp"

namespace bear
{
  namespace universe
  {
    class rectangle;

    /**
     * \brief A rectangle with a curved top.
     * \author Julien Jorge
     */
    class UNIVERSE_EXPORT curved_box:
      public shape_base
    {
    public:
      virtual curved_box* clone() const;

      virtual bool intersects( const rectangle& that ) const;
      virtual bool intersects( const curved_box& that ) const;

    private:
      virtual coordinate_type do_get_bottom() const;
      virtual void do_set_bottom( coordinate_type p );

      virtual coordinate_type do_get_left() const;
      virtual void do_set_left( coordinate_type p );

      virtual size_type do_get_width() const;
      virtual void do_set_width( size_type s );

      virtual size_type do_get_height() const;
      virtual void do_set_height( size_type s );

      virtual bool do_intersects( const shape_base& that ) const;

    private:
      /** \brief The reference position. */
      position_type m_bottom_left;

      /** \brief The size of the rectangle. */
      size_box_type m_size;

    }; // class curved_box
  } // namespace universe
} // namespace bear

#endif // __UNIVERSE_CURVED_BOX_HPP__