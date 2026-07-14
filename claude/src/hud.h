#pragma once

struct World;

void HudDraw(const World& w);          // status bar, messages, crosshair, damage flash
void ScreenTitle();
void ScreenDead(const World& w);
void ScreenEscaped(const World& w);
