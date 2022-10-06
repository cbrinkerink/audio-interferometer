// Disclaimer: this code is incredibly ugly! But it works.
#include "glad.h"
#include "glfw3.h"
#include "stb_image.h"

#include "shader_s.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <iomanip>
#include <boost/asio.hpp> 
#include <termios.h>

#include <boost/array.hpp>
#include <unistd.h>
#include <boost/lexical_cast.hpp>

#include <math.h>
#include <cmath>

#include "json.hpp"

#define NUMLAGS 256

using namespace std;
using namespace boost;
using boost::asio::ip::udp;
using json = nlohmann::json;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// Ugly global variables for key presses
bool peakMode = true;
int selectedBaseline = -1;
char selectedMic = 0;
float micpos1[3] = {-0.073, -0.065, 0.};
float micpos2[3] = {0.073, -0.065, 0.};
float micpos3[3] = {-0.038, 0.065, 0.};
float micpos4[3] = {0.04, 0.065, 0.};
float micpos5[3] = {0.118, 0.285, 0.};
float micpos6[3] = {0.268, 0.285, 0.};
float micpos7[3] = {0.026, -0.065, 0.};
float micpos8[3] = {-0.026, -0.065, 0.};
int m1loc, m2loc, m3loc, m4loc, m5loc, m6loc, m7loc, m8loc, smloc, sbloc, loloc, ascloc, ashloc;
bool jDown = false;
bool eDown = false;
bool zDown = false;
bool xDown = false;
bool cDown = false;
bool vDown = false;
bool bDown = false;
bool nDown = false;
bool mDown = false;
bool lDown = false;
bool commaDown = false;
bool periodDown = false;
bool slashDown = false;
bool ampSelected = false;
bool autoScale = true;
float lagoffsets[28] = {NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.};
float ampscales[28] = {1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1.};
float ampshifts[28] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};

bool gotConnection = false;
char* commandLineArgs[3];

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Ethernet settings
boost::asio::io_service io_service;
udp::endpoint local_endpoint;
udp::socket sock(io_service);
udp::endpoint sender_endpoint;

std::string uchar2hex(unsigned char inchar)
{
  std::ostringstream oss (std::ostringstream::out);
  oss << std::setw(2) << std::setfill('0') << std::hex << (int)(inchar);
  return oss.str();
}

// Function to load settings from JSON config file
// TODO: add lag offsets and amplitude settings
void loadConfig(void) {
  try {
    std::ifstream f("config.json");
    json data = json::parse(f);
  
    micpos1[0] = data["config"]["positions"]["micpos1"]["x"].get<float>();
    micpos1[1] = data["config"]["positions"]["micpos1"]["y"].get<float>();
    micpos1[2] = data["config"]["positions"]["micpos1"]["z"].get<float>();
    micpos2[0] = data["config"]["positions"]["micpos2"]["x"].get<float>();
    micpos2[1] = data["config"]["positions"]["micpos2"]["y"].get<float>();
    micpos2[2] = data["config"]["positions"]["micpos2"]["z"].get<float>();
    micpos3[0] = data["config"]["positions"]["micpos3"]["x"].get<float>();
    micpos3[1] = data["config"]["positions"]["micpos3"]["y"].get<float>();
    micpos3[2] = data["config"]["positions"]["micpos3"]["z"].get<float>();
    micpos4[0] = data["config"]["positions"]["micpos4"]["x"].get<float>();
    micpos4[1] = data["config"]["positions"]["micpos4"]["y"].get<float>();
    micpos4[2] = data["config"]["positions"]["micpos4"]["z"].get<float>();
    micpos5[0] = data["config"]["positions"]["micpos5"]["x"].get<float>();
    micpos5[1] = data["config"]["positions"]["micpos5"]["y"].get<float>();
    micpos5[2] = data["config"]["positions"]["micpos5"]["z"].get<float>();
    micpos6[0] = data["config"]["positions"]["micpos6"]["x"].get<float>();
    micpos6[1] = data["config"]["positions"]["micpos6"]["y"].get<float>();
    micpos6[2] = data["config"]["positions"]["micpos6"]["z"].get<float>();
    micpos7[0] = data["config"]["positions"]["micpos7"]["x"].get<float>();
    micpos7[1] = data["config"]["positions"]["micpos7"]["y"].get<float>();
    micpos7[2] = data["config"]["positions"]["micpos7"]["z"].get<float>();
    micpos8[0] = data["config"]["positions"]["micpos8"]["x"].get<float>();
    micpos8[1] = data["config"]["positions"]["micpos8"]["y"].get<float>();
    micpos8[2] = data["config"]["positions"]["micpos8"]["z"].get<float>();
    lagoffsets[0]  = data["config"]["lagoffsets"]["lagoffset1"].get<float>();
    lagoffsets[1]  = data["config"]["lagoffsets"]["lagoffset2"].get<float>();
    lagoffsets[2]  = data["config"]["lagoffsets"]["lagoffset3"].get<float>();
    lagoffsets[3]  = data["config"]["lagoffsets"]["lagoffset4"].get<float>();
    lagoffsets[4]  = data["config"]["lagoffsets"]["lagoffset5"].get<float>();
    lagoffsets[5]  = data["config"]["lagoffsets"]["lagoffset6"].get<float>();
    lagoffsets[6]  = data["config"]["lagoffsets"]["lagoffset7"].get<float>();
    lagoffsets[7]  = data["config"]["lagoffsets"]["lagoffset8"].get<float>();
    lagoffsets[8]  = data["config"]["lagoffsets"]["lagoffset9"].get<float>();
    lagoffsets[9]  = data["config"]["lagoffsets"]["lagoffset10"].get<float>();
    lagoffsets[10] = data["config"]["lagoffsets"]["lagoffset11"].get<float>();
    lagoffsets[11] = data["config"]["lagoffsets"]["lagoffset12"].get<float>();
    lagoffsets[12] = data["config"]["lagoffsets"]["lagoffset13"].get<float>();
    lagoffsets[13] = data["config"]["lagoffsets"]["lagoffset14"].get<float>();
    lagoffsets[14] = data["config"]["lagoffsets"]["lagoffset15"].get<float>();
    lagoffsets[15] = data["config"]["lagoffsets"]["lagoffset16"].get<float>();
    lagoffsets[16] = data["config"]["lagoffsets"]["lagoffset17"].get<float>();
    lagoffsets[17] = data["config"]["lagoffsets"]["lagoffset18"].get<float>();
    lagoffsets[18] = data["config"]["lagoffsets"]["lagoffset19"].get<float>();
    lagoffsets[19] = data["config"]["lagoffsets"]["lagoffset20"].get<float>();
    lagoffsets[20] = data["config"]["lagoffsets"]["lagoffset21"].get<float>();
    lagoffsets[21] = data["config"]["lagoffsets"]["lagoffset22"].get<float>();
    lagoffsets[22] = data["config"]["lagoffsets"]["lagoffset23"].get<float>();
    lagoffsets[23] = data["config"]["lagoffsets"]["lagoffset24"].get<float>();
    lagoffsets[24] = data["config"]["lagoffsets"]["lagoffset25"].get<float>();
    lagoffsets[25] = data["config"]["lagoffsets"]["lagoffset26"].get<float>();
    lagoffsets[26] = data["config"]["lagoffsets"]["lagoffset27"].get<float>();
    lagoffsets[27] = data["config"]["lagoffsets"]["lagoffset28"].get<float>();
    ampscales[0]  = data["config"]["ampscales"]["ampscale1"].get<float>();
    ampscales[1]  = data["config"]["ampscales"]["ampscale2"].get<float>();
    ampscales[2]  = data["config"]["ampscales"]["ampscale3"].get<float>();
    ampscales[3]  = data["config"]["ampscales"]["ampscale4"].get<float>();
    ampscales[4]  = data["config"]["ampscales"]["ampscale5"].get<float>();
    ampscales[5]  = data["config"]["ampscales"]["ampscale6"].get<float>();
    ampscales[6]  = data["config"]["ampscales"]["ampscale7"].get<float>();
    ampscales[7]  = data["config"]["ampscales"]["ampscale8"].get<float>();
    ampscales[8]  = data["config"]["ampscales"]["ampscale9"].get<float>();
    ampscales[9]  = data["config"]["ampscales"]["ampscale10"].get<float>();
    ampscales[10] = data["config"]["ampscales"]["ampscale11"].get<float>();
    ampscales[11] = data["config"]["ampscales"]["ampscale12"].get<float>();
    ampscales[12] = data["config"]["ampscales"]["ampscale13"].get<float>();
    ampscales[13] = data["config"]["ampscales"]["ampscale14"].get<float>();
    ampscales[14] = data["config"]["ampscales"]["ampscale15"].get<float>();
    ampscales[15] = data["config"]["ampscales"]["ampscale16"].get<float>();
    ampscales[16] = data["config"]["ampscales"]["ampscale17"].get<float>();
    ampscales[17] = data["config"]["ampscales"]["ampscale18"].get<float>();
    ampscales[18] = data["config"]["ampscales"]["ampscale19"].get<float>();
    ampscales[19] = data["config"]["ampscales"]["ampscale20"].get<float>();
    ampscales[20] = data["config"]["ampscales"]["ampscale21"].get<float>();
    ampscales[21] = data["config"]["ampscales"]["ampscale22"].get<float>();
    ampscales[22] = data["config"]["ampscales"]["ampscale23"].get<float>();
    ampscales[23] = data["config"]["ampscales"]["ampscale24"].get<float>();
    ampscales[24] = data["config"]["ampscales"]["ampscale25"].get<float>();
    ampscales[25] = data["config"]["ampscales"]["ampscale26"].get<float>();
    ampscales[26] = data["config"]["ampscales"]["ampscale27"].get<float>();
    ampscales[27] = data["config"]["ampscales"]["ampscale28"].get<float>();
    ampshifts[0]  = data["config"]["ampoffsets"]["ampoffset1"].get<float>();
    ampshifts[1]  = data["config"]["ampoffsets"]["ampoffset2"].get<float>();
    ampshifts[2]  = data["config"]["ampoffsets"]["ampoffset3"].get<float>();
    ampshifts[3]  = data["config"]["ampoffsets"]["ampoffset4"].get<float>();
    ampshifts[4]  = data["config"]["ampoffsets"]["ampoffset5"].get<float>();
    ampshifts[5]  = data["config"]["ampoffsets"]["ampoffset6"].get<float>();
    ampshifts[6]  = data["config"]["ampoffsets"]["ampoffset7"].get<float>();
    ampshifts[7]  = data["config"]["ampoffsets"]["ampoffset8"].get<float>();
    ampshifts[8]  = data["config"]["ampoffsets"]["ampoffset9"].get<float>();
    ampshifts[9]  = data["config"]["ampoffsets"]["ampoffset10"].get<float>();
    ampshifts[10] = data["config"]["ampoffsets"]["ampoffset11"].get<float>();
    ampshifts[11] = data["config"]["ampoffsets"]["ampoffset12"].get<float>();
    ampshifts[12] = data["config"]["ampoffsets"]["ampoffset13"].get<float>();
    ampshifts[13] = data["config"]["ampoffsets"]["ampoffset14"].get<float>();
    ampshifts[14] = data["config"]["ampoffsets"]["ampoffset15"].get<float>();
    ampshifts[15] = data["config"]["ampoffsets"]["ampoffset16"].get<float>();
    ampshifts[16] = data["config"]["ampoffsets"]["ampoffset17"].get<float>();
    ampshifts[17] = data["config"]["ampoffsets"]["ampoffset18"].get<float>();
    ampshifts[18] = data["config"]["ampoffsets"]["ampoffset19"].get<float>();
    ampshifts[19] = data["config"]["ampoffsets"]["ampoffset20"].get<float>();
    ampshifts[20] = data["config"]["ampoffsets"]["ampoffset21"].get<float>();
    ampshifts[21] = data["config"]["ampoffsets"]["ampoffset22"].get<float>();
    ampshifts[22] = data["config"]["ampoffsets"]["ampoffset23"].get<float>();
    ampshifts[23] = data["config"]["ampoffsets"]["ampoffset24"].get<float>();
    ampshifts[24] = data["config"]["ampoffsets"]["ampoffset25"].get<float>();
    ampshifts[25] = data["config"]["ampoffsets"]["ampoffset26"].get<float>();
    ampshifts[26] = data["config"]["ampoffsets"]["ampoffset27"].get<float>();
    ampshifts[27] = data["config"]["ampoffsets"]["ampoffset28"].get<float>();
    glUniform3f(m1loc, micpos1[0], micpos1[1], micpos1[2]);
    glUniform3f(m2loc, micpos2[0], micpos2[1], micpos2[2]);
    glUniform3f(m3loc, micpos3[0], micpos3[1], micpos3[2]);
    glUniform3f(m4loc, micpos4[0], micpos4[1], micpos4[2]);
    glUniform3f(m5loc, micpos5[0], micpos5[1], micpos5[2]);
    glUniform3f(m6loc, micpos6[0], micpos6[1], micpos6[2]);
    glUniform3f(m7loc, micpos7[0], micpos7[1], micpos7[2]);
    glUniform3f(m8loc, micpos8[0], micpos8[1], micpos8[2]);
    glUniform1fv(loloc, 28, lagoffsets);
    glUniform1fv(ascloc, 28, ampscales);
    glUniform1fv(ashloc, 28, ampshifts);
  } catch (std::exception& e) {
    std::cout << "Could not load JSON config file!!" << std::endl;
  }
}

void setupEthernetConnection(char* argv[]) {
  try {
    std::cout << "Trying to set up UDP listener on IP " << argv[1] << " using port " << argv[2] << std::endl;
    sock.close();
    sock.open(udp::v4());
    local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(argv[1]), boost::lexical_cast<int>(argv[2]));
    sock.bind(local_endpoint);
    gotConnection = true;
    std::cout << "Local bind " << local_endpoint << std::endl;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cout << "Could not connect to remote IP address, using static placeholder data" << std::endl;
    gotConnection = false;
  }
}

int main(int argc, char* argv[])
{
    try
    {
      // Check command line arguments
      if (argc != 3)
      {
        std::cerr << "Usage: client <host> <port>" << std::endl;
        return 1;
      }

      commandLineArgs[0] = argv[0];
      commandLineArgs[1] = argv[1];
      commandLineArgs[2] = argv[2];

      // Initialise ethernet necessities
      // TODO: make toggle-able with a keypress!
      setupEthernetConnection(commandLineArgs);

      boost::array<char, NUMLAGS * 4> recv_buf; // Current buffer size: 1 baseline, 256 lags, 4 bytes per lag
      size_t bytes_available;

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
      GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lag functions", NULL, NULL);
      if (window == NULL)
      {
          std::cout << "Failed to create GLFW window" << std::endl;
          glfwTerminate();
          return -1;
      }
      glfwMakeContextCurrent(window);
  
      glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
  
      glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  
      // glad: load all OpenGL function pointers
      // ---------------------------------------
      if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
      {
          std::cout << "Failed to initialize GLAD" << std::endl;
          return -1;
      }
  
      // build and compile our shader program
      // ------------------------------------
      Shader ourShader("client-ethernet-scalable.vs", "client-ethernet-scalable.fs"); 
  
      // set up vertex data (and buffer(s)) and configure vertex attributes
      float vertices[] = {
          // positions          // colors           // texture coords
           1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
           1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
          -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
          -1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
      };
      unsigned int indices[] = {  
          0, 1, 3, // first triangle
          1, 2, 3  // second triangle
      };
      unsigned int VBO, VAO, EBO;
      glGenVertexArrays(1, &VAO);
      glGenBuffers(1, &VBO);
      glGenBuffers(1, &EBO);
  
      glBindVertexArray(VAO);
  
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
  
      // position attribute
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
      // color attribute
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
      glEnableVertexAttribArray(1);
      // texture coord attribute
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
      glEnableVertexAttribArray(2);
  
  
      // load and create a texture 
      // -------------------------
      unsigned int texture;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
      // set the texture wrapping parameters
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      // set texture filtering parameters
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      // load image, create texture and generate mipmaps
      
      //int width, height, nrChannels;
      //unsigned char *data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
  
      float pixels[3 * NUMLAGS * 64];
      for (int i = 0; i < 3 * NUMLAGS * 64; i++) {
          //pixels[i] = 0;
          pixels[i] = 0.;
      }
  
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, NUMLAGS, 64, 0, GL_RGB, GL_FLOAT, pixels);
      glGenerateMipmap(GL_TEXTURE_2D);
  
      //stbi_image_free(data);
  
      float lagvals[28][NUMLAGS];
      for (int i = 0; i < 28; i++) {
        for (int j = 0; j < NUMLAGS; j++) {
          //lagvals[i][j] = 0;
          lagvals[i][j] = 0.;
        }
      }
  
      float stdminvals[28] = {100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000., 100000000.};
      // For use in peak tracking		         
      float stdmaxvals[28] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
      float minvals[28];
      float maxvals[28];
      float ranges[28];
      int maxbin[28] = {NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2, NUMLAGS/2};
 
      // Get the locations of all our uniform variables in the shader,
      // so we can update them according to the user's input later
      m1loc = glGetUniformLocation(ourShader.ID, "mic1pos");
      m2loc = glGetUniformLocation(ourShader.ID, "mic2pos");
      m3loc = glGetUniformLocation(ourShader.ID, "mic3pos");
      m4loc = glGetUniformLocation(ourShader.ID, "mic4pos");
      m5loc = glGetUniformLocation(ourShader.ID, "mic5pos");
      m6loc = glGetUniformLocation(ourShader.ID, "mic6pos");
      m7loc = glGetUniformLocation(ourShader.ID, "mic7pos");
      m8loc = glGetUniformLocation(ourShader.ID, "mic8pos");
      smloc = glGetUniformLocation(ourShader.ID, "selectedMic");
      sbloc = glGetUniformLocation(ourShader.ID, "selectedBaseline");
      loloc = glGetUniformLocation(ourShader.ID, "lagoffsets");
      ascloc = glGetUniformLocation(ourShader.ID, "ampscales");
      ashloc = glGetUniformLocation(ourShader.ID, "ampshifts");

      // Initialise the uniform variables properly with values we have here
      // (even though they also get initialised in the shader code itself)
      glUniform3f(m1loc, micpos1[0], micpos1[1], micpos1[2]);
      glUniform3f(m2loc, micpos2[0], micpos2[1], micpos2[2]);
      glUniform3f(m3loc, micpos3[0], micpos3[1], micpos3[2]);
      glUniform3f(m4loc, micpos4[0], micpos4[1], micpos4[2]);
      glUniform3f(m5loc, micpos5[0], micpos5[1], micpos5[2]);
      glUniform3f(m6loc, micpos6[0], micpos6[1], micpos6[2]);
      glUniform3f(m7loc, micpos7[0], micpos7[1], micpos7[2]);
      glUniform3f(m8loc, micpos8[0], micpos8[1], micpos8[2]);
      glUniform1i(smloc, selectedMic);
      glUniform1i(sbloc, selectedBaseline);
      glUniform1fv(loloc, 28, lagoffsets);
      glUniform1fv(ascloc, 28, ampscales);
      glUniform1fv(ashloc, 28, ampshifts);

      // render loop
      // -----------
      while (!glfwWindowShouldClose(window))
      {
	if (gotConnection) {
          bytes_available = sock.available();
          while (bytes_available > 0) {
            if (gotConnection == false) break;
	    //std::cout << std::endl;
            //std::cout << "Bytes in RX buffer: " << bytes_available << std::endl;
            size_t len = sock.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);
	    //for (int i = 0; i < 4; i++) {
            //  std::cout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(uint8_t)recv_buf.data()[i] << " ";
  	    //}
            bytes_available = sock.available();

	    int baseline = -1;

	    if ((unsigned int)(uint8_t)recv_buf.data()[0] == (unsigned int)(uint8_t)recv_buf.data()[1] &&
	        (unsigned int)(uint8_t)recv_buf.data()[0] == (unsigned int)(uint8_t)recv_buf.data()[2] &&
	        (unsigned int)(uint8_t)recv_buf.data()[0] == (unsigned int)(uint8_t)recv_buf.data()[3] &&
	        (unsigned int)(uint8_t)recv_buf.data()[0] != 255) {
              baseline = (unsigned int)(uint8_t)recv_buf.data()[0];
	      //std::cout << "B" << baseline << " ";
	    } else {
	      std::cout << "Unknown packet detected! ";
	      std::cout << (unsigned int)(uint8_t)recv_buf.data()[0] << " " << (unsigned int)(uint8_t)recv_buf.data()[1] << " " << (unsigned int)(uint8_t)recv_buf.data()[2] << " " << (unsigned int)(uint8_t)recv_buf.data()[3] << std::endl;
	    }
      
	    //std::cout << std::endl;

            if (baseline != -1) {
	      minvals[baseline] = stdminvals[baseline];
  	      maxvals[baseline] = stdmaxvals[baseline];
	      if (selectedBaseline == -1 || selectedBaseline == baseline) { // FOR DEBUGGING
	      //if (true) { // FOR DEBUGGING
  	        for (int j = 5; j < NUMLAGS; j++) {
  	          lagvals[baseline][j] = 0.;
  	          lagvals[baseline][j] = lagvals[baseline][j] + (float)((unsigned int)(uint8_t)recv_buf.data()[j * 4 + 3] << 24);
  	          lagvals[baseline][j] = lagvals[baseline][j] + (float)((unsigned int)(uint8_t)recv_buf.data()[j * 4 + 2] << 16);
  	          lagvals[baseline][j] = lagvals[baseline][j] + (float)((unsigned int)(uint8_t)recv_buf.data()[j * 4 + 1] << 8);
  	          lagvals[baseline][j] = lagvals[baseline][j] + (float)((unsigned int)(uint8_t)recv_buf.data()[j * 4 + 0]);
	          //std::cout << lagvals[baseline][j] << " ";
  	          if (lagvals[baseline][j] > maxvals[baseline]) {
    	            maxvals[baseline] = lagvals[baseline][j];
  	            maxbin[baseline] = j;
  	          } 
  	          if (lagvals[baseline][j] < minvals[baseline] && lagvals[baseline][j] != 0) minvals[baseline] = lagvals[baseline][j];
  	        }
	      } else {
  	        for (int j = 5; j < NUMLAGS; j++) {
  	          lagvals[baseline][j] = 0.;
	          minvals[baseline] = 0;
	          maxvals[baseline] = 100;
	          maxbin[baseline] = 0;
  	        } 
	      }
	      //std::cout << minvals[baseline] << " " << maxvals[baseline] << std::endl;
  	      ranges[baseline] = maxvals[baseline] - minvals[baseline];
	      //if (maxvals[baseline] == 0) {
	      //  std::cout << "Warning: baseline " << baseline << " has max of zero!" << std::endl;
	      //}
	    }
	  }
	} else {
	  // Full max lag variables with placeholder data
	  for (int i = 0; i < 28; i++) {
	    maxbin[i] = NUMLAGS/2;
	    minvals[i] = -100000.;
	    maxvals[i] = 100000.;
	    ranges[i] = 200000.;
	    for (int j = 0; j < NUMLAGS; j++) {
	      lagvals[i][j] = 100000. * cos(10. * 3.141592654 * float(j - NUMLAGS/2) / float(NUMLAGS));
	    }
	  }
	}

        // input
        // -----
        processInput(window);
  
        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
  
        // bind Texture
        glBindTexture(GL_TEXTURE_2D, texture);
  
  	// Texture updates here
  	
  	for (int i = 0; i < 28; i++) {
  	  if (ranges[i] < 100000) ranges[i] = 100000;
          for (int j = 5; j < NUMLAGS; j++) {
            for (int k = 0; k < 2; k++) {
  	      // baseline i, lag j.
  	      // We use 2 pixelrows per baseline.
              // Experiment to see if we can just track the peak
  	      if (peakMode) {
                //int pv = 255 * (lagvals[i][j] - minvals[i]) / (ranges[i]);
  	        if (j == maxbin[i] && (i == selectedBaseline || selectedBaseline == -1)) {
  	          //pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3] = (unsigned char)(255);
  	          pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3] = 1.;
  	          pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3 + 1] = 0.;
  	          pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3 + 2] = 0.;
  	        } else {
  	          pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3] = 0.;
  	          pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3 + 1] = 0.;
  	          pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3 + 2] = 0.;
  	        }
              } else {
  	        // Use normal, full lag functions here
		if (selectedBaseline == -1 || i == selectedBaseline) {
		  if (autoScale) {
	            float pv = (lagvals[i][j] - minvals[i]) / (ranges[i]);
  	            pv < 0. ? pv = 0. : pv = pv;
  	            pv > 1. ? pv = 1. : pv = pv;
  	            pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3] = pv;
		  } else {
  	            pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3] = lagvals[i][j];
		  }
		} else {
	          pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3] = 0.;
		}
  	        pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3 + 1] = 0;
  	        pixels[i * 3 * NUMLAGS * 2 + k * NUMLAGS * 3 + j * 3 + 2] = 0;
  	      }
  	    }
  	  }
	  //std::cout << std::endl;
        }
  
  	// Upload the updated texture to the GPU
  	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, NUMLAGS, 64, 0, GL_RGB, GL_FLOAT, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
  
        // render container
        ourShader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
      }
  
      // optional: de-allocate all resources once they've outlived their purpose:
      // ------------------------------------------------------------------------
      glDeleteVertexArrays(1, &VAO);
      glDeleteBuffers(1, &VBO);
      glDeleteBuffers(1, &EBO);
  
      // glfw: terminate, clearing all previously allocated GLFW resources.
      // ------------------------------------------------------------------
      glfwTerminate();
      return 0;
    }
    catch (std::exception& e)
    {
      std::cerr << e.what() << std::endl;
    }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	peakMode = true;

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
	peakMode = false;

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
	if (jDown == false) {
          jDown = true;
	  std::cout << "Loading config JSON file..." << std::endl;
	  loadConfig();
	}
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_RELEASE) {
        if (jDown == true) {
	  jDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	if (eDown == false) {
          eDown = true;
	  std::cout << "Trying to set up ethernet connection..." << std::endl;
	  setupEthernetConnection(commandLineArgs);
	}
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) {
        if (eDown == true) {
	  eDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
	if (selectedMic == 0) {
	  micpos1[1] = micpos1[1] + 0.001f;
          glUniform3f(m1loc, micpos1[0], micpos1[1], micpos1[2]);
	} else if (selectedMic == 1) {
	  micpos2[1] = micpos2[1] + 0.001f;
          glUniform3f(m2loc, micpos2[0], micpos2[1], micpos2[2]);
	} else if (selectedMic == 2) {
	  micpos3[1] = micpos3[1] + 0.001f;
          glUniform3f(m3loc, micpos3[0], micpos3[1], micpos3[2]);
	} else if (selectedMic == 3) {
	  micpos4[1] = micpos4[1] + 0.001f;
          glUniform3f(m4loc, micpos4[0], micpos4[1], micpos4[2]);
	} else if (selectedMic == 4) {
	  micpos5[1] = micpos5[1] + 0.001f;
          glUniform3f(m5loc, micpos5[0], micpos5[1], micpos5[2]);
	} else if (selectedMic == 5) {
	  micpos6[1] = micpos6[1] + 0.001f;
          glUniform3f(m6loc, micpos6[0], micpos6[1], micpos6[2]);
	} else if (selectedMic == 6) {
	  micpos7[1] = micpos7[1] + 0.001f;
          glUniform3f(m7loc, micpos7[0], micpos7[1], micpos7[2]);
	} else if (selectedMic == 7) {
	  micpos8[1] = micpos8[1] + 0.001f;
          glUniform3f(m8loc, micpos8[0], micpos8[1], micpos8[2]);
	}
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
	if (selectedMic == 0) {
	  micpos1[1] = micpos1[1] - 0.001f;
          glUniform3f(m1loc, micpos1[0], micpos1[1], micpos1[2]);
	} else if (selectedMic == 1) {
	  micpos2[1] = micpos2[1] - 0.001f;
          glUniform3f(m2loc, micpos2[0], micpos2[1], micpos2[2]);
	} else if (selectedMic == 2) {
	  micpos3[1] = micpos3[1] - 0.001f;
          glUniform3f(m3loc, micpos3[0], micpos3[1], micpos3[2]);
	} else if (selectedMic == 3) {
	  micpos4[1] = micpos4[1] - 0.001f;
          glUniform3f(m4loc, micpos4[0], micpos4[1], micpos4[2]);
	} else if (selectedMic == 4) {
	  micpos5[1] = micpos5[1] - 0.001f;
          glUniform3f(m5loc, micpos5[0], micpos5[1], micpos5[2]);
	} else if (selectedMic == 5) {
	  micpos6[1] = micpos6[1] - 0.001f;
          glUniform3f(m6loc, micpos6[0], micpos6[1], micpos6[2]);
	} else if (selectedMic == 6) {
	  micpos7[1] = micpos7[1] - 0.001f;
          glUniform3f(m7loc, micpos7[0], micpos7[1], micpos7[2]);
	} else if (selectedMic == 7) {
	  micpos8[1] = micpos8[1] - 0.001f;
          glUniform3f(m8loc, micpos8[0], micpos8[1], micpos8[2]);
	}
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
	if (selectedMic == 0) {
	  micpos1[0] = micpos1[0] - 0.001f;
          glUniform3f(m1loc, micpos1[0], micpos1[1], micpos1[2]);
	} else if (selectedMic == 1) {
	  micpos2[0] = micpos2[0] - 0.001f;
          glUniform3f(m2loc, micpos2[0], micpos2[1], micpos2[2]);
	} else if (selectedMic == 2) {
	  micpos3[0] = micpos3[0] - 0.001f;
          glUniform3f(m3loc, micpos3[0], micpos3[1], micpos3[2]);
	} else if (selectedMic == 3) {
	  micpos4[0] = micpos4[0] - 0.001f;
          glUniform3f(m4loc, micpos4[0], micpos4[1], micpos4[2]);
	} else if (selectedMic == 4) {
	  micpos5[0] = micpos5[0] - 0.001f;
          glUniform3f(m5loc, micpos5[0], micpos5[1], micpos5[2]);
	} else if (selectedMic == 5) {
	  micpos6[0] = micpos6[0] - 0.001f;
          glUniform3f(m6loc, micpos6[0], micpos6[1], micpos6[2]);
	} else if (selectedMic == 6) {
	  micpos7[0] = micpos7[0] - 0.001f;
          glUniform3f(m7loc, micpos7[0], micpos7[1], micpos7[2]);
	} else if (selectedMic == 7) {
	  micpos8[0] = micpos8[0] - 0.001f;
          glUniform3f(m8loc, micpos8[0], micpos8[1], micpos8[2]);
	}
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
	if (selectedMic == 0) {
	  micpos1[0] = micpos1[0] + 0.001f;
          glUniform3f(m1loc, micpos1[0], micpos1[1], micpos1[2]);
	} else if (selectedMic == 1) {
	  micpos2[0] = micpos2[0] + 0.001f;
          glUniform3f(m2loc, micpos2[0], micpos2[1], micpos2[2]);
	} else if (selectedMic == 2) {
	  micpos3[0] = micpos3[0] + 0.001f;
          glUniform3f(m3loc, micpos3[0], micpos3[1], micpos3[2]);
	} else if (selectedMic == 3) {
	  micpos4[0] = micpos4[0] + 0.001f;
          glUniform3f(m4loc, micpos4[0], micpos4[1], micpos4[2]);
	} else if (selectedMic == 4) {
	  micpos5[0] = micpos5[0] + 0.001f;
          glUniform3f(m5loc, micpos5[0], micpos5[1], micpos5[2]);
	} else if (selectedMic == 5) {
	  micpos6[0] = micpos6[0] + 0.001f;
          glUniform3f(m6loc, micpos6[0], micpos6[1], micpos6[2]);
	} else if (selectedMic == 6) {
	  micpos7[0] = micpos7[0] + 0.001f;
          glUniform3f(m7loc, micpos7[0], micpos7[1], micpos7[2]);
	} else if (selectedMic == 7) {
	  micpos8[0] = micpos8[0] + 0.001f;
          glUniform3f(m8loc, micpos8[0], micpos8[1], micpos8[2]);
	}
    }

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
	if (zDown == false) {
          // First press, do something here
          zDown = true;
	  selectedMic = selectedMic - 1;
	  if (selectedMic < -1) selectedMic = 7;
	  glUniform1i(smloc, selectedMic);
	  std::cout << " Selected mic number " << selectedMic + 1 << std::endl;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE) {
        if (zDown == true) {
	  // First release, do something here
	  zDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
	if (xDown == false) {
          // First press, do something here
          xDown = true;
	  selectedMic = selectedMic + 1;
	  if (selectedMic > 7) selectedMic = -1;
	  glUniform1i(smloc, selectedMic);
	  std::cout << " Selected mic number " << selectedMic + 1 << std::endl;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE) {
        if (xDown == true) {
	  // First release, do something here
	  xDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
	if (cDown == false) {
          // First press, do something here
          cDown = true;
	  selectedBaseline = selectedBaseline - 1;
	  if (selectedBaseline < -1) selectedBaseline = 27;
	  glUniform1i(sbloc, selectedBaseline);
	  std::cout << " Selected baseline number " << selectedBaseline << std::endl;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        if (cDown == true) {
	  // First release, do something here
	  cDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
	if (vDown == false) {
          // First press, do something here
          vDown = true;
	  selectedBaseline = selectedBaseline + 1;
	  if (selectedBaseline > 27) selectedBaseline = -1;
	  glUniform1i(sbloc, selectedBaseline);
	  std::cout << " Selected baseline number " << selectedBaseline << std::endl;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) {
        if (vDown == true) {
	  // First release, do something here
	  vDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
	if (bDown == false) {
          // First press, do something here
          bDown = true;
	  lagoffsets[selectedBaseline] = fmod(lagoffsets[selectedBaseline] - 1., float(NUMLAGS + 1));
	  glUniform1fv(loloc, 28, lagoffsets);
	  std::cout << "Changed baseline " << selectedBaseline + 1 << " lag offset to " << lagoffsets[selectedBaseline] << std::endl;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE) {
        if (bDown == true) {
	  // First release, do something here
	  bDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
	if (nDown == false) {
          // First press, do something here
          nDown = true;
	  lagoffsets[selectedBaseline] = fmod(lagoffsets[selectedBaseline] + 1., float(NUMLAGS + 1));
	  glUniform1fv(loloc, 28, lagoffsets);
	  std::cout << "Changed baseline " << selectedBaseline + 1 << " lag offset to " << lagoffsets[selectedBaseline] << std::endl;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE) {
        if (nDown == true) {
	  // First release, do something here
	  nDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
	if (mDown == false) {
          // First press, do something here
          mDown = true;
	  ampSelected = !ampSelected;
	  if (ampSelected) std::cout << "Now toggled to amplitude scale editing" << std::endl;
	  else std::cout << "Now toggled to amplitude offset editing" << std::endl;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
        if (mDown == true) {
	  // First release, do something here
	  mDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) {
	if (commaDown == false) {
          // First press, do something here
          commaDown = true;
	  if (ampSelected) {
            //Change the amplitude scale down
	    if (selectedBaseline != -1) {
              ampscales[selectedBaseline] = ampscales[selectedBaseline] / 1.01;
	      std::cout << "Amp scale for baseline " << selectedBaseline << " reduced to " << ampscales[selectedBaseline] << std::endl;
	    } else {
	      for (int i = 0; i < 28; i++) {
	        ampscales[i] = ampscales[i] / 1.01;
	      }
	      std::cout << "Adjusted amplitude scale for ALL baselines down by 1%" << std::endl;
	    }
	  } else {
            // Change the amplitude offset down
	    if (selectedBaseline != -1) {
	      ampshifts[selectedBaseline] = ampshifts[selectedBaseline] - 0.01;
	      std::cout << "Amp offset for baseline " << selectedBaseline << " reduced to " << ampshifts[selectedBaseline] << std::endl;
	    } else {
	      for (int i = 0; i < 28; i++) {
	        ampshifts[i] = ampshifts[i] - 0.01;
	      }
	      std::cout << "Adjusted amplitude shift for ALL baselines down by 0.01" << std::endl;
	    }
	  }
	  glUniform1fv(ascloc, 28, ampscales);
          glUniform1fv(ashloc, 28, ampshifts);
	}
    }

    if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_RELEASE) {
        if (commaDown == true) {
	  // First release, do something here
	  commaDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) {
	if (periodDown == false) {
          // First press, do something here
          periodDown = true;
	  if (ampSelected) {
            //Change the amplitude scale down
	    if (selectedBaseline != -1) {
              ampscales[selectedBaseline] = ampscales[selectedBaseline] * 1.01;
	      std::cout << "Amp scale for baseline " << selectedBaseline << " increased to " << ampscales[selectedBaseline] << std::endl;
	    } else {
	      for (int i = 0; i < 28; i++) {
	        ampscales[i] = ampscales[i] * 1.01;
	      }
	      std::cout << "Adjusted amplitude scale for ALL baselines up by 1%" << std::endl;
	    }
	  } else {
            // Change the amplitude offset down
	    if (selectedBaseline != -1) {
	      ampshifts[selectedBaseline] = ampshifts[selectedBaseline] + 0.01;
	      std::cout << "Amp offset for baseline " << selectedBaseline << " increased to " << ampshifts[selectedBaseline] << std::endl;
	    } else {
	      for (int i = 0; i < 28; i++) {
	        ampshifts[i] = ampshifts[i] + 0.01;
	      }
	      std::cout << "Adjusted amplitude shift for ALL baselines up by 0.01" << std::endl;
	    }
	  }
	  glUniform1fv(ascloc, 28, ampscales);
          glUniform1fv(ashloc, 28, ampshifts);
	}
    }

    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_RELEASE) {
        if (periodDown == true) {
	  // First release, do something here
	  periodDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
	if (slashDown == false) {
          // First press, do something here
          slashDown = true;
	  autoScale = !autoScale;
	  std::cout << "Autoscaling toggled to " << autoScale << std::endl;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_RELEASE) {
        if (slashDown == true) {
	  // First release, do something here
	  slashDown = false;
	}
    }

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
	if (lDown == false) {
          // First press, do something here
          lDown = true;
	  // Output all configurable settings
	  std::cout << "Mic pos 1: " << micpos1[0] << " " << micpos1[1] << " " << micpos1[2] << std::endl;
	  std::cout << "Mic pos 2: " << micpos2[0] << " " << micpos2[1] << " " << micpos2[2] << std::endl;
	  std::cout << "Mic pos 3: " << micpos3[0] << " " << micpos3[1] << " " << micpos3[2] << std::endl;
	  std::cout << "Mic pos 4: " << micpos4[0] << " " << micpos4[1] << " " << micpos4[2] << std::endl;
	  std::cout << "Mic pos 5: " << micpos5[0] << " " << micpos5[1] << " " << micpos5[2] << std::endl;
	  std::cout << "Mic pos 6: " << micpos6[0] << " " << micpos6[1] << " " << micpos6[2] << std::endl;
	  std::cout << "Mic pos 7: " << micpos7[0] << " " << micpos7[1] << " " << micpos7[2] << std::endl;
	  std::cout << "Mic pos 8: " << micpos8[0] << " " << micpos8[1] << " " << micpos8[2] << std::endl;
	  for (int i = 0; i < 28; i++) {
	    std::cout << "Lag offset " << std::setw(2) << i << ": " << lagoffsets[i] << std::endl;
	  }
	  for (int i = 0; i < 28; i++) {
	    std::cout << "Amp scale " << std::setw(5) << i << ": " << ampscales[i] << std::endl;
	  }
	  for (int i = 0; i < 28; i++) {
	    std::cout << "Amp offset " << std::setw(5) << i << ": " << ampshifts[i] << std::endl;
	  }
	}
    }

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
        if (lDown == true) {
	  // First release, do something here
	  lDown = false;
	}
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

