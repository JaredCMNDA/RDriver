#pragma once
#include "dataTypes.h"

void ConvertToRange(Vec2& Point)
{
    Point.X /= 1920.0f;
    Point.X *= 2.0f;
    Point.X -= 1.0f;

    Point.Y /= 1080.0f;
    Point.Y *= 2.0f;
    Point.Y -= 1.0f;
}