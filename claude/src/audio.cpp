#include "audio.h"

#include <cmath>
#include <initializer_list>

#include "raylib.h"
#include "raymath.h"

// Nothing is loaded from disk — the asset pack ships no audio, and SPEC §2 forbids
// downloading any. Every sound here is a few oscillators and a noise generator,
// rendered into a Wave once at startup. It sounds cheap. A supermarket at 3am also
// sounds cheap.

namespace {

constexpr int kRate = 22050;
constexpr int kSfxCount = (int)Sfx::LevelDone + 1;

bool gReady = false;
Sound gSounds[kSfxCount]{};
Sound gHum{};
float gHumTimer = 0.0f;

unsigned int gNoise = 0x13579BDFu;

float Noise() {
    gNoise = gNoise * 1664525u + 1013904223u;
    return (float)((gNoise >> 9) & 0xFFFFu) / 32767.5f - 1.0f;
}

int Step(float t, float length, int count) {
    const int i = (int)(t / length);
    return (i < 0) ? 0 : (i >= count ? count - 1 : i);
}

float Square(float phase) { return (phase - floorf(phase) < 0.5f) ? 1.0f : -1.0f; }
float Saw(float phase) { return 2.0f * (phase - floorf(phase)) - 1.0f; }
float Sine(float phase) { return sinf(phase * 2.0f * PI); }

// `fn` is called once per sample, in order, so it can carry its own phase.
template <typename Fn>
Sound Render(float seconds, Fn fn) {
    Wave wave{};
    wave.frameCount = (unsigned int)(seconds * kRate);
    wave.sampleRate = kRate;
    wave.sampleSize = 16;
    wave.channels = 1;

    short* samples = (short*)MemAlloc(wave.frameCount * sizeof(short));
    for (unsigned int i = 0; i < wave.frameCount; i++) {
        const float t = (float)i / (float)kRate;
        samples[i] = (short)(Clamp(fn(t), -1.0f, 1.0f) * 30000.0f);
    }
    wave.data = samples;

    const Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}

}  // namespace

void AudioInit() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;   // no sound card, no problem: the game plays on
    gReady = true;

    gSounds[(int)Sfx::Swing] = Render(0.22f, [lp = 0.0f](float t) mutable {
        lp += (Noise() - lp) * 0.22f;                       // a baguette moves a lot of air
        const float e = sinf(PI * Clamp(t / 0.22f, 0.0f, 1.0f));
        return lp * e * e * 1.6f;
    });

    gSounds[(int)Sfx::Shot] = Render(0.16f, [ph = 0.0f](float t) mutable {
        const float f = 700.0f * expf(-t * 26.0f) + 110.0f;
        ph += f / kRate;
        const float env = expf(-t * 30.0f);
        return (Square(ph) * 0.5f + Noise() * 0.5f) * env;   // ka-chunk
    });

    gSounds[(int)Sfx::DryFire] = Render(0.05f, [](float t) {
        return Noise() * expf(-t * 90.0f) * 0.5f;
    });

    gSounds[(int)Sfx::Hit] = Render(0.12f, [ph = 0.0f](float t) mutable {
        ph += 150.0f / kRate;
        return (Noise() * 0.7f + Sine(ph) * 0.5f) * expf(-t * 26.0f);
    });

    gSounds[(int)Sfx::EnemyShoot] = Render(0.13f, [ph = 0.0f](float t) mutable {
        const float f = 2400.0f * expf(-t * 24.0f) + 260.0f;
        ph += f / kRate;
        return (Square(ph) * 0.6f + Noise() * 0.2f) * expf(-t * 16.0f);
    });

    gSounds[(int)Sfx::CanThrow] = Render(0.20f, [lp = 0.0f](float t) mutable {
        lp += (Noise() - lp) * 0.10f;
        return lp * expf(-t * 9.0f) * 1.4f;
    });

    gSounds[(int)Sfx::DeathCart] = Render(0.65f, [ph = 0.0f, ph2 = 0.0f](float t) mutable {
        ph += 380.0f / kRate;
        ph2 += 517.0f / kRate;
        // Four impacts: a trolley does not fall over quietly.
        float env = 0.0f;
        for (const float hit : {0.0f, 0.11f, 0.26f, 0.42f}) {
            if (t >= hit) env += expf(-(t - hit) * 26.0f);
        }
        env = Clamp(env, 0.0f, 1.4f);
        return (Square(ph) * 0.3f + Square(ph2) * 0.25f + Noise() * 0.55f) * env * 0.8f;
    });

    gSounds[(int)Sfx::DeathStocker] = Render(0.75f, [ph = 0.0f](float t) mutable {
        const float f = 112.0f - t * 55.0f + sinf(t * 17.0f) * 4.0f;
        ph += fmaxf(40.0f, f) / kRate;
        const float env = fminf(1.0f, t * 14.0f) * expf(-t * 3.0f);
        return (Saw(ph) * 0.6f + Noise() * 0.12f) * env;
    });

    gSounds[(int)Sfx::DeathScanner] = Render(0.7f, [ph = 0.0f](float t) mutable {
        const int step = (int)(t / 0.14f);
        const float f = (step < 3) ? 1250.0f - step * 330.0f : 0.0f;
        ph += f / kRate;
        const float local = fmodf(t, 0.14f);
        const float beep = (step < 3) ? Square(ph) * expf(-local * 16.0f) : 0.0f;
        const float fizzle = (t > 0.42f) ? Noise() * expf(-(t - 0.42f) * 9.0f) * 0.45f : 0.0f;
        return beep * 0.5f + fizzle;
    });

    gSounds[(int)Sfx::Pickup] = Render(0.13f, [ph = 0.0f](float t) mutable {
        ph += (t < 0.05f ? 1046.0f : 1568.0f) / kRate;       // the barcode scanner, obviously
        return Square(ph) * 0.35f * expf(-fmodf(t, 0.05f) * 8.0f);
    });

    gSounds[(int)Sfx::KeycardGet] = Render(0.55f, [ph = 0.0f](float t) mutable {
        const float notes[4] = {784.0f, 1046.0f, 1318.0f, 1568.0f};
        ph += notes[Step(t, 0.14f, 4)] / kRate;
        return Square(ph) * 0.3f * expf(-fmodf(t, 0.14f) * 9.0f);
    });

    gSounds[(int)Sfx::DoorLocked] = Render(0.4f, [ph = 0.0f](float t) mutable {
        ph += 108.0f / kRate;
        const float gate = (fmodf(t, 0.1f) < 0.06f) ? 1.0f : 0.0f;
        return Square(ph) * gate * 0.4f * expf(-t * 2.5f);
    });

    gSounds[(int)Sfx::DoorOpen] = Render(0.9f, [ph = 0.0f, lp = 0.0f](float t) mutable {
        ph += (180.0f + t * 200.0f) / kRate;
        lp += (Noise() - lp) * 0.05f;                        // hydraulics, and rust
        const float env = sinf(PI * Clamp(t / 0.9f, 0.0f, 1.0f));
        return (Sine(ph) * 0.25f + lp * 1.6f) * env;
    });

    gSounds[(int)Sfx::Hurt] = Render(0.3f, [ph = 0.0f](float t) mutable {
        ph += (150.0f - t * 220.0f) / kRate;
        return (Saw(ph) * 0.5f + Noise() * 0.4f) * expf(-t * 12.0f);
    });

    gSounds[(int)Sfx::Died] = Render(1.5f, [ph = 0.0f](float t) mutable {
        const float f = 190.0f * expf(-t * 1.6f) + 34.0f;
        ph += f / kRate;
        return (Saw(ph) * 0.55f + Noise() * 0.1f) * expf(-t * 1.5f);
    });

    gSounds[(int)Sfx::Escaped] = Render(1.3f, [ph = 0.0f](float t) mutable {
        const float notes[6] = {523.0f, 659.0f, 784.0f, 1046.0f, 1318.0f, 1568.0f};
        ph += notes[Step(t, 0.16f, 6)] / kRate;
        const float env = expf(-fmodf(t, 0.16f) * 6.0f) * (t > 0.96f ? expf(-(t - 0.96f) * 3.0f) : 1.0f);
        return Square(ph) * 0.28f * env;
    });

    gSounds[(int)Sfx::Scattergun] = Render(0.32f, [lp = 0.0f, ph = 0.0f](float t) mutable {
        lp += (Noise() - lp) * 0.35f;                        // a crate of glass, all at once
        ph += (90.0f * expf(-t * 18.0f) + 48.0f) / kRate;
        return (lp * 1.2f + Saw(ph) * 0.4f) * expf(-t * 11.0f) * 1.5f;
    });

    gSounds[(int)Sfx::RocketLaunch] = Render(0.5f, [lp = 0.0f](float t) mutable {
        lp += (Noise() - lp) * (0.08f + t * 0.5f);           // fuse, then whoosh
        return lp * 2.2f * fminf(1.0f, t * 12.0f) * expf(-t * 4.5f);
    });

    gSounds[(int)Sfx::Explosion] = Render(0.9f, [lp = 0.0f, ph = 0.0f](float t) mutable {
        lp += (Noise() - lp) * 0.12f;
        ph += (120.0f * expf(-t * 3.0f) + 30.0f) / kRate;
        return (lp * 1.6f + Sine(ph) * 0.6f) * expf(-t * 3.2f) * 1.4f;
    });

    gSounds[(int)Sfx::WeaponUp] = Render(0.45f, [ph = 0.0f](float t) mutable {
        const float notes[3] = {659.0f, 880.0f, 1318.0f};
        ph += notes[Step(t, 0.15f, 3)] / kRate;
        return Square(ph) * 0.3f * expf(-fmodf(t, 0.15f) * 7.0f);
    });

    gSounds[(int)Sfx::WeaponSwitch] = Render(0.06f, [ph = 0.0f](float t) mutable {
        ph += 300.0f / kRate;
        return (Noise() * 0.4f + Square(ph) * 0.3f) * expf(-t * 70.0f);
    });

    gSounds[(int)Sfx::GuardShot] = Render(0.18f, [ph = 0.0f](float t) mutable {
        const float f = 1400.0f * expf(-t * 30.0f) + 180.0f;
        ph += f / kRate;
        return (Square(ph) * 0.5f + Noise() * 0.4f) * expf(-t * 22.0f);
    });

    gSounds[(int)Sfx::DeathGuard] = Render(0.7f, [ph = 0.0f, lp = 0.0f](float t) mutable {
        ph += fmaxf(50.0f, 130.0f - t * 90.0f) / kRate;
        lp += (Noise() - lp) * 0.3f;
        const float radio = (t < 0.25f) ? lp * 1.2f : 0.0f;   // the radio dies first
        return radio + Saw(ph) * 0.5f * fminf(1.0f, t * 10.0f) * expf(-t * 3.5f);
    });

    gSounds[(int)Sfx::AlertBoss] = Render(0.8f, [ph = 0.0f, ph2 = 0.0f](float t) mutable {
        ph += (70.0f + sinf(t * 30.0f) * 6.0f) / kRate;       // a bellow from the office
        ph2 += 141.0f / kRate;
        const float env = fminf(1.0f, t * 8.0f) * expf(-t * 2.2f);
        return (Saw(ph) * 0.6f + Saw(ph2) * 0.25f + Noise() * 0.15f) * env * 1.2f;
    });

    gSounds[(int)Sfx::LevelDone] = Render(0.8f, [ph = 0.0f](float t) mutable {
        const float notes[4] = {523.0f, 659.0f, 784.0f, 1046.0f};
        ph += notes[Step(t, 0.13f, 4)] / kRate;
        const float tail = (t > 0.52f) ? expf(-(t - 0.52f) * 4.0f) : 1.0f;
        return Square(ph) * 0.3f * expf(-fmodf(t, 0.13f) * 7.0f) * tail;
    });

    gSounds[(int)Sfx::DeathBoss] = Render(1.4f, [ph = 0.0f, lp = 0.0f](float t) mutable {
        ph += (100.0f * expf(-t * 1.8f) + 28.0f) / kRate;
        lp += (Noise() - lp) * 0.1f;
        float thud = 0.0f;                                    // he lands twice
        for (const float hit : {0.55f, 0.8f}) {
            if (t >= hit) thud += expf(-(t - hit) * 20.0f);
        }
        return Saw(ph) * 0.5f * expf(-t * 1.6f) + lp * 1.4f * thud;
    });

    // The building itself: compressors, a strip light, the till that never sleeps.
    gHum = Render(3.6f, [a = 0.0f, b = 0.0f, lp = 0.0f](float t) mutable {
        a += 49.5f / kRate;
        b += 99.0f / kRate;
        lp += (Noise() - lp) * 0.008f;
        const float breathe = 0.75f + 0.25f * sinf(t * 0.9f);
        const float fade = fminf(1.0f, fminf(t, 3.6f - t) / 0.2f);
        return (Sine(a) * 0.5f + Sine(b) * 0.18f + lp * 2.2f) * breathe * fade;
    });
}

void AudioShutdown() {
    if (!gReady) return;
    for (Sound& s : gSounds) UnloadSound(s);
    UnloadSound(gHum);
    CloseAudioDevice();
}

void AudioUpdate(float dt, bool playing) {
    if (!gReady) return;

    if (!playing) {
        gHumTimer = 0.0f;
        return;
    }
    gHumTimer -= dt;
    if (gHumTimer <= 0.0f) {
        SetSoundVolume(gHum, 0.35f);
        PlaySound(gHum);
        gHumTimer = 3.4f;   // just short of the loop, so the seam sits inside the fade
    }
}

void PlaySfx(Sfx id, float volume) {
    if (!gReady) return;
    Sound& s = gSounds[(int)id];
    SetSoundVolume(s, Clamp(volume, 0.0f, 1.0f));
    PlaySound(s);
}

void PlaySfxAt(Sfx id, float distance, float volume) {
    const float falloff = Clamp(1.0f - distance / 20.0f, 0.06f, 1.0f);
    PlaySfx(id, volume * falloff);
}
