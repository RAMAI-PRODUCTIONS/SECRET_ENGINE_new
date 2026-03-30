#pragma once
#include "../Collision/CollisionPair.h"

class ImpulseSolver {
public:
    void Resolve(const CollisionPair& pair);
};