#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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
    float cameraFOVAngle = glm::half_pi<float>();

    // 0 = cameraFOV is the horizontal FOV
    // 1 = vertical
    // 2 = diagonal (only used for fisheye lens)
    int cameraFOVDirection = 0;

    // Integers stored as floats to avoid conversion, = render_target.dims()
    glm::vec2 imageSize = {1280.f, 720.f};

    // Center of projection from cylinder to plane ,
    // can be any positive number
    float paniniDistance = 1.f;

    // 0-1 value to force straightening of horizontal lines
    // (0 = no straightening , 1 = full straightening)
    float paniniVerticalCompression = 0.f;

    // Scalar field of view in m, used for orthographic projection
    float cameraFovDistance = 1.0f;

    // Lens focal length in meters ,
    // would be measured in millimeters for a physical camera
    float lensFocalLength = 0.050f;

    // Ratio of focal length to aperture diameter
    float fStop = 8.0f;

    // Distance from the image plane to the lens
    // The camera is modeled at the center of the lens.
    float imagePlaneDistance = lensFocalLength;

    CameraType type = CameraType::FishEye;

    // to ensure multiple of 4 bytes
    float padding = 0.f;
};
