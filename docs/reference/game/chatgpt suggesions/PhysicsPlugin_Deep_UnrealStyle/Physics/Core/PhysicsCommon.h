#pragma once
#include <vector>
#include <cstdint>
#include "../../Common/Math.h"

using BodyID = uint32_t;
static constexpr BodyID INVALID_BODY = 0;

enum class EBodyType {
    Static,
    Dynamic,
    Kinematic
};