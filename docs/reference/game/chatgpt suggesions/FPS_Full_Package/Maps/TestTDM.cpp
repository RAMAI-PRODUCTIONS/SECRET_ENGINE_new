#include "SecretEngine/World.h"
#include "../Common/Math.h"

void LoadTestTDM(World* world) {
    world->Clear();
    auto* p = world->CreatePlayer();
    p->position = Vec3(0,0,0);
    p->health = 100;
    p->team = 0;
}