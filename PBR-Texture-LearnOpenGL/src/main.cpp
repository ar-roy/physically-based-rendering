#include "imgui.h"
#include "OBJ_Loader.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void MouseButtonCallback(GLFWwindow* window, int button, int state, int mods);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void renderSphere();
bool loadOBJ();
void renderCustomModel();

std::vector<glm::vec3> loadedModelPos;
std::vector<glm::vec2> loadedModelUv;
std::vector<glm::vec3> loadedModelNormals;

struct TextureProfile {
    string path;
    bool loaded;
    unsigned int albedo, normal, metallic, roughness, ao;

    explicit TextureProfile(const string& path) : path(path), loaded(false),
                                                  albedo(-1), normal(-1), metallic(-1), roughness(-1), ao(-1) {
    }

    void load() {
        string tmp;
        tmp = path + "/albedo.png";
        albedo = loadTexture(tmp.c_str());
        tmp = path + "/normal.png";
        normal = loadTexture(tmp.c_str());
        tmp = path + "/metallic.png";
        metallic = loadTexture(tmp.c_str());
        tmp = path + "/roughness.png";
        roughness = loadTexture(tmp.c_str());
        tmp = path + "/ao.png";
        ao = loadTexture(tmp.c_str());
        loaded = true;
    }

    void apply() {
        if (!loaded) load();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, metallic);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, roughness);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ao);
    }
};

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;
bool mouseLeft = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);


    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader shader("shaders/pbr.vs", "shaders/pbr.fs");

    shader.use();
    shader.setInt("albedoMap", 0);
    shader.setInt("normalMap", 1);
    shader.setInt("metallicMap", 2);
    shader.setInt("roughnessMap", 3);
    shader.setInt("aoMap", 4);

    // load PBR material textures
    // --------------------------
    TextureProfile txGold("resources/textures/pbr/gold");
    TextureProfile txGrass("resources/textures/pbr/grass");
    TextureProfile txPlastic("resources/textures/pbr/plastic");
    TextureProfile txRusted("resources/textures/pbr/rusted_iron");
    TextureProfile txWall("resources/textures/pbr/wall");
    TextureProfile txCamera("model/kcar");

 	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

    // lights
    // ------
    // light color
	float tmp[4] = {300.f/255.f, 300.f/255.f, 300.f/255.f, 1.f};
	bool turnonlight = true;
    glm::vec3 lightPositions[] = {
        glm::vec3(0.0f,  10.0f, 10.0f),
        glm::vec3(0.0f,  -10.0f, 10.0f),
        glm::vec3(10.0f, 0.0f, 10.0f),
        glm::vec3(-10.0f, 0.0f, 10.0f)
    };
    glm::vec3 lightColors[] = {
        glm::vec3(150.f, 150.f, 150.f),
        glm::vec3(150.f, 150.f, 150.f),
        glm::vec3(150.f, 150.f, 150.f),
        glm::vec3(150.f, 150.f, 150.f)
    };
    int nrRows = 7;
    int nrColumns = 7;
    float spacing = 2.5;

    // initialize static shader uniforms before rendering
    // --------------------------------------------------
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    shader.use();
    shader.setMat4("projection", projection);
    enum Shape { sphere, cylinder, custome };
    int  renderObj= sphere;

    loadOBJ();
    glDisable(GL_CULL_FACE);
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGUI window creation
        ImGui::Begin("PBR");
        static const char* texture_names[] = { "color", "gold", "grass", "plastic", "rusted", "wall" };
        static TextureProfile* texture_files[] = { nullptr, &txGold, &txGrass, &txPlastic, &txRusted, &txWall };
        static int selected_texture = 0;
        static float roughness = .2, metallic = 0.;
        static float albedo[3] = { 1., 0., 0. };
        ImGui::Combo("texture", &selected_texture, texture_names, IM_ARRAYSIZE(texture_names));
        if (renderObj == custome) {
            txCamera.apply();
        }
        else if (texture_files[selected_texture]) {
            shader.setFloat("useColor", 0.);
            texture_files[selected_texture]->apply();
        }
        else {
            ImGui::ColorEdit3("albedo", albedo);
            ImGui::SliderFloat("roughness", &roughness, 0., 1., "%.4f");
            ImGui::SliderFloat("metallic", &metallic, 0., 1., "%.4f");
            shader.setFloat("useColor", 1.);
            shader.setVec3("albedoVal", glm::vec3(albedo[0], albedo[1], albedo[2]));
            shader.setFloat("roughnessVal", roughness);
            shader.setFloat("metallicVal", metallic);
            shader.setFloat("aoVal", 1.);
        }

        static float ambient = .03;
        static bool hdr_gamma = true;
        ImGui::Checkbox("HDR / Gamma Correction", &hdr_gamma);
        ImGui::SliderFloat("ambient", &ambient, 0, 0.1, "%.4f");
        shader.setFloat("ambientVal", ambient);
        shader.setFloat("useCorrection", hdr_gamma ? 1. : 0.);
        
        static bool fresnel_schlick = true;
        static float fresnel0 = 0.04;
        ImGui::Checkbox("Schlick Fresnel", &fresnel_schlick);
        ImGui::SliderFloat("F0", &fresnel0, 0., 1., "%.4f");
        shader.setFloat("useFresnelSchlick", fresnel_schlick ? 1. : 0.);
        shader.setFloat("f0Val", fresnel0);

        static const char* ndf_options[] = { "GGX Trowbridge-Reitz", "Blinn-Phong", "off"};
        static const char* geo_options[] = { "Smith Schlick-GGX", "Kelemen", "off" };
        static int selected_ndf = 0, selected_geo = 0;
        ImGui::Combo("Normal Distribution", &selected_ndf, ndf_options, IM_ARRAYSIZE(ndf_options));
        ImGui::Combo("Geometry", &selected_geo, geo_options, IM_ARRAYSIZE(geo_options));
        shader.setFloat("useNDF", selected_ndf);
        shader.setFloat("useGeometry", selected_geo);
        ImGui::RadioButton("sphere", &renderObj, sphere); ImGui::SameLine();
        ImGui::RadioButton("custome", &renderObj, custome);
		if (ImGui::Checkbox("light", &turnonlight)) {
            if (!turnonlight)
            {
				for (int i = 0; i < 4; i++)
				{
					lightColors[i][0] = 0.f;
					lightColors[i][1] = 0.f;
					lightColors[i][2] = 0.f;
				}
            }
            else
            {
				for (int i = 0; i < 4; i++)
				{
				lightColors[i][0] = tmp[0] * 255;
				lightColors[i][1] = tmp[1] * 255;
				lightColors[i][2] = tmp[2] * 255;
				}

            }
		}
        if ((ImGui::ColorEdit3("light color", tmp)) && turnonlight)
        {
			for (int i = 0; i < 4; i++)
			{
				lightColors[i][0] = tmp[0] * 255;
				lightColors[i][1] = tmp[1] * 255;
				lightColors[i][2] = tmp[2] * 255;
			}

        }

		// Ends the window
		ImGui::End();

        shader.use();
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("view", view);
        shader.setVec3("camPos", camera.Position);
                if (renderObj == custome)
        {
            nrRows = 1;
            nrColumns = 1;
        }
        else {
            nrRows = 7;
            nrColumns = 7;
        }

        // render rows*column number of spheres with material properties defined by textures (they all have the same material properties)
        glm::mat4 model = glm::mat4(1.0f);
        for (int row = 0; row < nrRows; ++row)
        {
            for (int col = 0; col < nrColumns; ++col)
            {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(
                    (float)(col - (nrColumns / 2)) * spacing,
                    (float)(row - (nrRows / 2)) * spacing,
                    0.0f
                ));
                shader.setMat4("model", model);
                if (renderObj == custome) {
                    renderCustomModel();
                }
                else if (renderObj == sphere){
                    renderSphere();
                }
            }
        }

        // render light source (simply re-render sphere at light positions)
        // this looks a bit off as we use the same shader, but it'll make their positions obvious and 
        // keeps the codeprint small.
        for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i)
        {
            glm::vec3 newPos = lightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
            newPos = lightPositions[i];
            shader.setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
            shader.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);

            model = glm::mat4(1.0f);
            model = glm::translate(model, newPos);
            model = glm::scale(model, glm::vec3(0.5f));
            shader.setMat4("model", model);
            renderSphere();
        }

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
        Sleep(1);
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime*2);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime*2);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime*2);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime*2);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!mouseLeft) return;
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void MouseButtonCallback(GLFWwindow* window, int button, int state, int mods) 
{
    double x, y;
    //do not forget to pass the events to ImGUI!
	
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, state);
	if (io.WantCaptureMouse) return; //make sure you do not call this callback when over a menu

    //process them
	if (button == GLFW_MOUSE_BUTTON_LEFT && state == GLFW_PRESS)
	{
		glfwGetCursorPos(window, &x, &y);
        lastX = x;
        lastY = y;
		mouseLeft = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT  && state == GLFW_RELEASE)
	{
		mouseLeft = false;
	}
}

// renders (and builds at first invocation) a sphere
// -------------------------------------------------
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

bool loadOBJ()
{
	objl::Loader Loader;

    std::string objfile("model/kcar/kcar.obj");
    /* const std::string path("model/plane"); */

    /*  boost::filesystem::path dir(path); */
    /* if(boost::filesystem::exists(path) && boost::filesystem::is_directory(path)) */
    /* { */
    /*      boost::filesystem::directory_iterator it(path); */
    /*         boost::filesystem::directory_iterator endit; */
    /*         while (it != endit) { */
    /*             if(boost::filesystem::is_regular_file(*it) && (extension=="")?true:it->path().extension() == extension) { */
    /*                 objfile = it->path().string(); */
    /*                 break; */
    /*             } */
    /*             ++it; */
    /*         } */

    /* } */
    bool status = Loader.LoadFile(objfile);


    if (!status)
    {
        cout << "Model not found...\n";
        exit(1);
    }
    Loader.LoadedMeshes[0].Vertices[0].Position;
    objl::Mesh model = Loader.LoadedMeshes[0];
    for (int i = 0; i < model.Vertices.size(); i++)
	{
        objl::Vector3 pos = model.Vertices[i].Position;
        objl::Vector2 uv = model.Vertices[i].TextureCoordinate;
        objl::Vector3 normal = model.Vertices[i].Normal;
        loadedModelPos.push_back(glm::vec3(pos.X, pos.Y, pos.Z));
        loadedModelUv.push_back(glm::vec2(uv.X, 1.f - uv.Y));
        loadedModelNormals.push_back(glm::vec3(normal.X, normal.Y, normal.Z));
	}

    return true;
}

unsigned int modelVAO = 0, modelVBO = 0;
// render third party model
void renderCustomModel()
{
	glGenVertexArrays(1, &modelVAO);

	//unsigned int vbo, ebo;
	glGenBuffers(1, &modelVBO);
	//glGenBuffers(1, &ebo);

	std::vector<glm::vec3> positions = loadedModelPos;
	std::vector<glm::vec2> uv = loadedModelUv;
	std::vector<glm::vec3> normals = loadedModelNormals;
	//std::vector<unsigned int> indices;

	//indexCount = static_cast<unsigned int>(indices.size());

	std::vector<float> data;
	for (unsigned int i = 0; i < positions.size(); ++i)
	{
		data.push_back(positions[i].x);
		data.push_back(positions[i].y);
		data.push_back(positions[i].z);
		if (normals.size() > 0)
		{
			data.push_back(normals[i].x);
			data.push_back(normals[i].y);
			data.push_back(normals[i].z);
		}
		if (uv.size() > 0)
		{
			data.push_back(uv[i].x);
			data.push_back(uv[i].y);
		}
	}
    //cout << "size" << uv.size() << " " << normals.size() << " " << positions.size() << "\n";
	glBindVertexArray(modelVAO);
	glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, positions.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	unsigned int stride = (3 + 2 + 3) * sizeof(float);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(modelVAO);
    //glDrawElements(GL_POINTS, data.size()/3, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, data.size()/3);
}
