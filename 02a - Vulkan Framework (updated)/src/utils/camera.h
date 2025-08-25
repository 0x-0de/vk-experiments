#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "linalg.h"

#include "GLFW/glfw3.h"

class camera3d
{
    public:
        camera3d(math::vec pos);
        ~camera3d();

        void add_pos(math::vec val);
        void add_rot(double x, double y, double sensitivity);

        void freemove(GLFWwindow* win, double speed);

        math::vec get_pos() const;
        math::vec get_rot() const;

        void set_pos(math::vec val);
        void set_rot(math::vec val);

        void update_rot(GLFWwindow* window, double sensitivity, bool rotate);

        math::mat view_matrix() const;
    private:
        math::vec pos, rot;
        bool usable;

        double rot_x, rot_y;
        double prev_cursor_x, prev_cursor_y;
};

#endif