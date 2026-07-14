#include "map.h"

#include <cmath>

#include "raymath.h"

// ---------------------------------------------------------------------------
// The level, hand-authored. One character per tile, 40x40.
//
//   #  perimeter wall      C  checkout lane      S  stocked gondola
//   s  picked-clean gondola  F  freezer          M  magazijn (back of house)
//   K  keycard door        X  loading dock door  .  floor
//   @  you                 1  winkelwagen  2  vakkenvuller  3  zelfscanner
//   a  appelflap  r  rookworst  l  labels  b  bonuskaart  k  the keycard
//
// Read it north-up: the magazijn is the strip along the top, behind the locked
// door at column 19. You start at the bottom, in front of the checkouts. The
// keycard sits in the far corner of the freezer, which is the one room in the
// store with a turret already looking at the door you have to come through.
// ---------------------------------------------------------------------------
static const char* const kLevel[Map::H] = {
    "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMXMMMMMM",
    "M................3................l....M",
    "M..MM...MM...MM.....3...MM...MM........M",
    "M..MM...MM...MM.........MM...MM........M",
    "M.....2.............r............2.....M",
    "M.....MM...MM.........MM...MM...MM.....M",
    "M.....MM...MM.........MM...MM...MM.....M",
    "M..l....1.....................1........M",
    "MMMMMMMMMMMMMMMMMMMKMMMMMMMMMMMMMMMMMMMM",
    "#.....3.......................1........#",
    "#............2.....a....1..............#",
    "#..........................FFFFFFFFFFFFF",
    "#.b........................F....2......F",
    "#...S...S...S...S...s...S..F.....3...k.F",
    "#...S...S...S...S...s...S..F..2........F",
    "#...S.1.S...S...S...s...S..F..FF..FF...F",
    "#...S...S.a.S...S...s...S..F..FF..FF...F",
    "#...S...S...S...S...s.2.S..............F",
    "#...S...S...S...S...s...S......3.......F",
    "#...S...S...S...S...s...S..F..FF..FF...F",
    "#...S...S...S...S...s...S..F..FF..FF...F",
    "#.....l.......1............F...........F",
    "#.3........................F..FF..FF...F",
    "#.r........................F..FF..FF...F",
    "#...S...S...S...s.a.s...S..F...........F",
    "#...S...S...S...s...s...S..FFFFFFFFFFFFF",
    "#...S...S.2.S...s...s.l.S..............#",
    "#...S...S...S...s...s...S....bSS..1ss..#",
    "#...S...S...S...s.1.s...S.....SS...ss..#",
    "#...S...S...S...s...s...S.2......a.....#",
    "#..............................3.......#",
    "#......................................#",
    "#......................................#",
    "#.........1...................l........#",
    "#...CCCCC...CCCCC...CCCCC...CCCCC......#",
    "#...CCCCC...CCCCC...CCCCC...CCCCC......#",
    "#...CCCCC...CCCCC...CCCCC...CCCCC......#",
    "#......................................#",
    "#..................@...................#",
    "########################################",
};

static Tile TileFor(char c) {
    switch (c) {
        case '#': return Tile::Plain;
        case 'C': return Tile::Checkout;
        case 'S': return Tile::ShelfFull;
        case 's': return Tile::ShelfEmpty;
        case 'F': return Tile::Freezer;
        case 'M': return Tile::Magazijn;
        case 'K': return Tile::DoorKeycard;
        case 'X': return Tile::DoorExit;
        default:  return Tile::Empty;
    }
}

// The zones the level was drawn in. Keep in step with the ASCII above.
static Zone ZoneFor(int x, int y) {
    if (y <= 8) return Zone::Magazijn;
    if (y >= 33) return Zone::Checkout;
    if (x >= 27 && y >= 11 && y <= 25) return Zone::Freezer;
    return Zone::Store;
}

Map LoadLevel(std::vector<Spawn>& spawns) {
    Map m;
    spawns.clear();

    for (int y = 0; y < Map::H; y++) {
        for (int x = 0; x < Map::W; x++) {
            const char c = kLevel[y][x];
            m.tiles[y][x] = TileFor(c);
            m.zones[y][x] = ZoneFor(x, y);

            if (m.tiles[y][x] == Tile::DoorKeycard || m.tiles[y][x] == Tile::DoorExit) {
                m.doors.push_back({x, y, m.tiles[y][x] == Tile::DoorExit, false, 0.0f});
            } else if (c != '.' && m.tiles[y][x] == Tile::Empty) {
                spawns.push_back({x, y, c});
            }
        }
    }
    return m;
}

bool Map::Solid(int x, int y) const {
    const Tile t = At(x, y);
    if (t == Tile::Empty) return false;
    if (t == Tile::DoorKeycard) {
        const Door* d = DoorAt(x, y);
        return !(d && d->slide > 0.85f);
    }
    return true;  // the exit door stays shut; reaching it is what ends the level
}

Door* Map::DoorAt(int x, int y) {
    for (Door& d : doors) {
        if (d.x == x && d.y == y) return &d;
    }
    return nullptr;
}

const Door* Map::DoorAt(int x, int y) const {
    for (const Door& d : doors) {
        if (d.x == x && d.y == y) return &d;
    }
    return nullptr;
}

void Map::Update(float dt) {
    for (Door& d : doors) {
        if (d.open && d.slide < 1.0f) d.slide = fminf(1.0f, d.slide + dt * 1.4f);
    }
}

float Map::RayToWall(Vector2 origin, Vector2 dir, float maxDist) const {
    // Grid DDA: step tile to tile along the ray, stop at the first solid one.
    int mx = (int)floorf(origin.x);
    int my = (int)floorf(origin.y);
    if (Solid(mx, my)) return 0.0f;

    const float invX = (fabsf(dir.x) < 1e-6f) ? 1e30f : fabsf(1.0f / dir.x);
    const float invY = (fabsf(dir.y) < 1e-6f) ? 1e30f : fabsf(1.0f / dir.y);
    const int stepX = (dir.x < 0.0f) ? -1 : 1;
    const int stepY = (dir.y < 0.0f) ? -1 : 1;

    float nextX = (dir.x < 0.0f) ? (origin.x - mx) * invX : (mx + 1.0f - origin.x) * invX;
    float nextY = (dir.y < 0.0f) ? (origin.y - my) * invY : (my + 1.0f - origin.y) * invY;

    for (;;) {
        const float dist = fminf(nextX, nextY);
        if (dist > maxDist) return maxDist;

        if (nextX < nextY) {
            nextX += invX;
            mx += stepX;
        } else {
            nextY += invY;
            my += stepY;
        }
        if (Solid(mx, my)) return dist;
    }
}

bool Map::LineOfSight(Vector2 a, Vector2 b) const {
    const Vector2 delta = Vector2Subtract(b, a);
    const float dist = Vector2Length(delta);
    if (dist < 1e-4f) return true;
    const Vector2 dir = Vector2Scale(delta, 1.0f / dist);
    return RayToWall(a, dir, dist) >= dist - 1e-3f;
}

bool Map::Fits(Vector2 p, float radius) const {
    const int x0 = (int)floorf(p.x - radius);
    const int x1 = (int)floorf(p.x + radius);
    const int y0 = (int)floorf(p.y - radius);
    const int y1 = (int)floorf(p.y + radius);

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            if (!Solid(x, y)) continue;
            // Closest point on the tile to the circle centre.
            const float cx = Clamp(p.x, (float)x, (float)x + 1.0f);
            const float cy = Clamp(p.y, (float)y, (float)y + 1.0f);
            const float dx = p.x - cx;
            const float dy = p.y - cy;
            if (dx * dx + dy * dy < radius * radius) return false;
        }
    }
    return true;
}

Vector2 Map::SlideMove(Vector2 pos, Vector2 delta, float radius) const {
    Vector2 out = pos;
    const Vector2 tryX = {out.x + delta.x, out.y};
    if (Fits(tryX, radius)) out = tryX;
    const Vector2 tryY = {out.x, out.y + delta.y};
    if (Fits(tryY, radius)) out = tryY;
    return out;
}
