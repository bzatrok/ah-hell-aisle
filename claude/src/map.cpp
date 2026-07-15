#include "map.h"

#include <cmath>

#include "raymath.h"

// ---------------------------------------------------------------------------
// The three levels, hand-authored. One character per tile, 40x40 each.
//
//   #  perimeter wall      C  checkout lane      S  stocked gondola
//   s  picked-clean gondola  F  freezer          M  magazijn (back of house)
//   K  keycard door        X  loading dock door  .  floor
//   @  you                 1  winkelwagen  2  vakkenvuller  3  zelfscanner
//   4  beveiliger          5  bedrijfsleider (the boss)
//   a  appelflap  r  rookworst  l  labels  b  bonuskaart  k  the keycard
//   f  flessen (ammo 3)  v  vuurwerk (ammo 4)  g  statiegeldkanon  p  vuurwerkpijl
//
// Level 1, the store. Read it north-up: the magazijn is the strip along the
// top, behind the locked door at column 19. You start at the bottom, in front
// of the checkouts. The keycard sits in the far corner of the freezer, which is
// the one room with a turret already looking at the door you have to come
// through.
// ---------------------------------------------------------------------------
static const char* const kLevel1[Map::H] = {
    "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMXMMMMMM",
    "M................3................l....M",
    "M..MM...MM...MM.....3...MM...MM........M",
    "M..MM...MM...MM.....l...MM...MM........M",
    "M.....2...4.........r............2.....M",
    "M.....MM...MM.........MM...MM...MM.....M",
    "M.....MM...MM.........MM...MM...MM.....M",
    "M..l....1................4....1........M",
    "MMMMMMMMMMMMMMMMMMMKMMMMMMMMMMMMMMMMMMMM",
    "#.....3.......................1........#",
    "#............2.....a.g..1..............#",
    "#..........................FFFFFFFFFFFFF",
    "#.b.....................l..F....2......F",
    "#...S...S...S...S...s...S..F.....3...k.F",
    "#...S...S...S...S...s...S..F..2........F",
    "#...S.1.S...S...S...s...S..F..FF..FF...F",
    "#...S...S.a.S...S...s...S..F..FF..FF...F",
    "#...S...S...S...S...s.2.S..............F",
    "#...S...S.l.S...S...s...S......3.......F",
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
    "#...CCCCC.l.CCCCC...CCCCC...CCCCC......#",
    "#...CCCCC...CCCCC...CCCCC...CCCCC......#",
    "#...CCCCC...CCCCC...CCCCC...CCCCC......#",
    "#......................................#",
    "#..................@...................#",
    "########################################",
};

// ---------------------------------------------------------------------------
// Level 2, "Het Distributiecentrum". Long parallel racking with a single wide
// centre aisle: every lane is a firing line, and the beveiligers know it. The
// keycard waits in the vriescel in the far corner; the dock bay behind the
// keycard door at the top is the way out.
// ---------------------------------------------------------------------------
static const char* const kLevel2[Map::H] = {
    "MMMMMMMMMMMMMMMMMMMMXMMMMMMMMMMMMMMMMMMM",
    "M...4................a...............v.M",
    "M.r..........................4.........M",
    "M......................................M",
    "MMMMMMMMMMKMMMMMMMMMMMMMMMMMMMMMMMMMMMMM",
    "M......................................M",
    "M.1...............3................1...M",
    "M..MMMMMMMMMMMMMM......MMMMMMMMMMMMMM..M",
    "M....................4.................M",
    "M.l.......2..................f.........M",
    "M..MMMMMMMMMMMMMM......MMMMMMMMMMMMMM..M",
    "M...............1......1...............M",
    "M.4...........a...............2......l.M",
    "M..MMMMMMMMMMMMMM......MMMMMMMMMMMMMM..M",
    "M......3...............................M",
    "M..............1......f.........4......M",
    "M..MMMMMMMMMMMMMM......MMMMMMMMMMMMMM..M",
    "M......................................M",
    "M.b.........2.................1........M",
    "M..MMMMMMMMMMMMMM......MMMMMMMMMMMMMM..M",
    "M...........................3..........M",
    "M...1...............p..................M",
    "M..MMMMMMMMMMMMMM......MMMMMMMMMMMMMM..M",
    "M..................4...................M",
    "M.r.......1....................2.....a.M",
    "M..MMMMMMMMMMMMMM......MMMMMMMMMMMMMM..M",
    "M......................................M",
    "M.2..........1.........................M",
    "M...........................FFF..FFFFFFM",
    "M.....s.....................F..........M",
    "M.1...............2.........F......3...M",
    "M...........................F..........M",
    "M....ss.....................F.......4..M",
    "M..2........................F..........M",
    "M...........................F....3.....M",
    "M.....S..1......S...........F..........M",
    "M.....S.........S...........F..........M",
    "M..@........................F.3......k.M",
    "M.r.........................F.........bM",
    "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM",
};

// ---------------------------------------------------------------------------
// Level 3, "Het Laadperron". Cold cells at the bottom, a tight freezer/steel
// tangle in the middle, and the boss arena in front of the final dock. There is
// no keycard on the floor this time: De Bedrijfsleider carries it.
// ---------------------------------------------------------------------------
static const char* const kLevel3[Map::H] = {
    "MMMMMMMMMMMMMMMMMMMXMMMMMMMMMMMMMMMMMMMM",
    "M..............r..........a............M",
    "M......................................M",
    "MMMMMMMMMMMMMMMMMMMKMMMMMMMMMMMMMMMMMMMM",
    "M....4...........3..............4......M",
    "M...MM..............................MM.M",
    "M.....1..........................1.....M",
    "M..................5...................M",
    "M.4................................4...M",
    "M...MM..............................MM.M",
    "M..........f...............v...........M",
    "M......2....................2..........M",
    "MMMMMMMMM....MMMMMMMMMMMMMM....MMMMMMMMM",
    "M....3.............................3...M",
    "M..FF..FFFFFF..MMMM..MMMM..FFFFFF..FF..M",
    "M.1..............2.............4.......M",
    "M..FF..FFFFFF..MMMM..MMMM..FFFFFF..FF..M",
    "M......4..........1..............2.....M",
    "M..FF..FFFFFF..MMMM..MMMM..FFFFFF..FF..M",
    "M...l.............v.................l..M",
    "M..FF..FFFFFF..MMMM..MMMM..FFFFFF..FF..M",
    "M.2.............1...............1......M",
    "M..FF..FFFFFF..MMMM..MMMM..FFFFFF..FF..M",
    "M........3.....................3.......M",
    "M..FF..FFFFFF..MMMM..MMMM..FFFFFF..FF..M",
    "M....a...........b..............r......M",
    "MMMMMMM....MMMMMMMMMM....MMMMMMMMMMMMMMM",
    "M........1...................1.........M",
    "M..4..............3...............4....M",
    "M..FFFF..FFFF..FFFF..FFFF..FFFF..FFFF..M",
    "M....2.........1..........2............M",
    "M..FFFF..FFFF..FFFF..FFFF..FFFF..FFFF..M",
    "M.l...........a.............1........b.M",
    "M..FFFF..FFFF..FFFF..FFFF..FFFF..FFFF..M",
    "M......1..........2..............1.....M",
    "M.....2..........4...............2.....M",
    "M...ss................3...........ss...M",
    "M.r................................f...M",
    "M..................@...................M",
    "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM",
};

// ---------------------------------------------------------------------------
// The arena, "Nachtdienst". Not part of the run: this is the multiplayer floor.
// Eight spawn corners and edges, a contested statiegeldkanon dead centre, the
// vuurwerkpijlen locked in two freezer rooms whose doorways face the plaza, and
// enough gondola cover that no lane is safe to hold. Four trolleys and four
// scanners come with the building.
// ---------------------------------------------------------------------------
static const char* const kArena[Map::H] = {
    "########################################",
    "#.@.....M..........rr..........M.....@.#",
    "#.......M..MM....................M.....#",
    "#..MM...M..MM...l......l...MM....M.....#",
    "#..MM......MM..............MM..MM...a..#",
    "#............................1.........#",
    "#...a......1...........................#",
    "#......................................#",
    "#...SSSSSSSSSS....v.....SSSSSSSSSS.....#",
    "#......................................#",
    "#...SSSSSSSSSS...FFF....SSSSSSSSSS..l..#",
    "#..l..............3....................#",
    "#...SSSSSSSSSS...FFF....SSSSSSSSSS.....#",
    "#......................................#",
    "#.@...................................@#",
    "#FFFFFF......................FFFFFFFFF.#",
    "#F....F......................F.......F.#",
    "#F.p..F.......FF.....FF......F..p....F.#",
    "#F....F.......FF.....FF......F.......F.#",
    "#F.3..........f...g...f..........3...F.#",
    "#F...................................F.#",
    "#F....F.......FF.....FF......F.......F.#",
    "#F.b..F.......FF.....FF......F....b..F.#",
    "#F....F......................F.......F.#",
    "#FFFFFF......................FFFFFFFFF.#",
    "#.@...................................@#",
    "#......................................#",
    "#...SSSSSSSSSS...FFF....SSSSSSSSSS.....#",
    "#....................3................l#",
    "#...SSSSSSSSSS...FFF....SSSSSSSSSS.....#",
    "#..l...................................#",
    "#...SSSSSSSSSS....v.....SSSSSSSSSS.....#",
    "#......................................#",
    "#...........1.............1......a.....#",
    "#...CCCC....CCCC....CCCC....CCCC.......#",
    "#...CCCC....CCCC....CCCC....CCCC...f...#",
    "#...CCCC....CCCC....CCCC....CCCC.......#",
    "#..a...................................#",
    "#.@.....l...........rr...........l...@.#",
    "########################################",
};

static Zone ZoneForArena(int x, int y) {
    if (y <= 6) return Zone::Magazijn;                            // the dock strip
    if (y >= 33) return Zone::Checkout;                           // the lanes
    if (y >= 15 && y <= 24 && (x <= 6 || x >= 29)) return Zone::Freezer;
    return Zone::Store;
}

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

// The zones each level was drawn in. Keep in step with the ASCII above.
static Zone ZoneFor1(int x, int y) {
    if (y <= 8) return Zone::Magazijn;
    if (y >= 33) return Zone::Checkout;
    if (x >= 27 && y >= 11 && y <= 25) return Zone::Freezer;
    return Zone::Store;
}

static Zone ZoneFor2(int x, int y) {
    if (y <= 4) return Zone::Store;                  // the dock bay, lights left on
    if (x >= 28 && y >= 28) return Zone::Freezer;    // the vriescel
    return Zone::Magazijn;
}

static Zone ZoneFor3(int /*x*/, int y) {
    if (y <= 12) return Zone::Magazijn;              // arena and the final dock
    if (y >= 27) return Zone::Freezer;               // the cold cells you start in
    return Zone::Store;
}

struct LevelDef {
    const char* const* rows;          // [Map::H]
    Zone (*zoneFor)(int x, int y);
    const char* intro;                // HUD message on entry
};

static const LevelDef kLevels[kLevelCount] = {
    {kLevel1, ZoneFor1, "02:14 - DE WINKEL IS GESLOTEN"},
    {kLevel2, ZoneFor2, "02:47 - HET DISTRIBUTIECENTRUM"},
    {kLevel3, ZoneFor3, "03:33 - HET LAADPERRON. HIJ WACHT."},
};

static int ClampLevel(int level) {
    return (level < 0) ? 0 : (level >= kLevelCount ? kLevelCount - 1 : level);
}

const char* LevelIntro(int level) {
    return kLevels[ClampLevel(level)].intro;
}

static Map BuildMap(const char* const* rows, Zone (*zoneFor)(int, int),
                    std::vector<Spawn>& spawns) {
    Map m;
    spawns.clear();

    for (int y = 0; y < Map::H; y++) {
        for (int x = 0; x < Map::W; x++) {
            const char c = rows[y][x];
            m.tiles[y][x] = TileFor(c);
            m.zones[y][x] = zoneFor(x, y);

            if (m.tiles[y][x] == Tile::DoorKeycard || m.tiles[y][x] == Tile::DoorExit) {
                m.doors.push_back({x, y, m.tiles[y][x] == Tile::DoorExit, false, 0.0f});
            } else if (c != '.' && m.tiles[y][x] == Tile::Empty) {
                spawns.push_back({x, y, c});
            }
        }
    }
    return m;
}

Map LoadLevel(int level, std::vector<Spawn>& spawns) {
    const LevelDef& def = kLevels[ClampLevel(level)];
    return BuildMap(def.rows, def.zoneFor, spawns);
}

Map LoadArena(std::vector<Spawn>& spawns) {
    return BuildMap(kArena, ZoneForArena, spawns);
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
