#pragma once

#include <string>
#include <glm/glm.hpp>

// Format the count as #G, #M, #K, depending on its magnitude
std::string pretty_print_count(const double count);

uint64_t align_to(uint64_t val, uint64_t align);

void ortho_basis(glm::vec3 &v_x, glm::vec3 &v_y, const glm::vec3 &n);

void canonicalize_path(std::string &path);

std::string get_cpu_brand();
