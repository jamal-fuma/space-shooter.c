#define STB_IMAGE_IMPLEMENTATION
#include "game-renderer.h"
#include "../../lib/stb_image.h"

static GLuint pixelSizeLocation;
static GLuint panelPixelSizeLocation;
static GLuint spriteSheetLocation;
static GLuint spriteSheetDimensionsLocation;
static GLuint panelIndexLocation;
static GLuint pixelOffsetLocation;
static GLuint spriteScaleLocation;

void renderer_draw(RenderList* list, uint8_t count) {
    if (count == 0) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, list->texture);
    glUniform2fv(panelPixelSizeLocation, 1, list->panelDims);
    glUniform2fv(spriteSheetDimensionsLocation, 1, list->sheetDims);

    for (uint8_t i = 0; i < count; ++i) {
        glUniform2fv(pixelOffsetLocation, 1, list->objects[i].position);
        glUniform3f(panelIndexLocation, (float) list->objects[i].currentSpritePanel[0], list->objects[i].currentSpritePanel[1], (float) list->objects[i].flipX);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}


bool renderer_loadTexture(const char* fileName, GLuint* texture) {
    int imageWidth, imageHeight, imageChannels;
    uint8_t *imageData = stbi_load(fileName, &imageWidth, &imageHeight, &imageChannels, 4);

    if (!imageData) {
        return false;
    }

    glGenTextures(1, texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    stbi_image_free(imageData);

    return true;
}

void renderer_resize(int width, int height) {
     glViewport(0, 0, width, height);
     glUniform2f(pixelSizeLocation, 2.0f / width, 2.0f / height);
}

void renderer_init() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    const char* vsSource = "#version 450\n"
    "layout (location=0) in vec2 position;\n"
    "uniform vec2 pixelOffset;\n"
    "uniform vec2 pixelSize;\n"
    "uniform vec2 panelPixelSize;\n"
    "uniform float spriteScale;\n"
    "uniform vec3 panelIndex;\n" // z is whether to flip horizontally
    "uniform vec2 spriteSheetDimensions;\n"
    "out vec2 vUV;\n"
    "void main() {\n"
    "    vec2 uv = position;\n"
    "    if (panelIndex.z == 1.0) uv.x = 1.0 - position.x;\n"
    "    vUV = (uv + panelIndex.xy) / spriteSheetDimensions;\n"
    "    vec2 clipOffset = pixelOffset * pixelSize - 1.0;\n"
    "    gl_Position = vec4((position * panelPixelSize * pixelSize * spriteScale + clipOffset) * vec2(1.0, -1.0), 0.0, 1.0);\n"
    "}\n";

    const char* fsSource = "#version 450\n"
    "in vec2 vUV;\n"
    "uniform sampler2D spriteSheet;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = texture(spriteSheet, vUV);\n"
    "    fragColor.rgb *= fragColor.a;\n"
    "}\n";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vsSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fsSource, NULL);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint result;
    glGetProgramiv(program, GL_LINK_STATUS, &result);

    // TODO(Tarek): Get rid of these.
    if (result != GL_TRUE) {
        MessageBoxA(NULL, "Program failed to link!", "FAILURE", MB_OK);
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
        if (result != GL_TRUE) {
            MessageBoxA(NULL, "Vertex shader failed to compile!", "FAILURE", MB_OK);
        }
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
        if (result != GL_TRUE) {
            MessageBoxA(NULL, "Fragment shader failed to compile!", "FAILURE", MB_OK);
        }
    }


    glUseProgram(program);
    pixelSizeLocation = glGetUniformLocation(program, "pixelSize");
    panelPixelSizeLocation = glGetUniformLocation(program, "panelPixelSize");
    spriteSheetLocation = glGetUniformLocation(program, "spriteSheet");
    spriteSheetDimensionsLocation = glGetUniformLocation(program, "spriteSheetDimensions");
    panelIndexLocation = glGetUniformLocation(program, "panelIndex");
    pixelOffsetLocation = glGetUniformLocation(program, "pixelOffset");
    spriteScaleLocation = glGetUniformLocation(program, "spriteScale");

    glUniform1f(spriteScaleLocation, SPRITE_SCALE); 

    float positions[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f,  1.0f
    };

    GLuint spriteArray;
    glGenVertexArrays(1, &spriteArray);
    glBindVertexArray(spriteArray);

    GLuint positionBuffer;
    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glUniform1i(spriteSheetLocation, 0);
}