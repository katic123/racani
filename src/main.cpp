// clang-format off
#include <glm/detail/qualifier.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <iostream>
#include "../glad/include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// clang-format on
using namespace std;

class Mesh {
public:
  vector<glm::vec3> vertices;
  vector<glm::ivec3> faces;

  int loadMesh(const std::string &filepath) {
    cout << "Loading file: " << filepath << endl;
    ifstream file(filepath);
    if (!file.is_open()) {
      cout << "Failed to open file: " << filepath << endl;
      return 1;
    }

    string line;
    while (getline(file, line)) {
      istringstream ss(line);
      string prefix;
      ss >> prefix;

      if (prefix == "v") {
        float x, y, z;
        if (ss >> x >> y >> z) {
          this->vertices.push_back(glm::vec3(x, y, z));
        }
      } else if (prefix == "f") {
        string v1, v2, v3;
        if (ss >> v1 >> v2 >> v3) {
          int idx1 = stoi(v1.substr(0, v1.find('/')));
          int idx2 = stoi(v2.substr(0, v2.find('/')));
          int idx3 = stoi(v3.substr(0, v3.find('/')));
          this->faces.push_back(glm::ivec3(idx1 - 1, idx2 - 1, idx3 - 1));
        }
      }
    }
    cout << "Loaded " << vertices.size() << " vertices, " << faces.size()
         << " faces" << endl;
    return 0;
  }
  virtual void moveMesh(glm::vec3 m) {
    for (int i = 0; i < vertices.size(); i++) {
      vertices[i] += m;
    }
  }
  virtual void scaleMesh(float s) {
    for (int i = 0; i < vertices.size(); i++) {
      vertices[i].x *= s;
      vertices[i].y *= s;
      vertices[i].z *= s;
    }
  }
};
class Sphere : public Mesh {
public:
  float radius = 1;
  float mass = 1;
  glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 vel;
  Sphere() {
    vel = glm::vec3(0.0f, 0.0f, 0.0f);
    this->loadMesh("../src/objects/sphere.obj");
  }
  Sphere(glm::vec3 velocity, glm::vec3 cen, float r, float mass) {
    this->mass = mass;
    vel = velocity;
    this->loadMesh("../src/objects/sphere.obj");
    this->scaleMesh(r);
    this->moveMesh(cen);
  }
  void scaleMesh(float s) override {
    for (int i = 0; i < vertices.size(); i++) {
      vertices[i].x *= s;
      vertices[i].y *= s;
      vertices[i].z *= s;
    }
    radius *= s;
  }
  virtual void moveMesh(glm::vec3 m) override {
    for (int i = 0; i < vertices.size(); i++) {
      vertices[i] += m;
    }
    center += m;
  }
};
class Camera {
public:
  glm::vec3 eye;
  glm::vec3 looking_at;
  glm::vec3 up;
  glm::vec3 front;
  glm::vec3 right;

  float yaw = -90.0f;
  float pitch = 0.0f;
  Camera(glm::vec3 eye, glm::vec3 looking_at, glm::vec3 up) {
    this->eye = eye;
    this->looking_at = looking_at;
    this->up = glm::normalize(up);

    front = glm::normalize(looking_at - eye);

    right = glm::normalize(glm::cross(front, this->up));
  }

  void moveCameraInDirection(float frontspeed, float rightspeed, float upspeed,
                             float deltaTime) {
    eye += 100.0f * deltaTime * frontspeed *
           normalize(glm::vec3(front.x, 0, front.z));
    eye += 100.0f * deltaTime * rightspeed *
           normalize(glm::vec3(right.x, 0, right.z));
    eye += 100.0f * deltaTime * upspeed * up;
    looking_at = eye + front;
  }

  void rotate(float yawDelta, float pitchDelta) {
    yaw += yawDelta;
    pitch += pitchDelta;

    if (pitch > 89.0f)
      pitch = 89.0f;
    if (pitch < -89.0f)
      pitch = -89.0f;

    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, up));
    looking_at = eye + front;
  }
};

class Scene {
public:
  vector<Mesh> staticMeshes;
  vector<Sphere> movingMeshes;

  vector<float> sceneVertices;
  vector<unsigned int> sceneFaces;
  Camera *camera;
  Scene(Camera *camera) { this->camera = camera; }
  void addToScene(Mesh &mesh) {
    unsigned int offset = sceneVertices.size() / 6; // 6 = 3 pozicije + 3 boje

    for (int j = 0; j < mesh.vertices.size(); j++) {

      sceneVertices.push_back(mesh.vertices[j].x);
      sceneVertices.push_back(mesh.vertices[j].y);
      sceneVertices.push_back(mesh.vertices[j].z);

      sceneVertices.push_back(1.0f);
      sceneVertices.push_back(1.0f);
      sceneVertices.push_back(1.0f);
    }

    for (int j = 0; j < mesh.faces.size(); j++) {
      sceneFaces.push_back(offset + mesh.faces[j].x);
      sceneFaces.push_back(offset + mesh.faces[j].y);
      sceneFaces.push_back(offset + mesh.faces[j].z);
    }
  }
  void addToMovingScene(Sphere &sphere) {
    movingMeshes.push_back(sphere);
    this->addToScene(sphere);
  }
  void addToStaticScene(Mesh &mesh) {
    staticMeshes.push_back(mesh);
    this->addToScene(mesh);
  }
  void clearVerticiesAndFaces() {
    sceneVertices.clear();
    sceneFaces.clear();
  }
  void clearMovingMeshes() { movingMeshes.clear(); }

  unsigned int offset = 0;
  template <typename T> void updateVerticiesAndFaces(vector<T> &meshes) {

    for (int i = 0; i < meshes.size(); i++) {
      for (int j = 0; j < meshes[i].vertices.size(); j++) {
        // x
        sceneVertices.push_back(meshes[i].vertices[j].x);
        // y
        sceneVertices.push_back(meshes[i].vertices[j].y);
        // z
        sceneVertices.push_back(meshes[i].vertices[j].z);

        // r
        sceneVertices.push_back(1.0f);
        // g
        sceneVertices.push_back(1.0f);
        // b
        sceneVertices.push_back(1.0f);
      }
      for (int j = 0; j < meshes[i].faces.size(); j++) {
        // f_v1
        sceneFaces.push_back(offset + meshes[i].faces[j].x);
        // f_v2
        sceneFaces.push_back(offset + meshes[i].faces[j].y);
        // f_v3
        sceneFaces.push_back(offset + meshes[i].faces[j].z);
      }

      offset += meshes[i].vertices.size();
    }
  }
  void updateScene() {
    clearVerticiesAndFaces();

    offset = 0;

    this->updateVerticiesAndFaces(staticMeshes);
    this->updateVerticiesAndFaces(movingMeshes);

    offset = 0;
  }
  static glm::vec3 closestPointOnTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 c,
                                          glm::vec3 p) {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 bc = c - b;

    float snom = glm::dot(p - a, ab), sdenom = glm::dot(p - b, a - b);
    float tnom = glm::dot(p - a, ac), tdenom = glm::dot(p - c, a - c);

    if (snom <= 0.f && tnom <= 0.f)
      return a;

    float unom = glm::dot(p - b, bc), udenom = glm::dot(p - c, b - c);

    if (sdenom <= 0.f && unom <= 0.f)
      return b;
    if (tdenom <= 0.f && udenom <= 0.f)
      return c;

    glm::vec3 n = glm::cross(b - a, c - a);
    float vc = glm::dot(n, glm::cross(a - p, b - p));
    if (vc <= 0.f && snom >= 0.f && sdenom >= 0.f)
      return a + snom / (snom + sdenom) * ab;

    float va = glm::dot(n, glm::cross(b - p, c - p));
    if (va <= 0.f && unom >= 0.f && udenom >= 0.f)
      return b + unom / (unom + udenom) * bc;

    float vb = glm::dot(n, glm::cross(c - p, a - p));
    if (vb <= 0.f && tnom >= 0.f && tdenom >= 0.f)
      return a + tnom / (tnom + tdenom) * ac;

    float u = va / (va + vb + vc);
    float v = vb / (va + vb + vc);
    float w = 1.f - u - v;
    return u * a + v * b + w * c;
  }

  void animateScene(double deltaTime) {
    float restitution = 0.4;
    float gravity = 0.001f;

    for (int i = 0; i < movingMeshes.size(); i++) {
      movingMeshes[i].vel.y -= gravity * deltaTime * 60;
      movingMeshes[i].moveMesh(movingMeshes[i].vel * (float)(deltaTime * 60));
    }

    for (int iter = 0; iter < 5; iter++) {

      for (int i = 0; i < movingMeshes.size(); i++) {
        for (int j = i + 1; j < movingMeshes.size(); j++) {
          glm::vec3 diff = movingMeshes[i].center - movingMeshes[j].center;
          float dist = glm::length(diff);
          float rsum = movingMeshes[i].radius + movingMeshes[j].radius;

          if (dist < rsum and dist > 0.0f) {
            glm::vec3 n = diff / dist;

            glm::vec3 v1 = movingMeshes[i].vel;
            glm::vec3 v2 = movingMeshes[j].vel;
            float m1 = movingMeshes[i].mass;
            float m2 = movingMeshes[j].mass;

            float vrel = glm::dot(v1 - v2, n);
            if (vrel < 0) {
              float impulse =
                  -(1.0f + restitution) * vrel / (1.0f / m1 + 1.0f / m2);
              movingMeshes[i].vel = v1 + (impulse / m1) * n;
              movingMeshes[j].vel = v2 - (impulse / m2) * n;
            }

            float penetration = rsum - dist;
            if (penetration > 0) {

              glm::vec3 correction = (penetration / (m1 + m2)) * n;
              movingMeshes[i].moveMesh(correction * m2);
              movingMeshes[j].moveMesh(-correction * m1);
            }
          }
        }
      }

      for (int i = 0; i < movingMeshes.size(); i++) {
        for (int j = 0; j < staticMeshes.size(); j++) {
          for (int k = 0; k < staticMeshes[j].faces.size(); k++) {
            glm::ivec3 f = staticMeshes[j].faces[k];
            glm::vec3 v1 = staticMeshes[j].vertices[f.x];
            glm::vec3 v2 = staticMeshes[j].vertices[f.y];
            glm::vec3 v3 = staticMeshes[j].vertices[f.z];

            glm::vec3 p =
                closestPointOnTriangle(v1, v2, v3, movingMeshes[i].center);
            float dist = glm::length(movingMeshes[i].center - p);
            float pen = dist - movingMeshes[i].radius;

            if (pen < 0) {
              glm::vec3 n = glm::normalize(movingMeshes[i].center - p);
              glm::vec3 vel = movingMeshes[i].vel;

              float vrel = glm::dot(vel, n);
              if (vrel < 0) {
                movingMeshes[i].vel = vel - (1.0f + restitution) * vrel * n;
              }
              movingMeshes[i].moveMesh(-pen * n);
            }
          }
        }
      }
    }
  }
  void shotBall(float radius, float speed) {
    float mass = 0.9 * (4.0f / 3.0f) * M_PI * radius * radius * radius;
    Sphere sphere(speed * camera->front, camera->eye, radius, mass);
    this->addToMovingScene(sphere);
  }
};

// loadaj shader
unsigned int make_module(const std::string &filepath,
                         unsigned int module_type) {
  std::ifstream file;
  std::stringstream bufferedLines;
  std::string line;
  file.open(filepath);
  while (std::getline(file, line)) {
    bufferedLines << line << "\n";
  }
  std::string shaderSource = bufferedLines.str();
  const char *shaderSrc = shaderSource.c_str();
  bufferedLines.str("");
  file.close();

  unsigned int shaderModule = glCreateShader(module_type);
  glShaderSource(shaderModule, 1, &shaderSrc, NULL);
  glCompileShader(shaderModule);

  GLint success;
  glGetShaderiv(shaderModule, GL_COMPILE_STATUS, &success);
  if (success != GL_TRUE) {
    char errorLog[1024];
    glGetShaderInfoLog(shaderModule, 1024, NULL, errorLog);
    std::cout << "Shader error:\n" << errorLog << std::endl;
  }
  return shaderModule;
}
// compajliraj shader
unsigned int make_shader(const std::string &vertex_filepath,
                         const std::string &fragment_filepath) {
  std::vector<unsigned int> modules;
  modules.push_back(make_module(vertex_filepath, GL_VERTEX_SHADER));
  modules.push_back(make_module(fragment_filepath, GL_FRAGMENT_SHADER));

  unsigned int shader = glCreateProgram();
  for (unsigned int shaderModule : modules)
    glAttachShader(shader, shaderModule);
  glLinkProgram(shader);

  GLint success;
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (success != GL_TRUE) {
    char errorLog[1024];
    glGetProgramInfoLog(shader, 1024, NULL, errorLog);
    std::cout << "Link error:\n" << errorLog << std::endl;
  }
  for (unsigned int shaderModule : modules)
    glDeleteShader(shaderModule);

  return shader;
}

float scrollOffset = 0.1f;

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
  float scOf = scrollOffset + (float)yoffset * 0.01f;
  if (scOf > 0.1f) {
    scrollOffset = scOf;
  }
}

int main() {
  Mesh mesh;
  mesh.loadMesh("../src/objects/kocka.obj");
  mesh.scaleMesh(3);

  Camera camera(glm::vec3(3.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f));
  for (int i = 0; i < mesh.vertices.size(); i++) {
    cout << mesh.vertices[i].x << " " << mesh.vertices[i].y << " "
         << mesh.vertices[i].z << endl;
  }
  for (int i = 0; i < mesh.faces.size(); i++) {
    cout << mesh.faces[i].x << " " << mesh.faces[i].y << " " << mesh.faces[i].z
         << endl;
  }
  Scene scene(&camera);
  scene.addToStaticScene(mesh);
  Mesh mesh2 = mesh;
  mesh2.moveMesh(glm::vec3(5.0f, 0.0f, 0.0f));
  scene.addToStaticScene(mesh2);
  Mesh mesh3 = mesh;
  mesh3.scaleMesh(2);
  scene.addToStaticScene(mesh3);
  Mesh mesh4;
  mesh4.loadMesh("../src/objects/piramida.obj");
  mesh4.scaleMesh(3);
  mesh4.moveMesh(glm::vec3(0.0f, 0.0f, 5.0f));
  scene.addToStaticScene(mesh4);
  for (int i = 0; i < scene.sceneVertices.size(); i++) {
    cout << scene.sceneVertices.data()[i] << endl;
  }
  for (int i = 0; i < scene.sceneFaces.size(); i++) {
    cout << scene.sceneFaces.data()[i] << endl;
  }

  if (!glfwInit())
    return -1;

  GLFWwindow *window = glfwCreateWindow(1920, 1080, "Cube", NULL, NULL);
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Couldn't load opengl" << std::endl;
    glfwTerminate();
    return -1;
  }

  unsigned int shader =
      make_shader("../src/shaders/shader.vert", "../src/shaders/shader.frag");

  glUseProgram(shader);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  float moveSpeed;
  unsigned int VAO, VBO, EBO;
  double xPrev = 1920.0 / 2;
  double yPrev = 1080.0 / 2;
  double sensitivity = 0.09;
  bool pause = false;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  while (!glfwWindowShouldClose(window)) {
    static double lastTime = glfwGetTime();
    double now = glfwGetTime();
    double deltaTime = now - lastTime;
    moveSpeed = 0.02;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
      moveSpeed = 0.06;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      camera.moveCameraInDirection(moveSpeed, 0, 0, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      camera.moveCameraInDirection(-moveSpeed, 0, 0, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      camera.moveCameraInDirection(0, -moveSpeed, 0, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      camera.moveCameraInDirection(0, moveSpeed, 0, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
      camera.moveCameraInDirection(0, 0, moveSpeed, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.moveCameraInDirection(0, 0, -moveSpeed, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if ((glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
      scene.clearMovingMeshes();
    }
    glfwSetScrollCallback(window, scrollCallback);

    static double lastPause = 0;
    double nowPause = glfwGetTime();
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS and
        nowPause - lastPause > 0.15) {
      pause = !pause;
      lastPause = nowPause;
    }

    static double lastShot = 0;
    double nowShot = glfwGetTime();
    if (nowShot - lastShot > 0.15) {
      if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        scene.shotBall(scrollOffset, 0.2);
        lastShot = nowShot;
      } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) ==
                 GLFW_PRESS) {
        scene.shotBall(scrollOffset, 0.07);
        lastShot = nowShot;
      }
    }

    double time = glfwGetTime();

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    double dx = xpos - xPrev;
    double dy = ypos - yPrev;

    xPrev = xpos;
    yPrev = ypos;

    if (dx != 0 or dy != 0) {
      camera.rotate(sensitivity * dx, -sensitivity * dy);
    }
    glm::mat4 view = glm::lookAt(camera.eye, camera.looking_at, camera.up);
    glm::mat4 projection =
        glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE,
                       glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * scene.sceneVertices.size(),
                 scene.sceneVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(unsigned int) * scene.sceneFaces.size(),
                 scene.sceneFaces.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glfwPollEvents();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader);

    for (int i = 0; i < scene.movingMeshes.size(); i++) {
      cout << i << ", v=" << glm::length(scene.movingMeshes[i].vel) << endl;
    }
    lastTime = now;

    if (pause == false) {
      scene.animateScene(deltaTime);
    }
    scene.updateScene();

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, scene.sceneFaces.size(), GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
  }

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteProgram(shader);
  glfwTerminate();
  return 0;
}