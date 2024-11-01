#ifndef CAMERA_HLSL
#define CAMERA_HLSL

#include "lcg_rng.hlsl"

/// view parameters in world space
struct ViewParams
{
    float4 cam_pos;
    // camera's right and up directions scaled by the image plane size.
    float4 cam_du;
    float4 cam_dv;
    float4 cam_dir_forward;  // center of image plane
    float4 cam_dir_top_left; // top-left corner
    
    uint32_t frame_id;
    uint32_t samples_per_pixel;
    float2 padding;
};

struct CameraParams
{
    float cameraFOVAngle;
    int cameraFOVDirection;
    float2 imageSize;
    
    float paniniDistance;
    float paniniVerticalCompression;
    float cameraFovDistance;
    float lensFocalLength;
    
    float fStop;
    float imagePlaneDistance;
    uint32_t camType; // no enum in HLSL, see util/Camera/camera.h
    float padding;
};


/// Utility function, convert a direction from camera space to world space
float3 dirCamToWorld(const in ViewParams viewParams, const in float3 direction)
{
    float3 dir_world = normalize(
        direction.x * viewParams.cam_du.xyz +
        direction.y * viewParams.cam_dv.xyz +
        direction.z * viewParams.cam_dir_forward.xyz
    );
    
    return dir_world;
}

///
/// The following computeXXXRay() functions compute the origin and direction
/// of the camera ray.
///

void computePinholeRay(
    const in uint2 pixel, const in float2 dims, inout LCGRand rng,
    const in ViewParams viewParams, const in CameraParams camParams,
    inout float3 origin, inout float3 dir
)
{
    const float2 d = (pixel + float2(lcg_randomf(rng), lcg_randomf(rng))) / dims;
    
    origin = viewParams.cam_pos.xyz;
    dir = normalize(d.x * viewParams.cam_du.xyz + d.y * viewParams.cam_dv.xyz + viewParams.cam_dir_top_left.xyz);
}


void computePaniniRay(
    const in uint2 pixel, const in float2 dims, inout LCGRand rng,
    const in ViewParams viewParams, const in CameraParams camParams,
    inout float3 origin, inout float3 dir
)
{
    float2 pixelCenterCoords = float2(pixel) + float2(lcg_randomf(rng), lcg_randomf(rng)) - (camParams.imageSize / 2.f);

    float halfFOV = camParams.cameraFOVAngle / 2.f;
    float halfPaniniFOV = atan2(sin(halfFOV), cos(halfFOV) + camParams.paniniDistance);
    float2 hvPan = tan(halfPaniniFOV) * (1.f + camParams.paniniDistance);
    
    hvPan *= pixelCenterCoords / (bool(camParams.cameraFOVDirection == 0) ? camParams.imageSize.x : camParams.imageSize.y);
    hvPan.x = atan(hvPan.x / (1.0 + camParams.paniniDistance));

    float M = sqrt(1 - pow2(sin(hvPan.x) * camParams.paniniDistance)) + camParams.paniniDistance * cos(hvPan.x);

    float x = sin(hvPan.x) * M;
    float z = (cos(hvPan.x) * M) - camParams.paniniDistance;

    float S = (camParams.paniniDistance + 1) / (camParams.paniniDistance + z);

    float y = lerp(hvPan.y / S, hvPan.y * z, camParams.paniniVerticalCompression);

    // camera space
    float3 direction = normalize(float3(x, -y, -z));
    
    // write
    origin = viewParams.cam_pos.xyz;
    dir = dirCamToWorld(viewParams, direction);
}
#endif