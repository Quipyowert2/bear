/*
  Copyright (C) 2012 Stuffomatic Ltd. <contact@stuff-o-matic.com>

  All rights reserved.
*/
/**
 * \file
 * \brief The action of uniformizing the horizontal space between the items of a
 *        selection.
 * \author Julien Jorge
 */
#ifndef __BF_ACTION_ARRANGE_SELECTION_VERTICALLY_HPP__
#define __BF_ACTION_ARRANGE_SELECTION_VERTICALLY_HPP__

#include "bf/history/action_group.hpp"

namespace bf
{
  class item_selection;

  /**
   * \brief The action of uniformizing the horizontal space between the items of
   *        a selection.
   * \author Julien Jorge
   */
  class action_arrange_selection_vertically:
    public action_group
  {
  public:
    action_arrange_selection_vertically( const item_selection& selection );

  }; // class action_arrange_selection_vertically
} // namespace bf

#endif // __BF_ACTION_ARRANGE_SELECTION_VERTICALLY_HPP__