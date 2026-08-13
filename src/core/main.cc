#include "raylib.h"
#include <exception>
#include <string>
#include <cstring>
#include <sstream>
#include <ctime>
#include <random>
#include <memory>
#include <cassert>
#include "log.hh"
#include "input.hh"
#include "../assets/sprites.hh"
#include "../assets/fonts.hh"
#include "../assets/sounds.hh"
#include "../game/constants.hh"
#include "../hud/hud.hh"
#include "../title/title.hh"
#include "../menu/menu.hh"
#include "../hiscores/hiscores.hh"
#include "../game/game.hh"
#include "rand.hh"
#include "math.hh"

namespace global
{
  int window_width_px = 600;
  int window_height_px = 600;
  bool is_debug_draw = false;
  bool is_debug_enabled = false;
  bool is_jm_invulnerable = false;
  bool is_classic_mode = true;
}

/** new width and height must be greater than world width and height. */
irect calculate_world_bitmap_blit_rect(int new_width_px, int new_height_px)
{
  global::window_width_px = new_width_px;
  global::window_height_px = new_height_px;

  assert(global::window_width_px >= con::world_width_px);
  assert(global::window_height_px >= con::world_height_px);

  auto scale_x = 1;
  while((con::world_width_px * (scale_x + 1)) < global::window_width_px){
    ++scale_x;
  }
  auto scale_y = 1;
  while((con::world_height_px * (scale_y + 1)) < global::window_height_px){
    ++scale_y;
  }
  auto world_bitmap_draw_scale = std::min(scale_x, scale_y);

  auto drawn_world_width_px = con::world_width_px * world_bitmap_draw_scale;
  auto drawn_world_height_px = con::world_height_px * world_bitmap_draw_scale;

  return {
    (global::window_width_px - drawn_world_width_px) / 2,
    (global::window_height_px - drawn_world_height_px) / 2,
    drawn_world_width_px,
    drawn_world_height_px,
  };
}

int main()
{
  float target_fps_hz = 60.f;
  double real_time_s = 0.0;
  double game_time_s = 0.0;
  float tick_delta_s;
  bool is_paused = false;
  bool is_stat_draw = false;
  bool is_fullscreen = false;
  int fps_tick_counter = 0;
  int ticks_done_this_frame = 0;
  int ticks_accumulated = 0;
  double fps_timer_s = 0.0;
  double fps = 0.0;
  int top_hiscore = 0;
  RenderTexture2D world_render_target;  // world drawn to this 224x256px texture, then scale drawn to window.
  irect world_bitmap_blit_rect;   // area of window to draw world_bitmap to.
  std::unique_ptr<game_data> game;
  double last_frame_time_s = 0.0;

  hiscores::reg::initialise();

  enum class app_state { title, menu, game, hi_score };
  app_state app_state_;
  const auto change_app_state = [&game, &app_state_](app_state new_state, int new_score = 0)
  {
    switch(new_state){
      case app_state::title: {title::on_enter(); break;}
      case app_state::menu: {menu::on_enter(); break;}
      case app_state::hi_score: {hiscores::reg::on_enter(new_score); break;}
      case app_state::game: {game::on_enter(*game); break;}
      default: assert(0);
    }
    app_state_ = new_state;
  };
  change_app_state(app_state::title, 1000);

  // load config file.
  {
    log(log_lvl::info, "loading config");

    // Note: Raylib doesn't have built-in config file loading, so we'll use default values
    // In a full implementation, you'd want to add a config file parser
    bool config_loaded = false;
    if(config_loaded){
      const auto log_set_value = [](const char* value_name, const char* value){
        std::stringstream ss{};
        ss << "set '" << fmt_bold << value_name << fmt_clear
        << "' to '" << fmt_bold << value << fmt_clear << "'.";
        log(log_lvl::info, ss.str());
      };

      auto log_invalid_value = [](const char* value_name, const char* value, const char* reason){
        std::stringstream ss{};
        ss << "invalid config value '" << fmt_bold << value << fmt_clear
        << "' for config property '" << fmt_bold << value_name << fmt_clear
        << "'; " << reason;
        log(log_lvl::warning, ss.str());
      };

      const char* value = nullptr;
      value = nullptr; // Placeholder - would read from config
      if(value != nullptr){
        try {
          target_fps_hz = std::stof(value);
          log_set_value("target_fps_hz", value);
        }
        catch(const std::exception& e){
          log_invalid_value("target_fps_hz", value, "expected double.");
        }
      }

      auto is_valid_window_size = true;
      value = nullptr; // Placeholder - would read from config
      if(value != nullptr){
        try {
          global::window_width_px = std::stoi(value);
          log_set_value("window_width_px", value);
        }
        catch(const std::exception& e){
          log_invalid_value("window_width_px", value, "expected integer.");
          is_valid_window_size = false;
        }
      }
      value = nullptr; // Placeholder - would read from config
      if(value != nullptr){
        try {
          global::window_height_px = std::stoi(value);
          log_set_value("window_height_px", value);
        }
        catch(const std::exception& e){
          log_invalid_value("window_height_px", value, "expected integer.");
          is_valid_window_size = false;
        }
      }
      if(global::window_width_px < con::world_width_px){
        log(log_lvl::info, "config value for window width too small! ignoring...");
        is_valid_window_size = false;
      }
      if(global::window_height_px < con::world_height_px){
        log(log_lvl::info, "config value for window height too small! ignoring...");
        is_valid_window_size = false;
      }
      if(!is_valid_window_size){
        global::window_width_px = con::world_width_px;
        global::window_height_px = con::world_height_px;
        log_set_value("window_width_px", std::to_string(global::window_width_px).c_str());
        log_set_value("window_height_px", std::to_string(global::window_height_px).c_str());
      }

      value = nullptr; // Placeholder - would read from config
      if(value != nullptr){
        if(std::strcmp("wasd", value) == 0){
          input::set_control_scheme(input::wasd_control_scheme);
          log_set_value("control_scheme", value);
        }
        else if(std::strcmp("arrows", value) == 0){
          input::set_control_scheme(input::arrows_control_scheme);
          log_set_value("control_scheme", value);
        }
        else {
          log_invalid_value("control_scheme", value, "unknown option");
        }
      }

      value = nullptr; // Placeholder - would read from config
      if(value != nullptr){
        try {
          auto debug_enabled = std::stoi(value);
          debug_enabled = debug_enabled > 0 ? 1 : 0;
          log_set_value("debug_mode", debug_enabled ? "true" : "false");
          global::is_debug_enabled = static_cast<bool>(debug_enabled);
        }
        catch(const std::exception& e){
          log_invalid_value("debug_mode", value, "expected integer (boolean).");
          global::is_debug_enabled = false;
        }
      }
      value = nullptr; // Placeholder - would read from config
      if(value != nullptr){
        try {
          auto invulnerable_enabled = std::stoi(value);
          invulnerable_enabled = invulnerable_enabled > 0 ? 1 : 0;
          log_set_value("invulnerable_mode", invulnerable_enabled ? "true" : "false");
          global::is_jm_invulnerable = static_cast<bool>(invulnerable_enabled);
        }
        catch(const std::exception& e){
          log_invalid_value("invulnerable_mode", value, "expected integer (boolean).");
          global::is_jm_invulnerable = false;
        }
      }
      value = nullptr; // Placeholder - would read from config
      if(value != nullptr){
        try {
          auto classic_enabled = std::stoi(value);
          classic_enabled = classic_enabled > 0 ? 1 : 0;
          log_set_value("classic mode", classic_enabled ? "true" : "false");
          global::is_classic_mode = static_cast<bool>(classic_enabled);
        }
        catch(const std::exception& e){
          log_invalid_value("invulnerable_mode", value, "expected integer (boolean).");
          global::is_classic_mode = true;
        }
      }
      value = nullptr; // Placeholder - would read from config
      if(value != nullptr){
        try {
          is_fullscreen = std::stoi(value) > 0;
          log_set_value("fullscreen mode", is_fullscreen ? "true" : "false");
        }
        catch(const std::exception& e){
          log_invalid_value("fullscreen", value, "expected integer (boolean).");
          is_fullscreen = false;
        }
      }
    }
    else log(log_lvl::warning, "failed to load config file");
  }

  // load hiscores
  {
    auto table = hiscores::load_hiscores();
    top_hiscore = hiscores::get_top_hiscore();
    hiscores::log_hiscores();
  }

  // initialise core
  {
    log(log_lvl::info, "initialising core.");

    // Initialize Raylib window
    if(is_fullscreen){
      InitWindow(global::window_width_px, global::window_height_px, "Donkey Kong 1981");
      ToggleFullscreen();
    } else {
      SetConfigFlags(FLAG_WINDOW_RESIZABLE);
      InitWindow(global::window_width_px, global::window_height_px, "Donkey Kong 1981");
    }
    
    global::window_width_px = GetScreenWidth();
    global::window_height_px = GetScreenHeight();
    
    SetTargetFPS(static_cast<int>(target_fps_hz));
    tick_delta_s = 1.f / target_fps_hz;

    // Create render texture for world (224x256)
    world_render_target = LoadRenderTexture(con::world_width_px, con::world_height_px);
    world_bitmap_blit_rect = calculate_world_bitmap_blit_rect(global::window_width_px, global::window_height_px);

    // seed random number generator.
    auto seq = std::seed_seq{std::time(nullptr)};
    auto seed_state = rnd::xorwow::state_type{};
    seq.generate(seed_state.begin(), seed_state.end());
    rnd::generator.seed(seed_state);
  }

  // initialise game.
  {
    log(log_lvl::info, "initialising game.");
    if(!load_sprite_sheets()){
      exit(EXIT_FAILURE);
    }
    if(!load_fonts()){
      exit(EXIT_FAILURE);
    }
    if(!load_sounds()){
      exit(EXIT_FAILURE);
    }
    game = std::move(game::create());
  }

  play_sound(SND_BOOT);

  // runloop
  {
    last_frame_time_s = GetTime();
    while(!WindowShouldClose())
    {
      // Handle input
      if(IsKeyPressed(KEY_ESCAPE)){
        break;
      }
      if(IsKeyPressed(KEY_TAB)){
        global::is_debug_draw = !global::is_debug_draw;
        auto debug_state_string = global::is_debug_draw ? "on" : "off";
        log(log_lvl::info, std::string{"debug mode "} + debug_state_string);
      }
      if(IsKeyPressed(KEY_O)){
        is_paused = !is_paused;
        auto pause_string = is_paused ? "paused game" : "unpaused game";
        log(log_lvl::info, pause_string);
      }
      if(IsKeyPressed(KEY_P)){
        is_stat_draw = !is_stat_draw;
        auto stats_string = is_stat_draw ? "enabling" : "disabling";
        log(log_lvl::info, stats_string + std::string{" runtime stats output"});
      }

      // Poll all keys for input system
      for(int key = 0; key < 512; key++){
        if(IsKeyPressed(key)){
          input::record_key_pressed(key);
        }
        if(IsKeyReleased(key)){
          input::record_key_released(key);
        }
      }

      // Handle window resize
      if(IsWindowResized()){
        global::window_width_px = GetScreenWidth();
        global::window_height_px = GetScreenHeight();
        if(global::window_width_px >= con::world_width_px && global::window_height_px >= con::world_height_px){
          world_bitmap_blit_rect = calculate_world_bitmap_blit_rect(global::window_width_px, global::window_height_px);
        }
      }

      // Calculate frame time and accumulate ticks
      double current_time_s = GetTime();
      double frame_time_s = current_time_s - last_frame_time_s;
      last_frame_time_s = current_time_s;
      
      real_time_s += frame_time_s;
      fps_timer_s += frame_time_s;
      ticks_accumulated += static_cast<int>(frame_time_s / tick_delta_s);

      // limiting ticks per frame prevents a 'spiral of death'.
      constexpr auto max_ticks_per_frame = 5;
      ticks_done_this_frame = 0;
      while(ticks_accumulated > 0 && ticks_done_this_frame < max_ticks_per_frame){
        if(!is_paused){
          switch(app_state_){
            case app_state::title:
              title::update(tick_delta_s);
              if(title::is_done()) change_app_state(app_state::menu); // TODO: switch to menu not game.
              break;
            case app_state::menu:
              menu::update(tick_delta_s);
              if(menu::is_playtime()) change_app_state(app_state::game);
              else if(menu::is_title_time()) change_app_state(app_state::title);
              break;
            case app_state::game:
              game::update(*game, tick_delta_s);
              if(game::is_done(*game)){
                auto final_score = game::get_game_stats(*game).score;
                if(hiscores::is_hiscore(final_score)) change_app_state(app_state::hi_score, final_score);
                else change_app_state(app_state::title);
              }
              break;
            case app_state::hi_score:
              hiscores::reg::update(tick_delta_s);
              if(hiscores::reg::is_done()) {
                top_hiscore = hiscores::get_top_hiscore();
                change_app_state(app_state::title);
              }
              break;
          }
          game_time_s += tick_delta_s;
        }
        update_sounds(); // Update Raylib music streams
        hud::update(tick_delta_s);
        input::update_key_states();
        --ticks_accumulated;
        ++ticks_done_this_frame;
        ++fps_tick_counter;
      }
      if(fps_timer_s > 1.0){
        fps = fps_tick_counter / fps_timer_s;
        fps_timer_s = 0.0;
        fps_tick_counter = 0;
      }

      // Draw to render texture (world)
      BeginTextureMode(world_render_target);
      ClearBackground(BLACK);
      switch(app_state_){
          case app_state::title: {title::draw(); break;}
          case app_state::menu: {menu::draw(); break;}
          case app_state::game: {game::draw(*game); break;}
          case app_state::hi_score: {hiscores::reg::draw(); break;}
          default: assert(0);
      }
      auto stats = app_state_ == app_state::game ? game::get_game_stats(*game) : game_stats{0, 0, 0};
      hud::draw({top_hiscore, stats.score, stats.level_number, stats.life_count});
      EndTextureMode();

      // Draw to screen
      BeginDrawing();
      ClearBackground(BLACK);
      
      // Draw the world render texture scaled to fit window
      Rectangle source = {0, 0, static_cast<float>(con::world_width_px), static_cast<float>(-con::world_height_px)};
      Rectangle dest = {
        static_cast<float>(world_bitmap_blit_rect.x),
        static_cast<float>(world_bitmap_blit_rect.y),
        static_cast<float>(world_bitmap_blit_rect.w),
        static_cast<float>(world_bitmap_blit_rect.h)
      };
      DrawTexturePro(world_render_target.texture, source, dest, {0, 0}, 0.0f, WHITE);
      if(is_stat_draw){
        // draw the runtime statistics window in bottom-left.
        constexpr auto box_w_px = 200;
        constexpr auto box_h_px = 70;
        DrawRectangle(
          0,
          global::window_height_px - box_h_px,
          box_w_px,
          box_h_px,
          BLUE
        );
        std::stringstream ss{};
        ss.precision(3);
        ss << "real_time(secs)=" << real_time_s << "\n"
           << "game_time(secs)=" << game_time_s << "\n"
           << "fps=" << fps << "\n"
           << "ticks_per_frame=" << ticks_done_this_frame;
        constexpr auto box_padding_px = 10;
        DrawText(
          ss.str().c_str(),
          box_padding_px,
          global::window_height_px - box_h_px + box_padding_px,
          10,
          WHITE
        );
      }
      if(is_paused){
        // draw the paused icon.
        Color fg_color = RED;
        constexpr auto radius_px = 30;
        constexpr auto diameter_px = radius_px * 2;
        auto center_x_px = 20 + radius_px;
        auto center_y_px = center_x_px;
        constexpr auto border_thickness_px = 4.f;
        DrawCircleLines(center_x_px, center_y_px, radius_px, fg_color);
        DrawCircleLines(center_x_px, center_y_px, radius_px - 1, fg_color);
        DrawCircleLines(center_x_px, center_y_px, radius_px - 2, fg_color);
        DrawCircleLines(center_x_px, center_y_px, radius_px - 3, fg_color);
        
        constexpr int stripe_half_gap_px = diameter_px * 0.06f;
        constexpr int stripe_w_px = diameter_px * 0.16f;
        constexpr int stripe_h_px = diameter_px * 0.4f;
        constexpr int stripe_half_h_px = stripe_h_px / 2.f;
        
        DrawRectangle(
          center_x_px - stripe_half_gap_px - stripe_w_px,
          center_y_px - stripe_half_h_px,
          stripe_w_px,
          stripe_h_px,
          fg_color
        );
        DrawRectangle(
          center_x_px + stripe_half_gap_px,
          center_y_px - stripe_half_h_px,
          stripe_w_px,
          stripe_h_px,
          fg_color
        );
      }
      
      EndDrawing();
    }
  }

  // shutdown game
  {
    log(log_lvl::info, "shutting down game.");
    unload_sprite_sheets();
    unload_fonts();
    unload_sounds();
  }

  // shutdown core
  {
    log(log_lvl::info, "shutting down core.");
    UnloadRenderTexture(world_render_target);
    CloseWindow();
  }
}
