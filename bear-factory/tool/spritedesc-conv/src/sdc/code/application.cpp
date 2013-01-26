/*
  Copyright (C) 2012 Stuffomatic Ltd. <contact@stuff-o-matic.com>

  All rights reserved.
*/
/**
 * \file
 * \brief Implementation of the sdc::application class.
 * \author Julien Jorge
 */
#include "application.hpp"

#include <fstream>
#include <iostream>
#include <cstdio>

#include "parser.hpp"
#include "spritedesc.hpp"
#include "xcf_info.hpp"
#include "xcf_map.hpp"

#include <claw/string_algorithm.hpp>
#include <boost/filesystem/convenience.hpp>

/*----------------------------------------------------------------------------*/
/**
 * \brief Compare two sprites by decreasing order of their areas.
 * \param a The left operand.
 * \param b The right operand.
 */
bool sdc::application::sprite_area_comp::operator()
  ( const spritedesc::sprite& a, const spritedesc::sprite& b ) const
{
  return a.result_box.area() > b.result_box.area();
} // application::sprite_area_comp::operator()()

/*----------------------------------------------------------------------------*/
/**
 * \brief Compare two sprites by decreasing order of their heights.
 * \param a The left operand.
 * \param b The right operand.
 */
bool sdc::application::sprite_height_comp::operator()
  ( const spritedesc::sprite& a, const spritedesc::sprite& b ) const
{
  return a.result_box.height > b.result_box.height;
} // application::sprite_height_comp::operator()()




/*----------------------------------------------------------------------------*/
/**
 * \brief Constructor.
 * \param argc Number of program arguments.
 * \param argv Program arguments.
 */
sdc::application::application( int& argc, char** &argv )
  : claw::application(argc, argv), m_quit(false), m_generate_spritepos(true),
    m_gimp_console_program( "gimp-console" ), m_xcfinfo_program( "xcfinfo" )
{
  check_arguments( argc, argv );
} // application::application()

/*----------------------------------------------------------------------------*/
/**
 * \brief Run the application : split the image.
 */
int sdc::application::run()
{
  int result = 0;

  if (!m_quit)
    process_file(m_input_file);

  return result;
} // application::run()

/*----------------------------------------------------------------------------*/
/**
 * \brief Print program usage.
 */
void sdc::application::help() const
{
  m_arguments.help("file.spritedesc");
} // application::help()

/*----------------------------------------------------------------------------*/
/**
 * \brief Check program arguments.
 * \param argc Number of program arguments.
 * \param argv Program arguments.
 */
void sdc::application::check_arguments( int& argc, char** &argv )
{
  m_arguments.add( "-h", "--help", "Print this message and exit.", true );
  m_arguments.add
    ( "-s", "--scheme-directory",
      "A directory where the utility Scheme scripts can be found.", true );
  m_arguments.add
    ( "-g", "--gimp-console",
      "The path to the gimp-console executable.", true );
  m_arguments.add
    ( "-x", "--xcfinfo", "The path to the xcfinfo executable.", true );
  m_arguments.add_long
    ( "--no-spritepos", "Tells to not generate the spritepos file.", true );

  m_arguments.parse( argc, argv );

  if ( m_arguments.get_bool("--help") || (argc != 1) )
    {
      help();
      m_quit = true;
    }

  if ( m_arguments.has_value("--gimp-console") )
    m_gimp_console_program = m_arguments.get_string("--gimp-console");

  if ( m_arguments.has_value("--scheme-directory") )
    m_scheme_directory = m_arguments.get_all_of_string("--scheme-directory");

  m_generate_spritepos = !m_arguments.get_bool("--no-spritepos");

  if ( argc > 0 )
    m_input_file = argv[0];
} // application::check_arguments()

/*----------------------------------------------------------------------------*/
/**
 * \brief Process a file.
 * \param name The name of the file to process.
 */
void sdc::application::process_file( const std::string& name )
{
  const boost::filesystem::path file_path( name, boost::filesystem::native );
  const boost::filesystem::path file_directory( file_path.parent_path() );
  xcf_map xcf( file_directory.string(), m_xcfinfo_program );

  parser p;
  std::list<spritedesc> desc;

  if ( !p.run( xcf, desc, name ) )
    std::cerr << "Failed to process file '" << name << "'" << std::endl;

  for ( std::list<spritedesc>::iterator it=desc.begin(); it!=desc.end(); ++it )
    {
      std::clog << "Processing " << it->output_name << std::endl;

      set_sprite_position( *it );
      generate_output( *it );
    }
} // application::process_file()

/*----------------------------------------------------------------------------*/
/**
 * \brief Executes gimp-console on a given Scheme script.
 * \param script The script to pass to gimp.
 */
void sdc::application::execute_gimp_scheme_process( std::string script ) const
{
  const std::string command( m_gimp_console_program + " --batch -" );
  FILE* process = popen( command.c_str(), "w" );

  if ( process == NULL )
    {
      std::cerr << "Failed to execute gimp console: '" << command << "'"
                << std::endl;
      return;
    }

  fputs( script.c_str(), process );

  pclose( process );
} // application::execute_gimp_scheme_process()

/*----------------------------------------------------------------------------*/
/**
 * \brief Generate a sprite sheets.
 * \param desc The sprite sheet to generate.
 */
void sdc::application::generate_output( const spritedesc& desc ) const
{
  std::ostringstream oss;
  generate_scm( oss, desc );
  execute_gimp_scheme_process( oss.str() );

  if ( m_generate_spritepos )
    {
      std::ofstream f( (desc.output_name + ".spritepos").c_str() );
      generate_spritepos( f, desc );
    }
} // application::generate_output()

/*----------------------------------------------------------------------------*/
/**
 * \brief Generate the spritepos file for the described sprites.
 * \param os The stream in which the spritepos is generated.
 * \param desc The sprites to generate.
 */
void sdc::application::generate_spritepos
( std::ostream& os, const spritedesc& desc ) const
{
  os << "# The format of the lines is\n"
     << "# picture: x y width height\n\n";

  for ( spritedesc::const_sprite_iterator it = desc.sprite_begin();
        it != desc.sprite_end(); ++it )
    os << it->name << ": " << it->result_box.position.x << ' '
       << it->result_box.position.y << ' '
       << it->result_box.width << ' ' << it->result_box.height << '\n';
} // application::generate_spritepos()

/*----------------------------------------------------------------------------*/
/**
 * \brief Gets the full path of a Scheme file.
 *
 * The file is searched in m_scheme_directory and the first match is returned.
 *
 * \param filename The name of the file to search.
 * \return The path to the first file found with the paths of m_scheme_directory
 *         or filename if the file was not found.
 */
std::string sdc::application::get_scheme_path( std::string filename ) const
{
  path_list_type candidates( m_scheme_directory );

#ifdef BEAR_SDC_DEFAULT_SCHEME_PATH
  candidates.push_back( BEAR_SDC_DEFAULT_SCHEME_PATH );
#endif

  for ( path_list_type::const_iterator it=candidates.begin();
        it!=candidates.end();
        ++it )
    {
      boost::filesystem::path p( *it, boost::filesystem::native );
      p /= filename;

      if ( boost::filesystem::exists( p ) )
        return p.string();
    }

  return filename;
} // application::get_scheme_path()

/*----------------------------------------------------------------------------*/
/**
 * \brief Generate the Scheme script that builds the sprites of a given
 *        spritedesc.
 * \param os The stream in which the script is generated.
 * \param desc The sprites to generate.
 */
void sdc::application::generate_scm
( std::ostream& os, const spritedesc& desc ) const
{
  os << "(load \"" << get_scheme_path( "common.scm" ) << "\")\n";

  os << "(let ( ";

  for ( spritedesc::id_to_file_map::const_iterator it = desc.xcf.begin();
        it != desc.xcf.end(); ++it )
    os << "(" << make_image_varname( it->first )
       << " (car (gimp-file-load 1 \"" << it->second << "\" \"" << it->second
       << "\" )))\n";

  os << "(" << make_image_varname(desc.output_name)
     << " (new-image " << desc.width << ' ' << desc.height << "))\n";

  os << ")\n";

  for ( spritedesc::const_sprite_iterator it = desc.sprite_begin();
        it != desc.sprite_end(); ++it )
    generate_scm( os, *it, desc.output_name );

  os << "(save-frames \"" << desc.output_name << ".png\" "
     << make_image_varname(desc.output_name) << ")\n";

  os << ")\n";

  os << "(gimp-quit 1)";
} // application::generate_scm()

/*----------------------------------------------------------------------------*/
/**
 * \brief Generate the Scheme command that builds the given sprite.
 * \param os The stream in which the script is generated.
 * \param s The sprite to generate.
 * \param target_id The identifier of the image receiving the sprite in the
 *         script.
 */
void sdc::application::generate_scm
( std::ostream& os, const spritedesc::sprite& s,
  const std::string& target_id ) const
{
  os << "(create-layer-crop " << make_image_varname(s.xcf_id) << " '(";

  for ( std::list<layer_info>::const_iterator it=s.layers.begin();
        it != s.layers.end(); ++it )
    os << it->index << ' ';

  os << ") " << s.source_box.position.x << ' ' << s.source_box.position.y << ' '
     << s.source_box.width << ' ' << s.source_box.height << ' '
     << s.result_box.position.x << ' ' << s.result_box.position.y << ' '
     << s.result_box.width << ' ' << s.result_box.height;

  os << ' ' << make_image_varname(target_id) << " '(";

  for ( std::list<layer_info>::const_iterator it=s.mask.begin();
        it != s.mask.end(); ++it )
    os << it->index << ' ';

  os << "))\n";
} // application::generate_scm()

/*----------------------------------------------------------------------------*/
/**
 * \brief Get the name of the variable representing the xcf file identified with
 *        a given id.
 * \param id The identifier of the xcf file.
 */
std::string sdc::application::make_image_varname( const std::string& id ) const
{
  return id + "_image";
} // application::make_image_varname()

/*----------------------------------------------------------------------------*/
/**
 * \brief Set the position of the sprites in the target image.
 * \param desc The spritedesc whose sprites are going to be placed.
 */
void sdc::application::set_sprite_position( spritedesc& desc ) const
{
  switch( desc.order )
    {
    case decreasing_area:
      desc.sort_sprites( sprite_area_comp() );
      break;
    case decreasing_height:
      desc.sort_sprites( sprite_height_comp() );
      break;
    case declaration_order:
      // nothing to do
      break;
    }
  
  std::list<rectangle_type> empty_places;
  empty_places.push_back
    ( rectangle_type(0, 0, desc.width, desc.height) );

  rectangle_type final_size( 0, 0, 1, 1 );
  spritedesc::sprite_iterator it=desc.sprite_begin();

  while ( it != desc.sprite_end() )
    {
      if ( find_place_for( empty_places, *it, desc.margin ) )
        {
          final_size = final_size.join( it->result_box );
          ++it;
        }
      else
        {
          const spritedesc::sprite_iterator prev( it );
          ++it;
          desc.erase_sprite( prev );
        }
    }

  desc.width = final_size.width + desc.margin;
  desc.height = final_size.height + desc.margin;
} // application::set_sprite_position()

/*----------------------------------------------------------------------------*/
/**
 * \brief Find a place for a given sprite.
 * \param empty_places The empty rectangles where the sprite can be placed.
 * \param s The sprite to place.
 * \param m The margin around the sprites.
 */
bool sdc::application::find_place_for
( std::list<rectangle_type>& empty_places, spritedesc::sprite& s,
  std::size_t m ) const
{
  std::size_t area = std::numeric_limits<std::size_t>::max();
  std::list<rectangle_type>::iterator selected = empty_places.end();
  std::list<rectangle_type> failed;

  const std::size_t w( s.result_box.width + m );
  const std::size_t h( s.result_box.height + m );

  for ( std::list<rectangle_type>::iterator it = empty_places.begin();
        it != empty_places.end(); ++it )
    if ( (it->width >= w) && (it->height >= h) )
      {
        if ( it->area() < area )
          {
            area = it->area();
            selected = it;
          }
      }
    else
      failed.push_back(*it);

  if ( selected == empty_places.end() )
    {
      std::cerr << "Can't find a place for sprite " << s.name 
                << " (with a size equal to " << s.result_box.width << 'x'
                << s.result_box.height << " and an initial size equal to "
                << s.source_box.width << 'x' << s.source_box.height << ")."
                << std::endl;

      for ( std::list<rectangle_type>::iterator it = failed.begin();
            it != failed.end(); ++it )
        std::cerr << "\t(" << (int)(w - it->width) << ", "
                  << (int)(h - it->height) << ')' << std::endl;

      return false;
    }
  else
    {
      const rectangle_type r = *selected;
      empty_places.erase(selected);

      s.result_box.position = r.position;

      const rectangle_type r_right
        ( r.left() + w, r.top(), r.width - w, h );
      const rectangle_type r_bottom
        ( r.left(), r.top() + h, r.width, r.height - h );

      CLAW_POSTCOND( r.includes( r_right ) );
      CLAW_POSTCOND( r.includes( r_bottom ) );

      if ( r_right.area() != 0 )
        empty_places.push_back( r_right );

      if ( r_bottom.area() != 0 )
        empty_places.push_back( r_bottom );

      return true;
    }
} // application::find_place_for()
