#include "main_util.h"

void resetControlCamera(FirstPersonCamera &camera,
                        glm::vec3 eye,
                        glm::vec3 center,
                        glm::vec3 up)
{
    camera.reset(eye, center, up);
}

bool process_SDL_Event(SDL_Event &event,
                       ImGuiIO &io,
                       FirstPersonCamera &camera,
                       SDL_Window *window,
                       std::unique_ptr<RenderBackend> &renderer,
                       Display* display,
                       bool &camera_changed,
                       glm::vec2 &prev_mouse,
                       const float fov_y
)
{
    bool done = false;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            done = true;
        }
        if (!io.WantCaptureKeyboard && event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                done = true;
            } else if (event.key.keysym.sym == SDLK_p) {
                auto eye = camera.get_position();
                auto center = camera.get_direction();
                auto up = camera.get_up();
                std::cout << "-eye " << eye.x << " " << eye.y << " " << eye.z << " -center "
                          << center.x << " " << center.y << " " << center.z << " -up " << up.x
                          << " " << up.y << " " << up.z << " -fov " << fov_y << "\n";
            } else if (event.key.keysym.sym == SDLK_r) {
                camera.reset();
                camera_changed = true;
            }
            // WASD controls for camera movement, NOTE: +z is toward viewer in camera space.
            else if (event.key.keysym.sym == SDLK_w) {
                camera.move(glm::vec3(0, 0, -1));  // Move forward
                camera_changed = true;
            } else if (event.key.keysym.sym == SDLK_s) {
                camera.move(glm::vec3(0, 0, 1));  // Move backward
                camera_changed = true;
            } else if (event.key.keysym.sym == SDLK_a) {
                camera.move(glm::vec3(-1, 0, 0));  // Move left
                camera_changed = true;
            } else if (event.key.keysym.sym == SDLK_d) {
                camera.move(glm::vec3(1, 0, 0));  // Move right
                camera_changed = true;
            } else if (event.key.keysym.sym == SDLK_q) {
                camera.move(glm::vec3(0, 1, 0));  // Move up
                camera_changed = true;
            } else if (event.key.keysym.sym == SDLK_e) {
                camera.move(glm::vec3(0, -1, 0));  // Move down
                camera_changed = true;
            }
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window)) {
            done = true;
        }
        if (!io.WantCaptureMouse) {
            if (event.type == SDL_MOUSEMOTION) {
                const glm::vec2 cur_mouse =
                    transform_mouse(glm::vec2(event.motion.x, event.motion.y));
                if (prev_mouse != glm::vec2(-2.f)) {
                    if (event.motion.state & SDL_BUTTON_LMASK) {
                        camera.rotate(prev_mouse, cur_mouse);
                        camera_changed = true;
                    }
                }
                prev_mouse = cur_mouse;
            } else if (event.type == SDL_MOUSEWHEEL) {
                // camera.zoom(event.wheel.y * 0.1);
                // camera_changed = true;
            }
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            camera_changed = true;
            win_width = event.window.data1;
            win_height = event.window.data2;
            io.DisplaySize.x = win_width;
            io.DisplaySize.y = win_height;

            display->resize(win_width, win_height);
            renderer->initialize(win_width, win_height);
        }
    }

    return done;
}
