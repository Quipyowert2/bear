/*
  Copyright (C) 2012 Stuffomatic Ltd. <contact@stuff-o-matic.com>

  All rights reserved.

  See the accompanying license file for details about usage, modification and
  distribution of this file.
*/
/**
 * \file
 * \brief Implementation of the bear::visual::gl_renderer class.
 * \author Julien Jorge
 */
#include "visual/gl_renderer.hpp"

#include "visual/gl_error.hpp"
#include "visual/sdl_error.hpp"

#include "time/time.hpp"

#include <claw/logger.hpp>

/*----------------------------------------------------------------------------*/
bear::visual::gl_renderer::renderer_pointer
bear::visual::gl_renderer::s_instance( (gl_renderer*)NULL );

/*----------------------------------------------------------------------------*/
/**
 * \brief Gets the single instance of this class and starts the rendering
 *        process if it has not been started yet.
 */
bear::visual::gl_renderer& bear::visual::gl_renderer::get_instance()
{
  if ( s_instance == NULL )
    s_instance = renderer_pointer( new gl_renderer() );

  return *s_instance;
} // gl_renderer::get_instance()

/*----------------------------------------------------------------------------*/
/**
 * \brief Terminates the rendering process.
 */
void bear::visual::gl_renderer::terminate()
{
  if ( s_instance != NULL )
    {
      s_instance->stop();
      delete s_instance;
      s_instance = NULL;
    }
} // gl_renderer::terminate()

/*----------------------------------------------------------------------------*/
/**
 * \brief Creates a new texture.
 * \param size The size of the texture.
 */
GLuint bear::visual::gl_renderer::create_texture( screen_size_type& size )
{
  boost::mutex::scoped_lock lock( m_mutex.gl_access );

  unsigned int v;
  for ( v=1; (v < size.x) && /* overflow */ (v != 0); v *= 2 ) { }

  size.x = v;

  for ( v=1; (v < size.y) && /* overflow */ (v != 0); v *= 2 ) { }

  size.y = v;

  GLuint texture_id;

  make_current();
  
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  VISUAL_GL_ERROR_THROW();

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, NULL );
  VISUAL_GL_ERROR_THROW();

  release_context();

  return texture_id;
} // gl_renderer::create_texture()

/*----------------------------------------------------------------------------*/
/**
 * \brief Replaces a portion of a texture with a given data.
 * \param texture_id The identifier of the texture in which we write the pixels.
 * \param data The pixels to copy in the image.
 * \param pos The position in the image where data must be copied.
 * \return true If there is a transparent pixel in the copied data.
 */
bool bear::visual::gl_renderer::draw_texture
( GLuint texture_id, const claw::graphic::image& data,
  const screen_position_type& pos )
{
  boost::mutex::scoped_lock lock( m_mutex.gl_access );

  make_current();

  glBindTexture(GL_TEXTURE_2D, texture_id);

  const claw::graphic::rgba_pixel_8::component_type opaque =
    std::numeric_limits<claw::graphic::rgba_pixel_8::component_type>::max();

  const std::size_t pixels_count( data.width() * data.height() );
  claw::graphic::rgba_pixel_8* const pixels =
    new claw::graphic::rgba_pixel_8[ pixels_count ];

  std::copy( data.begin(), data.end(), pixels );
  glTexSubImage2D
    ( GL_TEXTURE_2D, 0, pos.x, pos.y, data.width(), data.height(), GL_RGBA,
      GL_UNSIGNED_BYTE, pixels );

  release_context();

  bool has_transparency( false );

  for ( claw::graphic::rgba_pixel_8* p = pixels;
        (p != pixels + pixels_count) && !has_transparency; ++p )
    has_transparency = p->components.alpha != opaque;
  
  delete[] pixels;

  return has_transparency;
} // gl_renderer::draw_texture()

/*----------------------------------------------------------------------------*/
/**
 * \brief Deletes a texture.
 * \param texture_id The identifier of the texture to delete.
 */
void bear::visual::gl_renderer::delete_texture( GLuint texture_id )
{
  boost::mutex::scoped_lock lock( m_mutex.gl_access );

  make_current();

  if ( glIsTexture( texture_id ) )
    glDeleteTextures(1, &texture_id);

  release_context();
} // gl_renderer::delete_texture()

/*----------------------------------------------------------------------------*/
/**
 * \brief Does a screen shot.
 * \param img The image in which we save the content of the gl_renderer.
 */
void bear::visual::gl_renderer::shot( claw::graphic::image& img )
{
  boost::mutex::scoped_lock lock( m_mutex.gl_access );
  make_current();

  GLint p[4];
  glGetIntegerv( GL_VIEWPORT, p );
  const unsigned int w = p[2];
  const unsigned int h = p[3];

  img.set_size( w, h );
  const std::size_t pixels_count(w * h);

  glReadPixels( 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, m_screenshot_buffer );
  VISUAL_GL_ERROR_THROW();

  for ( claw::graphic::rgba_pixel_8* it=m_screenshot_buffer;
        it!=m_screenshot_buffer + pixels_count;
        ++it )
    it->components.alpha = 255;

  for (unsigned int y=0; y!=h; ++y)
    std::copy( m_screenshot_buffer + y * w,
               m_screenshot_buffer + (y+1) * w,
               img[h - y - 1].begin() );

  release_context();
} // gl_renderer::shot()

/*----------------------------------------------------------------------------*/
/**
 * \brief Gets the size of the view on the rendered elements.
 */
bear::visual::gl_renderer::screen_size_type
bear::visual::gl_renderer::get_size()
{
  boost::mutex::scoped_lock lock( m_mutex.window );

  return m_view_size;
} //gl_renderer::get_size()

/*----------------------------------------------------------------------------*/
/**
 * \brief Gets the size of the window in which we render.
 */
bear::visual::gl_renderer::screen_size_type
bear::visual::gl_renderer::get_container_size()
{
  boost::mutex::scoped_lock lock( m_mutex.window );

  return m_window_size;
} //gl_renderer::get_container_size()

/*----------------------------------------------------------------------------*/
/**
 * \brief Sets the title of the window.
 * \param title The new title of the window.
 */
void bear::visual::gl_renderer::set_title( const std::string& title )
{
  boost::mutex::scoped_lock lock( m_mutex.window );

  SDL_SetWindowTitle( m_window, title.c_str() );
} // gl_renderer::set_title()

/*----------------------------------------------------------------------------*/
/**
 * \brief Sets the size of the screen.
 * \param w The width of the screen.
 * \param h The height of the screen.
 * \param f Tell if we want a fullscreen mode.
 */
void bear::visual::gl_renderer::set_video_mode
( const screen_size_type& size, bool f )
{
  boost::mutex::scoped_lock lock( m_mutex.window );

  m_view_size = size;
  m_window_size = size;
  m_fullscreen = f;

  m_video_mode_is_set = true;
} // gl_renderer::set_video_mode()

/*----------------------------------------------------------------------------*/
/**
 * \brief Sets the elements to render.
 * \param states States to render. The function steals the states and leaves
 *        this parameter empty when returning.
 */
void bear::visual::gl_renderer::set_gl_states( state_list& states )
{
  boost::mutex::scoped_lock lock( m_mutex.gl_set_states );

  m_states.clear();
  m_states.swap( states );
} // gl_renderer::set_gl_states()

/*----------------------------------------------------------------------------*/
/**
 * \brief Gets the color of the background.
 */
bear::visual::color_type bear::visual::gl_renderer::get_background_color()
{
  boost::mutex::scoped_lock lock( m_mutex.background_color );

  return m_background_color;
} // gl_renderer::get_background_color()

/*----------------------------------------------------------------------------*/
/**
 * \brief Sets the color of the background, used to clear the screen.
 * \param c The color.
 */
void bear::visual::gl_renderer::set_background_color( const color_type& c )
{
  boost::mutex::scoped_lock lock( m_mutex.background_color );

  m_background_color = c;
} // gl_renderer::set_background_color()

/*----------------------------------------------------------------------------*/
/**
 * \brief Tells not to render anything for a while.
 */
void bear::visual::gl_renderer::set_pause()
{
  boost::mutex::scoped_lock lock( m_mutex.loop_state );

  m_mutex.gl_access.lock();

  m_pause = true;
} // gl_renderer::set_pause()

/*----------------------------------------------------------------------------*/
/**
 * \brief Turns the rendering process on again.
 */
void bear::visual::gl_renderer::unset_pause()
{
  boost::mutex::scoped_lock lock( m_mutex.loop_state );

  if ( m_pause == false )
    return;

  m_mutex.gl_access.unlock();

  m_pause = false;
} // gl_renderer::unset_pause()

/*----------------------------------------------------------------------------*/
/**
 * \brief Tells to stop the rendering process.
 */
void bear::visual::gl_renderer::stop()
{
  {
    boost::mutex::scoped_lock lock( m_mutex.loop_state );
    m_stop = true;
  }

  m_render_thread.detach();
} // gl_renderer::stop()

/*----------------------------------------------------------------------------*/
/**
 * \brief Executes the rendering process. The function returns when m_stop
 *        becomes true. Otherwise it renders the states from m_states.
 */
void bear::visual::gl_renderer::render_loop()
{
  // render every 15 milliseconds
  const systime::milliseconds_type render_delta( 15 );

  while ( true )
    {
      // lock m_stop to ensure that stop() will block if called during the loop.
      boost::mutex::scoped_lock lock( m_mutex.loop_state );
      
      if ( m_stop )
        break;

      ensure_window_exists();

      const systime::milliseconds_type start_date( systime::get_date_ms() );

      if ( !m_pause )
        render_states();

      const systime::milliseconds_type end_date( systime::get_date_ms() );

      if ( end_date - start_date < render_delta )
        systime::sleep( render_delta - (end_date - start_date) );
    }
} // gl_renderer::render_loop()

/*----------------------------------------------------------------------------*/
/**
 * \brief Render the elements of m_states.
 */
void bear::visual::gl_renderer::render_states()
{
  boost::mutex::scoped_lock gl_lock( m_mutex.gl_access );

  if ( m_gl_context == NULL )
    return;
    
  boost::mutex::scoped_lock states_lock( m_mutex.gl_set_states );

  if ( m_states.empty() )
    return;

  make_current();

  set_background_color();

  glClear( GL_COLOR_BUFFER_BIT );

  for ( state_list::const_iterator it( m_states.begin() );
        it != m_states.end(); ++it )
    it->draw();
  VISUAL_GL_ERROR_THROW();

  SDL_GL_SwapWindow( m_window );
  VISUAL_GL_ERROR_THROW();

  m_states.clear();

  release_context();
} // gl_renderer::render_states()

/*----------------------------------------------------------------------------*/
/**
 * \brief Passes the background color to OpenGL.
 */
void bear::visual::gl_renderer::set_background_color()
{
  boost::mutex::scoped_lock lock( m_mutex.background_color );

  const GLfloat max
    ( std::numeric_limits<claw::graphic::rgb_pixel::component_type>::max() );

  glClearColor( (GLfloat)m_background_color.components.red / max,
                (GLfloat)m_background_color.components.green / max,
                (GLfloat)m_background_color.components.blue / max,
                (GLfloat)m_background_color.components.alpha / max );
} // gl_renderer::set_background_color()

/*----------------------------------------------------------------------------*/
/**
 * \brief Sets a new dimension for the resulting projection to match the size of
 *        the screen.
 */
void bear::visual::gl_renderer::resize_view()
{
  glViewport( 0, 0, m_window_size.x, m_window_size.y );
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho( 0, m_view_size.x, 0, m_view_size.y, -1, 0 );
  glMatrixMode(GL_MODELVIEW);

  VISUAL_GL_ERROR_THROW();
} // gl_renderer::resize_view()

/*----------------------------------------------------------------------------*/
/**
 * \brief Makes m_gl_context the current context of the thread.
 */
void bear::visual::gl_renderer::make_current()
{
  if ( SDL_GL_MakeCurrent( m_window, m_gl_context ) != 0 )
    claw::logger << claw::log_error
                 << SDL_GetError() << std::endl;
} // gl_renderer::make_current()

/*----------------------------------------------------------------------------*/
/**
 * \brief Releases the context associated with this thread, in order to let it
 *        available for other threads.
 */
void bear::visual::gl_renderer::release_context()
{
  if ( SDL_GL_MakeCurrent( m_window, NULL ) != 0 )
    claw::logger << claw::log_error
                 << SDL_GetError() << std::endl;
} // gl_renderer::release_context()

/*----------------------------------------------------------------------------*/
/**
 * \brief Creates the window and the GL context if it has not been created yet.
 */
void bear::visual::gl_renderer::ensure_window_exists()
{
  boost::mutex::scoped_lock lock( m_mutex.window );

  if ( !m_video_mode_is_set || (m_gl_context != NULL) )
    return;

  Uint32 flags = SDL_WINDOW_OPENGL;

  m_window_size = get_best_screen_size();

  if ( m_fullscreen )
    flags |= SDL_WINDOW_FULLSCREEN;

  claw::logger << claw::log_verbose << "Setting video mode to "
               << m_window_size.x << 'x' << m_window_size.y
               << ' ' << (m_fullscreen ? "fullscreen" : "windowed")
               << std::endl;

  SDL_EventState( SDL_QUIT, SDL_DISABLE );

  m_window =
    SDL_CreateWindow
    ( "Bear", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      m_window_size.x, m_window_size.y, flags );

  if ( m_window == NULL )
    VISUAL_SDL_ERROR_THROW();

  m_gl_context = SDL_GL_CreateContext( m_window );

  if ( m_gl_context == NULL )
    VISUAL_SDL_ERROR_THROW();

  claw::logger << claw::log_verbose << "OpenGL version is "
               << glGetString( GL_VERSION )
               << ", vendor is " << glGetString(GL_VENDOR)
               << std::endl;

  delete[] m_screenshot_buffer;
  m_screenshot_buffer =
    new claw::graphic::rgba_pixel_8[ m_window_size.x * m_window_size.y ];

  SDL_ShowCursor(0);

#ifdef _WIN32
  const GLenum err = glewInit();

  if ( err != GLEW_OK )
    claw::logger << claw::log_error << "Failed to initialize Glew: "
                 << glewGetErrorString(err) << std::endl;
#endif
  
  glEnable(GL_TEXTURE_2D);
  VISUAL_GL_ERROR_THROW();
  
  glEnable(GL_BLEND);
  VISUAL_GL_ERROR_THROW();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  VISUAL_GL_ERROR_THROW();

  resize_view();

  release_context();

  m_mutex.gl_access.unlock();
} // gl_renderer::ensure_window_exists()

/*----------------------------------------------------------------------------*/
/**
 * \brief Finds a screen resolution that has is the nearest of a given size.
 * \param w The width of the screen.
 * \param h The height of the screen.
 * \param f Tell if we want a fullscreen mode.
 */
bear::visual::gl_renderer::screen_size_type
bear::visual::gl_renderer::get_best_screen_size() const
{
  screen_size_type result( m_view_size );

  claw::logger << claw::log_verbose << "Requested screen resolution is "
               << result.x << 'x' << result.y << '.' << std::endl;

  if ( m_fullscreen )
    {
      claw::logger << claw::log_verbose << "Available screen resolutions:"
                   << std::endl;
      
      const std::vector<SDL_DisplayMode> modes( get_sdl_display_modes() );

      for ( std::size_t i(0); i != modes.size(); ++i )
        claw::logger << claw::log_verbose << modes[i].w << 'x' << modes[i].h
                     << std::endl;

      result = get_best_screen_size( modes );
    }
  else
    {
      claw::logger << claw::log_verbose
                   << "Setting resolution in windowed mode."
                   << std::endl;
      
      SDL_DisplayMode m;
      SDL_GetDesktopDisplayMode(0, &m);

      const double r_x = (double)m.w / m_view_size.x;
      const double r_y = (double)m.h / m_view_size.y;
      double resize_ratio = std::min( r_x, r_y );

      if ( resize_ratio < 1 )
        result =
          screen_size_type
          ( resize_ratio * m_view_size.x, resize_ratio * m_view_size.y );
    }

  claw::logger << claw::log_verbose << "Selected screen resolution is "
               << result.x << 'x' << result.y << '.' << std::endl;

  return result;
} // gl_renderer::get_best_screen_size()

/*----------------------------------------------------------------------------*/
/**
 * \brief Gets the display modes as returned by SDL.
 */
std::vector<SDL_DisplayMode>
bear::visual::gl_renderer::get_sdl_display_modes() const
{
  const int count( SDL_GetNumDisplayModes(0) );

  if ( count < 1 )
    VISUAL_SDL_ERROR_THROW();

  std::vector<SDL_DisplayMode> result( count );

  for ( int i(0); i!=count; ++i )
    SDL_GetDisplayMode( 0, i, &result[i] );

  return result;
} // gl_renderer::get_sdl_display_modes()

/*----------------------------------------------------------------------------*/
/**
 * \brief Finds a screen resolution that has is the nearest of a given size.
 * \param modes The available resolutions.
 */
bear::visual::gl_renderer::screen_size_type
bear::visual::gl_renderer::get_best_screen_size
( const std::vector<SDL_DisplayMode>& modes ) const
{
  SDL_DisplayMode m;
  SDL_GetDesktopDisplayMode(0, &m);

  const double requested_ratio( (double)m_view_size.x / m_view_size.y );
  screen_size_type result( m.w, m.h );
  double display_ratio( (double)result.x / result.y );
  double best_scale_factor_distance;
  
  if ( (display_ratio >= 1) && (requested_ratio >= 1) )
    best_scale_factor_distance = std::abs( (double)m.w / m_view_size.x - 1 );
  else
    best_scale_factor_distance = std::abs( (double)m.h / m_view_size.y - 1 );

  for ( std::size_t i(0); i!=modes.size(); ++i )
    {
      const unsigned int width( modes[i].w );
      const unsigned int height( modes[i].h );

      const double scale_factor
        ( std::min
          ( (double)height / m_view_size.y, (double)width / m_view_size.x ) );

      const double scale_factor_distance( std::abs( scale_factor - 1 ) );

      if ( scale_factor_distance < best_scale_factor_distance )
        {
          best_scale_factor_distance = scale_factor_distance;
          result = screen_size_type( width, height );
        }
    }

  return result;
} // gl_renderer::get_best_screen_size()

/*----------------------------------------------------------------------------*/
/**
 * Constructs a gl_renderer instance with no rendering informations.
 */
bear::visual::gl_renderer::gl_renderer()
  : m_stop( false ), m_pause( false ), m_window( NULL ), m_gl_context( NULL ),
    m_background_color(0, 0, 0), m_window_size( 640, 480 ),
    m_view_size( 640, 480 ), m_fullscreen( false ),
    m_video_mode_is_set( false ), m_screenshot_buffer( NULL )
{
  m_mutex.gl_access.lock();

  m_render_thread =
    boost::thread( boost::bind(&gl_renderer::render_loop, this) );
} // gl_renderer::gl_renderer()
