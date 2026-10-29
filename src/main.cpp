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

class Mesh
{
public:
  vector<glm::vec3> vertices;
  vector<glm::vec3> originalVertices;
  vector<glm::ivec3> faces;

  int loadMesh(const std::string &filepath)
  {
    cout << "Loading file: " << filepath << endl;
    ifstream file(filepath);
    if (!file.is_open())
    {
      cout << "Failed to open file: " << filepath << endl;
      return 1;
    }

    string line;
    while (getline(file, line))
    {
      istringstream ss(line);
      string prefix;
      ss >> prefix;

      if (prefix == "v")
      {
        float x, y, z;
        if (ss >> x >> y >> z)
        {
          this->vertices.push_back(glm::vec3(x, y, z));
          this->originalVertices.push_back(glm::vec3(x, y, z));
        }
      }
      else if (prefix == "f")
      {
        string v1, v2, v3;
        if (ss >> v1 >> v2 >> v3)
        {
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
  virtual void rotateMesh(float angle, glm::vec3 axis)
  {
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);
    for (int i = 0; i < vertices.size(); i++)
    {
      glm::vec4 rotatedVertex = rotationMatrix * glm::vec4(vertices[i], 1.0f);
      vertices[i] = glm::vec3(rotatedVertex);
      originalVertices[i] = glm::vec3(rotatedVertex);
    }
  }
  virtual void moveMeshFor(glm::vec3 m)
  {
    for (int i = 0; i < vertices.size(); i++)
    {
      vertices[i] += m;
    }
  }
  virtual void moveMeshTo(glm::vec3 m)
  {
    for (int i = 0; i < vertices.size(); i++)
    {
      vertices[i] = originalVertices[i] + m;
    }
  }
  virtual void scaleMesh(float s)
  {
    for (int i = 0; i < vertices.size(); i++)
    {
      vertices[i].x *= s;
      vertices[i].y *= s;
      vertices[i].z *= s;
    }
    originalVertices = vertices;
  }
  virtual void orientMesh(float angleRad, glm::vec3 axis, glm::vec3 pos)
  {
    glm::mat4 R = glm::rotate(glm::mat4(1.0f), angleRad, axis);
    for (int i = 0; i < vertices.size(); i++)
    {
      vertices[i] = glm::vec3(R * glm::vec4(originalVertices[i], 1.0f)) + pos;
    }
  }
};

class Line : public Mesh

{

public:
  Line(glm::vec3 a, glm::vec3 b)
  {
    vertices.push_back(a);
    vertices.push_back(b);
    faces.push_back(glm::ivec3(0, 0, 1));
  }
};

class CoordiateSystem : public Mesh
{
public:
  CoordiateSystem()
  {
    vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    vertices.push_back(glm::vec3(0.0f, 0.0f, 10.0f));
    vertices.push_back(glm::vec3(0.0f, 10.0f, 0.0f));
    vertices.push_back(glm::vec3(10.0f, 0.0f, 0.0f));
    faces.push_back(glm::ivec3(0, 0, 1));
    faces.push_back(glm::ivec3(0, 0, 2));
    faces.push_back(glm::ivec3(0, 0, 3));
  }
};

class SplineCurve : public Mesh
{
public:
  vector<glm::vec3> controlPoints;
  vector<glm::vec3> linePoints;
  vector<glm::vec3> goalOrientations;
  vector<glm::vec3> rotationAxes;
  vector<float> rotationAngles;
  vector<glm::vec3> secondDer;
  SplineCurve()
  {
    controlPoints = vector<glm::vec3>();
  }

  int loadControlPoints(const std::string &filepath)
  {
    cout << "Loading file: " << filepath << endl;
    ifstream file(filepath);
    if (!file.is_open())
    {
      cout << "Failed to open file: " << filepath << endl;
      return 1;
    }

    string line;
    while (getline(file, line))
    {
      istringstream ss(line);
      float x, y, z;
      ss >> x >> y >> z;

      controlPoints.push_back(glm::vec3(x, z, y));
    }
    cout << "Loaded " << controlPoints.size() << " control points" << endl;
    return 0;
  }
  int createSplineCurve()
  {
    for (int i = 1; i < controlPoints.size() - 2; i++)
    {
      for (float t = 0; t <= 1; t += 0.01)
      {
        glm::vec4 t_mat = glm::vec4(t * t * t, t * t, t, 1);
        glm::mat4 matrix = glm::mat4(
            -1.0f, 3.0f, -3.0f, 1.0f,
            3.0f, -6.0f, 3.0f, 0.0f,
            -3.0f, 0.0f, 3.0f, 0.0f,
            1.0f, 4.0f, 1.0f, 0.0f);

        glm::mat4x3 r = glm::mat4x3(controlPoints[i - 1].x, controlPoints[i - 1].y, controlPoints[i - 1].z,
                                    controlPoints[i].x, controlPoints[i].y, controlPoints[i].z,
                                    controlPoints[i + 1].x, controlPoints[i + 1].y, controlPoints[i + 1].z,
                                    controlPoints[i + 2].x, controlPoints[i + 2].y, controlPoints[i + 2].z);
        glm::vec3 p = t_mat * (1.0f / 6.0f) * glm::transpose(matrix) * glm::transpose(r);
        linePoints.push_back(p);

        glm::vec3 t_tangent = glm::vec3(t * t, t, 1.0f);
        glm::mat3x4 mat_d = glm::mat3x4(-1.0f, 3.0f, -3.0f, 1.0f,
                                        2.0f, -4.0f, 2.0f, 0.0f,
                                        -1.0f, 0.0f, 1.0f, 0.0f);
        glm::vec3 tangent = t_tangent * (1.0f / 2.0f) * glm::transpose(mat_d) * glm::transpose(r);
        goalOrientations.push_back(tangent);

        glm::vec2 t_sec_d = glm::vec2(2 * t, 1.0f);
        glm::mat2x4 mat_sec_d = glm::mat2x4(-1.0f, 3.0f, -3.0f, 1.0f,
                                            2.0f, -4.0f, 2.0f, 0.0f);
        glm::vec3 p_sec_d = t_sec_d * (1.0f / 2.0f) * glm::transpose(mat_sec_d) * glm::transpose(r);
        secondDer.push_back(p_sec_d);
      }
    }

    return 0;
  }
  int loadSplineCurve(const std::string &filepath)
  {
    int res = loadControlPoints(filepath);
    int create = createSplineCurve();
    if (res != 0)
      return res;
    if (controlPoints.size() < 4)
    {
      cout << "Need at least 4 control points, got " << controlPoints.size() << endl;
      return 1;
    }
    /* for (int i = 0; i < controlPoints.size() - 1; i++)
     {
       vertices.push_back(controlPoints[i]);
       vertices.push_back(controlPoints[i + 1]);
       vertices.push_back(controlPoints[i + 1]);

       faces.push_back(glm::ivec3(3 * i, 3 * i + 1, 3 * i + 2));
     }*/
    for (int i = 0; i < linePoints.size() - 1; i++)
    {
      cout << "Line point " << i << ": " << linePoints[i].x << ", " << linePoints[i].y << ", " << linePoints[i].z << endl;
    }
    for (int i = 0; i < linePoints.size() - 1; i++)
    {
      vertices.push_back(linePoints[i]);
      vertices.push_back(linePoints[i + 1]);
      vertices.push_back(linePoints[i + 1]);

      faces.push_back(glm::ivec3(3 * i, 3 * i + 1, 3 * i + 2));
    }

    return 0;
  }
};
class Camera
{
public:
  glm::vec3 eye;
  glm::vec3 looking_at;
  glm::vec3 up;
  glm::vec3 front;
  glm::vec3 right;

  float yaw = -90.0f;
  float pitch = 0.0f;
  Camera(glm::vec3 eye, glm::vec3 looking_at, glm::vec3 up)
  {
    this->eye = eye;
    this->looking_at = looking_at;
    this->up = glm::normalize(up);

    front = glm::normalize(looking_at - eye);

    right = glm::normalize(glm::cross(front, this->up));
  }

  void moveCameraInDirection(float frontspeed, float rightspeed, float upspeed,
                             float deltaTime)
  {
    eye += 100.0f * deltaTime * frontspeed *
           normalize(glm::vec3(front.x, 0, front.z));
    eye += 100.0f * deltaTime * rightspeed *
           normalize(glm::vec3(right.x, 0, right.z));
    eye += 100.0f * deltaTime * upspeed * up;
    looking_at = eye + front;
  }

  void rotate(float yawDelta, float pitchDelta)
  {
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

class Scene
{
public:
  vector<Mesh> staticMeshes;

  vector<float> sceneVertices;
  vector<unsigned int> sceneFaces;
  Camera *camera;
  Scene(Camera *camera) { this->camera = camera; }
  void addToScene(Mesh &mesh)
  {
    unsigned int offset = sceneVertices.size() / 6; // 6 = 3 pozicije + 3 boje

    for (int j = 0; j < mesh.vertices.size(); j++)
    {

      sceneVertices.push_back(mesh.vertices[j].x);
      sceneVertices.push_back(mesh.vertices[j].y);
      sceneVertices.push_back(mesh.vertices[j].z);

      sceneVertices.push_back(1.0f);
      sceneVertices.push_back(1.0f);
      sceneVertices.push_back(1.0f);
    }

    for (int j = 0; j < mesh.faces.size(); j++)
    {
      sceneFaces.push_back(offset + mesh.faces[j].x);
      sceneFaces.push_back(offset + mesh.faces[j].y);
      sceneFaces.push_back(offset + mesh.faces[j].z);
    }
  }

  void addToStaticScene(Mesh &mesh)
  {
    staticMeshes.push_back(mesh);
    this->addToScene(mesh);
  }
  void clearVerticiesAndFaces()
  {
    sceneVertices.clear();
    sceneFaces.clear();
  }

  unsigned int offset = 0;
  template <typename T>
  void updateVerticiesAndFaces(vector<T> &meshes)
  {

    for (int i = 0; i < meshes.size(); i++)
    {
      for (int j = 0; j < meshes[i].vertices.size(); j++)
      {
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
      for (int j = 0; j < meshes[i].faces.size(); j++)
      {
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
  void updateScene()
  {
    clearVerticiesAndFaces();

    offset = 0;

    this->updateVerticiesAndFaces(staticMeshes);

    offset = 0;
  }
};

// loadaj shader
unsigned int make_module(const std::string &filepath,
                         unsigned int module_type)
{
  std::ifstream file;
  std::stringstream bufferedLines;
  std::string line;
  file.open(filepath);
  while (std::getline(file, line))
  {
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
  if (success != GL_TRUE)
  {
    char errorLog[1024];
    glGetShaderInfoLog(shaderModule, 1024, NULL, errorLog);
    std::cout << "Shader error:\n"
              << errorLog << std::endl;
  }
  return shaderModule;
}
// compajliraj shader
unsigned int make_shader(const std::string &vertex_filepath,
                         const std::string &fragment_filepath)
{
  std::vector<unsigned int> modules;
  modules.push_back(make_module(vertex_filepath, GL_VERTEX_SHADER));
  modules.push_back(make_module(fragment_filepath, GL_FRAGMENT_SHADER));

  unsigned int shader = glCreateProgram();
  for (unsigned int shaderModule : modules)
    glAttachShader(shader, shaderModule);
  glLinkProgram(shader);

  GLint success;
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (success != GL_TRUE)
  {
    char errorLog[1024];
    glGetProgramInfoLog(shader, 1024, NULL, errorLog);
    std::cout << "Link error:\n"
              << errorLog << std::endl;
  }
  for (unsigned int shaderModule : modules)
    glDeleteShader(shaderModule);

  return shader;
}

int main()
{
  Camera camera(glm::vec3(50.0f, 50.0f, 50.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f));

  Scene scene(&camera);
  // 0
  CoordiateSystem cor;
  scene.addToStaticScene(cor);

  // 1
  SplineCurve spline;
  spline.loadSplineCurve("../src/splines/spline1.txt");
  scene.addToStaticScene(spline);
  // 2
  Line line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 10.0f, 10.0f));
  scene.addToStaticScene(line);
  // 3
  Mesh ship;
  ship.loadMesh("../src/objects/Ncc.obj");
  ship.scaleMesh(5.0f);
  scene.addToStaticScene(ship);

  if (!glfwInit())
    return -1;

  GLFWwindow *window = glfwCreateWindow(1920, 1080, "Animation", NULL, NULL);
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
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
  int frameCount = 0;
  while (!glfwWindowShouldClose(window))
  {
    static double lastTime = glfwGetTime();
    double now = glfwGetTime();
    double deltaTime = now - lastTime;
    moveSpeed = 0.08;

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
      moveSpeed = 0.06;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
      camera.moveCameraInDirection(moveSpeed, 0, 0, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
      camera.moveCameraInDirection(-moveSpeed, 0, 0, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
      camera.moveCameraInDirection(0, -moveSpeed, 0, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
      camera.moveCameraInDirection(0, moveSpeed, 0, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
      camera.moveCameraInDirection(0, 0, moveSpeed, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
      camera.moveCameraInDirection(0, 0, -moveSpeed, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if ((glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS))
    {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    double dx = xpos - xPrev;
    double dy = ypos - yPrev;

    xPrev = xpos;
    yPrev = ypos;

    if (dx != 0 or dy != 0)
    {
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

    lastTime = now;

    if (pause == false)
    {
      glm::vec3 w = spline.goalOrientations[frameCount % spline.goalOrientations.size()];
      glm::vec3 orient = 1 / 3.0f * w;
      glm::vec3 pos = spline.linePoints[frameCount % spline.linePoints.size()];
      scene.staticMeshes[2].vertices[1] = orient + pos;
      scene.staticMeshes[2].vertices[0] = -orient + pos;

      glm::vec3 initialOrientation = glm::vec3(1.0f, 0.0f, 0.0f);
      glm::vec3 dir = glm::normalize(orient);

      float d = glm::dot(initialOrientation, dir);
      float angle = glm::acos(d);
      glm::vec3 axis = glm::cross(initialOrientation, dir);

      scene.staticMeshes[3].orientMesh(angle, glm::normalize(axis), pos);

      // transformirane koordinate sustava
      glm::vec3 localPoint = glm::vec3(1.0f, 1.0f, 2.0f);

      glm::vec3 u = glm::cross(w, spline.secondDer[frameCount % spline.secondDer.size()]);
      glm::vec3 v = glm::cross(w, u);
      glm::mat3x3 R = glm::mat3x3(glm::normalize(u), glm::normalize(v), glm::normalize(w));

      glm::vec3 globalPoint = R * localPoint+pos;
      printf("Local point: %f, %f, %f", localPoint.x, localPoint.y, localPoint.z);
      printf(" Global point: %f, %f, %f\n", globalPoint.x, globalPoint.y, globalPoint.z);
      frameCount++;
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