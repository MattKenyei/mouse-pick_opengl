#include "Shader.h"
#include "Camera.h"

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};
struct ModelTransform
{
    glm::vec3 pos;
    glm::vec3 rotation;
    glm::vec3 scale;

    void setScale(float s)
    {
        scale.x = s;
        scale.y = s;
        scale.z = s;
    }

};
class Object {
public:
    glm::vec3 position;
    float radius;
    glm::vec3 color;
    Object(const glm::vec3& pos, float rad, const glm::vec3& col) : position(pos), radius(rad), color(col) {}
    bool intersect(const Ray& ray) const {
        glm::vec3 oc = ray.origin - position;
        float a = glm::dot(ray.direction, ray.direction);
        float b = 2.0f * glm::dot(oc, ray.direction);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        return (discriminant > 0);
    }
};


class Cube : public Object {
public:
    Cube(const glm::vec3& pos, float sideLength, const glm::vec3& col) : Object(pos, sideLength / 2.0f, col), sideLength(sideLength) {}
    float dot(const glm::vec3& a, const glm::vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    bool intersect(const Ray& ray) const {
        // Calculate ray origin in object space
        glm::vec3 localOrigin = ray.origin - position;

        // Calculate half extents of the cube
        float halfLength = sideLength / 2.0f;

        // Calculate intersection with each slab of the cube
        float tNear = -FLT_MAX;
        float tFar = FLT_MAX;

        for (int i = 0; i < 3; ++i) {
            if (abs(ray.direction[i]) < EPSILON) {
                // Ray is parallel to slab, no intersection if origin is not within slab
                if (localOrigin[i] < -halfLength || localOrigin[i] > halfLength)
                    return false;
            }
            else {
                // Compute intersection t values of the two slab boundaries
                float t1 = (-halfLength - localOrigin[i]) / ray.direction[i];
                float t2 = (halfLength - localOrigin[i]) / ray.direction[i];

                // Make t1 the intersection with the near plane, t2 with the far plane
                if (t1 > t2)
                    std::swap(t1, t2);

                // Update tNear and tFar
                tNear = std::max(tNear, t1);
                tFar = std::min(tFar, t2);

                // If intersection is outside slab, no intersection
                if (tNear > tFar)
                    return false;

                // If slab intersection is behind ray, no intersection
                if (tFar < 0)
                    return false;
            }
        }
        return true;
    }

private:
    float sideLength;
    const float EPSILON = 0.0001f;
};


Shader* shaderProgram;
std::vector<Object> objects;
std::vector<Cube> cubes;
Camera camera(glm::vec3(0.f, 0.f, -2.f));
void onScroll(GLFWwindow* win, double x, double y)
{
    camera.ChangeFOV(y);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    uint32_t dir = 0;
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        dir |= CAM_UP;
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        dir |= CAM_DOWN;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        dir |= CAM_FORWARD;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        dir |= CAM_BACKWARD;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        dir |= CAM_LEFT;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        dir |= CAM_RIGHT;
    /*static double  newx = 0.f, newy = 0.f;
    glfwGetCursorPos(window, &newx, &newy);
    static double  x = newx, y = newy;
    double xoffset = newx - x, yoffset = newy - y;
    x = newx;
    y = newy;*/
    camera.Move(dir, dt);
    //camera.Rotate(xoffset, -yoffset);
}


void pickObject(const Ray& ray) {
    for (const auto& obj : objects) {
        if (obj.intersect(ray)) {
            std::cout << "Object picked!" << std::endl;
            // Do shit
        }
    }
}

Ray calculateRayFromMouse(double xpos, double ypos, GLFWwindow* window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // Convert screen coordinates to NDC (Normalized Device Coordinates)
    float x = (2.0f * xpos) / width - 1.0f;
    float y = 1.0f - (2.0f * ypos) / height;

    // Convert NDC to clip coordinates
    glm::vec4 clipCoords(x, y, -1.0f, 1.0f);

    // Convert clip coordinates to eye coordinates
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    glm::mat4 view = glm::mat4(1.0f); // Assuming camera is at the origin and looking along the negative z-axis
    glm::vec4 eyeCoords = glm::inverse(projection * view) * clipCoords;
    eyeCoords.z = -1.0f; // Direction along the negative z-axis

    // Convert eye coordinates to world coordinates
    glm::vec4 worldCoords = glm::inverse(view) * eyeCoords;

    Ray ray;
    ray.origin = glm::vec3(0.0f); // Origin at camera position
    ray.direction = glm::normalize(glm::vec3(worldCoords));
    return ray;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        Ray ray = calculateRayFromMouse(xpos, ypos, window);
        pickObject(ray);
    }
}

std::vector<glm::vec3> createSphereVertices(float radius, int segments, int stacks) {
    std::vector<glm::vec3> vertices;
    for (int j = 0; j <= segments; j++) {
        float theta = j * glm::pi<float>() / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int i = 0; i <= stacks; i++) {
            float phi = i * 2 * glm::pi<float>() / stacks;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float x = cosPhi * sinTheta;
            float z = cosTheta;
            float y = sinPhi * sinTheta;

            vertices.push_back(radius * glm::vec3(x, y, z));
        }
    }
    //float deltaTheta = glm::pi<float>() / 120;
    //float deltaPhi = 2* glm::pi<float>() / 100;
    //float theta = 0, phi = 0;
    //for (int ring = 0; ring < 10; ring++) { //move to a new z - offset 
    //    theta += deltaTheta;
    //    for (int point = 0; point < 10; point++) { // draw a ring
    //        phi += deltaPhi;
    //        float x = sin(theta) * cos(phi);
    //        float y = sin(theta) * sin(phi);
    //        float z = cos(theta);
    //        vertices.push_back(radius * glm::vec3(x, y, z));
    //    }
    //}
    return vertices;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set GLFW window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    int winw = 800, winh = 600;
    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(winw, winh, "Pick Object using Ray Tracing", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set viewport size callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Set mouse button callback
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glViewport(0, 0, winw, winh);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    shaderProgram = new Shader("Vertex.vert", "Fragment.frag");
    ModelTransform polygonTrans = { glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.0f, 1.0f, 1.0f) };
    Cube cube(glm::vec3(0.0f, 0.0f, -2.0f), 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    objects.push_back(cube);
    cubes.push_back(cube);

    // Создание вершинного буфера и массива вершин для куба
    std::vector<glm::vec3> cubeVertices = {
        // Первая грань
        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f),
        // Вторая грань
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        // Третья грань
        glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f),
        // Четвёртая грань
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        // Пятая грань
        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, -0.5f),
        // Шестая грань
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f)
    };
    //objects.emplace_back(glm::vec3(0.0f, 0.0f, -2.2f), 0.7f, glm::vec3(1.0f, 0.0f, 0.0f)); // Sphere at the center
    // Create sphere vertices
    //std::vector<glm::vec3> sphereVertices = createSphereVertices(objects[0].radius, 1000, 4);

    // Create vertex buffer object
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(glm::vec3), &cubeVertices[0], GL_STATIC_DRAW);
   /* for(int i = 0; i< sphereVertices.size(); i++)
        std::cout << sphereVertices[i].x << " " << sphereVertices[i].y << " " << sphereVertices[i].z << std::endl;*/
   
    // Set vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); // Координаты вершин
    glEnableVertexAttribArray(0);
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // Цвета вершин
    //glEnableVertexAttribArray(1);
    double oldTime = glfwGetTime(), newTime, deltaTime;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        newTime = glfwGetTime();
        deltaTime = newTime - oldTime;
        oldTime = newTime;
        processInput(window, deltaTime);
        polygonTrans.rotation.x = glfwGetTime() * 40.0;
        //polygonTrans.pos.x = 0.8f * cos(glfwGetTime() * 3);
        //polygonTrans.pos.y = 0.8f * sin(glfwGetTime() * 3);
        //polygonTrans.setScale(0.2f);
        // Render
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shaderProgram->use();


        glm::mat4 pv = camera.GetProjectionMatrix() * camera.GetViewMatrix();


        //1
        glm::mat4 model = glm::mat4(1.0f);
        float color[3] = { objects[0].color.x, objects[0].color.y, objects[0].color.z };

        model = glm::translate(model, polygonTrans.pos);
        model = glm::rotate(model, glm::radians(polygonTrans.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(polygonTrans.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        glm::mat4 pvm = pv * model;

        shaderProgram->SetMatrix4F("pvm", pvm);
        shaderProgram->SetColor("aColor", color);

        glBindVertexArray(VAO);

        // Draw the sphere
        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    delete shaderProgram;
    glfwTerminate();
    return 0;
}