#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 vs_position;
out vec3 vs_normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords;
    vs_position = vec4(model * vec4(aPos, 1.0f)).xyz;
    vs_normal = mat3(1.0f) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

//------------------------------------------------------------------------------
#shader fragment

#version 330 core
out vec4 FragColor;

in vec3 vs_position;
in vec3 vs_normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec3 lightpos;

void main()
{
    vec3 ambientLight = vec3(0.3f, 0.3f, 0.2f);

    // diffuse light
    vec3 posToLightDirVec = normalize(lightpos - vs_position);

    vec3 diffuseColor = vec3(1.0f, 1.0f, 1.0f);
    float diffuse = clamp(dot(posToLightDirVec, vs_normal), 0, 1);
    vec3 diffuseFinal = diffuseColor * diffuse;

    //FragColor =  vec4(vs_color, 1.0f) * (vec4(ambientLight, 1.0f) + vec4(diffuseFinal, 1.0f));

    //FragColor = texture(texture_diffuse1, TexCoords) * vec4(ambientLight, 1.0f);// + vec4(diffuseFinal, 1);
    //vec4 texture_col = 
    FragColor = texture(texture_diffuse1, TexCoords) * vec4(diffuseFinal, 1);
}

