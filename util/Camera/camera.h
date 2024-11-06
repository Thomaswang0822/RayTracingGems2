#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <SDL_stdinc.h>

static const float fovDefaultDeg = 90.f;

struct Camera {
    glm::vec3 position, center, up;
    float fov_y;  // in degree, default to 90
};

enum CameraType : uint32_t 
{
    Pinhole,
    ThinLens,
    Panini,
    FishEye,
    Orthographic
};

/// Implement RTG2 chapter 3, with some default value
struct CameraParams {
    // Edge-to-edge field of view , in radians
    float cameraFOVAngle = fovDefaultDeg * M_PI / 180.f;

    // 0 = cameraFOV is the horizontal FOV
    // 1 = vertical
    // 2 = diagonal (only used for fisheye lens)
    int cameraFOVDirection = 0;

    // Integers stored as floats to avoid conversion, = render_target.dims()
    glm::vec2 imageSize = {1920.f, 1080.f};

    // Center of projection from cylinder to plane ,
    // can be any positive number
    float paniniDistance = 1.f;

    // 0-1 value to force straightening of horizontal lines
    // (0 = no straightening , 1 = full straightening)
    float paniniVerticalCompression = 1.f;

    // Scalar field of view in m, used for orthographic projection
    float cameraFovDistance = 6.0f;

    // Lens focal length in meters ,
    // would be measured in millimeters for a physical camera
    float lensFocalLength = 0.030f;

    // Ratio of focal length to aperture diameter
    float fStop = lensFocalLength / 0.035f;

    // Distance from the image plane to the lens
    // want focus distance ~= 5.f
    float imagePlaneDistance = 0.0302f;

    CameraType type = CameraType::Pinhole;

    // to ensure multiple of 4 bytes
    float padding = 0.f;
};
