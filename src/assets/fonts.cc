#include <array>
#include "raylib.h"
#include "fonts.hh"
#include "../core/log.hh"

static Font game_font;
static std::array<Color, TEXT_COLOR_COUNT> text_color_palette;

bool load_fonts()
{
  text_color_palette[TEXT_WHITE] = {255, 255, 255, 255};
  text_color_palette[TEXT_RED] = {255, 0, 0, 255};
  text_color_palette[TEXT_BLUE] = {0, 0, 170, 255};
  text_color_palette[TEXT_GREEN] = {0, 255, 0, 255};
  text_color_palette[TEXT_YELLOW] = {255, 184, 0, 255};
  text_color_palette[TEXT_CYAN] = {0, 255, 255, 255};
  text_color_palette[TEXT_ORANGE] = {255, 121, 0, 255};
  text_color_palette[TEXT_PINK] = {255, 33, 85, 255};

  constexpr const char* font_path = "assets/fonts/PressStart2P-Regular.ttf";
  game_font = LoadFontEx(font_path, 8, nullptr, 0);
  if(!IsFontReady(game_font)){
    std::string msg{"failed to load game font '"};
    msg += font_path;
    msg += "'";
    log(log_lvl::fatal, msg);
    return false;
  }
  return true;
}

void unload_fonts()
{
  if(IsFontReady(game_font)){
    UnloadFont(game_font);
  }
}

void draw_text(int left_px, int bottom_px, const char* text, text_color color)
{
  Vector2 position = {static_cast<float>(left_px), static_cast<float>(bottom_px)};
  DrawTextEx(game_font, text, position, 8.0f, 0.0f, text_color_palette[color]);
}

void draw_text(int left_px, int bottom_px, std::string text, text_color color)
{
  Vector2 position = {static_cast<float>(left_px), static_cast<float>(bottom_px)};
  DrawTextEx(game_font, text.c_str(), position, 8.0f, 0.0f, text_color_palette[color]);
}
