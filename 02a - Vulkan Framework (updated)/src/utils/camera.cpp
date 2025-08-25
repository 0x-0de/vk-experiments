#include "camera.h"

#include <cmath>

camera3d::camera3d(math::vec pos) : pos(pos)
{
    if(pos.size() != 3)
    {
        std::cerr << "[UTIL|ERR] Cannot assign a non-3D vector to a 3D camera's position." << std::endl;
        usable = false;
    }
    else usable = true;

    rot = math::vec3(0, 0, -1);

    rot_x = 0;
    rot_y = 0;
}

camera3d::~camera3d() {}

void camera3d::add_pos(math::vec val)
{
    pos += val;
}

void camera3d::add_rot(double x, double y, double sensitivity)
{
    rot_x -= (double) x * (0.001 * sensitivity);
    rot_y -= (double) y * (0.001 * sensitivity);

    if(rot_y < -(3.1415926 / 2)) rot_y = -(3.1415926 / 2);
    if(rot_y > (3.1415926 / 2)) rot_y = (3.1415926 / 2);
    
    rot[0] = std::sin(rot_x);
    rot[1] = std::tan(rot_y);
    rot[2] = std::cos(rot_x);

    rot = normalize(rot);
}

void camera3d::freemove(GLFWwindow* win, double speed)
{
    if(glfwGetKey(win, GLFW_KEY_W))
    {
        add_pos(rot * speed);
    }
    if(glfwGetKey(win, GLFW_KEY_S))
    {
        add_pos(-rot * speed);
    }

    if(glfwGetKey(win, GLFW_KEY_A))
    {
        add_pos(-math::normalize(math::cross(rot, math::vec3(0, 1, 0))) * speed);
    }
    if(glfwGetKey(win, GLFW_KEY_D))
    {
        add_pos(math::normalize(math::cross(rot, math::vec3(0, 1, 0))) * speed);
    }
}

math::vec camera3d::get_pos() const
{
    return pos;
}

void camera3d::set_pos(math::vec val)
{
    pos = val;

    if(pos.size() != 3)
    {
        std::cerr << "[UTIL|ERR] Cannot assign a non-3D vector to a 3D camera's position." << std::endl;
        usable = false;
    }
    else usable = true;
}

math::vec camera3d::get_rot() const
{
    return rot;
}

void camera3d::set_rot(math::vec val)
{
    rot = val;

    rot_x = std::asin(rot[0]);
    rot_y = std::atan(val[1]);

    if(rot.size() != 3)
    {
        std::cerr << "[UTIL|ERR] Cannot assign a non-3D vector to a 3D camera's rotation." << std::endl;
        usable = false;
    }
    else usable = true;
}

void camera3d::update_rot(GLFWwindow* win, double sensitivity, bool rotate)
{
    double cursor_x, cursor_y;
    glfwGetCursorPos(win, &cursor_x, &cursor_y);

    if(rotate) add_rot(cursor_x - prev_cursor_x, prev_cursor_y - cursor_y, sensitivity);

    prev_cursor_x = cursor_x;
    prev_cursor_y = cursor_y;
}

math::mat camera3d::view_matrix() const
{
    return usable ? math::look_at(pos, pos + rot, math::vec3(0, 1, 0)) : math::mat(4, 4);
}