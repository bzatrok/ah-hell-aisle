#include "game.h"

// ---------------------------------------------------------------------------
// The one hand-authored level (SPEC.md §8). 44x40 tiles.
//
//   #  perimeter wall        S/s  stocked / looted gondola   F  freezer wall
//   C  checkout stand        M    magazijn wall              .  floor
//   K  keycard door          X    loading-dock exit door
//   p  player spawn          k    keycard
//   w/v/z  winkelwagen / vakkenvuller / zelfscanner
//   a/r/l/b  appelflap / rookworst / labels / bonuskaart
//
// Zones: entrance+checkouts south, aisle maze centre, freezer west (dark,
// keycard at the far end), magazijn north-east behind the keycard door with
// the exit in its east wall.
// ---------------------------------------------------------------------------
static const char* kLevel[] = {
    "############################################",
    "#......................M.................l.#",
    "#...ss...ss...ss...ss..M....MM.....MM.v....#",
    "#...ss..vss...ss...ss..M....MM.....MM......X",
    "#...ss...ss...ss...ss..M..w................#",
    "#...........w..........M................r..#",
    "#......................MMMMMMMMMMKMMMMMMMMMM",
    "#.............................v..z.........#",
    "#FFFFFFFFFFFF..............................#",
    "#.r.........F..S.vS..S..S..S..Sv.s..S..Sl..#",
    "#.FF..FF..b.F..S.aS..S..S..S..S..s..S..S...#",
    "#.FF..FF.......S..S..S..S..S..S..s..S..S...#",
    "#..............S..S..S..S..S..S..s..S..S...#",
    "#...........F...l...w......................#",
    "#.FF..FF..w.F................w.............#",
    "#.FF..FF....F..S..S..s..S..S..S..S..S..S...#",
    "#..............S..S..s..S..Sa.S..S..S..S...#",
    "#...z..........S..S..s..S..S..S..S..S..S...#",
    "#k....w.....F..S..S..s..S..S..S..S..S..S...#",
    "#FFFFFFFFFFFF..S..S..s..S..S..S..S..S..S...#",
    "#..............S..S..s..S..S..S..S..S..S...#",
    "#................w........z................#",
    "#........z...............l......w.....a....#",
    "#..............S..S..S..S..s..S..S..s..S...#",
    "#.............vS..Sa.S..S..s..S..S..s..S...#",
    "#..............S..S..S..S..s..S..Sa.s..S...#",
    "#..............S..S..Sv.S..s..S..S.vs..S...#",
    "#..........................................#",
    "#..........................................#",
    "#..........................................#",
    "#....CC...CC...CC...CC...CC...CC...CC......#",
    "#..l.CC...CC...CC...CC...CC...CC...CC.z..b.#",
    "#..........................................#",
    "#.......a..................................#",
    "#.......ss....................ss...........#",
    "#.......ss....................ss...........#",
    "#.....................p....................#",
    "#..........................................#",
    "#..........................................#",
    "############################################",
};

// Lighting zones as rectangles over the grid above (x0,y0,x1,y1 inclusive).
static const struct { int x0, y0, x1, y1; Zone z; } kZones[] = {
    { 1, 8, 12, 19, Zone::Freezer },
    { 23, 0, 43, 6, Zone::Magazijn },
};

static Vector2 Centre(int x, int y) { return Vector2{ x + 0.5f, y + 0.5f }; }

static void SpawnEnemy(Game& g, EnemyType type, int x, int y) {
  static const float kHp[3] = { 30, 60, 100 };
  Enemy e{};
  e.type = type;
  e.pos = Centre(x, y);
  e.lastSeen = e.pos;
  e.hp = kHp[(int)type];
  g.enemies.push_back(e);
}

void LoadLevel(Game& g) {
  Map& m = g.map;
  m.h = (int)(sizeof(kLevel) / sizeof(kLevel[0]));
  m.w = (int)TextLength(kLevel[0]);
  m.tiles.assign((size_t)m.w * m.h, Tile::Empty);
  m.zones.assign((size_t)m.w * m.h, Zone::Store);

  for (const auto& zr : kZones)
    for (int y = zr.y0; y <= zr.y1; y++)
      for (int x = zr.x0; x <= zr.x1; x++)
        m.zones[(size_t)y * m.w + x] = zr.z;

  for (int y = 0; y < m.h; y++) {
    for (int x = 0; x < m.w; x++) {
      char c = kLevel[y][x];
      Tile t = Tile::Empty;
      switch (c) {
        case '#': t = Tile::Plain; break;
        case 'S': t = Tile::ShelfFull; break;
        case 's': t = Tile::ShelfEmpty; break;
        case 'F': t = Tile::Freezer; break;
        case 'C': t = Tile::Checkout; break;
        case 'M': t = Tile::Magazijn; break;
        case 'K': t = Tile::DoorKey;  g.doors.push_back({ x, y, Tile::DoorKey }); break;
        case 'X': t = Tile::DoorExit; g.doors.push_back({ x, y, Tile::DoorExit }); break;
        case 'p': g.pl.pos = Centre(x, y); g.pl.angle = -PI / 2; break;  // face north, into the store
        case 'w': SpawnEnemy(g, EnemyType::Winkelwagen, x, y); break;
        case 'v': SpawnEnemy(g, EnemyType::Vakkenvuller, x, y); break;
        case 'z': SpawnEnemy(g, EnemyType::Zelfscanner, x, y); break;
        case 'a': g.pickups.push_back({ PickupType::Appelflap, Centre(x, y) }); break;
        case 'r': g.pickups.push_back({ PickupType::Rookworst, Centre(x, y) }); break;
        case 'l': g.pickups.push_back({ PickupType::Labels, Centre(x, y) }); break;
        case 'b': g.pickups.push_back({ PickupType::Bonuskaart, Centre(x, y) }); break;
        case 'k': g.pickups.push_back({ PickupType::Keycard, Centre(x, y) }); break;
        default: break;
      }
      m.tiles[(size_t)y * m.w + x] = t;
    }
  }
  g.totalEnemies = (int)g.enemies.size();
}
