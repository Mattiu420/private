#include "aimbot.h"
#include <thread>
#include <chrono>
#include "../sdk/vectorh.h"
#include <iostream>
#include "../Hotkey/hotkey.h"
#include <cmath>
#include <limits>
#include <algorithm>

i_aim& i_aim::aim()
{
    static i_aim ins;
    return ins;
}

uintptr_t i_aim::BestTarget()
{
    uintptr_t ClosestPlayer = 0;
    uintptr_t ReplayInterface = i_dupa::skid().i_replay;
    uintptr_t localplayer = i_dupa::skid().i_localplayer;
    float crosshair_dist;
    float closestdist = FLT_MAX;

    if (ReplayInterface)
    {
        uintptr_t PedReplayInterface = i_memory::reeq().Read<uintptr_t>(ReplayInterface + 0x18);
        uintptr_t PedList = i_memory::reeq().Read<uintptr_t>(PedReplayInterface + 0x100);
        int entitylist = i_memory::reeq().Read<int>(PedReplayInterface + 0x108);

        for (int i = 0U; i < entitylist; i++)
        {
            if (!PedList) continue;

            uintptr_t Ped = i_memory::reeq().Read<uintptr_t>(PedList + (i * 0x10));
            if (!Ped) continue;

            uint64_t playerinfos = i_memory::reeq().Read<uint64_t>(Ped + i_dupa::skid().playerinfo);
            int playeridaddr = i_memory::reeq().Read<int>(playerinfos + 0x88);

            if (Ped == localplayer) continue;

            if (ignoreped && !i_memory::reeq().Read<uintptr_t>(Ped + i_dupa::skid().playerinfo))
                continue;

            float HealthPed = i_memory::reeq().Read<float>(Ped + 0x280);

            if (ignoredeath && HealthPed == 0)
                continue;

            if (ignoredown && HealthPed == 150)
                continue;

            D3DXVECTOR3 GetCordLocal = i_memory::reeq().Read<D3DXVECTOR3>(localplayer + 0x90);
            D3DXVECTOR3 GetCordPed = i_memory::reeq().Read<D3DXVECTOR3>(Ped + 0x90);
            D3DXVECTOR3 DistanceCalculation = (GetCordLocal - GetCordPed);
            double Distance = sqrtf(DistanceCalculation.x * DistanceCalculation.x + DistanceCalculation.y * DistanceCalculation.y + DistanceCalculation.z * DistanceCalculation.z);

            auto iop = i_sdk::sdk().get_bone_position(Ped, 0);
            D3DXVECTOR2 Head = i_sdk::sdk().world_to_screen(iop);
            float x = Head.x - GetSystemMetrics(SM_CXSCREEN) / 2.f;
            float y = Head.y - GetSystemMetrics(SM_CYSCREEN) / 2.f;
            crosshair_dist = sqrtf(x * x + y * y);

            if (Distance <= distance && crosshair_dist < closestdist)
            {
                closestdist = crosshair_dist;
                ClosestPlayer = Ped;
            }
        }
    }

    return ClosestPlayer;
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

void interpolate_mouse_movement(float target_x, float target_y, float sensitivity, int steps, int delay_ms) {
    for (int i = 1; i <= steps; ++i) {
        // Calculate interpolation factor (t)
        float t = static_cast<float>(i) / steps;

        // Perform linear interpolation for x and y
        float interpolated_x = lerp(0, target_x, t);
        float interpolated_y = lerp(0, target_y, t);

        // Apply the interpolated mouse movement
        mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(interpolated_x * sensitivity),
            static_cast<DWORD>(interpolated_y * sensitivity), 0, 0);

        // Introduce a delay to control the speed of interpolation
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}


void i_aim::aimbot(uintptr_t Ped)
{
    auto get_distance = [](double x1, double y1, double x2, double y2) {
        return sqrtf(static_cast<float>(pow(x2 - x1, 2.0) + pow(y2 - y1, 2.0)));
        };

    int bones = (boneid == 0) ? 0 : 1;

    auto bone_pos = i_sdk::sdk().get_bone_position(Ped, bones);
    D3DXVECTOR2 screen = i_sdk::sdk().world_to_screen(bone_pos);

    if (screen == D3DXVECTOR2(0, 0))
        return;

    const auto center_x = GetSystemMetrics(SM_CXSCREEN) / 2;
    const auto center_y = GetSystemMetrics(SM_CYSCREEN) / 2;
    const auto fov1 = get_distance(center_x, center_y, screen.x, screen.y);

    
    const float smooth_factor = smooth - 0.5;

    float target_x = 0;
    float target_y = 0;

    if (screen.x != 0) {
        target_x = (screen.x > center_x) ? -(center_x - screen.x) : screen.x - center_x;
        target_x /= smooth_factor;
        target_x = (target_x + center_x > center_x * 2 || target_x + center_x < 0) ? 0 : target_x;
    }

    if (screen.y != 0) {
        target_y = (screen.y > center_y) ? -(center_y - screen.y) : screen.y - center_y;
        target_y /= smooth_factor;
        target_y = (target_y + center_y > center_y * 2 || target_y + center_y < 0) ? 0 : target_y;
    }

    constexpr float fov_threshold = 100.0f;  // Adjust as needed
    constexpr float sensitivity = 3.5;      // Adjust as needed
    constexpr int interpolation_steps = 5; // Adjust as needed
    constexpr int interpolation_delay_ms = 0; // Adjust as needed

    if (fov1 < fov_threshold && (GetAsyncKeyState(Hotkey::hot().Aimbotkey) & 0x8000)) {
        interpolate_mouse_movement(target_x, target_y, sensitivity, interpolation_steps, interpolation_delay_ms);
    }
}

Vec3 D3DXVECTOR3ToVec3(const D3DXVECTOR3& v)
{
    return Vec3{ v.x, v.y, v.z };
}

void i_aim::MemoryAim(const D3DXVECTOR3& targetPoint, uintptr_t pedAddress, float smooth) const
{
    // Get the camera address from the game memory
    const DWORD64 cameraAddress = i_memory::reeq().Read<DWORD64>(i_memory::reeq().base + i_dupa::skid().i_camera);

    // Check if the camera address is valid
    if (cameraAddress)
    {
        // Read view angles and camera position from the camera address
        const Vec3 viewAngles = i_memory::reeq().Read<Vec3>(cameraAddress + 0x40);
        const Vec3 cameraPosition = i_memory::reeq().Read<Vec3>(cameraAddress + 0x60);

        // Convert the target point to a Vec3
        const Vec3 target_bone = D3DXVECTOR3ToVec3(targetPoint);

        // Calculate delta vector and distance between camera and target
        const Vec3 delta = target_bone - cameraPosition;
        const float distance = delta.Length();

        // Calculate the final aiming angle
        const Vec3 finalAngle = viewAngles + (smooth > 0.0f ? (target_bone - viewAngles) / smooth : target_bone - viewAngles);

        // Normalize the final angle
        Vec3 normalizedFinalAngle = finalAngle;
        normalizedFinalAngle.Normalize();

        // Write the normalized final angle back to the camera address
        i_memory::reeq().Write<Vec3>(cameraAddress + 0x40, normalizedFinalAngle);
    }
}

void i_aim::HookMemoryAim()
{
    auto get_distance = [](double x1, double y1, double x2, double y2) {
        return sqrtf(static_cast<float>(pow(x2 - x1, 2.0) + pow(y2 - y1, 2.0)));
    };

    auto center_x = GetSystemMetrics(SM_CXSCREEN) / 2;
    auto center_y = GetSystemMetrics(SM_CYSCREEN) / 2;

    int bones2 = (boneid == 0) ? 0 : 1;
    uintptr_t entity = BestTarget();

    if (entity)
    {
        auto bone_pos = i_sdk::sdk().get_bone_position(entity, bones2);
        D3DXVECTOR2 screen = i_sdk::sdk().world_to_screen(bone_pos);
        auto fov2 = get_distance(center_x, center_y, screen.x, screen.y);

        if (GetAsyncKeyState(Hotkey::hot().Aimbotkey) & 0x8000 && fov2 < fov)
        {
            MemoryAim(bone_pos, entity, smooth);  // Pass the smooth parameter
        }
    }
}

void i_aim::start()
{
    if (i_dupa::skid().i_localplayer && aimenable)
    {
        if (aimbottype == 0 || aimbottype == 1)
        {
            uintptr_t target = BestTarget();
            if (target)
            {
                if (aimbottype == 0)
                {
                    aimbot(target);
                }
                else if (aimbottype == 1)
                {
                    HookMemoryAim();
                }
            }
        }
    }
}