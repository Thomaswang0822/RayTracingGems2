#pragma once

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>
#include <SDL.h>
#include "arcball_camera.h"
#include "first_person_camera.h"
#include "imgui.h"
#include "scene.h"
#include "stb_image_write.h"
#include "util.h"
#include "display/display.h"
#include "display/imgui_impl_sdl.h"
#include "render_plugin.h"

/// First some constants

static int win_width = 1280;
static int win_height = 720;


/// Then some short functions

static glm::vec2 transform_mouse(glm::vec2 in)
{
    return glm::vec2(in.x * 2.f / win_width - 1.f, 1.f - 2.f * in.y / win_height);
}

static void DisplayVec3(const char *label, const glm::vec3 vec)
{
    ImGui::Text("%s: (%.3f, %.3f, %.3f)", label, vec.x, vec.y, vec.z);
}


/// Last, signatures of long funtions

void resetControlCamera(FirstPersonCamera &camera,
                        glm::vec3 eye = {0, 3, 5},
                        glm::vec3 center = {0, 0, -1},
                        glm::vec3 up = {0, 1, 0});

// return bool done
bool process_SDL_Event(SDL_Event &event,
                       ImGuiIO &io,
                       FirstPersonCamera &camera,
                       Scene &scene,
                       SDL_Window *window,
                       std::unique_ptr<RenderBackend> &renderer,
                       Display *display,
                       bool &camera_changed,
                       glm::vec2 &prev_mouse,
                       const float fov_y);

bool camParamsDropdown(CameraParams &camParams);
