/*
  Copyright (C) 2012 Stuffomatic Ltd. <contact@stuff-o-matic.com>

  All rights reserved.
*/
/**
 * \file
 * \brief Align a rectangle at the top of an other.
 * \author Julien Jorge
 */
#ifndef __UNIVERSE_ALIGN_TOP_HPP__
#define __UNIVERSE_ALIGN_TOP_HPP__

#include "universe/alignment/alignment.hpp"

#include "universe/class_export.hpp"

namespace bear
{
  namespace universe
  {
    /**
     * \brief Align a rectangle at the top of an other.
     */
    class UNIVERSE_EXPORT align_top:
      public alignment
    {
    public:
      virtual void align
      ( const rectangle_type& this_box, const position_type& that_old_pos,
        rectangle_type& that_new_box ) const;
    }; // class align_top
  } // namespace universe
} // namespace bear

#endif // __UNIVERSE_ALIGN_TOP_HPP__
