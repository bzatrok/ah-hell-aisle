#include "net.h"

#include <cmath>

#include "audio.h"
#include "config.h"
#include "raymath.h"
#include "web_net.h"
#include "world.h"

NetState gNet;

namespace {

constexpr float kStateInterval = 1.0f / 15.0f;   // my position, 15 Hz
constexpr float kEnemyInterval = 1.0f / 10.0f;   // the host's monsters, 10 Hz

// A jump this large between packets is a respawn or a rejoin, not motion.
constexpr float kTeleportDist = 3.0f;

float WrapAngle(float a) {
    while (a > PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

void TickPeers(float dt) {
    for (RemotePlayer& p : gNet.peers) {
        p.sinceLast += dt;
        p.fireAnim = fmaxf(0.0f, p.fireAnim - dt);
        p.hurtFlash = fmaxf(0.0f, p.hurtFlash - dt * 3.6f);
        if (p.corpseTimer >= 0.0f) p.corpseTimer += dt;

        if (p.lerpT < 1.0f) {
            p.lerpT = fminf(1.0f, p.lerpT + dt / p.lerpDur);
            p.pos = Vector2Lerp(p.fromPos, p.toPos, p.lerpT);
            p.yaw = p.fromYaw + WrapAngle(p.toYaw - p.fromYaw) * p.lerpT;
        }

        // Walk frames come from covered ground, not received flags: a segment
        // longer than ~2 cm means their feet are doing something.
        p.moving = p.lerpT < 1.0f &&
                   Vector2DistanceSqr(p.fromPos, p.toPos) > 0.0004f;
        if (p.moving) {
            p.animTimer += dt;
            if (p.animTimer >= 0.44f) p.animTimer = 0.0f;
        } else {
            p.animTimer = 0.0f;
        }
    }
}

void ApplyFired(World& w, const PendingEvent& ev) {
    RemotePlayer* p = NetFindPeer(ev.from);
    if (!p) return;
    p->fireAnim = 0.25f;

    const float dist = Vector2Distance(p->pos, w.player.pos);
    switch ((WeaponId)(int)ev.a) {
        case WeaponId::Stokbrood:       PlaySfxAt(Sfx::Swing, dist); break;
        case WeaponId::Prijspistool:    PlaySfxAt(Sfx::Shot, dist); break;
        case WeaponId::Statiegeldkanon: PlaySfxAt(Sfx::Scattergun, dist); break;
        case WeaponId::Vuurwerkpijl:    PlaySfxAt(Sfx::RocketLaunch, dist); break;
    }
}

void ApplyDied(const PendingEvent& ev) {
    if ((int)ev.a == gNet.myId) gNet.frags++;
    if (RemotePlayer* p = NetFindPeer(ev.from)) {
        p->alive = false;
        p->health = 0;
        p->corpseTimer = 0.0f;
    }
}

void ApplyPickupTaken(World& w, int idx) {
    if (idx < 0 || idx >= (int)w.pickups.size()) return;
    w.pickups[idx].taken = true;
    if (gNet.isHost && idx < kNetMaxPickups) {
        gNet.pickupTimer[idx] = kNetPickupRespawn;
    }
}

void ApplyEvents(World& w) {
    // The inbox only grows from the JS side, which runs between frames — this
    // drain never races an append.
    for (const PendingEvent& ev : gNet.inbox) {
        switch (ev.type) {
            case NetEvent::Fired:
                ApplyFired(w, ev);
                break;

            case NetEvent::HitMe:
                gNet.lastHitBy = ev.from;
                gNet.lastHitByAge = 0.0f;
                PlayerDamage(w, (int)ev.a);
                break;

            case NetEvent::EnemyHit: {
                const int idx = (int)ev.a;
                if (gNet.isHost && idx >= 0 && idx < (int)w.enemies.size()) {
                    EnemyHurt(w, w.enemies[idx], (int)ev.b);
                }
                break;
            }

            case NetEvent::Died:
                ApplyDied(ev);
                break;

            case NetEvent::PickupTaken:
                ApplyPickupTaken(w, (int)ev.a);
                break;

            case NetEvent::PickupRespawn: {
                const int idx = (int)ev.a;
                if (idx >= 0 && idx < (int)w.pickups.size()) {
                    w.pickups[idx].taken = false;
                }
                break;
            }

            case NetEvent::Rocket: {
                // A peer's vuurwerkpijl: fly the replica for the light show. The
                // shooter owns its damage; `remote` mutes the blast here.
                Projectile r;
                r.kind = Projectile::Kind::Rocket;
                r.ownerIsPlayer = true;
                r.remote = true;
                r.pos = {ev.a, ev.b};
                r.vel = {ev.c, ev.d};
                r.height = kRocketLaunchH;
                r.life = 5.0f;
                w.projectiles.push_back(r);
                break;
            }
        }
    }
    gNet.inbox.clear();
}

void SendMyState(const World& w, float dt) {
    gNet.stateAccum += dt;
    if (gNet.stateAccum < kStateInterval) return;
    gNet.stateAccum = 0.0f;

    const Player& p = w.player;
    const int flags = p.dead() ? 0 : 1;
    JsNetState(p.pos, p.yaw, p.health, (int)p.weapon, flags);
}

void HostStreamEnemies(const World& w, float dt) {
    gNet.enemyAccum += dt;
    if (gNet.enemyAccum < kEnemyInterval) return;
    gNet.enemyAccum = 0.0f;

    JsNetEnemyBegin();
    const int count = (int)w.enemies.size() < kNetMaxEnemies ? (int)w.enemies.size()
                                                             : kNetMaxEnemies;
    for (int i = 0; i < count; i++) {
        const Enemy& e = w.enemies[i];
        const int flags = (e.aimBeam > 0.0f ? 1 : 0) | (e.beam > 0.0f ? 2 : 0);
        JsNetEnemy(i, (int)e.kind, e.pos, e.health, (int)e.state, e.aimDir, flags);
    }
    JsNetEnemyFlush();
}

void HostRespawnPickups(World& w, float dt) {
    const int count = (int)w.pickups.size() < kNetMaxPickups ? (int)w.pickups.size()
                                                             : kNetMaxPickups;
    for (int i = 0; i < count; i++) {
        if (gNet.pickupTimer[i] <= 0.0f) continue;
        gNet.pickupTimer[i] -= dt;
        if (gNet.pickupTimer[i] <= 0.0f) {
            w.pickups[i].taken = false;
            JsNetEvent(0, (int)NetEvent::PickupRespawn, (float)i, 0, 0, 0);
        }
    }
}

// Applies the host's 10 Hz stream to the local puppet enemies. Positions smooth
// exponentially toward the stream (stateless, good enough at this rate); anim
// and decay timers tick locally so walk cycles and death frames stay fluid
// between packets.
void PuppetEnemies(World& w, float dt) {
    for (int i = 0; i < kNetMaxEnemies; i++) {
        NetEnemyIn& in = gNet.enemyIn[i];
        if (!in.fresh) continue;
        in.fresh = false;

        while ((int)w.enemies.size() <= i) {
            w.enemies.push_back(MakeEnemy(EnemyKind::Winkelwagen, in.pos));
        }
        Enemy& e = w.enemies[i];

        e.kind = (EnemyKind)in.kind;
        e.health = in.health;
        e.aimDir = in.aim;
        e.aimBeam = in.aimBeam ? 1.0f : 0.0f;

        const EnemyState newState = (EnemyState)in.state;
        if (newState != e.state) {
            e.state = newState;
            e.stateTimer = 0.0f;   // restart the dying frames at the right frame
        }

        if (Vector2Distance(e.pos, in.pos) > kTeleportDist) {
            e.pos = in.pos;   // a respawned trolley walks out of nowhere — snap
        } else {
            e.pos = Vector2Lerp(e.pos, in.pos, fminf(1.0f, dt * 12.0f));
        }
    }

    for (Enemy& e : w.enemies) {
        e.hurtFlash = fmaxf(0.0f, e.hurtFlash - dt * 6.0f);
        e.beam = fmaxf(0.0f, e.beam - dt);
        if (e.state == EnemyState::Chase) {
            e.animTimer += dt;
            if (e.animTimer >= 0.44f) e.animTimer = 0.0f;
        } else if (e.state == EnemyState::Dying) {
            e.stateTimer += dt;
            if (e.stateTimer >= 0.42f) e.state = EnemyState::Dead;
        }
    }
}

}  // namespace

RemotePlayer* NetFindPeer(int id) {
    for (RemotePlayer& p : gNet.peers) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

bool NetConsumeStartRequest() {
    const bool v = gNet.startRequested;
    gNet.startRequested = false;
    return v;
}

void NetLeave() {
    if (!gNet.active) return;
    gNet.active = false;
    gNet.isHost = false;
    gNet.peers.clear();
    NetResetMatch();
    JsNetLeave();
}

void NetResetMatch() {
    gNet.inbox.clear();
    gNet.feed.clear();
    gNet.scoreline.clear();
    for (NetEnemyIn& e : gNet.enemyIn) e = NetEnemyIn{};
    gNet.frags = 0;
    gNet.deaths = 0;
    gNet.respawnTimer = 0.0f;
    gNet.lastHitBy = 0;
    gNet.lastHitByAge = 0.0f;
    gNet.stateAccum = 0.0f;
    gNet.enemyAccum = 0.0f;
    for (float& t : gNet.pickupTimer) t = 0.0f;
    for (RemotePlayer& p : gNet.peers) {   // roster survives, match state resets
        p.corpseTimer = -1.0f;
        p.alive = true;
        p.fireAnim = 0.0f;
        p.hurtFlash = 0.0f;
    }
}

void NetUpdate(World& w, float dt) {
    if (!gNet.active) return;

    for (const std::string& line : gNet.feed) w.Message(line);
    gNet.feed.clear();

    gNet.lastHitByAge += dt;
    if (gNet.lastHitByAge > 4.0f) gNet.lastHitBy = 0;   // old grudges expire

    ApplyEvents(w);
    TickPeers(dt);

    if (gNet.isHost) {
        HostStreamEnemies(w, dt);
        HostRespawnPickups(w, dt);
    } else {
        PuppetEnemies(w, dt);
    }

    SendMyState(w, dt);
}

void NetSendFired(int weaponId) {
    if (!gNet.active) return;
    JsNetEvent(0, (int)NetEvent::Fired, (float)weaponId, 0, 0, 0);
}

void NetSendHitPlayer(int targetId, int damage) {
    if (!gNet.active) return;
    JsNetEvent(targetId, (int)NetEvent::HitMe, (float)damage, 0, 0, 0);
}

void NetSendDied(int killerId) {
    if (!gNet.active) return;
    gNet.deaths++;
    JsNetEvent(0, (int)NetEvent::Died, (float)killerId, 0, 0, 0);
}

void NetSendRocket(Vector2 pos, Vector2 vel) {
    if (!gNet.active) return;
    JsNetEvent(0, (int)NetEvent::Rocket, pos.x, pos.y, vel.x, vel.y);
}

void NetDamageEnemy(World& w, int enemyIdx, int damage) {
    if (!gNet.active) return;
    if (gNet.isHost) {
        if (enemyIdx >= 0 && enemyIdx < (int)w.enemies.size()) {
            EnemyHurt(w, w.enemies[enemyIdx], damage);
        }
    } else {
        JsNetEvent(gNet.hostId, (int)NetEvent::EnemyHit, (float)enemyIdx,
                   (float)damage, 0, 0);
    }
}

void NetOnLocalPickupTaken(int idx) {
    if (!gNet.active) return;
    JsNetEvent(0, (int)NetEvent::PickupTaken, (float)idx, 0, 0, 0);
    if (gNet.isHost && idx >= 0 && idx < kNetMaxPickups) {
        gNet.pickupTimer[idx] = kNetPickupRespawn;
    }
}
