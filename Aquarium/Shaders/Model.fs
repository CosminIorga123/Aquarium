#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform float factor;

void main()
{   
    vec4 color = texture(texture_diffuse1, TexCoords);
    color.r -= color.r * factor;
    color.g -= color.g * factor;
    color.b -= color.b * factor;
    FragColor = color;
}