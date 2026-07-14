#include "assets.h"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <string>

// ---------------------------------------------------------------------------
// Asset directory lookup. The grader runs ./build/ah_hell_aisle from the
// competitor folder root, so ../assets is the expected location; the fallbacks
// cover running from build/ or the repo root during development.
// ---------------------------------------------------------------------------
static std::string FindAssetDir() {
  const char* candidates[] = { "../assets", "assets", "../../assets" };
  for (const char* c : candidates) {
    if (FileExists((std::string(c) + "/MANIFEST.md").c_str())) return c;
  }
  std::string byExe = std::string(GetApplicationDirectory()) + "../../assets";
  if (FileExists((byExe + "/MANIFEST.md").c_str())) return byExe;
  TraceLog(LOG_FATAL, "assets/ not found - run ./build/ah_hell_aisle from the folder root");
  return "../assets";
}

static Texture2D LoadTex(const std::string& dir, const char* name) {
  Texture2D t = LoadTexture((dir + "/" + name).c_str());
  if (t.id == 0) TraceLog(LOG_FATAL, "missing asset: %s", name);
  return t;
}

// ---------------------------------------------------------------------------
// Sound synthesis. MANIFEST.md ships no audio; everything below is generated
// at load time: 22050 Hz, 16-bit mono, short envelopes.
// ---------------------------------------------------------------------------
static constexpr int kRate = 22050;

static float Noise() { return (float)rand() / RAND_MAX * 2.0f - 1.0f; }

static Sound Synth(float dur, const std::function<float(float)>& sample) {
  unsigned int frames = (unsigned int)(dur * kRate);
  short* data = (short*)MemAlloc(frames * sizeof(short));
  for (unsigned int i = 0; i < frames; i++) {
    float t = (float)i / kRate;
    float v = sample(t);
    if (v > 1) v = 1;
    if (v < -1) v = -1;
    data[i] = (short)(v * 32000);
  }
  Wave w{};
  w.frameCount = frames;
  w.sampleRate = kRate;
  w.sampleSize = 16;
  w.channels = 1;
  w.data = data;
  Sound s = LoadSoundFromWave(w);
  UnloadWave(w);
  return s;
}

static float EnvOut(float t, float dur) { return 1.0f - t / dur; }

static void LoadSounds(Assets& A) {
  if (!IsAudioDeviceReady()) return;
  A.audio = true;
  const float PI2 = 2.0f * PI;

  A.sShoot = Synth(0.10f, [PI2](float t) {  // price gun: snappy click + zap
    float f = 1600.0f - 9000.0f * t;
    return (sinf(PI2 * f * t) * 0.6f + Noise() * 0.4f) * EnvOut(t, 0.10f) * EnvOut(t, 0.10f);
  });
  A.sClick = Synth(0.05f, [PI2](float t) {  // dry-fire
    return sinf(PI2 * 900 * t) * 0.4f * EnvOut(t, 0.05f);
  });
  A.sSwing = Synth(0.18f, [](float t) {  // baguette whoosh: swept noise
    static float lp = 0;
    lp += (Noise() - lp) * (0.10f + 0.5f * t / 0.18f);
    float env = sinf(PI * t / 0.18f);
    return lp * env * 1.6f;
  });
  A.sEnemyHit = Synth(0.09f, [PI2](float t) {
    return (Noise() * 0.5f + sinf(PI2 * 220 * t) * 0.5f) * EnvOut(t, 0.09f);
  });
  A.sEnemyDie = Synth(0.38f, [PI2](float t) {
    float f = 320.0f - 640.0f * t;
    if (f < 50) f = 50;
    return (sinf(PI2 * f * t) * 0.55f + Noise() * 0.30f) * EnvOut(t, 0.38f);
  });
  A.sHurt = Synth(0.25f, [PI2](float t) {
    float f = 150.0f - 280.0f * t;
    if (f < 55) f = 55;
    float sq = sinf(PI2 * f * t) > 0 ? 1.0f : -1.0f;
    return sq * 0.35f * EnvOut(t, 0.25f);
  });
  A.sPickup = Synth(0.16f, [PI2](float t) {
    float f = t < 0.08f ? 740.0f : 1108.0f;
    return sinf(PI2 * f * t) * 0.4f * EnvOut(t, 0.16f);
  });
  A.sKeycard = Synth(0.42f, [PI2](float t) {  // ascending triad
    float f = t < 0.14f ? 523.0f : (t < 0.28f ? 659.0f : 784.0f);
    return sinf(PI2 * f * t) * 0.42f * EnvOut(t, 0.42f);
  });
  A.sDoorOpen = Synth(0.65f, [PI2](float t) {  // steel roller door rumble
    static float lp = 0;
    lp += (Noise() - lp) * 0.06f;
    return (lp * 1.2f + sinf(PI2 * 70 * t) * 0.3f) * sinf(PI * t / 0.65f);
  });
  A.sLocked = Synth(0.28f, [PI2](float t) {  // double buzz
    float gate = (t < 0.10f || (t > 0.15f && t < 0.25f)) ? 1.0f : 0.0f;
    float sq = sinf(PI2 * 180 * t) > 0 ? 1.0f : -1.0f;
    return sq * 0.30f * gate;
  });
  A.sScanner = Synth(0.11f, [PI2](float t) {  // barcode laser zap
    float f = 1800.0f - 13000.0f * t;
    if (f < 250) f = 250;
    return sinf(PI2 * f * t) * 0.5f * EnvOut(t, 0.11f);
  });
  A.sThrow = Synth(0.14f, [](float t) {
    static float lp = 0;
    lp += (Noise() - lp) * 0.25f;
    return lp * sinf(PI * t / 0.14f) * 1.1f;
  });
  A.sWin = Synth(0.85f, [PI2](float t) {  // major arpeggio
    float f = t < 0.2f ? 523.0f : (t < 0.4f ? 659.0f : (t < 0.6f ? 784.0f : 1046.0f));
    return sinf(PI2 * f * t) * 0.4f * EnvOut(t, 0.85f);
  });
  A.sLose = Synth(0.95f, [PI2](float t) {  // descending minor
    float f = t < 0.3f ? 392.0f : (t < 0.6f ? 311.0f : 261.0f);
    return (sinf(PI2 * f * t) * 0.45f + Noise() * 0.06f) * EnvOut(t, 0.95f);
  });
}

Assets LoadAssets() {
  Assets A{};
  const std::string dir = FindAssetDir();

  A.wallPlain      = LoadTex(dir, "wall_plain.png");
  A.wallShelfFull  = LoadTex(dir, "wall_shelf_full.png");
  A.wallShelfEmpty = LoadTex(dir, "wall_shelf_empty.png");
  A.wallFreezer    = LoadTex(dir, "wall_freezer.png");
  A.wallCheckout   = LoadTex(dir, "wall_checkout.png");
  A.wallMagazijn   = LoadTex(dir, "wall_magazijn.png");
  A.doorKeycard    = LoadTex(dir, "door_keycard.png");
  A.doorExit       = LoadTex(dir, "door_exit.png");
  A.floor          = LoadTex(dir, "floor.png");
  A.ceiling        = LoadTex(dir, "ceiling.png");

  A.enemy[(int)EnemyType::Winkelwagen]  = LoadTex(dir, "enemy_winkelwagen.png");
  A.enemy[(int)EnemyType::Vakkenvuller] = LoadTex(dir, "enemy_vakkenvuller.png");
  A.enemy[(int)EnemyType::Zelfscanner]  = LoadTex(dir, "enemy_zelfscanner.png");
  A.can = LoadTex(dir, "proj_soepblik.png");

  A.pickups[(int)PickupType::Appelflap]  = LoadTex(dir, "pickup_appelflap.png");
  A.pickups[(int)PickupType::Rookworst]  = LoadTex(dir, "pickup_rookworst.png");
  A.pickups[(int)PickupType::Labels]     = LoadTex(dir, "pickup_labels.png");
  A.pickups[(int)PickupType::Bonuskaart] = LoadTex(dir, "pickup_bonuskaart.png");
  A.pickups[(int)PickupType::Keycard]    = LoadTex(dir, "pickup_keycard.png");

  A.weaponStok = LoadTex(dir, "weapon_stokbrood.png");
  A.weaponPist = LoadTex(dir, "weapon_prijspistool.png");
  A.hudPanel   = LoadTex(dir, "hud_panel.png");
  A.hudFace    = LoadTex(dir, "hud_face.png");

  LoadSounds(A);
  return A;
}

void UnloadAssets(Assets& A) {
  Texture2D* all[] = {
    &A.wallPlain, &A.wallShelfFull, &A.wallShelfEmpty, &A.wallFreezer,
    &A.wallCheckout, &A.wallMagazijn, &A.doorKeycard, &A.doorExit, &A.floor,
    &A.ceiling, &A.enemy[0], &A.enemy[1], &A.enemy[2], &A.can, &A.pickups[0],
    &A.pickups[1], &A.pickups[2], &A.pickups[3], &A.pickups[4], &A.weaponStok,
    &A.weaponPist, &A.hudPanel, &A.hudFace,
  };
  for (Texture2D* t : all) UnloadTexture(*t);
  if (A.audio) {
    Sound* snds[] = { &A.sShoot, &A.sClick, &A.sSwing, &A.sEnemyHit,
                      &A.sEnemyDie, &A.sHurt, &A.sPickup, &A.sKeycard,
                      &A.sDoorOpen, &A.sLocked, &A.sScanner, &A.sThrow,
                      &A.sWin, &A.sLose };
    for (Sound* s : snds) UnloadSound(*s);
  }
}

void Sfx(const Assets& A, const Sound& s) {
  if (A.audio && s.frameCount > 0) PlaySound(s);
}

Texture2D WallTexture(const Assets& A, Tile t) {
  switch (t) {
    case Tile::Plain:      return A.wallPlain;
    case Tile::ShelfFull:  return A.wallShelfFull;
    case Tile::ShelfEmpty: return A.wallShelfEmpty;
    case Tile::Freezer:    return A.wallFreezer;
    case Tile::Checkout:   return A.wallCheckout;
    case Tile::Magazijn:   return A.wallMagazijn;
    case Tile::DoorKey:    return A.doorKeycard;
    case Tile::DoorExit:   return A.doorExit;
    default:               return A.wallPlain;
  }
}
