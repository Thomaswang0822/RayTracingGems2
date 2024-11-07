#include "first_person_camera.h"
#include "first_person_camera.h"
#include <glm/gtx/transform.hpp>

FirstPersonCamera::FirstPersonCamera(const glm::vec3 &position,
                                     const glm::vec3 &direction,
                                     const glm::vec3 &up)
    : position(position),
      direction(glm::normalize(direction)),
      up(glm::normalize(up)),
      speed(0.1f)
{
}

void FirstPersonCamera::reset(glm::vec3 eye,
                              glm::vec3 camView,
                              glm::vec3 up)
{
    this->position = eye;
    this->direction = glm::normalize(camView);
    this->up = glm::normalize(up);
    this->speed = 0.1f;
}

void FirstPersonCamera::rotate(glm::vec2 prev_mouse, glm::vec2 cur_mouse)
{
    // Calculate mouse movement delta and rotation angles
    const float sensitivity = 0.5f;  // Control how fast the rotation responds
    glm::vec2 delta = (cur_mouse - prev_mouse) * sensitivity;

    // Calculate horizontal rotation (yaw)
    glm::mat4 yaw_matrix = glm::rotate(-delta.x, up);
    direction = glm::normalize(glm::vec3(yaw_matrix * glm::vec4(direction, 0.0)));

    // Calculate vertical rotation (pitch), using the right vector for rotation axis
    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    glm::mat4 pitch_matrix = glm::rotate(-delta.y, right);
    direction = glm::normalize(glm::vec3(pitch_matrix * glm::vec4(direction, 0.0)));

    // Adjust up vector if necessary (optional for first-person cameras)
    up = glm::normalize(glm::cross(right, direction));
}

void FirstPersonCamera::move(glm::vec3 move_dir)
{
    // Move the camera position in the direction of the arrow/WASD input
    glm::vec3 right = glm::normalize(glm::cross(direction, up));

    // Calculate movement in local space
    position += speed * (move_dir.z * direction + move_dir.x * right + move_dir.y * up);
}

glm::mat4 FirstPersonCamera::transform() const
{
    return glm::lookAt(position, position + direction, up);
}

glm::vec3 FirstPersonCamera::get_position() const
{
    return position;
}

glm::vec3 FirstPersonCamera::get_direction() const
{
    return direction;
}

glm::vec3 FirstPersonCamera::get_up() const
{
    return up;
}
