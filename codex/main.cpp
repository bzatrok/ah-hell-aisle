#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

constexpr int W=960,H=600, MW=32,MH=32;
constexpr float FOV=3.14159265f/3.0f;
struct V { float x,y; };
static V add(V a,V b){return {a.x+b.x,a.y+b.y};} static V sub(V a,V b){return {a.x-b.x,a.y-b.y};}
static V mul(V a,float s){return {a.x*s,a.y*s};} static float len(V a){return sqrtf(a.x*a.x+a.y*a.y);}
static float dist(V a,V b){return len(sub(a,b));} static V norm(V a){float l=len(a);return l?mul(a,1/l):V{0,0};}
static float cross(V a,V b){return a.x*b.y-a.y*b.x;}
enum Wall { EMPTY=0, SHELF=1, EMPTY_SHELF=2, FREEZER=3, PLAIN=4, CHECKOUT=5, MAG=6, KEYDOOR=7, EXITDOOR=8 };
enum Kind { TROLLEY, STOCKER, SCANNER };
enum Pick { APPLE, SAUSAGE, LABELS, BONUS, KEYCARD };
struct Enemy { V p; Kind kind; int hp; float cooldown=0, anim=0; bool dead=false; };
struct Projectile { V p,v; float life=4; };
struct Pickup { V p; Pick kind; bool taken=false; };
struct Tex { Texture2D shelf,emptyShelf,freezer,plain,checkout,mag,keydoor,exitdoor,floor,ceiling; Texture2D enemies[3], pickups[5], soup, weapons[2],panel,face; } tex;
static int map[MH][MW]; static std::vector<float> zbuf(W); static std::vector<Enemy> enemies; static std::vector<Projectile> projectiles; static std::vector<Pickup> pickups;
static V player{3.5f,3.5f}; static float angle=0; static int health=100, armour=0, ammo=40, weapon=0, kills=0; static bool key=false; static float fireTimer=0,weaponAnim=0,elapsed=0,messageTimer=0; static std::string message; static int state=0;

static Texture2D load(const char* n){ return LoadTexture(TextFormat("../assets/%s",n)); }
static void note(const char* s){message=s;messageTimer=2.2f;}
static bool inMap(int x,int y){return x>=0&&y>=0&&x<MW&&y<MH;}
static bool solidCell(int x,int y){if(!inMap(x,y))return true; return map[y][x]!=EMPTY;}
static bool blocked(V p,float r=.20f){ return solidCell((int)floorf(p.x-r),(int)floorf(p.y-r))||solidCell((int)floorf(p.x+r),(int)floorf(p.y-r))||solidCell((int)floorf(p.x-r),(int)floorf(p.y+r))||solidCell((int)floorf(p.x+r),(int)floorf(p.y+r)); }
static bool canMove(V p,float r=.20f){return !blocked(p,r);}
static bool los(V a,V b){ V d=sub(b,a); float l=len(d); d=norm(d); for(float t=.12f;t<l;t+=.12f)if(solidCell((int)(a.x+d.x*t),(int)(a.y+d.y*t)))return false; return true; }
static void damage(int d){int absorbed=std::min(armour,(d+1)/2);armour-=absorbed;health-=d-absorbed;if(health<=0){health=0;state=2;EnableCursor();}}
static Texture2D& wallTex(int m){switch(m){case SHELF:return tex.shelf;case EMPTY_SHELF:return tex.emptyShelf;case FREEZER:return tex.freezer;case CHECKOUT:return tex.checkout;case MAG:return tex.mag;case KEYDOOR:return tex.keydoor;case EXITDOOR:return tex.exitdoor;default:return tex.plain;}}
static void loadMap(){
  const char* rows[MH]={
  "################################","#....#.............#...........#","#.##.#.###########.#.#########.#","#....#.#.........#.#.#.......#.#","######.#.#######.#.#.#.#####.#.#","#......#.#.....#.#.#.#.#...#.#.#","#.######.#.###.#.#.#.#.#.#.#.#.#","#.#......#.#.#.#.#.#.#.#.#.#.#.#","#.#.######.#.#.#.#.#.#.#.#.#.#.#","#.#.#......#.#.#.#.#.#.#.#.#.#.#","#.#.#.######.#.#.#.#.#.#.#.#.#.#","#.#.#.#......#.#.#.#.#.#.#.#.#.#","#.#.#.#.######.#.#.#.#.#.#.#.#.#","#.#.#.#.#......#.#.#.#.#.#.#.#.#","#.#.#.#.#.######.#.#.#.#.#.#.#.#","#.#...#.#........#.#.#.#...#.#.#","#.#####.##########.#.#.#####.#.#","#.....#............#.#.......#.#","#.###.##############.#######.#.#","#.#.#......................#.#.#","#.#.#.####################.#.#.#","#.#.#.#..................#.#.#.#","#.#.#.#.################.#.#.#.#","#.#...#.#..............#.#...#.#","#.#####.#.############.#.#####.#","#.......#.#..........#.#.......#","#########.#.########.#.#########","#.........#.#......#.#.........#","#.#########.#.####.#.#########.#","#...........#....#.#...........#","#.##############.#.#############","################################"};
  for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){char c=rows[y][x];map[y][x]=(c=='#'?PLAIN:EMPTY);} 
  for(int y=1;y<MH-1;y++)for(int x=1;x<MW-1;x++)if(map[y][x]==PLAIN)map[y][x]=EMPTY;
  // aisle gondolas and zone dressing
  for(int x=6;x<=23;x+=3)for(int y=3;y<=17;y++)if(map[y][x]==EMPTY)map[y][x]=(x%2?SHELF:EMPTY_SHELF);
  for(int x=2;x<7;x++)map[5][x]=CHECKOUT;
  for(int y=3;y<19;y+=3)map[y][25]=FREEZER;
  for(int x=1;x<MW-1;x++)map[25][x]=MAG;
  for(int x=3;x<30;x+=6)map[28][x]=MAG;
  map[25][15]=KEYDOOR; map[29][28]=EXITDOOR;
}
static void reset(){loadMap();player={3.5f,3.5f};angle=0;health=100;armour=0;ammo=40;weapon=0;key=false;kills=0;fireTimer=weaponAnim=elapsed=0;projectiles.clear();pickups={{ {18.5f,19.5f},KEYCARD},{ {5.5f,15.5f},APPLE},{ {26.5f,8.5f},SAUSAGE},{ {20.5f,23.5f},LABELS},{ {27.5f,27.5f},BONUS},{ {11.5f,18.5f},APPLE},{ {28.5f,5.5f},LABELS}}; enemies.clear();
 auto e=[](float x,float y,Kind k){int hp=k==TROLLEY?30:k==STOCKER?60:100;enemies.push_back({{x,y},k,hp});};
 e(5.5,7.5,TROLLEY);e(8.5,5.5,STOCKER);e(10.5,9.5,SCANNER);e(14.5,5.5,TROLLEY);e(16.5,12.5,STOCKER);e(20.5,6.5,SCANNER);e(24.5,16.5,TROLLEY);e(28.5,8.5,STOCKER);e(28.5,15.5,SCANNER);e(5.5,18.5,TROLLEY);e(9.5,19.5,STOCKER);e(13.5,19.5,SCANNER);e(18.5,19.5,TROLLEY);e(22.5,21.5,STOCKER);e(26.5,20.5,SCANNER);e(5.5,25.5,TROLLEY);e(11.5,24.5,STOCKER);e(16.5,27.5,SCANNER);e(20.5,28.5,TROLLEY);e(27.5,27.5,STOCKER);e(29.5,27.5,SCANNER);state=1;DisableCursor();note("02:14 — VIND DE KEYCARD");}
static void moveSlide(V &p,V d,float r=.2f){V q={p.x+d.x,p.y};if(canMove(q,r))p.x=q.x;q={p.x,p.y+d.y};if(canMove(q,r))p.y=q.y;}
static void moveEnemy(Enemy &self,V d){V next=add(self.p,d);for(const auto&o:enemies)if(&o!=&self&&!o.dead&&dist(next,o.p)<.48f)return;moveSlide(self.p,d,.25f);}
static bool wallBetween(V a,V b){return !los(a,b);}
static void hurtEnemy(Enemy &e,int d){if(e.dead)return;e.hp-=d;e.anim=0;if(e.hp<=0){e.dead=true;kills++;note("DOODGECHECKT");}}
static void fire(){if(fireTimer>0)return;if(weapon==1&&ammo<=0){note("GEEN LABELS");return;}fireTimer=weapon?0.25f:0.5f;weaponAnim=.32f; V f{cosf(angle),sinf(angle)};
 if(weapon==1)ammo--; float best=1e9;Enemy* target=nullptr;for(auto &e:enemies)if(!e.dead){V d=sub(e.p,player);float forward=d.x*f.x+d.y*f.y;float side=fabsf(cross(f,d));float range=weapon?99.f:1.5f;if(forward>0&&forward<range&&side<(.34f+forward*.08f)&&!wallBetween(player,e.p)&&forward<best){best=forward;target=&e;}}if(target)hurtEnemy(*target,weapon?20:25);
}
static void update(float dt){if(IsKeyPressed(KEY_ESCAPE)){state=5;return;}elapsed+=dt;messageTimer-=dt;fireTimer=std::max(0.f,fireTimer-dt);weaponAnim=std::max(0.f,weaponAnim-dt);if(IsKeyPressed(KEY_ONE))weapon=0;if(IsKeyPressed(KEY_TWO))weapon=1;float mouse=GetMouseDelta().x;angle+=mouse*.0024f; if(IsKeyDown(KEY_LEFT))angle-=2.1f*dt;if(IsKeyDown(KEY_RIGHT))angle+=2.1f*dt;V f{cosf(angle),sinf(angle)},r{-f.y,f.x},d{};if(IsKeyDown(KEY_W))d=add(d,f);if(IsKeyDown(KEY_S))d=sub(d,f);if(IsKeyDown(KEY_A))d=sub(d,r);if(IsKeyDown(KEY_D))d=add(d,r);if(len(d)>0){d=mul(norm(d),3.5f*dt);moveSlide(player,d);}
 if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)||IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))fire();
 for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){
   float doorDistance=dist(player,{x+.5f,y+.5f});
   if(map[y][x]==EXITDOOR&&doorDistance<.85f){state=3;EnableCursor();}
   if(map[y][x]==KEYDOOR&&(doorDistance<.85f||(IsKeyPressed(KEY_E)&&doorDistance<1.45f))){
     if(key){map[y][x]=EMPTY;note("MAGAZIJN ONTGRENDELD");}
     else note("BEDRIJFSLEIDERSPAS VEREIST");
   }
 }
 for(auto &p:pickups)if(!p.taken&&dist(player,p.p)<.55f){bool take=true;switch(p.kind){case APPLE:if(health==100)take=false;else health=std::min(100,health+25);break;case SAUSAGE:if(health==100)take=false;else health=std::min(100,health+50);break;case LABELS:if(ammo==200)take=false;else ammo=std::min(200,ammo+20);break;case BONUS:if(armour==100)take=false;else armour=std::min(100,armour+50);break;case KEYCARD:key=true;break;}if(take){p.taken=true;note(p.kind==KEYCARD?"KEYCARD GEVONDEN":"OPGEPAKT");}}
 for(auto &pr:projectiles){pr.p=add(pr.p,mul(pr.v,dt));pr.life-=dt;if(blocked(pr.p,.08f))pr.life=0;if(dist(pr.p,player)<.30f){damage(15);pr.life=0;}}projectiles.erase(std::remove_if(projectiles.begin(),projectiles.end(),[](const Projectile&p){return p.life<=0;}),projectiles.end());
 for(auto &e:enemies){if(e.dead){e.anim+=dt;continue;}e.cooldown-=dt;e.anim+=dt;float D=dist(e.p,player);if(D>15||!los(e.p,player))continue;V toward=norm(sub(player,e.p));if(e.kind==TROLLEY){if(D>.48f)moveEnemy(e,mul(toward,4*dt));else if(e.cooldown<=0){damage(10);e.cooldown=1;e.anim=0;}}else if(e.kind==STOCKER){if(D>7)moveEnemy(e,mul(toward,1.8f*dt));if(D<10&&e.cooldown<=0){V v=mul(toward,3.0f);projectiles.push_back({e.p,v});e.cooldown=2;e.anim=0;}}else if(D<14&&e.cooldown<=0){damage(8);e.cooldown=1.5f;e.anim=0;}}
}

static Color shade(Color c,float d,bool cold=false){float k=std::max(.20f,1.f-d/18.f);if(cold)k*=.62f;return {(unsigned char)(c.r*k),(unsigned char)(c.g*k),(unsigned char)(c.b*(cold?std::min(1.f,k*1.25f):k)),255};}
static void column(Texture2D t,int sx,int sy,int sw,int sh,int x,int y,int h,Color tint=WHITE){if(h>0)DrawTexturePro(t,{(float)sx,(float)sy,(float)sw,(float)sh},{(float)x,(float)y,1,(float)h},{0,0},0,tint);}
static void renderWorld(){
 ClearBackground({13,21,27,255});
 // Textured horizontal strips establish the store's flat floor and ceiling.
 for(int y=0;y<H/2;y+=2){int sy=(y/3)%tex.ceiling.height;DrawTexturePro(tex.ceiling,{0,(float)sy,(float)tex.ceiling.width,1},{0,(float)y,(float)W,2},{0,0},0,{105,115,120,255});}
 for(int y=H/2;y<H-76;y+=2){int sy=((y-H/2)/3)%tex.floor.height;DrawTexturePro(tex.floor,{0,(float)sy,(float)tex.floor.width,1},{0,(float)y,(float)W,2},{0,0},0,{120,120,120,255});}
 for(int x=0;x<W;x++){
  float camera=2.f*x/W-1.f;float rayAngle=angle+atanf(camera*tanf(FOV*.5f));V ray{cosf(rayAngle),sinf(rayAngle)};int mx=(int)player.x,my=(int)player.y;float ddx=fabsf(1/ray.x),ddy=fabsf(1/ray.y),sdx,sdy;int stepx,stepy;
  if(ray.x<0){stepx=-1;sdx=(player.x-mx)*ddx;}else{stepx=1;sdx=(mx+1-player.x)*ddx;}if(ray.y<0){stepy=-1;sdy=(player.y-my)*ddy;}else{stepy=1;sdy=(my+1-player.y)*ddy;}int side=0;while(true){if(sdx<sdy){sdx+=ddx;mx+=stepx;side=0;}else{sdy+=ddy;my+=stepy;side=1;}if(solidCell(mx,my))break;}
  float raw=side?(my-player.y+(1-stepy)/2)/ray.y:(mx-player.x+(1-stepx)/2)/ray.x;float d=std::max(.01f,raw*cosf(rayAngle-angle));zbuf[x]=d;int line=(int)(H/d),start=std::max(0,H/2-line/2),end=std::min(H-76,H/2+line/2);float wall=side?player.x+raw*ray.x:player.y+raw*ray.y;wall-=floorf(wall);Texture2D &t=wallTex(inMap(mx,my)?map[my][mx]:PLAIN);int tx=(int)(wall*t.width);if((side==0&&ray.x>0)||(side==1&&ray.y<0))tx=t.width-tx-1;bool cold=inMap(mx,my)&&map[my][mx]==FREEZER;column(t,tx,0,1,t.height,x,start,end-start,shade(WHITE,d,cold));
 }
}
struct SpriteDraw { V p; Texture2D* t; int frame; float size; float d; };
static void drawSprite(const SpriteDraw&s){V rel=sub(s.p,player);float ca=cosf(angle),sa=sinf(angle);float forward=rel.x*ca+rel.y*sa,side=-rel.x*sa+rel.y*ca;if(forward<=.08f)return;float sx=W/2+(side/forward)*(W/(2*tanf(FOV/2)));int sz=(int)(H*s.size/forward);int y=H/2-sz/2;int x0=(int)sx-sz/2;int frames=s.t->width/s.t->height;int fw=s.t->width/frames;for(int x=std::max(0,x0);x<std::min(W,x0+sz);x++)if(forward<zbuf[x]){int tx=(x-x0)*fw/sz;column(*s.t,s.frame*fw+tx,0,1,s.t->height,x,y,sz,WHITE);}}
static void renderSprites(){std::vector<SpriteDraw> a;for(auto&e:enemies){int fr;if(e.dead)fr=std::min(5,3+(int)(e.anim/.18f));else fr=(e.anim<.20f?0:e.anim<.40f?1:0);a.push_back({e.p,&tex.enemies[e.kind],fr,1.0f,dist(e.p,player)});}for(auto&p:pickups)if(!p.taken)a.push_back({p.p,&tex.pickups[p.kind],0,.55f,dist(p.p,player)});for(auto&p:projectiles)a.push_back({p.p,&tex.soup,0,.32f,dist(p.p,player)});std::sort(a.begin(),a.end(),[](auto&a,auto&b){return a.d>b.d;});for(auto&s:a)drawSprite(s);}
static void hud(){int base=H-72;DrawTexturePro(tex.panel,{0,0,(float)tex.panel.width,(float)tex.panel.height},{0,(float)base,(float)W,72},{0,0},0,WHITE);int f=health<=0?2:health<50?1:0;DrawTexturePro(tex.face,{(float)(f*48),0,48,56},{W/2-36.f,(float)base+6,72,56},{0,0},0,WHITE);DrawText(TextFormat("HEALTH  %3d%%",health),18,base+25,18,RAYWHITE);DrawText(TextFormat("AMMO  %3d",ammo),W/2+55,base+25,18,RAYWHITE);DrawText(TextFormat("ARMOUR  %3d%%",armour),W-260,base+25,18,RAYWHITE);DrawText(key?"KEY  [PASS]":"KEY  ----",W-125,base+7,14,key?YELLOW:LIGHTGRAY);if(messageTimer>0){int tw=MeasureText(message.c_str(),20);DrawRectangle(W/2-tw/2-10,H-116,tw+20,28,{0,0,0,180});DrawText(message.c_str(),W/2-tw/2,H-112,20,YELLOW);}}
static void weaponDraw(){Texture2D&t=tex.weapons[weapon];int fr=weaponAnim>.21?1:weaponAnim>0?2:0;float bob=sinf(elapsed*11)*((IsKeyDown(KEY_W)||IsKeyDown(KEY_A)||IsKeyDown(KEY_S)||IsKeyDown(KEY_D))?7:0);int fw=t.width/3;DrawTexturePro(t,{(float)(fr*fw),0,(float)fw,(float)t.height},{W/2-190.f,(float)(H-220+bob),384,288},{0,0},0,WHITE);}
static void title(){ClearBackground({7,18,27,255});DrawRectangle(0,H/2-115,W,2,{0,160,220,255});DrawText("AH: HELL AISLE",W/2-220,H/2-90,52,RAYWHITE);DrawText("NACHTDIENST // 02:14",W/2-154,H/2-25,24,{90,190,235,255});DrawText("PRESS ANY KEY",W/2-100,H/2+48,22,YELLOW);DrawText("Vind de keycard. Open het magazijn. Overleef de winkel.",W/2-245,H/2+100,18,LIGHTGRAY);if(GetKeyPressed()||IsMouseButtonPressed(MOUSE_BUTTON_LEFT))reset();}
static void endScreen(bool win){ClearBackground(win?Color{12,47,35,255}:Color{42,8,11,255});const char* head=win?"LOADING DOCK BEREIKT":"YOU DIED — GESLOTEN";int tw=MeasureText(head,42);DrawText(head,W/2-tw/2,H/2-80,42,win?LIME:RED);char line[100];snprintf(line,sizeof(line),"TIJD %02d:%02d   KILLS %d / %d",(int)elapsed/60,(int)elapsed%60,kills,(int)enemies.size());tw=MeasureText(line,24);DrawText(line,W/2-tw/2,H/2-5,24,RAYWHITE);DrawText("R — OPNIEUW BEGINNEN",W/2-155,H/2+60,20,YELLOW);if(IsKeyPressed(KEY_R))reset();}
int main(){InitWindow(W,H,"AH: Hell Aisle");SetExitKey(KEY_NULL);SetTargetFPS(60);tex.shelf=load("wall_shelf_full.png");tex.emptyShelf=load("wall_shelf_empty.png");tex.freezer=load("wall_freezer.png");tex.plain=load("wall_plain.png");tex.checkout=load("wall_checkout.png");tex.mag=load("wall_magazijn.png");tex.keydoor=load("door_keycard.png");tex.exitdoor=load("door_exit.png");tex.floor=load("floor.png");tex.ceiling=load("ceiling.png");tex.enemies[0]=load("enemy_winkelwagen.png");tex.enemies[1]=load("enemy_vakkenvuller.png");tex.enemies[2]=load("enemy_zelfscanner.png");tex.pickups[0]=load("pickup_appelflap.png");tex.pickups[1]=load("pickup_rookworst.png");tex.pickups[2]=load("pickup_labels.png");tex.pickups[3]=load("pickup_bonuskaart.png");tex.pickups[4]=load("pickup_keycard.png");tex.soup=load("proj_soepblik.png");tex.weapons[0]=load("weapon_stokbrood.png");tex.weapons[1]=load("weapon_prijspistool.png");tex.panel=load("hud_panel.png");tex.face=load("hud_face.png");loadMap();for(Texture2D*t:{&tex.shelf,&tex.emptyShelf,&tex.freezer,&tex.plain,&tex.checkout,&tex.mag,&tex.keydoor,&tex.exitdoor,&tex.floor,&tex.ceiling,&tex.enemies[0],&tex.enemies[1],&tex.enemies[2],&tex.soup,&tex.weapons[0],&tex.weapons[1],&tex.panel,&tex.face})SetTextureFilter(*t,TEXTURE_FILTER_POINT);while(!WindowShouldClose()){if(IsKeyPressed(KEY_ENTER)&&(IsKeyDown(KEY_LEFT_ALT)||IsKeyDown(KEY_RIGHT_ALT)))ToggleFullscreen();float dt=std::min(.05f,GetFrameTime());if(state==1)update(dt);if(state==5)break;BeginDrawing();if(state==0)title();else if(state==1){renderWorld();renderSprites();weaponDraw();hud();}else endScreen(state==3);EndDrawing();}CloseWindow();}
