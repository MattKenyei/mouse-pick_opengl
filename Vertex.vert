#version 330 core

layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aColor; // Атрибут цвета

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Color; // Передаем цвет во фрагментный шейдер

void main() {
    gl_Position =  vec4(aPos, 1.0);
    FragPos = aPos;
    Color = vec3(1.f, 0.f, 0.f); // Передаем цвет во фрагментный шейдер
}
