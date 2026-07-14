#include "pickup.h"

#include <cmath>

#include "audio.h"
#include "config.h"
#include "raymath.h"
#include "world.h"

// Returns false if the player is already full of whatever this is — in which case it
// stays on the floor for later, exactly like Doom.
static bool Consume(World& w, PickupKind kind) {
    Player& p = w.player;

    switch (kind) {
        case PickupKind::Appelflap:
            if (p.health >= kMaxHealth) return false;
            p.health = (p.health + 25 > kMaxHealth) ? kMaxHealth : p.health + 25;
            w.Message("APPELFLAP  +25");
            return true;

        case PickupKind::Rookworst:
            if (p.health >= kMaxHealth) return false;
            p.health = (p.health + 50 > kMaxHealth) ? kMaxHealth : p.health + 50;
            w.Message("ROOKWORST  +50");
            return true;

        case PickupKind::Labels:
            if (p.ammo >= kMaxAmmo) return false;
            p.ammo = (p.ammo + 20 > kMaxAmmo) ? kMaxAmmo : p.ammo + 20;
            w.Message("ROL LABELS  +20");
            return true;

        case PickupKind::Bonuskaart:
            if (p.armour >= kMaxArmour) return false;
            p.armour = (p.armour + 50 > kMaxArmour) ? kMaxArmour : p.armour + 50;
            w.Message("BONUSKAART  +50 BESCHERMING");
            return true;

        case PickupKind::Keycard:
            p.hasKeycard = true;
            w.Message("PAS VAN DE BEDRIJFSLEIDER - HET MAGAZIJN IS NU OPEN");
            return true;
    }
    return false;
}

void PickupsUpdate(World& w, float dt) {
    if (w.player.dead()) return;

    for (Pickup& p : w.pickups) {
        if (p.taken) continue;
        p.phase += dt * 2.0f;

        if (Vector2Distance(p.pos, w.player.pos) > kPlayerRadius + 0.28f) continue;
        if (!Consume(w, p.kind)) continue;

        p.taken = true;
        PlaySfx(p.kind == PickupKind::Keycard ? Sfx::KeycardGet : Sfx::Pickup);
        w.player.muzzleFlash = fmaxf(w.player.muzzleFlash, 0.45f);   // the pickup flash
    }
}
