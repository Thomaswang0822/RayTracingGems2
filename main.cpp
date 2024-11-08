#include "main_util.h"

const std::string USAGE =
    "Usage: <backend> <mesh.obj/gltf/glb> [options]\n"
    "Render backend libraries should be named following (lib)crt_<backend>.(dll|so)\n"
    "Options:\n"
    "\t-eye <x> <y> <z>       Set the camera position\n"
    "\t-camView <x> <y> <z>    Set the camera focus point\n"
    "\t-up <x> <y> <z>        Set the camera up vector\n"
    "\t-fov <fovy>            Specify the camera field of view (in degrees)\n"
    "\t-spp <n>               Specify the number of samples to take per-pixel. Defaults to 1\n"
    "\t-camera <n>            If the scene contains multiple cameras, specify which\n"
    "\t                       should be used. Defaults to the first camera\n"
    "\t-img <x> <y>           Specify the window dimensions. Defaults to 1280x720\n"
    "\t-mat-mode <MODE>       Specify the material mode, default (the default) or "
    "white_diffuse\n"
    "\n";

const size_t max_frames = 1024;

void run_app(const std::vector<std::string> &args,
             SDL_Window *window,
             Display *display,
             RenderPlugin *render_plugin);

// Helper function to display an ImGui dropdown for CameraType


int main(int argc, const char **argv)
{
    const std::vector<std::string> args(argv, argv + argc);
    auto fnd_help = std::find_if(args.begin(), args.end(), [](const std::string &a) {
        return a == "-h" || a == "--help";
    });

    if (argc < 3 || fnd_help != args.end()) {
        std::cout << USAGE;
        return 1;
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::cerr << "Failed to init SDL: " << SDL_GetError() << "\n";
        return -1;
    }

    std::unique_ptr<RenderPlugin> render_plugin =
        std::make_unique<RenderPlugin>("crt_" + args[1]);
    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "-img") {
            win_width = std::stoi(args[++i]);
            win_height = std::stoi(args[++i]);
            continue;
        }
    }

    const uint32_t window_flags = render_plugin->get_window_flags() | SDL_WINDOW_RESIZABLE;
    if (window_flags & SDL_WINDOW_OPENGL) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    }

    SDL_Window *window = SDL_CreateWindow("gemsRT",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          win_width,
                                          win_height,
                                          window_flags);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_Init(window);

    render_plugin->set_imgui_context(ImGui::GetCurrentContext());
    {
        std::unique_ptr<Display> display = render_plugin->make_display(window);
        run_app(args, window, display.get(), render_plugin.get());
    }

    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void run_app(const std::vector<std::string> &args,
             SDL_Window *window,
             Display *display,
             RenderPlugin *render_plugin)
{
    ImGuiIO &io = ImGui::GetIO();

    std::string scene_file;
    bool got_camera_args = false;
    // camDefault defined in first_person_camera.h
    glm::vec3 eye = camDefault[0];
    glm::vec3 camView = camDefault[1];
    glm::vec3 up = camDefault[2];
    float fov_y = fovDefaultDeg;  // 120
    uint32_t samples_per_pixel = 1;
    size_t camera_id = 0;
    size_t benchmark_frames = 0;
    std::string validation_img_prefix;
    MaterialMode material_mode = MaterialMode::DEFAULT;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-eye") {
            eye.x = std::stof(args[++i]);
            eye.y = std::stof(args[++i]);
            eye.z = std::stof(args[++i]);
            got_camera_args = true;
        } else if (args[i] == "-camView") {
            camView.x = std::stof(args[++i]);
            camView.y = std::stof(args[++i]);
            camView.z = std::stof(args[++i]);
            got_camera_args = true;
        } else if (args[i] == "-up") {
            up.x = std::stof(args[++i]);
            up.y = std::stof(args[++i]);
            up.z = std::stof(args[++i]);
            got_camera_args = true;
        } else if (args[i] == "-fov") {
            fov_y = std::stof(args[++i]);
            got_camera_args = true;
        } else if (args[i] == "-spp") {
            samples_per_pixel = std::stoi(args[++i]);
        } else if (args[i] == "-camera") {
            camera_id = std::stol(args[++i]);
        } else if (args[i] == "-validation") {
            validation_img_prefix = args[++i];
        } else if (args[i] == "-img") {
            i += 2;
        } else if (args[i] == "-mat-mode") {
            if (args[++i] == "white_diffuse") {
                material_mode = MaterialMode::WHITE_DIFFUSE;
            }
        } else if (args[i] == "-benchmark-frames") {
            benchmark_frames = std::stoi(args[++i]);
        } else if (args[i][0] != '-') {
            scene_file = args[i];
            canonicalize_path(scene_file);
        }
    }

    std::unique_ptr<RenderBackend> renderer = render_plugin->make_renderer(display);

    if (!renderer) {
        std::cout << "Error: No renderer backend or invalid backend name specified\n" << USAGE;
        std::exit(1);
    }
    if (scene_file.empty()) {
        std::cout << "Error: No model file specified\n" << USAGE;
        std::exit(1);
    }

    display->resize(win_width, win_height);
    renderer->initialize(win_width, win_height);

    std::string scene_info;
    //{
        Scene scene(scene_file, material_mode);
        scene.samples_per_pixel = samples_per_pixel;

        std::stringstream ss;
        ss << "Scene '" << scene_file << "':\n"
           << "# Unique Triangles: " << pretty_print_count(scene.unique_tris()) << "\n"
           << "# Total Triangles: " << pretty_print_count(scene.total_tris()) << "\n"
           << "# Geometries: " << scene.num_geometries() << "\n"
           << "# Meshes: " << scene.meshes.size() << "\n"
           << "# Parameterized Meshes: " << scene.parameterized_meshes.size() << "\n"
           << "# Instances: " << scene.instances.size() << "\n"
           << "# Materials: " << scene.materials.size() << "\n"
           << "# Textures: " << scene.textures.size() << "\n"
           << "# Lights: " << scene.lights.size() << "\n"
           << "# Cameras: " << scene.cameras.size() << "\n"
           << "# Camera Type: " << scene.camParams.type << "\n"
           << "# Samples per Pixel: " << scene.samples_per_pixel;

        scene_info = ss.str();
        std::cout << scene_info << "\n";

        renderer->set_scene(scene);

        if (!got_camera_args && !scene.cameras.empty() && camera_id <= scene.cameras.size()) {
            eye = scene.cameras[camera_id].position;
            camView = glm::normalize(scene.cameras[camera_id].center -
                                     scene.cameras[camera_id].position);
            up = scene.cameras[camera_id].up;
            fov_y = scene.cameras[camera_id].fov_y;
            scene.camParams.cameraFOVAngle = fov_y * M_PI / 180.f;
        }
    //}

    FirstPersonCamera camera(eye, camView, up);

    const std::string rt_backend = renderer->name();
    const std::string cpu_brand = get_cpu_brand();
    const std::string gpu_brand = display->gpu_brand();
    std::string image_output = "screenshot.png";
    const std::string image_dir = "screenshots/";
    const std::string display_frontend = display->name();
    char textBuf[256];  // to hold input text

    size_t frame_id = 0;
    float render_time = 0.f;
    float rays_per_second = 0.f;
    glm::vec2 prev_mouse(-2.f);
    bool done = false;
    bool camera_changed = true;
    bool save_image = false;
    while (!done) {
        SDL_Event event;
        done = process_SDL_Event(
            event, io, camera, scene, window, renderer, display, camera_changed, prev_mouse);

        if (camera_changed) {
            frame_id = 0;
        }

        bool benchmark_done = false;
        if (benchmark_frames > 0 && frame_id + 1 == benchmark_frames) {
            save_image = true;
            benchmark_done = true;
        }

        const bool need_readback = save_image || !validation_img_prefix.empty() || frame_id == max_frames - 1;
        RenderStats stats = frame_id < max_frames ? renderer->render(camera.get_position(),
                                                                     camera.get_direction(),
                                                                     camera.get_up(),
                                                                     fov_y,
                                                                     camera_changed,
                                                                     need_readback)
                                                  : RenderStats();
        
        if (frame_id < max_frames)
            frame_id++;
        camera_changed = false;

        if (save_image) {
            save_image = false;
            std::cout << "Image saved to " << image_dir + image_output << "\n";
            stbi_write_png((image_dir + image_output).c_str(),
                           win_width,
                           win_height,
                           4,
                           renderer->img.data(),
                           4 * win_width);
        }
        if (!validation_img_prefix.empty()) {
            const std::string img_name = validation_img_prefix + render_plugin->get_name() +
                                         "-f" + std::to_string(frame_id) + ".png";
            stbi_write_png((image_dir + img_name).c_str(),
                           win_width,
                           win_height,
                           4,
                           renderer->img.data(),
                           4 * win_width);
        }

        if (frame_id == 1) {
            render_time = stats.render_time;
            rays_per_second = stats.rays_per_second;
        } else {
            render_time += stats.render_time;
            rays_per_second += stats.rays_per_second;
        }
        if (benchmark_done) {
            std::cout << "Benchmarked " << benchmark_frames << " frames\n"
                      << "Render Time: " << render_time / frame_id << "ms/frame ("
                      << 1000.f / (render_time / frame_id) << " FPS)\n";
            if (stats.rays_per_second > 0) {
                const std::string rays_per_sec =
                    pretty_print_count(rays_per_second / frame_id);
                std::cout << "Rays per-second " << rays_per_second / frame_id << " Ray/s ("
                          << rays_per_sec << "Ray/s)\n";
            }
            done = true;
        }

        display->new_frame();

        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImGui::Begin("Render Info");
        ImGui::Text("Render Time: %.3f ms/frame (%.1f FPS)",
                    render_time / frame_id,
                    1000.f / (render_time / frame_id));

        if (stats.rays_per_second > 0) {
            const std::string rays_per_sec = pretty_print_count(rays_per_second / frame_id);
            ImGui::Text("Rays per-second: %sRay/s", rays_per_sec.c_str());
        }

        ImGui::Text("Total Application Time: %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
        ImGui::Text("RT Backend: %s", rt_backend.c_str());
        ImGui::Text("CPU: %s", cpu_brand.c_str());
        ImGui::Text("GPU: %s", gpu_brand.c_str());
        ImGui::Text("Accumulated Frames: %llu", frame_id);
        ImGui::Text("Display Frontend: %s", display_frontend.c_str());
        ImGui::Text("%s", scene_info.c_str());

        // Let user pick image name
        std::strncpy(textBuf, image_output.c_str(), sizeof(textBuf));
        textBuf[sizeof(textBuf) - 1] = '\0';
        if (ImGui::InputText("Image name ending with '.png' ", textBuf, sizeof(textBuf))) {
            image_output = textBuf;
        }

        if (ImGui::Button("Save Image")) {
            save_image = true;
        }

        if (camParamsDropdown(scene.camParams))
        {
            renderer->update_scene(scene);
            fov_y = scene.camParams.cameraFOVAngle * 180.f / M_PI;
            camera_changed = true;
        }
            
        ImGui::End();
        ImGui::Render();

        display->display(renderer.get());
    }
}
