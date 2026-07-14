#pragma once
#include "raylib.h"

// Tuning constants for the whole game. Everything the SPEC pins down lives here so
// it can be checked against the spec at a glance.
//
// World layout convention, used everywhere: the level is a 2D grid, so positions are
// Vector2 where .x is the world X axis and .y is the world Z axis. Height (world Y)
// is never a gameplay quantity — it only exists for the renderer.

// --- world scale -------------------------------------------------------------
constexpr float kTile  = 1.0f;   // one tile, one world unit
constexpr float kWallH = 1.4f;   // walls run floor-to-ceiling: no seeing over a gondola
constexpr float kEyeH  = 0.75f;

// --- player ------------------------------------------------------------------
constexpr float kMoveSpeed    = 3.5f;
constexpr float kTurnSpeed    = 120.0f * DEG2RAD;   // keyboard turn, radians/sec
constexpr float kMouseSens    = 0.0024f;            // radians per pixel of mouse X
constexpr float kPlayerRadius = 0.28f;

constexpr int kMaxHealth = 100;
constexpr int kMaxArmour = 100;
constexpr int kMaxAmmo   = 200;
constexpr int kStartAmmo = 40;

// --- weapons -----------------------------------------------------------------
constexpr int   kMeleeDamage   = 25;
constexpr float kMeleeRange    = 1.5f;
constexpr float kMeleeArc      = 35.0f * DEG2RAD;   // half-angle of the swing cone
constexpr float kMeleeCooldown = 0.5f;              // 2 swings/sec
constexpr float kMeleeAnim     = 0.42f;

constexpr int   kGunDamage   = 20;
constexpr float kGunCooldown = 0.25f;               // 4 shots/sec
constexpr float kGunAnim     = 0.2f;
constexpr float kGunSpread   = 0.018f;              // radians, each shot
constexpr float kGunRange    = 64.0f;               // "unlimited", but bounded by the map

// Slot 3: statiegeldkanon — a bottle-return intake that fires the deposit back.
constexpr int   kScatterPellets  = 7;
constexpr int   kScatterDamage   = 6;               // per pellet
constexpr float kScatterSpread   = 0.09f;           // radians, per pellet
constexpr float kScatterCooldown = 0.9f;
constexpr float kScatterAnim     = 0.3f;
constexpr int   kMaxFlessen      = 50;

// Slot 4: vuurwerkpijl — flat-flying rocket, splash hurts everyone. You included.
constexpr int   kRocketDamageMax = 80;              // at the blast centre
constexpr int   kRocketDamageMin = 20;              // at the blast edge
constexpr float kRocketRadius    = 1.6f;
constexpr float kRocketSpeed     = 10.0f;
constexpr float kRocketLaunchH   = 0.6f;
constexpr float kRocketCooldown  = 1.2f;
constexpr float kRocketAnim      = 0.35f;
constexpr int   kMaxVuurwerk     = 20;

// How far the noise of your own weapon wakes the shop up.
constexpr float kGunNoiseRange   = 11.0f;
constexpr float kMeleeNoiseRange = 4.5f;

// --- presentation ------------------------------------------------------------
constexpr int   kScreenW = 1280;
constexpr int   kScreenH = 720;
constexpr float kHudH    = 96.0f;   // status bar height in screen pixels
constexpr float kFovY    = 62.0f;
