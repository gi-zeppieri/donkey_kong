#include <array>
#include <string>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <unordered_map>
#include "raylib.h"
#include "sounds.hh"
#include "../core/log.hh"

namespace
{
  struct sound_instance {
    bool is_music;  // true if Music, false if Sound
    union {
      Sound* sound;
      Music* music;
    };
    float speed;
    float gain;
    bool should_loop;
  };

  /** internal store of all loaded sound effects. */
  std::array<Sound, SND_COUNT> audio_samples;
  std::array<Music, SND_COUNT> audio_music;
  
  /** track playing sound instances */
  static int next_play_id = 1;
  std::unordered_map<int, sound_instance> playing_sounds;

  std::array<const char*, SND_COUNT> snd_filenames {
    "music_1.wav",
    "music_2.wav",
    "music_hammer.wav",
    "death.wav",
    "smash.wav",
    "how_high.wav",
    "intro.wav",
    "jump.wav",
    "score.wav",
    "run.wav",
    "fall.wav",
    "win_1.wav",
    "win_2.wav",
    "win_level.wav",
    "spring_bounce.wav",
    "spring_fall.wav",
    "oil_light.wav",
    "boot.wav",
    "countdown.wav",
  };

  const std::string snd_path {"assets/sounds/"};
}

bool load_sounds()
{
  log(log_lvl::info, "loading sounds");
  
  InitAudioDevice();

  auto log_loading = [](const char* snd_filename){
    std::stringstream ss{};
    ss << "loading sound '" << fmt_bold << snd_filename << fmt_clear << "'.";
    log(log_lvl::info, ss.str());
  };

  auto log_load_fail = [](const char* snd_filename) {
    std::stringstream ss{};
    ss << "failed to load sound '" << fmt_bold << snd_filename << fmt_clear << "'.";
    log(log_lvl::fatal, ss.str());
  };

  for(auto snd = 0; snd < SND_COUNT; ++snd){
    log_loading(snd_filenames[snd]);
    std::string filepath = snd_path + snd_filenames[snd];
    
    // Load both as Sound and Music
    audio_samples[snd] = LoadSound(filepath.c_str());
    audio_music[snd] = LoadMusicStream(filepath.c_str());
    
    if(!IsSoundReady(audio_samples[snd]) || !IsMusicReady(audio_music[snd])){
      log_load_fail(snd_filenames[snd]);
      return false;
    }
  }

  return true;
}

void unload_sounds()
{
  log(log_lvl::info, "unloading sounds");
  
  // Stop all playing sounds
  playing_sounds.clear();
  
  for(auto& sample : audio_samples){
    UnloadSound(sample);
  }
  for(auto& music : audio_music){
    UnloadMusicStream(music);
  }
  
  CloseAudioDevice();
}

void update_sounds()
{
  // Update all music streams
  for(auto& [id, instance] : playing_sounds){
    if(instance.is_music){
      UpdateMusicStream(*instance.music);
      
      // Check if music finished and should loop
      if(instance.should_loop && !IsMusicStreamPlaying(*instance.music)){
        PlayMusicStream(*instance.music);
      }
    }
  }
  
  // Clean up finished non-looping sounds
  for(auto it = playing_sounds.begin(); it != playing_sounds.end();){
    auto& instance = it->second;
    bool is_playing = instance.is_music ? 
      IsMusicStreamPlaying(*instance.music) : 
      IsSoundPlaying(*instance.sound);
    
    if(!is_playing && !instance.should_loop){
      it = playing_sounds.erase(it);
    } else {
      ++it;
    }
  }
}

snd_play_id play_sound(sound_id snd, float speed, bool loop, float gain)
{
  assert(0 <= snd && snd < SND_COUNT);
  speed = std::clamp(speed, 0.f, 1.f);
  
  int play_id = next_play_id++;
  sound_instance instance;
  instance.speed = speed;
  instance.gain = gain;
  instance.should_loop = loop;
  
  if(loop){
    // Use Music for looping sounds
    instance.is_music = true;
    instance.music = &audio_music[snd];
    
    SetMusicVolume(*instance.music, gain);
    SetMusicPitch(*instance.music, speed);
    PlayMusicStream(*instance.music);
  } else {
    // Use Sound for one-shot sounds
    instance.is_music = false;
    instance.sound = &audio_samples[snd];
    
    SetSoundVolume(*instance.sound, gain);
    SetSoundPitch(*instance.sound, speed);
    PlaySound(*instance.sound);
  }
  
  playing_sounds[play_id] = instance;
  return play_id;
}

void stop_sound(snd_play_id id)
{
  if(!id.has_value()) return;
  
  auto it = playing_sounds.find(id.value());
  if(it != playing_sounds.end()){
    auto& instance = it->second;
    if(instance.is_music){
      StopMusicStream(*instance.music);
    } else {
      StopSound(*instance.sound);
    }
    playing_sounds.erase(it);
  }
}
