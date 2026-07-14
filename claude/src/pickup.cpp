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

        case PickupKind::Flessen:
            if (p.flessen >= kMaxFlessen) return false;
            p.flessen = (p.flessen + 8 > kMaxFlessen) ? kMaxFlessen : p.flessen + 8;
            w.Message("KRAT STATIEGELDFLESSEN  +8");
            return true;

        case PickupKind::Vuurwerk:
            if (p.vuurwerk >= kMaxVuurwerk) return false;
            p.vuurwerk = (p.vuurwerk + 4 > kMaxVuurwerk) ? kMaxVuurwerk : p.vuurwerk + 4;
            w.Message("VUURWERK  +4");
            return true;

        case PickupKind::WeaponScatter: {
            const bool owned = p.hasWeapon[(int)WeaponId::Statiegeldkanon];
            if (owned && p.flessen >= kMaxFlessen) return false;
            p.flessen = (p.flessen + 12 > kMaxFlessen) ? kMaxFlessen : p.flessen + 12;
            if (!owned) {
                p.hasWeapon[(int)WeaponId::Statiegeldkanon] = true;
                p.weapon = WeaponId::Statiegeldkanon;
                w.Message("3  STATIEGELDKANON - HET STATIEGELD KOMT TERUG");
            } else {
                w.Message("KRAT STATIEGELDFLESSEN  +12");
            }
            return true;
        }

        case PickupKind::WeaponRocket: {
            const bool owned = p.hasWeapon[(int)WeaponId::Vuurwerkpijl];
            if (owned && p.vuurwerk >= kMaxVuurwerk) return false;
            p.vuurwerk = (p.vuurwerk + 4 > kMaxVuurwerk) ? kMaxVuurwerk : p.vuurwerk + 4;
            if (!owned) {
                p.hasWeapon[(int)WeaponId::Vuurwerkpijl] = true;
                p.weapon = WeaponId::Vuurwerkpijl;
                w.Message("4  VUURWERKPIJL - NIET OP DE VERKOOPVLOER RICHTEN");
            } else {
                w.Message("VUURWERK  +4");
            }
            return true;
        }
    }
    return false;
}

// A first-time weapon grab gets the fanfare; everything else keeps its old voice.
static Sfx SfxFor(const Player& p, PickupKind kind) {
    switch (kind) {
        case PickupKind::Keycard:
            return Sfx::KeycardGet;
        case PickupKind::WeaponScatter:
            return p.hasWeapon[(int)WeaponId::Statiegeldkanon] ? Sfx::Pickup
                                                               : Sfx::WeaponUp;
        case PickupKind::WeaponRocket:
            return p.hasWeapon[(int)WeaponId::Vuurwerkpijl] ? Sfx::Pickup
                                                            : Sfx::WeaponUp;
        default:
            return Sfx::Pickup;
    }
}

void PickupsUpdate(World& w, float dt) {
    if (w.player.dead()) return;

    for (Pickup& p : w.pickups) {
        if (p.taken) continue;
        p.phase += dt * 2.0f;

        if (Vector2Distance(p.pos, w.player.pos) > kPlayerRadius + 0.28f) continue;
        const Sfx voice = SfxFor(w.player, p.kind);   // before Consume flips ownership
        if (!Consume(w, p.kind)) continue;

        p.taken = true;
        PlaySfx(voice);
        w.player.muzzleFlash = fmaxf(w.player.muzzleFlash, 0.45f);   // the pickup flash
    }
}
