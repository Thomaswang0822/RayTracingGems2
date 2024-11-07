#ifndef CAMERA_GLSL
#define CAMERA_GLSL

#include "lcg_rng.glsl"

/// view parameters in world space
struct ViewParams {
    vec4 cam_pos;
    // camera's right and up directions scaled by the image plane size.
    vec4 cam_du;
    vec4 cam_dv;
    vec4 cam_dir_forward;   // -z dir, default (0,0,-1)
    vec4 cam_dir_top_left;  // top-left corner

    int frame_id;
    int samples_per_pixel;
    vec2 padding;
};

struct CameraParams {
    float cameraFOVAngle;
    int cameraFOVDirection;
    vec2 imageSize;

    float paniniDistance;
    float paniniVerticalCompression;
    float cameraFovDistance;
    float lensFocalLength;

    float fStop;
    float imagePlaneDistance;
    uint camType;  // no enum in HLSL, see util/Camera/camera.h
    float padding;
};

///
/// About spaces
/// There are 3 spaces involved in this file: camera, world, image/pixel
/// - world space: x - right, y - up, z - backward (right-hand system);
/// - camera space: x - right, y - up, z - backward (toward viewer)
/// - pixel space (2D): x - right, y - down.

/// Utility function, convert a direction from camera space to world space
/// NOTE:
/// 1. cam_dv is down instead of up
/// 2. a camera-space (0, 0, -1) should be mapped to world-space cam_dir_forward.xyz
vec3 dirCamToWorld(const in ViewParams viewParams, const in vec3 direction)
{
    vec3 dir_world =
        normalize(direction.x * viewParams.cam_du.xyz - direction.y * viewParams.cam_dv.xyz -
                  direction.z * viewParams.cam_dir_forward.xyz);

    return dir_world;
}

///
/// The following computeXXXRay() functions compute the origin and direction
/// of the camera ray.
///

void computePinholeRay(const in ivec2 pixel,
                       const in vec2 dims,
                       inout LCGRand rng,
                       const in ViewParams viewParams,
                       const in CameraParams camParams,
                       out vec3 origin,
                       out vec3 dir)
{
    float tanHalfAngle = tan(camParams.cameraFOVAngle / 2.f);
    float aspectScale =
        ((camParams.cameraFOVDirection == 0) ? camParams.imageSize.x : camParams.imageSize.y) /
        2.f;
    vec2 pixel_cord =
        pixel + vec2(lcg_randomf(rng), lcg_randomf(rng)) - camParams.imageSize / 2.f;
    // y is flipped for image <-> camera space
    vec3 direction = normalize(
        vec3(vec2(pixel_cord.x, -pixel_cord.y) * tanHalfAngle / aspectScale, -1.f));

    // write
    origin = viewParams.cam_pos.xyz;
    dir = dirCamToWorld(viewParams, direction);
}

void computeThinLensRay(const in ivec2 pixel,
                        const in vec2 dims,
                        inout LCGRand rng,
                        const in ViewParams viewParams,
                        const in CameraParams camParams,
                        out vec3 origin,
                        out vec3 dir)
{
    // Step 1: pinhole ray dir in camera space
    // Ray pinholeRay = pinholeRay(pixel);
    vec3 pinholeDir, _useless;
    computePinholeRay(pixel, dims, rng, viewParams, camParams, _useless, pinholeDir);

    vec2 lensOffset = {lcg_randomf(rng), lcg_randomf(rng)};
    float theta = lensOffset.x * 2.f * M_PI;
    float sqrt_radius = sqrt(lensOffset.y);

    float u = cos(theta) * sqrt_radius;
    float v = sin(theta) * sqrt_radius;

    // Step 2: compute focal point
    float focusPlane = (camParams.imagePlaneDistance * camParams.lensFocalLength) /
                       (camParams.imagePlaneDistance - camParams.lensFocalLength);
    vec3 focusPoint =
        viewParams.cam_pos.xyz +
        pinholeDir * (focusPlane / dot(pinholeDir, viewParams.cam_dir_forward.xyz));

    // Step 3: draw sample on a disk and compute lens offset
    float circleOfConfusionRadius = camParams.lensFocalLength / (2.0f * camParams.fStop);

    // Step 4: compute origin and then direction
    origin = viewParams.cam_pos.xyz +
             circleOfConfusionRadius * (u * viewParams.cam_du.xyz + v * viewParams.cam_dv.xyz);
    dir = normalize(focusPoint - origin);
}

void computePaniniRay(const in ivec2 pixel,
                      const in vec2 dims,
                      inout LCGRand rng,
                      const in ViewParams viewParams,
                      const in CameraParams camParams,
                      out vec3 origin,
                      out vec3 dir)
{
    vec2 pixelCenterCoords = vec2(pixel) + vec2(lcg_randomf(rng), lcg_randomf(rng)) -
                               (camParams.imageSize / 2.f);

    float halfFOV = camParams.cameraFOVAngle / 2.f;
    float halfPaniniFOV = atan(sin(halfFOV), cos(halfFOV) + camParams.paniniDistance);
    vec2 hvPan = vec2(tan(halfPaniniFOV)) * (1.f + camParams.paniniDistance);

    // UNRESOLVED BUG in the book!
    hvPan *= pixelCenterCoords / (bool(camParams.cameraFOVDirection == 0)
                                      ? camParams.imageSize.x / 2.f
                                      : camParams.imageSize.y / 2.f);
    hvPan.x = atan(hvPan.x / (1.0 + camParams.paniniDistance));

    float M = sqrt(1 - pow2(sin(hvPan.x) * camParams.paniniDistance)) +
              camParams.paniniDistance * cos(hvPan.x);

    float x = sin(hvPan.x) * M;
    float z = (cos(hvPan.x) * M) - camParams.paniniDistance;

    float S = (camParams.paniniDistance + 1) / (camParams.paniniDistance + z);

    float y = mix(hvPan.y / S, hvPan.y * z, camParams.paniniVerticalCompression);

    // camera space
    vec3 direction = normalize(vec3(x, -y, -z));

    // write
    origin = viewParams.cam_pos.xyz;
    dir = dirCamToWorld(viewParams, direction);
}

void computeFishEyeRay(const in ivec2 pixel,
                       const in vec2 dims,
                       inout LCGRand rng,
                       const in ViewParams viewParams,
                       const in CameraParams camParams,
                       out vec3 origin,
                       out vec3 dir)
{
    vec2 clampedHalfFOV = vec2(min(camParams.cameraFOVAngle, M_PI)) / 2.f;
    vec2 angle = (pixel - camParams.imageSize / 2.f) * clampedHalfFOV;

    if (camParams.cameraFOVDirection == 0) {
        angle /= camParams.imageSize.x / 2.f;
    } else if (camParams.cameraFOVDirection == 1) {
        angle /= camParams.imageSize.y / 2.f;
    } else {
        angle /= length(camParams.imageSize) / 2.f;
    }

    // Don't generate rays for pixels outside the fisheye
    // (circle and cropped circle only).
    if (length(angle) > 0.5f * M_PI) {
        origin = vec3(0.f);
        dir = vec3(0.f);
    }

    vec3 direction = normalize(
        vec3(sin(angle.x), -sin(angle.y) * cos(angle.x), -cos(angle.x) * cos(angle.y)));
    // write
    origin = viewParams.cam_pos.xyz;
    dir = dirCamToWorld(viewParams, direction);
}

void computeOrthographicRay(const in ivec2 pixel,
                            const in vec2 dims,
                            inout LCGRand rng,
                            const in ViewParams viewParams,
                            const in CameraParams camParams,
                            out vec3 origin,
                            out vec3 dir)
{
    // Compute normalized pixel coordinates with subpixel jitter
    vec2 d = (vec2(pixel) + vec2(lcg_randomf(rng), lcg_randomf(rng))) / dims;

    // Map d from [0,1] to [-0.5, 0.5]
    vec2 ndc = d - 0.5f;

    // Compute the size of the image plane in world units
    float imagePlaneWidth = camParams.cameraFovDistance;
    float imagePlaneHeight = camParams.cameraFovDistance * (dims.y / dims.x);

    // Compute the offsets in world space
    vec3 offset = ndc.x * imagePlaneWidth * viewParams.cam_du.xyz +
                    ndc.y * imagePlaneHeight * viewParams.cam_dv.xyz;

    // write
    origin = viewParams.cam_pos.xyz + offset;
    dir = normalize(viewParams.cam_dir_forward.xyz);
}

#endif