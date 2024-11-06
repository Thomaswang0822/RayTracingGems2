#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>

static const glm::mat3 camDefault = {
    glm::vec3(0, 5, 2.4f),    // eye, pos
    glm::vec3(0, 0, -1),  // view dir
    glm::vec3(0, 1, 0)      // up
};

/* A first-person camera that moves based on WASD/arrow keys for position and
 * mouse dragging for orientation.
 */
class FirstPersonCamera {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;
    float speed;  // movement speed factor

public:
    // Initialize the camera with a position, look direction, and up vector
    FirstPersonCamera(const glm::vec3 &position,
                      const glm::vec3 &direction,
                      const glm::vec3 &up);

    void reset(glm::vec3 eye = camDefault[0],
               glm::vec3 camView = camDefault[1],
               glm::vec3 up = camDefault[2]);

    // Rotate the camera based on mouse delta
    void rotate(glm::vec2 prev_mouse, glm::vec2 cur_mouse);

    // Move the camera based on WASD or arrow key input
    void move(glm::vec3 direction);

    // Get the camera transformation matrix
    glm::mat4 transform() const;

    // Get the position of the camera in world space
    glm::vec3 get_position() const;

    // Get the direction the camera is facing
    glm::vec3 get_direction() const;

    // Get the up vector of the camera
    glm::vec3 get_up() const;
};
