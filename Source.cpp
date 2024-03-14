#include "Shader.h"

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
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

Shader* shaderProgram;
std::vector<Object> objects;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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
    for (int j = 0; j <= segments; ++j) {
        float theta = j * glm::pi<float>() / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int i = 0; i <= stacks; ++i) {
            float phi = i * 2 * glm::pi<float>() / stacks;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            vertices.push_back(radius * glm::vec3(x, z, y));
        }
    }
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

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(640, 480, "Pick Object using Ray Tracing", NULL, NULL);
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
    glViewport(0, 0, 640, 480);
    glEnable(GL_DEPTH_TEST);


    shaderProgram = new Shader("Vertex.vert", "Fragment.frag");

    objects.emplace_back(glm::vec3(0.0f, 0.0f, -2.2f), 0.7f, glm::vec3(1.0f, 0.0f, 0.0f)); // Sphere at the center

    // Create sphere vertices
    std::vector<glm::vec3> sphereVertices = createSphereVertices(objects[0].radius, 1000, 160);

    // Create vertex buffer object
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(glm::vec3), &sphereVertices[0], GL_STATIC_DRAW);
   /* for(int i = 0; i< sphereVertices.size(); i++)
        std::cout << sphereVertices[i].x << " " << sphereVertices[i].y << " " << sphereVertices[i].z << std::endl;*/
   
    // Set vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); // Координаты вершин
    glEnableVertexAttribArray(0);
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // Цвета вершин
    //glEnableVertexAttribArray(1);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // Render
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shaderProgram->use();
        

        glBindVertexArray(VAO);

        // Draw the sphere
        glDrawArrays(GL_TRIANGLES, 0, sphereVertices.size());


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    delete shaderProgram;
    glfwTerminate();
    return 0;
}