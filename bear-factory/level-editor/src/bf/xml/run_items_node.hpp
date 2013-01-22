/*
  Copyright (C) 2012 Stuffomatic Ltd. <contact@stuff-o-matic.com>

  All rights reserved.
*/
/**
 * \file
 * \brief Read an xml node "items".
 * \author Julien Jorge
 */
#ifndef __BF_XML_RUN_ITEMS_NODE_HPP__
#define __BF_XML_RUN_ITEMS_NODE_HPP__

#include <wx/xml/xml.h>
#include <iostream>

namespace bf
{
  class item_class_pool;
  class run_configuration;

  namespace xml
  {
    /**
     * \brief Read an xml node "items".
     * \author Julien Jorge
     */
    class run_items_node
    {
    public:
      void read
      ( run_configuration& config, const item_class_pool& pool,
        const wxXmlNode* node ) const;
      void write( const run_configuration& config, std::ostream& os ) const;

    }; // class run_items_node
  } // namespace xml
} // namespace bf

#endif // __BF_XML_RUN_ITEMS_NODE_HPP__
