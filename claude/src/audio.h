#pragma once

// Every sound is synthesised in code at startup — the asset pack ships no audio and
// SPEC §2 forbids fetching any. It is all crude, and that suits the building.
enum class Sfx {
    Swing,          // stokbrood
    Shot,           // prijspistool
    DryFire,
    Hit,            // you connected
    EnemyShoot,     // zelfscanner beam
    CanThrow,
    DeathCart,
    DeathStocker,
    DeathScanner,
    Pickup,         // a barcode beep, because of course it is
    KeycardGet,
    DoorLocked,
    DoorOpen,
    Hurt,
    Died,
    Escaped,
    Scattergun,     // statiegeldkanon
    RocketLaunch,   // vuurwerkpijl leaves the pipe
    Explosion,
    WeaponUp,       // picked up a new gun
    WeaponSwitch,
    GuardShot,      // the beveiliger's sidearm
    DeathGuard,
    AlertBoss,      // the bedrijfsleider has seen you
    DeathBoss,
    LevelDone,      // keep last: kSfxCount counts from here
};

void AudioInit();
void AudioShutdown();
void AudioUpdate(float dt, bool playing);   // keeps the refrigeration hum going
void PlaySfx(Sfx id, float volume = 1.0f);
void PlaySfxAt(Sfx id, float distance, float volume = 1.0f);   // quieter further away
