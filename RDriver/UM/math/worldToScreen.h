#pragma once
#include "dataTypes.h"
#include "convertToRange.h"

bool WorldToScreen(const Vec3& VecOrigin, Vec2& VecScreen, float* Matrix)
{
    //Matrix-vector Product, multiplying world(eye) coordinates by projection Matrix = clipCoords
    VecScreen.X = VecOrigin.X * Matrix[0] + VecOrigin.Y * Matrix[1] + VecOrigin.Z * Matrix[2] + Matrix[3];
    VecScreen.Y = VecOrigin.X * Matrix[4] + VecOrigin.Y * Matrix[5] + VecOrigin.Z * Matrix[6] + Matrix[7];
    float W = VecOrigin.X * Matrix[12] + VecOrigin.Y * Matrix[13] + VecOrigin.Z * Matrix[14] + Matrix[15];

    if (W < 0.01f)
        return false;

    //perspective division, dividing by clip.W = NormaliZed Device Coordinates
    Vec2 NDC;
    NDC.X = VecScreen.X / W;
    NDC.Y = VecScreen.Y / W;

    VecScreen.X = (1920 / 2 * NDC.X) + (NDC.X + 1920 / 2);
    VecScreen.Y = (1080 / 2 * NDC.Y) + (NDC.Y + 1080 / 2);

    ConvertToRange(VecScreen);

    return true;
}