#include <cstdio>
#include <array>
#include "utility.h"
#include "thread_pool.h"
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include "../extracted/combat.hpp"   // enemy Lorenz path + collision math

using namespace std;

const int MyResHorizontal = 1280;
const int MyResVertical = 720;
float movespeed = 100.0f;

struct Missile {
    glm::mat4 model_matrix;
    glm::mat4 quaternion_mat4;
    glm::mat4 inversemodel_matrix;
    glm::vec4 bl_rad_ifboom_ifactive_timesinceboom;
    glm::vec4 position_speed;
    glm::vec4 initialpos;
    glm::vec4 direction;
    glm::vec4 surfacenormal;
};

unsigned int missile_number = 256;
vector<Missile> mymissiles(missile_number);
unsigned char curmissile = 0;

struct Camera {
    glm::vec3 pos = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 upvec = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 gaze = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 quarternions = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 rightvec = glm::vec3(-1.0f, 0.0f, 0.0f);
    float screenwidth = 2560.0f;
    float screenlength = 1444.0f;
    float midx = screenwidth / 2.0f;
    float midy = screenlength / 2.0f;
    float x_temp_mouse_diff;
    float y_temp_mouse_diff;
    float last_x;
    float last_y;
    glm::quat myoldquat = glm::quat(1, 0, 0, 0);
    glm::quat mytempquat = glm::quat(1, 0, 0, 0);
    float sensitivity_qe = 0.2f;
    float sensitivity_mo_x = 1.0f;
    float sensitivity_mo_y = 1.0f;
    int height_multiplier;
    glm::quat myoldquatforplane = glm::quat(1, 0, 0, 0);
    glm::quat mytempquatforplane = glm::quat(1, 0, 0, 0);
    bool full_screen;
    int newx;
    int newy;
    float currentrotation;
    float water = 0.0f;
    float diff_x;
    float diff_y;
    float diff_z;
    glm::quat q_up;
    glm::quat q_down;
} mycam;

struct Plane {
    glm::vec3 position;
    glm::quat myoldquat = glm::quat(1, 0, 0, 0);
    int speed;
    glm::vec3 upvec;
    glm::vec3 gaze;
    float radius = 5.0f;
    float x_temp_mouse_diff;
    float y_temp_mouse_diff;
    float sensitivity_qe = 0.02f;
    float last_x;
    float last_y;
    float plane_gaze;
    float plane_up;
    float sensitivity_mo_x = 1.0f;
    float sensitivity_mo_y = 1.0f;
    unsigned int rot_angle = 0u;
    unsigned int rot_diff = 1u;
} myplane;

int find_inactive_missile(vector<Missile>& myvec) {
    int retval = (int)curmissile;
    curmissile++;
    return retval;
}

struct PointLiht {
    glm::vec3 position{ 1500.0f,1500.0f,200.0f };
} mylight;

struct Current_Inputs {
    bool left_pressed = false;
    bool right_pressed = false;
    bool q_pressed = false;
    bool e_pressed = false;
    bool w_pressed = false;
    bool a_pressed = false;
    bool s_pressed = false;
    bool d_pressed = false;
    bool j_pressed = false;
    bool k_pressed = false;
    bool LeftMouseclicked = false;
    bool wireframeMode = false;
    bool enter_pressed = false;
    bool RightMouseClicked = false;
    bool h_pressed = false;
    bool space_pressed = false;
    bool hdr_on = 1;
    int texture = 0;
} myinputs;

void WindowPositionCallback(GLFWwindow* wnd, int x, int y) {
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    state.curWndParams.pos[0] = x;
    state.curWndParams.pos[1] = y;
}

void WindowSizeCallback(GLFWwindow* wnd, int x, int y) {
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    state.curWndParams.size[0] = x;
    state.curWndParams.size[1] = y;
}

void MouseMoveCallback(GLFWwindow* wnd, double x, double y) {
    float nw_x = static_cast<float>(x);
    float nw_y = static_cast<float>(y);
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    if (!myinputs.LeftMouseclicked) {
        mycam.last_x = nw_x;
        mycam.last_y = nw_y;
    }
    else {
        float dx = (nw_x - mycam.last_x) / mycam.screenwidth;
        float dy = (nw_y - mycam.last_y) / mycam.screenlength;
        mycam.x_temp_mouse_diff = dx;
        mycam.y_temp_mouse_diff = dy;
        mycam.last_x = nw_x;
        mycam.last_y = nw_y;
    }
    if (!myinputs.RightMouseClicked) {
        myplane.last_x = nw_x;
        myplane.last_y = nw_y;
    }
    else {
        float dx = (-nw_x + myplane.last_x) / mycam.screenwidth;
        float dy = (nw_y - myplane.last_y) / mycam.screenlength;
        myplane.x_temp_mouse_diff = dx;
        myplane.y_temp_mouse_diff = dy;
        myplane.last_x = nw_x;
        myplane.last_y = nw_y;
    }
}

void MouseScrollCallback(GLFWwindow* wnd, double dx, double dy) {
    myplane.radius -= static_cast<float>(dy) * 10.0f;
    if (myplane.radius < 50.0f) {
        myplane.radius = 50.0f;
    }
    else if (myplane.radius > 250.0f) {
        myplane.radius = 250.0f;
    }
}

void MouseButtonCallback(GLFWwindow* wnd, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            myinputs.LeftMouseclicked = true;
        }
        else {
            myinputs.LeftMouseclicked = false;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            myinputs.RightMouseClicked = true;
        }
        else {
            myinputs.RightMouseClicked = false;
        }
    }
}

void FramebufferChangeCallback(GLFWwindow* wnd, int w, int h) {
    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    state.curWndParams.fbSize[0] = w;
    state.curWndParams.fbSize[1] = h;
}

void KeyboardCallback(GLFWwindow* wnd, int key, int scancode, int action, int modifier) {
    if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) {
            myinputs.w_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            myinputs.w_pressed = false;
            myplane.speed++;
            myplane.speed = std::min(50, myplane.speed);
        }
    }
    if (key == GLFW_KEY_H) {
        if (action == GLFW_RELEASE) {
            myinputs.h_pressed = !myinputs.h_pressed;
        }
    }
    if (key == GLFW_KEY_T) {
        if (action == GLFW_RELEASE) {
            if (myinputs.texture != 4) {
                myinputs.texture = 4;
            }
            else {
                myinputs.texture = 0;
            }
            myinputs.texture = 4;
        }
    }
    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) {
            myinputs.a_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            myinputs.a_pressed = false;
        }
    }
    if (key == GLFW_KEY_ENTER) {
        if (action == GLFW_PRESS) {
            myinputs.enter_pressed = true;
        }
    }
    if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS) {
            myinputs.s_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            myinputs.s_pressed = false;
            myplane.speed--;
            myplane.speed = std::max(0, myplane.speed);
        }
    }
    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            myinputs.d_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            myinputs.d_pressed = false;
        }
    }
    if (key == GLFW_KEY_Q) {
        if (action == GLFW_PRESS) {
            myinputs.q_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            myinputs.q_pressed = false;
        }
    }
    if (key == GLFW_KEY_E) {
        if (action == GLFW_PRESS) {
            myinputs.e_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            myinputs.e_pressed = false;
        }
    }
    if (key == GLFW_KEY_RIGHT) {
        if (action == GLFW_RELEASE) {
            myinputs.right_pressed = true;
        }
    }
    if (key == GLFW_KEY_LEFT) {
        if (action == GLFW_RELEASE) {
            myinputs.left_pressed = true;
        }
    }
    if (key == GLFW_KEY_L) {
        if (action == GLFW_RELEASE) {
            myinputs.wireframeMode = !myinputs.wireframeMode;
        }
    }
    if (key == GLFW_KEY_J) {
        if (action == GLFW_RELEASE) {
            myinputs.j_pressed = true;
        }
    }
    if (key == GLFW_KEY_K) {
        if (action == GLFW_RELEASE) {
            myinputs.k_pressed = true;
        }
    }
    if (key == GLFW_KEY_SPACE) {
        if (action == GLFW_RELEASE) {
            myinputs.space_pressed = true;
        }
    }

    GLState& state = *static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    uint32_t mode = state.mode;

    if (action != GLFW_RELEASE) return;
    if (key == GLFW_KEY_P) mode = (mode == 4) ? 0 : (mode + 1);
    if (key == GLFW_KEY_O) mode = (mode == 0) ? 2 : (mode - 1);

    state.mode = mode;
}

int main(int argc, const char* argv[]) {
    string path;
    char buffer[20];
    if (argc > 1) {
        path = argv[1];
    }
    else {
        path = "geo/n36_e029_1arc_v3.dt2";
    }

    GeoDataDTED mygeodata = GeoDataDTED(path);

    double latmin = mygeodata.latRange.x;
    double latmax = mygeodata.latRange.y;
    double lonmin = mygeodata.lonRange.x;
    double lonmax = mygeodata.lonRange.y;
    float minheight1 = mygeodata.minMax.x;
    float maxheight1 = mygeodata.minMax.y;
    int no_og_columns = mygeodata.dimensions.x;
    int no_of_rows = mygeodata.dimensions.y;

    std::cout << mygeodata(1800, 1800) << endl;

    int no_of_xdim_in_data = mygeodata.dimensions.x;
    int no_of_ydim_in_data = mygeodata.dimensions.y;

    int no_of_control_matrices_x_axis = (no_of_xdim_in_data - 3);
    int no_of_control_matrices_y_axis = (no_of_ydim_in_data - 3);

    float x_scale = (latmax - latmin) * 100000 / no_of_control_matrices_x_axis;
    float y_scale = (lonmax - lonmin) * 100000 / no_of_control_matrices_y_axis;

    vector<glm::mat4> ZxMatrices;
    vector<glm::mat4> ZyMatrices;
    glm::mat4 temp2;

    glm::vec4 myvec11 = { 0.0f, 1.0f, 2.0f, 3.0f };
    temp2 = { myvec11,myvec11,myvec11,myvec11 };
    ZxMatrices.push_back(glm::transpose(temp2));
    ZyMatrices.push_back(temp2);

    vector<glm::mat4> ZzMatrices(no_of_control_matrices_x_axis * no_of_control_matrices_y_axis);

    size_t THREAD = no_of_control_matrices_x_axis;
    ThreadPool tp({ .threadCount = std::thread::hardware_concurrency() });
    tp.SubmitDetachedBlocks(THREAD, [&](uint32_t start, uint32_t end) {
        for (int x = start; x < end; x++) {
            for (int y = 0; y < no_of_control_matrices_y_axis; y++) {
                glm::vec4 myvec1 = { mygeodata(x,y),mygeodata(x,y + 1),mygeodata(x,y + 2),mygeodata(x,y + 3) };
                glm::vec4 myvec2 = { mygeodata(x + 1, y),mygeodata(x + 1,y + 1),mygeodata(x + 1,y + 2),mygeodata(x + 1,y + 3) };
                glm::vec4 myvec3 = { mygeodata(x + 2, y),mygeodata(x + 2,y + 1),mygeodata(x + 2,y + 2),mygeodata(x + 2,y + 3) };
                glm::vec4 myvec4 = { mygeodata(x + 3, y),mygeodata(x + 3,y + 1),mygeodata(x + 3,y + 2),mygeodata(x + 3,y + 3) };
                glm::mat4 temp1 = { myvec1,myvec2,myvec3,myvec4 };
                ZzMatrices[y + x * no_of_control_matrices_y_axis] = (temp1 / x_scale);
            }
        }
        });
    tp.Wait();

    glm::mat4 B_splineMatrix;
    glm::mat4 B_splineMatrix_Transform;

    glm::vec4 myvec12 = { -1.0f / 6.0f,3.0f / 6.0f,-3.0f / 6.0f,1.0f / 6.0f };
    glm::vec4 myvec22 = { 1.0f / 2.0f,-1.0f,0.0f,4.0f / 6.0f };
    glm::vec4 myvec32 = { -3.0f / 6.0f,3.0f / 6.0f,3.0f / 6.0f,1.0f / 6.0f };
    glm::vec4 myvec42 = { 1.0f / 6.0f,0.0f,0.0f,0.0f };
    B_splineMatrix = { myvec12,myvec22,myvec32,myvec42 };
    B_splineMatrix_Transform = glm::transpose(B_splineMatrix);

    float current_time = 0;
    float last_time = 0;
    float diff = 0;
    float speed;
    glm::vec3 desired_normal;
    glm::vec3 current_normal;

    mycam.pos = { 1136.12f,26.7833f,1191.91f };
    myplane.position = { 1136.12f,26.7833f,1191.91f };
    mycam.pos = myplane.position - myplane.radius;
    mycam.height_multiplier = 1;
    mycam.full_screen = true;
    mycam.rightvec = glm::vec3(-1, 0, 0);
    mycam.upvec = glm::vec3(0, 1, 0);
    mycam.gaze = glm::vec3(0, 0, 1);

    myplane.speed = 0.0f;
    myplane.x_temp_mouse_diff = 0.0f;
    myplane.y_temp_mouse_diff = 0.0f;
    myplane.radius = 50.0f;
    myplane.gaze = glm::vec3(1, 0, 0);
    myplane.upvec = glm::vec3(0, 1, 0);

    mycam.diff_x = 0.0f;
    mycam.diff_y = 0.0f;
    mycam.diff_z = 1.0f;

    glm::quat worldquat;
    glm::quat quat_temp;
    glm::quat planequat;
    glm::quat planequat_temp;

    int tesellation_rate = 1;

    GLState state = GLState("Terrain Renderer", MyResHorizontal, MyResVertical, CallbackPointersGLFW());

    ThreadPool tp2({ .threadCount = std::thread::hardware_concurrency() });

    ShaderGL vShader = ShaderGL(ShaderGL::VERTEX, "shaders/generic.vert");
    ShaderGL fShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/debug.frag");

    ShaderGL vShaderPlane = ShaderGL(ShaderGL::VERTEX, "shaders/plane_body.vert");
    ShaderGL fShaderPlane = ShaderGL(ShaderGL::FRAGMENT, "shaders/plane_body.frag");

    ShaderGL vShaderSu57 = ShaderGL(ShaderGL::VERTEX, "shaders/su57.vert");
    ShaderGL fShaderSu57 = ShaderGL(ShaderGL::FRAGMENT, "shaders/su57.frag");

    ShaderGL vShaderMissile = ShaderGL(ShaderGL::VERTEX, "shaders/missile.vert");
    ShaderGL fShaderMissile = ShaderGL(ShaderGL::FRAGMENT, "shaders/missile.frag");

    MeshGL su57("meshes/su57_withiout_tyre.obj");
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    MeshGL missileFattah("meshes/Missile_FATEH_110.obj");
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    MeshGL sphereObjceet("meshes/sphere_2k.obj");
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    TextureGL tex_su57_body = TextureGL("textures/Body.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex_su57_intground = TextureGL("textures/internal_ground_ao_texture.jpeg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex_su57_metallic_r_G = TextureGL("textures/metallic-roughness@channels=G.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex_su57_metallic_r_B = TextureGL("textures/metallic-roughness@channels=B.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex_su57_normal = TextureGL("textures/normal.png", TextureGL::LINEAR, TextureGL::REPEAT);

    TextureGL tex_missile_normal = TextureGL("textures/Untitled_3_default_Normal.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex_missile_metallic = TextureGL("textures/Untitled_3_default_Metallic.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex_missile_BaseColor = TextureGL("textures/Untitled_3_default_BaseColor.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex_missile_roughness = TextureGL("textures/Untitled_3_default_Roughness.png", TextureGL::LINEAR, TextureGL::REPEAT);

    TextureGL tex = TextureGL("textures/mixed_brick_wall_diff_1k.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex5 = TextureGL("textures/grass_diff_2k.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex6 = TextureGL("textures/grass_rough_2k.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex7 = TextureGL("textures/rock_diff_2k.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex8 = TextureGL("textures/rock_rough_2k.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex9 = TextureGL("textures/shore_diff_2k.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex10 = TextureGL("textures/shore_rough_2k.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex11 = TextureGL("textures/snow_diff_2k.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex12 = TextureGL("textures/snow_rough_2k.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex13 = TextureGL("textures/church_stairway_2k.hdr", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex14 = TextureGL("textures/citrus_orchard_puresky_2k.hdr", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex15 = TextureGL("textures/qwantani_mid_morning_puresky_2k.hdr", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL tex27 = TextureGL("textures/qwantani_mid_morning_puresky_2k.hdr", TextureGL::LINEAR, TextureGL::REPEAT);

    std::cout << "tex27" << endl;
    std::cout << tex27.lightpos_int_tex.x << " ";
    std::cout << tex27.lightpos_int_tex.y << endl;
    std::cout << tex27.lightpos_3D.x << " " << tex27.lightpos_3D.y << " " << tex27.lightpos_3D.z << endl;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    GLuint perlingradient[2];
    GLuint ssbo_perlin;
    glGenBuffers(1, &ssbo_perlin);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_perlin);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(perlingradient), perlingradient, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_perlin);

    ShaderGL computeShaderCubemap = ShaderGL(ShaderGL::COMPUTE, "shaders/cubemap.comp");
    ShaderGL computeShaderMissile = ShaderGL(ShaderGL::COMPUTE, "shaders/missile.comp");

    glActiveTexture(GL_TEXTURE0);
    GLuint cumbemaptex;
    glGenTextures(1, &cumbemaptex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cumbemaptex);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA32F, 512, 512);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindImageTexture(0, cumbemaptex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex27.textureId);

    GLuint dispatch_buffer;
    glUseProgram(computeShaderCubemap.shaderId);
    glUniform3f(12, tex13.lightpos_3D.x, tex13.lightpos_3D.y, tex13.lightpos_3D.z);

    glGenBuffers(1, &dispatch_buffer);
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dispatch_buffer);
    static const struct {
        GLuint num_groups_x;
        GLuint num_groups_y;
        GLuint num_groups_z;
    } dispatch_params = { 512 / 16 , 512 / 16, 6 };
    glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(dispatch_params), &dispatch_params, GL_STATIC_DRAW);
    glDispatchComputeIndirect(0);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glUseProgram(0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GLuint quad_VertexArrayID;
    glGenVertexArrays(1, &quad_VertexArrayID);
    glBindVertexArray(quad_VertexArrayID);
    glEnableVertexAttribArray(6);

    static const GLfloat g_quad_vertex_buffer_data[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
    };
    GLuint quad_vertexbuffer;
    glGenBuffers(1, &quad_vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    ShaderGL quad_fragID = ShaderGL(ShaderGL::FRAGMENT, "shaders/simpletexture.frag");
    ShaderGL quad_vertID = ShaderGL(ShaderGL::VERTEX, "shaders/passthrough.vert");
    glBindVertexArray(0);

BURAYA:
    unsigned int myebo, vertexBuffer, normalBuffer, myVAO;
    glGenVertexArrays(1, &myVAO);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &myebo);
    glGenBuffers(1, &normalBuffer);

    unsigned int  myebo_water, myVAO_water, vertexBuffer_water;
    glGenVertexArrays(1, &myVAO_water);
    glGenBuffers(1, &vertexBuffer_water);
    glGenBuffers(1, &myebo_water);

    std::cout << "tesellation rate-" << tesellation_rate + 1 << endl;

    glm::vec4 S_Matrix;
    glm::vec4 T_Matrix;
    vector<glm::vec4> ST_Matrices;
    ST_Matrices.clear();
    vector<glm::vec4> ST_Matrices_derivatives;
    ST_Matrices_derivatives.clear();

    for (int l = 0; l <= tesellation_rate; l++) {
        float mytmpp = 1.0;
        mytmpp = (mytmpp / tesellation_rate) * l;
        S_Matrix = { mytmpp * mytmpp * mytmpp , mytmpp * mytmpp ,mytmpp ,1 };
        ST_Matrices.push_back(S_Matrix);
        S_Matrix = { 3 * mytmpp * mytmpp , 2 * mytmpp ,1 ,0 };
        ST_Matrices_derivatives.push_back(S_Matrix);
    }

    float QxTmp;
    vector<vector<float>> QxStore;
    QxStore.clear();
    vector<vector<float>> QxdtStore;
    QxdtStore.clear();
    vector<vector<float>> QxdsStore;
    QxdsStore.clear();
    vector<float> tmpvecc34;
    vector<float> tempvecc38;
    vector<float> tempvecc39;
    glm::mat4 yeterartik;

    for (int m = 0; m <= tesellation_rate; m++) {
        tmpvecc34.clear();
        tempvecc38.clear();
        tempvecc39.clear();
        for (int n = 0; n <= tesellation_rate; n++) {
            yeterartik = (B_splineMatrix * ZxMatrices[0] * B_splineMatrix_Transform);
            QxTmp = glm::dot(ST_Matrices[m], yeterartik * ST_Matrices[n]);
            tmpvecc34.push_back(QxTmp);
            QxTmp = glm::dot(ST_Matrices_derivatives[m], yeterartik * ST_Matrices[n]);
            tempvecc38.push_back(QxTmp);
            QxTmp = glm::dot(ST_Matrices[m], yeterartik * ST_Matrices_derivatives[n]);
            tempvecc39.push_back(QxTmp);
        }
        QxStore.push_back(tmpvecc34);
        QxdsStore.push_back(tempvecc38);
        QxdtStore.push_back(tempvecc39);
    }

    float QyTmp = glm::dot(S_Matrix, (B_splineMatrix * ZyMatrices[0] * B_splineMatrix_Transform * S_Matrix));
    vector<vector<float>> QyStore;
    QyStore.clear();
    vector<vector<float>> QydtStore;
    QydtStore.clear();
    vector<vector<float>> QydsStore;
    QydsStore.clear();
    glm::mat4 yeterartik1;
    vector<float> tmpvecc35;
    vector<float> tmpvecc36;
    vector<float> tmpvecc37;

    for (int m = 0; m <= tesellation_rate; m++) {
        tmpvecc35.clear();
        tmpvecc36.clear();
        tmpvecc37.clear();
        for (int n = 0; n <= tesellation_rate; n++) {
            yeterartik1 = (B_splineMatrix * ZyMatrices[0] * B_splineMatrix_Transform);
            QyTmp = glm::dot(ST_Matrices[m], yeterartik1 * ST_Matrices[n]);
            tmpvecc35.push_back(QyTmp);
            QyTmp = glm::dot(ST_Matrices_derivatives[m], yeterartik1 * ST_Matrices[n]);
            tmpvecc36.push_back(QyTmp);
            QyTmp = glm::dot(ST_Matrices[m], yeterartik1 * ST_Matrices_derivatives[n]);
            tmpvecc37.push_back(QyTmp);
        }
        QyStore.push_back(tmpvecc35);
        QydsStore.push_back(tmpvecc36);
        QydtStore.push_back(tmpvecc37);
    }

    vector<unsigned int> MyIndexBuffer(no_of_control_matrices_x_axis * no_of_control_matrices_y_axis * (tesellation_rate) * (tesellation_rate) * 6);
    int total_vertices = no_of_control_matrices_x_axis * no_of_control_matrices_y_axis * (tesellation_rate + 1) * (tesellation_rate + 1);
    vector<glm::vec3> MyVertexBuffer(total_vertices);
    vector<glm::vec3> MyNormalBuffer(total_vertices);
    float temps;
    float tempt;
    float derivative_s_x, derivative_s_y, derivative_s_z, derivative_t_x, derivative_t_y, derivative_t_z;
    int tesplus = (tesellation_rate + 1);

    tp2.SubmitDetachedBlocks(no_of_control_matrices_x_axis, [&](uint32_t start1, uint32_t end1) {
        for (uint32_t x = start1; x < end1; x++) {
            for (int y = 0; y < no_of_control_matrices_y_axis; y++) {
                glm::mat4 MyTempMatrix = B_splineMatrix * ZzMatrices[x * no_of_control_matrices_y_axis + y] * B_splineMatrix_Transform;
                for (int a = 0; a <= tesellation_rate; a++) {
                    for (int b = 0; b <= tesellation_rate; b++) {
                        int indexx1 = x * no_of_control_matrices_y_axis * tesplus * tesplus + y * tesplus * tesplus + a * tesplus + b;
                        float tmpforgivenxy_z = glm::dot(ST_Matrices[a], (MyTempMatrix * ST_Matrices[b]));
                        MyVertexBuffer[indexx1] = { QxStore[a][b] + x,tmpforgivenxy_z,QyStore[a][b] + y };
                        float QZds = glm::dot(ST_Matrices_derivatives[a], (MyTempMatrix * ST_Matrices[b]));
                        float QZdt = glm::dot(ST_Matrices[a], (MyTempMatrix * ST_Matrices_derivatives[b]));
                        glm::vec3 tr_maci_kacti = { QZdt * QydsStore[a][b] - QZds * QydtStore[a][b], QxdsStore[a][b] * QydtStore[a][b] - QxdtStore[a][b] * QydsStore[a][b],QZds * QxdtStore[a][b] - QZdt * QxdsStore[a][b] };
                        MyNormalBuffer[indexx1] = (-glm::normalize(tr_maci_kacti));
                    }
                }
                int current_vertex_pos = (tesellation_rate + 1) * (tesellation_rate + 1) * x * no_of_control_matrices_y_axis + (tesellation_rate + 1) * (tesellation_rate + 1) * y;
                int indexx12 = (x * no_of_control_matrices_y_axis * tesellation_rate * tesellation_rate + y * tesellation_rate * tesellation_rate) * 6;
                for (int a = 0; a < tesellation_rate; a++) {
                    for (int b = 0; b < tesellation_rate; b++) {
                        unsigned int top_left = current_vertex_pos + (a * tesplus) + b;
                        unsigned int top_right = current_vertex_pos + (a * tesplus) + (b + 1);
                        unsigned int bottom_left = current_vertex_pos + ((a + 1) * tesplus) + b;
                        unsigned int bottom_right = current_vertex_pos + ((a + 1) * tesplus) + (b + 1);
                        MyIndexBuffer[indexx12++] = (top_left);
                        MyIndexBuffer[indexx12++] = (bottom_left);
                        MyIndexBuffer[indexx12++] = (top_right);
                        MyIndexBuffer[indexx12++] = (top_right);
                        MyIndexBuffer[indexx12++] = (bottom_left);
                        MyIndexBuffer[indexx12++] = (bottom_right);
                    }
                }
            }
        }
        });
    tp2.Wait();

    vector<unsigned int> MyIndexBuffer_water(no_of_control_matrices_x_axis * no_of_control_matrices_y_axis * (tesellation_rate) * (tesellation_rate) * 6);
    vector<glm::vec3> MyVertexBuffer_water(total_vertices);

    tp2.SubmitDetachedBlocks(no_of_control_matrices_x_axis, [&](uint32_t start1, uint32_t end1) {
        for (uint32_t x = start1; x < end1; x++) {
            for (int y = 0; y < no_of_control_matrices_y_axis; y++) {
                glm::mat4 MyTempMatrix = B_splineMatrix * ZzMatrices[x * no_of_control_matrices_y_axis + y] * B_splineMatrix_Transform;
                for (int a = 0; a <= tesellation_rate; a++) {
                    for (int b = 0; b <= tesellation_rate; b++) {
                        int indexx1 = x * no_of_control_matrices_y_axis * tesplus * tesplus + y * tesplus * tesplus + a * tesplus + b;
                        MyVertexBuffer_water[indexx1] = { QxStore[a][b] + x,0,QyStore[a][b] + y };
                    }
                }
                int current_vertex_pos = (tesellation_rate + 1) * (tesellation_rate + 1) * x * no_of_control_matrices_y_axis + (tesellation_rate + 1) * (tesellation_rate + 1) * y;
                int indexx12 = (x * no_of_control_matrices_y_axis * tesellation_rate * tesellation_rate + y * tesellation_rate * tesellation_rate) * 6;
                for (int a = 0; a < tesellation_rate; a++) {
                    for (int b = 0; b < tesellation_rate; b++) {
                        unsigned int top_left = current_vertex_pos + (a * tesplus) + b;
                        unsigned int top_right = current_vertex_pos + (a * tesplus) + (b + 1);
                        unsigned int bottom_left = current_vertex_pos + ((a + 1) * tesplus) + b;
                        unsigned int bottom_right = current_vertex_pos + ((a + 1) * tesplus) + (b + 1);
                        MyIndexBuffer_water[indexx12++] = (top_left);
                        MyIndexBuffer_water[indexx12++] = (bottom_left);
                        MyIndexBuffer_water[indexx12++] = (top_right);
                        MyIndexBuffer_water[indexx12++] = (top_right);
                        MyIndexBuffer_water[indexx12++] = (bottom_left);
                        MyIndexBuffer_water[indexx12++] = (bottom_right);
                    }
                }
            }
        }
        });
    tp2.Wait();

    glBindVertexArray(myVAO_water);
    glEnableVertexAttribArray(8);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_water);
    glBufferData(GL_ARRAY_BUFFER, MyVertexBuffer_water.size() * sizeof(glm::vec3), MyVertexBuffer_water.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myebo_water);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MyIndexBuffer_water.size() * sizeof(unsigned int), MyIndexBuffer_water.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glBindVertexArray(myVAO);
    glEnableVertexAttribArray(4);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, MyVertexBuffer.size() * sizeof(glm::vec3), MyVertexBuffer.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MyIndexBuffer.size() * sizeof(unsigned int), MyIndexBuffer.data(), GL_STATIC_DRAW);

    glBindVertexArray(myVAO);
    glEnableVertexAttribArray(5);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, MyNormalBuffer.size() * sizeof(glm::vec3), MyNormalBuffer.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "Buraya geldik" << std::endl;
    std::cout << MyVertexBuffer.size() << std::endl;
    std::cout << no_of_control_matrices_x_axis << std::endl;
    std::cout << mygeodata.dimensions.x << " xdimension" << std::endl;
    std::cout << mygeodata.dimensions.y << " ydimension" << std::endl;
    std::cout << mygeodata.heightValues.size() << " height_dim" << std::endl;
    std::cout << ZzMatrices.size() << endl;
    std::cout << "Start Rendering";

    // The geometry above has already been uploaded to the GPU (lines just above).
    // Capture the index count, then release the multi-GB CPU mirrors: on the
    // integrated GPU these buffers live in the same RAM pool as the GPU copies,
    // so keeping both around exhausts memory. The data is rebuilt from scratch
    // whenever we jump back to BURAYA (tessellation change).
    GLsizei terrainIndexCount = static_cast<GLsizei>(MyIndexBuffer.size());

    auto releaseHostBuffer = [](auto& v)
    {
        v.clear();
        v.shrink_to_fit();
    };
    releaseHostBuffer(MyVertexBuffer);
    releaseHostBuffer(MyNormalBuffer);
    releaseHostBuffer(MyIndexBuffer);
    releaseHostBuffer(MyVertexBuffer_water);
    releaseHostBuffer(MyIndexBuffer_water);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    int windowwidth;
    int windowheight;

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    windowwidth = mode->width;
    windowheight = mode->height;

    mycam.midx = windowwidth / 2.0f;
    mycam.midy = windowheight / 2.0f;
    mycam.last_x = mycam.midx;
    mycam.last_y = mycam.midy;
    mycam.screenlength = windowheight * 3 / 4;
    mycam.screenwidth = windowwidth * 3 / 4;
    mycam.water = 0.0f;
    mycam.full_screen = !mycam.full_screen;
    myplane.last_x = mycam.midx;
    myplane.last_y = mycam.midy;

    GLuint FramebufferName = 0;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    GLuint renderedTexture;
    glGenTextures(1, &renderedTexture);
    glBindTexture(GL_TEXTURE_2D, renderedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowwidth, windowheight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLuint depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, windowwidth, windowheight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, renderedTexture, 0);

    GLenum DrawBuffers[2] = { GL_COLOR_ATTACHMENT0 ,GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, DrawBuffers);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;

    GLuint hdrSampler;
    glGenSamplers(1, &hdrSampler);
    glSamplerParameteri(hdrSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glBindSampler(1, hdrSampler);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderedTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    for (unsigned int i = 0; i < missile_number; i++) {
        mymissiles.at(i).bl_rad_ifboom_ifactive_timesinceboom.x = 5.0f;
        mymissiles.at(i).bl_rad_ifboom_ifactive_timesinceboom.y = 0.0f;
        mymissiles.at(i).bl_rad_ifboom_ifactive_timesinceboom.z = 0;
    }

    GLuint missileSSBO;
    glGenBuffers(1, &missileSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, missileSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, mymissiles.size() * sizeof(Missile), mymissiles.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, missileSSBO);

    // ============================================================
    //  Enemy plane (single, Lorenz attractor path) + crater params
    // ============================================================
    combat::EnemyPlane enemy;
    enemy.hitRadius      = 6.0f;                    // bounding sphere r = 6
    enemy.path.timeScale = 0.5f;                    // attractor speed
    enemy.path.scale     = glm::vec3(12.0f, 8.0f, 12.0f);
    enemy.path.translate = myplane.position + glm::vec3(0.0f, 50.0f, 0.0f);  // start in view, 50u above the player
    enemy.state          = glm::vec3(0.1f, 0.0f, 0.0f);

    float crater_depth = 8.0f;     // crater bowl depth fed to missile.comp
    float crater_rim   = 1.5f;     // raised lip height

    // Tiny SSBO (1 uint): missile.comp sets it to 1 the frame the enemy is hit.
    GLuint enemyHitSSBO;
    glGenBuffers(1, &enemyHitSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, enemyHitSSBO);
    { GLuint zero = 0; glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_COPY); }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, enemyHitSSBO);

    // CPU gate: only read enemyHitSSBO back while a missile can still be in flight.
    // Upper bound = max flight (~5000u / 450u/s ~ 11s) + 3s fuse.
    const float kMissileLifetime  = 15.0f;
    float       missileActiveUntil = -1.0f;
    bool        anyMissileActive   = false;  // gates the enemy-hit readback; cleared on expiry or hit

    glfwSetWindowMonitor(state.window, NULL, windowwidth / 8, windowheight / 8, 7 * (windowwidth / 8), 7 * (windowheight / 8), 0);
    myinputs.texture = 4;

    unsigned long frameCount = 0;   // debug: throttle the enemy position print
    while (!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();
        current_time = static_cast<float>(glfwGetTime());
        diff = current_time - last_time;
        last_time = current_time;
        speed = diff * movespeed;

        glm::quat rollquat(1, 0, 0, 0);
        glm::quat rollquat_plane(1, 0, 0, 0);
        glm::quat yawquat(1, 0, 0, 0);
        glm::quat yawquatplane(1, 0, 0, 0);
        glm::quat pitchquat(1, 0, 0, 0);
        glm::quat pitchquatplane(1, 0, 0, 0);
        glm::quat finalquat(1, 0, 0, 0);
        quat_temp = mycam.myoldquat;
        planequat_temp = myplane.myoldquat;

        if (myinputs.q_pressed && myinputs.e_pressed) {
        }
        else if (myinputs.e_pressed) {
            planequat = planequat_temp * glm::quat(0, 1, 0, 0) * glm::inverse(planequat_temp);
            planequat = glm::normalize(planequat);
            float sint_plane = sin(myplane.sensitivity_qe);
            float cost_plane = cos(myplane.sensitivity_qe);
            rollquat_plane = glm::quat(cost_plane, sint_plane * planequat.x, sint_plane * planequat.y, sint_plane * planequat.z);
        }
        else if (myinputs.q_pressed) {
            planequat = planequat_temp * glm::quat(0, 1, 0, 0) * glm::inverse(planequat_temp);
            planequat = glm::normalize(planequat);
            float sint_plane = sin(-myplane.sensitivity_qe);
            float cost_plane = cos(-myplane.sensitivity_qe);
            rollquat_plane = glm::quat(cost_plane, sint_plane * planequat.x, sint_plane * planequat.y, sint_plane * planequat.z);
        }
        if (myinputs.LeftMouseclicked) {
            float yawAngle = -mycam.x_temp_mouse_diff;
            float halfYaw = yawAngle * 0.5f;
            glm::quat yawQuat = glm::quat(cos(halfYaw), 0.0f, sin(halfYaw), 0.0f);
            mycam.gaze = glm::normalize(yawQuat * mycam.gaze);
            mycam.rightvec = glm::normalize(yawQuat * mycam.rightvec);
            mycam.upvec = glm::normalize(yawQuat * mycam.upvec);
            mycam.x_temp_mouse_diff = 0.0f;

            float pitchAngle = -mycam.y_temp_mouse_diff;
            float halfPitch = pitchAngle * 0.5f;
            float s = sin(halfPitch);
            float c = cos(halfPitch);
            glm::quat pitchQuat = glm::quat(c, mycam.rightvec.x * s, mycam.rightvec.y * s, mycam.rightvec.z * s);
            mycam.gaze = glm::normalize(pitchQuat * mycam.gaze);
            mycam.upvec = glm::normalize(pitchQuat * mycam.upvec);
            mycam.y_temp_mouse_diff = 0.0f;
            mycam.rightvec = glm::normalize(glm::cross(mycam.upvec, -mycam.gaze));
            mycam.upvec = glm::normalize(glm::cross(-mycam.gaze, mycam.rightvec));
        }
        else if (myinputs.RightMouseClicked) {
            planequat = planequat_temp * glm::quat(0, 0, 1, 0) * glm::inverse(planequat_temp);
            planequat = glm::normalize(planequat);
            float yawAngle = myplane.x_temp_mouse_diff * myplane.sensitivity_mo_x;
            float sint = sin(yawAngle / 2.0f);
            float cost = cos(yawAngle / 2.0f);
            yawquatplane = glm::quat(cost, sint * planequat.x, sint * planequat.y, sint * planequat.z);

            planequat = planequat_temp * glm::quat(0, 0, 0, 1) * glm::inverse(planequat_temp);
            planequat = glm::normalize(planequat);
            yawAngle = myplane.y_temp_mouse_diff * myplane.sensitivity_mo_y;
            sint = sin(-yawAngle / 2.0f);
            cost = cos(-yawAngle / 2.0f);
            pitchquatplane = glm::quat(cost, sint * planequat.x, sint * planequat.y, sint * planequat.z);
        }

        myplane.x_temp_mouse_diff = 0.0f;
        myplane.y_temp_mouse_diff = 0.0f;
        finalquat = rollquat_plane * yawquatplane * pitchquatplane;
        finalquat = glm::normalize(finalquat);
        myplane.myoldquat = normalize(finalquat * myplane.myoldquat);

        myplane.gaze = normalize(myplane.myoldquat * glm::vec3(1.0f, 0.0f, 0.0f));
        myplane.upvec = normalize(myplane.myoldquat * glm::vec3(0.0f, 1.0f, 0.0f));

        if (myinputs.w_pressed) {}
        if (myinputs.s_pressed) {}
        if (myinputs.d_pressed) {}
        if (myinputs.a_pressed) {}

        if (myinputs.left_pressed && mycam.height_multiplier > 1) {
            myinputs.left_pressed = false;
            mycam.height_multiplier = mycam.height_multiplier >> 1;
        }
        myinputs.left_pressed = false;
        if (myinputs.right_pressed && mycam.height_multiplier < 129) {
            myinputs.right_pressed = false;
            mycam.height_multiplier = mycam.height_multiplier << 1;
        }
        myinputs.right_pressed = false;

        if (myinputs.j_pressed && tesellation_rate > 1) {
            tesellation_rate--;
            myinputs.j_pressed = false;
            glDeleteVertexArrays(1, &myVAO);
            glDeleteBuffers(1, &vertexBuffer);
            glDeleteBuffers(1, &normalBuffer);
            glDeleteBuffers(1, &myebo);
            glDeleteBuffers(1, &myebo_water);
            glDeleteVertexArrays(1, &myVAO_water);
            glDeleteBuffers(1, &vertexBuffer_water);
            goto BURAYA;
        }
        else if (myinputs.k_pressed && tesellation_rate < 3) {
            tesellation_rate++;
            myinputs.k_pressed = false;
            glDeleteVertexArrays(1, &myVAO);
            glDeleteBuffers(1, &vertexBuffer);
            glDeleteBuffers(1, &normalBuffer);
            glDeleteBuffers(1, &myebo);
            glDeleteBuffers(1, &myebo_water);
            glDeleteVertexArrays(1, &myVAO_water);
            glDeleteBuffers(1, &vertexBuffer_water);
            goto BURAYA;
        }

        if (myinputs.enter_pressed) {
            myinputs.enter_pressed = false;
            mycam.full_screen = !mycam.full_screen;
            if (!mycam.full_screen) {
                mycam.screenlength = windowheight * 3 / 4;
                mycam.screenwidth = windowwidth * 3 / 4;
                glfwSetWindowMonitor(state.window, NULL, windowwidth / 8, windowheight / 8, 7 * (windowwidth / 8), 7 * (windowheight / 8), 0);
            }
            else {
                mycam.screenlength = windowheight;
                mycam.screenwidth = windowwidth;
                glfwSetWindowMonitor(state.window, NULL, 0, 0, windowwidth, windowheight, 0);
            }
        }

        float aspectRatio = float(state.curWndParams.fbSize[0]) / float(state.curWndParams.fbSize[1]);

        if (myplane.speed > 0) {
            myplane.position = myplane.position + myplane.gaze * (float)myplane.speed * (diff * 8.0f);
        }
        mycam.pos = myplane.position - mycam.gaze * myplane.radius;

        float mynear = 5.0f;
        float myfar = 5000.0f;
        float angle = 60.0f;
        glm::mat4x4 proj = glm::perspective(glm::radians(angle), aspectRatio, mynear, myfar);

        state.cam.pos = mycam.pos;
        state.cam.gaze = state.cam.pos + mycam.gaze;
        state.cam.up = mycam.upvec;

        glm::mat4x4 view = glm::lookAt(state.cam.pos, state.cam.gaze, state.cam.up);

        std::cout << "\rCamera Pos - X: " << myplane.position.x
            << " Y: " << myplane.position.y
            << " Z: " << myplane.position.z << std::flush;

        glViewport(0, 0, state.curWndParams.fbSize[0], state.curWndParams.fbSize[1]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);

        static constexpr GLuint U_TRANSFORM_MODEL = 0;
        static constexpr GLuint U_TRANSFORM_VIEW = 1;
        static constexpr GLuint U_TRANSFORM_PROJ = 2;
        static constexpr GLuint U_TRANSFORM_NORMAL = 3;
        static constexpr GLuint U_LIGHT_POS = 4;
        static constexpr GLuint U_ANGLE = 32;
        static constexpr GLuint U_ANGLE1 = 77;
        static constexpr GLuint U_TRANSFORM_MODEL_UNSCALED = 33;
        static constexpr GLuint U_NORMAL_UNSCALED = 34;
        static constexpr GLuint T_ALBEDO = 0;
        static constexpr GLuint U_MODE = 0;
        static constexpr GLuint U_PMODE = 91;
        static constexpr GLuint HDR = 24;

        static float maxheight = maxheight1 / x_scale;
        static float minheight = minheight1 / x_scale;

        mycam.water += 0.025f;
        myplane.rot_angle += myplane.rot_diff;
        if (myplane.rot_angle == 36000000) {
            myplane.rot_angle = 0;
            mycam.water = 0.0f;
        }
        mycam.currentrotation = float(myplane.rot_angle);

        glm::vec3 point123 = mylight.position + mycam.pos;

        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
        {
            glm::mat4x4 model = glm::identity<glm::mat4x4>();
            model = glm::identity<glm::mat4x4>();
            glm::mat4x4 unscaled_model = model;
            model = glm::scale(model, glm::vec3(1.0f, mycam.height_multiplier * 1.0f, 1.0f));

            glm::mat3x3 unscalednormalMatrix = glm::inverseTranspose(unscaled_model);
            glm::mat3x3 normalMatrix = glm::inverseTranspose(model);

            glUniformMatrix4fv(U_TRANSFORM_MODEL_UNSCALED, 1, false, glm::value_ptr(unscaled_model));
            glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
            glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
            glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
            glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(normalMatrix));
            glUniformMatrix3fv(U_NORMAL_UNSCALED, 1, false, glm::value_ptr(unscalednormalMatrix));
            glUniform1i(99, 0);
            glUniform1f(U_ANGLE, mycam.water);
            glUniform1f(U_ANGLE1, mycam.water / 2.0f);
            glUniform3f(U_LIGHT_POS, point123.x, point123.y, point123.z);
            glUniform3fv(69, 1, glm::value_ptr(mycam.pos));
        }

        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
        {
            glUniform1i(99, 0);
            static_assert(GL_TEXTURE0 + 1 == GL_TEXTURE1, "OGL API is wrong!");
            static_assert(GL_TEXTURE0 + 2 == GL_TEXTURE2, "OGL API is wrong!");
            static_assert(GL_TEXTURE0 + 16 == GL_TEXTURE16, "OGL API is wrong!");

            glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
            glBindTexture(GL_TEXTURE_2D, tex.textureId);
            glActiveTexture(GL_TEXTURE0 + 5);
            glBindTexture(GL_TEXTURE_2D, tex5.textureId);
            glActiveTexture(GL_TEXTURE0 + 6);
            glBindTexture(GL_TEXTURE_2D, tex6.textureId);
            glActiveTexture(GL_TEXTURE0 + 7);
            glBindTexture(GL_TEXTURE_2D, tex7.textureId);
            glActiveTexture(GL_TEXTURE0 + 8);
            glBindTexture(GL_TEXTURE_2D, tex8.textureId);
            glActiveTexture(GL_TEXTURE0 + 9);
            glBindTexture(GL_TEXTURE_2D, tex9.textureId);
            glActiveTexture(GL_TEXTURE0 + 10);
            glBindTexture(GL_TEXTURE_2D, tex10.textureId);
            glActiveTexture(GL_TEXTURE0 + 11);
            glBindTexture(GL_TEXTURE_2D, tex11.textureId);
            glActiveTexture(GL_TEXTURE0 + 12);
            glBindTexture(GL_TEXTURE_2D, tex12.textureId);

            glUniform1ui(U_MODE, state.mode);

            if (myinputs.h_pressed && !myinputs.hdr_on) {
                myinputs.hdr_on = true;
                myinputs.h_pressed = false;
            }
            else if (myinputs.h_pressed && myinputs.hdr_on) {
                myinputs.hdr_on = false;
                myinputs.h_pressed = false;
            }

            if (myinputs.hdr_on) {
                glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
                glUniform1ui(HDR, 1);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glViewport(0, 0, windowwidth, windowheight);
            }
            else {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glUniform1ui(HDR, 0);
            }

            int maxH_location = glGetUniformLocation(fShader.shaderId, "umaxheight");
            int minH_location = glGetUniformLocation(fShader.shaderId, "uminheight");
            glUniform1f(maxH_location, static_cast<float>(maxheight));
            glUniform1f(minH_location, static_cast<float>(minheight));
        }

        glBindVertexArray(myVAO);

        if (myinputs.wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);

        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
        {
            glUniform1ui(73, myinputs.texture);
            glUniform1i(99, 1);
            glm::mat4x4 model = glm::identity<glm::mat4x4>();
            glm::mat4x4 unscaled_model = model;
            model = glm::scale(model, glm::vec3(1.0f, mycam.height_multiplier * 1.0f, 1.0f));

            glm::mat3x3 unscalednormalMatrix = glm::inverseTranspose(unscaled_model);
            glm::mat3x3 normalMatrix = glm::inverseTranspose(model);

            glUniformMatrix4fv(U_TRANSFORM_MODEL_UNSCALED, 1, false, glm::value_ptr(unscaled_model));
            glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
            glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
            glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
            glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(normalMatrix));
            glUniformMatrix3fv(U_NORMAL_UNSCALED, 1, false, glm::value_ptr(unscalednormalMatrix));

            glUniform1f(U_ANGLE, mycam.water);
            glUniform1f(U_ANGLE1, mycam.water / 2.0f);
            glUniform3f(U_LIGHT_POS, point123.x, point123.y, point123.z);
            glUniform3fv(69, 1, glm::value_ptr(mycam.pos));
        }

        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
        {
            glUniform1ui(73, myinputs.texture);
            glUniform1i(99, 1);
            glUniform1ui(U_MODE, state.mode);

            if (myinputs.h_pressed && !myinputs.hdr_on) {
                myinputs.hdr_on = true;
                myinputs.h_pressed = false;
            }
            else if (myinputs.h_pressed && myinputs.hdr_on) {
                myinputs.hdr_on = false;
                myinputs.h_pressed = false;
            }

            if (myinputs.hdr_on) {
                glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
                glUniform1ui(HDR, 1);
                glViewport(0, 0, windowwidth, windowheight);
            }
            else {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glUniform1ui(HDR, 0);
            }

            glActiveTexture(GL_TEXTURE0 + 13);
            glBindTexture(GL_TEXTURE_2D, tex13.textureId);
            glActiveTexture(GL_TEXTURE0 + 14);
            glBindTexture(GL_TEXTURE_2D, tex14.textureId);
            glActiveTexture(GL_TEXTURE0 + 15);
            glBindTexture(GL_TEXTURE_2D, tex15.textureId);

            glActiveTexture(GL_TEXTURE0 + 27);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cumbemaptex);
        }

        glBindVertexArray(myVAO_water);

        if (myinputs.wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glBindVertexArray(myVAO);

        int update_this_pos = -1;
        if (myinputs.space_pressed) {
            myinputs.space_pressed = false;
            update_this_pos = find_inactive_missile(mymissiles);
            missileActiveUntil = current_time + kMissileLifetime;
            anyMissileActive   = true;
        }

        // --- Advance the enemy along its Lorenz path ---
        // Clamp the step: the first frame's diff spans the whole terrain-load
        // time (last_time starts at 0), and a large step makes the RK4 Lorenz
        // integrator overflow to inf -> NaN, which then sticks every frame.
        combat::updateEnemy(enemy, glm::min(diff, 0.05f));

        // Debug: confirm the enemy is alive and moving (every 60 frames).
        if (frameCount % 60 == 0) {
            std::cout << "\n[enemy] frame " << frameCount
                      << " pos = (" << enemy.position.x << ", "
                      << enemy.position.y << ", " << enemy.position.z << ")"
                      << " alive = " << (enemy.alive ? 1 : 0) << std::endl;
        }
        frameCount++;

        // Reset the per-frame enemy-hit flag before dispatch.
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, enemyHitSSBO);
        { GLuint zero = 0; glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero); }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, missileSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, vertexBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, normalBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, enemyHitSSBO);

        GLuint dispatch_buffer_3;
        glUseProgram(computeShaderMissile.shaderId);

        glUniform1f(12, current_time);
        glUniform1i(19, update_this_pos);
        glUniform3fv(44, 1, glm::value_ptr(myplane.position));
        glUniform3fv(45, 1, glm::value_ptr(myplane.gaze));

        glm::quat rotatemissile(cos(glm::half_pi<float>() / 2.0f), myplane.upvec * sin(glm::half_pi<float>() / 2.0f));
        glUniformMatrix4fv(53, 1, false, glm::value_ptr(glm::mat4_cast(rotatemissile * myplane.myoldquat)));
        glUniform1f(56, myplane.speed);

        glm::mat4x4 model = glm::identity<glm::mat4x4>();
        model = glm::identity<glm::mat4x4>();
        glm::mat4x4 unscaled_model = model;
        model = glm::scale(model, glm::vec3(1.0f, mycam.height_multiplier * 1.0f, 1.0f));
        glm::mat3x3 normalMatrix = glm::inverseTranspose(model);
        glUniformMatrix3fv(51, 1, false, glm::value_ptr(normalMatrix));
        glUniform1i(52, mycam.height_multiplier);

        float blast_radius = 5.0f;
        glUniform1f(46, blast_radius);
        float missile_speed = 50.0f;
        glUniform1f(50, missile_speed);
        glUniform1i(47, tesplus);
        glUniform1i(48, no_of_control_matrices_x_axis);
        glUniform1i(49, no_of_control_matrices_y_axis);
        glUniform3fv(54, 1, glm::value_ptr(myplane.upvec));
        glUniform1f(7, diff);
        glUniformMatrix4fv(8, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(9, 1, GL_FALSE, glm::value_ptr(proj));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glUniform1i(10, 0);

        // Crater + enemy-collision uniforms.
        glUniform1f(57, crater_depth);
        glUniform1f(58, crater_rim);
        glUniform3fv(60, 1, glm::value_ptr(enemy.position));
        glUniform1f(61, enemy.hitRadius);
        glUniform1i(62, enemy.alive ? 1 : 0);

        glDispatchCompute(16, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        glUseProgram(0);

        // No CPU-visible missile "active" flag exists (it's only set GPU-side),
        // so a missile is considered inactive once its lifetime window elapses.
        if (anyMissileActive && current_time >= missileActiveUntil)
            anyMissileActive = false;

        // Read back the enemy-hit flag only while a missile is in flight (gates the
        // VIDEO->HOST copy that otherwise ran every frame).
        if (anyMissileActive && enemy.alive) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, enemyHitSSBO);
            GLuint enemyHitFlag = 0;
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &enemyHitFlag);
            if (enemyHitFlag != 0) {
                enemy.alive = false;        // stop drawing it
                anyMissileActive = false;   // the missile is gone; stop polling immediately
                // Reset the SSBO flag immediately so it can't re-trigger next frame.
                GLuint zero = 0;
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);
                // The striking missile is already flagged exploded in the SSBO,
                // so its 3s BOOM state plays at the contact point. A sprite-sheet
                // billboard (combat::sampleSprite) would be spawned here once an
                // explosion render pass exists.
            }
        }

        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderMissile.shaderId);

        glBindVertexArray(missileFattah.vaoId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, missileSSBO);

        glActiveShaderProgram(state.renderPipeline, vShaderMissile.shaderId);
        {
            glUniform3fv(69, 1, glm::value_ptr(mycam.pos));
            glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
            glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
        }

        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderMissile.shaderId);
        glActiveShaderProgram(state.renderPipeline, fShaderMissile.shaderId);
        {
            glUniform1ui(HDR, myinputs.hdr_on ? 1 : 0);

            glActiveTexture(GL_TEXTURE0 + 22);
            glBindTexture(GL_TEXTURE_2D, tex_missile_BaseColor.textureId);

            glActiveTexture(GL_TEXTURE0 + 23);
            glBindTexture(GL_TEXTURE_2D, tex_missile_metallic.textureId);

            glActiveTexture(GL_TEXTURE0 + 24);
            glBindTexture(GL_TEXTURE_2D, tex_missile_normal.textureId);

            glActiveTexture(GL_TEXTURE0 + 25);
            glBindTexture(GL_TEXTURE_2D, tex_missile_roughness.textureId);

            glActiveTexture(GL_TEXTURE0 + 27);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cumbemaptex);
        }

        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(missileFattah.indexCount), GL_UNSIGNED_INT, nullptr, missile_number);

        glBindVertexArray(0);

        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderSu57.shaderId);

        glm::mat4 planerotate = glm::mat4_cast(myplane.myoldquat);
        glm::mat4x4 su57_model = glm::identity<glm::mat4x4>();
        su57_model = glm::translate(su57_model, myplane.position);
        su57_model = su57_model * planerotate;
        su57_model = glm::scale(su57_model, glm::vec3(0.3f, 0.3f, 0.3f));

        glm::mat3x3 su57_NormalMatrix = glm::inverseTranspose(su57_model);

        glBindVertexArray(su57.vaoId);

        glActiveShaderProgram(state.renderPipeline, vShaderSu57.shaderId);
        {
            glUniform3fv(69, 1, glm::value_ptr(mycam.pos));
            glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(su57_model));
            glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(su57_NormalMatrix));
            glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
            glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
        }

        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderSu57.shaderId);
        glActiveShaderProgram(state.renderPipeline, fShaderSu57.shaderId);
        {
            glUniform1ui(HDR, myinputs.hdr_on ? 1 : 0);

            glActiveTexture(GL_TEXTURE0 + 18);
            glBindTexture(GL_TEXTURE_2D, tex_su57_body.textureId);

            glActiveTexture(GL_TEXTURE0 + 19);
            glBindTexture(GL_TEXTURE_2D, tex_su57_metallic_r_G.textureId);

            glActiveTexture(GL_TEXTURE0 + 20);
            glBindTexture(GL_TEXTURE_2D, tex_su57_metallic_r_B.textureId);

            glActiveTexture(GL_TEXTURE0 + 21);
            glBindTexture(GL_TEXTURE_2D, tex_su57_normal.textureId);

            glActiveTexture(GL_TEXTURE0 + 27);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cumbemaptex);
        }

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(su57.indexCount), GL_UNSIGNED_INT, nullptr);

        // ============================================================
        //  Enemy plane (reuses the player's su57 model + shaders) +
        //  its debug wireframe bounding sphere.
        // ============================================================
        if (enemy.alive) {
            glm::mat4 enemy_model = glm::translate(glm::mat4(1.0f), enemy.position)
                                  * glm::mat4_cast(enemy.orientation)
                                  * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f));
            glm::mat3 enemy_NormalMatrix = glm::inverseTranspose(enemy_model);

            glBindVertexArray(su57.vaoId);
            glActiveShaderProgram(state.renderPipeline, vShaderSu57.shaderId);
            {
                glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(enemy_model));
                glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(enemy_NormalMatrix));
            }
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(su57.indexCount), GL_UNSIGNED_INT, nullptr);

            // Debug wireframe bounding sphere (radius == enemy.hitRadius == 6).
            glm::mat4 sphere_model = glm::translate(glm::mat4(1.0f), enemy.position)
                                   * glm::scale(glm::mat4(1.0f), glm::vec3(enemy.hitRadius));
            glm::mat3 sphere_NormalMatrix = glm::inverseTranspose(sphere_model);

            glBindVertexArray(sphereObjceet.vaoId);
            glActiveShaderProgram(state.renderPipeline, vShaderSu57.shaderId);
            {
                glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(sphere_model));
                glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(sphere_NormalMatrix));
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphereObjceet.indexCount), GL_UNSIGNED_INT, nullptr);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glDepthFunc(GL_LEQUAL);

        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderPlane.shaderId);
        glBindVertexArray(quad_VertexArrayID);

        glActiveShaderProgram(state.renderPipeline, vShaderPlane.shaderId); {
            glUniform3fv(69, 1, glm::value_ptr(mycam.pos));

            if (myinputs.hdr_on) {
                glUniform1ui(HDR, 1);
            }
            else {
                glUniform1ui(HDR, 0);
            }

            glUniform1ui(U_PMODE, 5);
            glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
            glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
        }

        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderPlane.shaderId);
        glActiveShaderProgram(state.renderPipeline, fShaderPlane.shaderId); {
            if (myinputs.hdr_on) {
                glUniform1ui(HDR, 1);
            }
            else {
                glUniform1ui(HDR, 0);
            }
            glm::mat4x4 angleview = glm::lookAt(glm::vec3(0, 0, 0), mycam.gaze, mycam.upvec);
            glm::mat4x4 inverse_proj = glm::inverse(proj * angleview);

            glUniformMatrix4fv(61, 1, false, glm::value_ptr(inverse_proj));
            glUniform1ui(55, mycam.screenwidth);
            glUniform1ui(54, mycam.screenlength);
            glUniform1ui(56, myfar);
            glUniform3f(63, mycam.upvec.x, mycam.upvec.y, mycam.upvec.z);
            glUniform3f(64, mycam.gaze.x, mycam.gaze.y, mycam.gaze.z);

            glUniform1ui(73, myinputs.texture);
            glUniform1ui(U_PMODE, 5);
            glActiveTexture(GL_TEXTURE0 + 13);
            glBindTexture(GL_TEXTURE_2D, tex13.textureId);
            glActiveTexture(GL_TEXTURE0 + 14);
            glBindTexture(GL_TEXTURE_2D, tex14.textureId);
            glActiveTexture(GL_TEXTURE0 + 15);
            glBindTexture(GL_TEXTURE_2D, tex15.textureId);
            glActiveTexture(GL_TEXTURE0 + 27);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cumbemaptex);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDepthFunc(GL_LESS);

        if (myinputs.hdr_on) {
            (1, hdrSampler);
            glGenerateMipmap(GL_TEXTURE_2D);

            glDisable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, quad_vertID.shaderId);
            glBindVertexArray(quad_VertexArrayID);

            glActiveShaderProgram(state.renderPipeline, quad_vertID.shaderId); {
                glUniform1i(83, hdrSampler);
            }

            glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, quad_fragID.shaderId);
            glActiveShaderProgram(state.renderPipeline, quad_fragID.shaderId); {
                glActiveTexture(GL_TEXTURE0 + 2);
                glBindTexture(GL_TEXTURE_2D, renderedTexture);
            }

            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);
        }

        glUseProgramStages(state.renderPipeline, GL_GEOMETRY_SHADER_BIT, 0);

        glfwSwapBuffers(state.window);
    }
}