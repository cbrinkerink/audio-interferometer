#include "glad.h"
#include "glfw3.h"
#include "stb_image.h"

#include "shader_s.h"

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include <boost/asio.hpp> 
#include <termios.h>

#include <boost/array.hpp>
#include <unistd.h>
#include <boost/lexical_cast.hpp>

#include <cmath>

using namespace std;
using namespace boost;
using boost::asio::ip::udp;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// Ugly global variables for key presses
bool peakMode = true;
char selectedBaseline = 0;

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

std::string uchar2hex(unsigned char inchar)
{
  std::ostringstream oss (std::ostringstream::out);
  oss << std::setw(2) << std::setfill('0') << std::hex << (int)(inchar);
  return oss.str();
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

      // Initialise ethernet necessities
      boost::asio::io_service io_service;
      udp::endpoint local_endpoint = boost::asio::ip::udp::endpoint(
  		    boost::asio::ip::address::from_string(argv[1]),
  		    boost::lexical_cast<int>(argv[2]));
      std::cout << "Local bind " << local_endpoint << std::endl;

      udp::socket socket(io_service);
      socket.open(udp::v4());
      socket.bind(local_endpoint);

      boost::array<char, 512> recv_buf; // Current buffer size: 1 baseline, 128 lags, 4 bytes per lag
      udp::endpoint sender_endpoint;
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
  
      // build and compile our shader zprogram
      // ------------------------------------
      Shader ourShader("client.vs", "client.fs"); 
  
      // set up vertex data (and buffer(s)) and configure vertex attributes
      // ------------------------------------------------------------------
      
      // Write new routine where we draw a 'wedge', to be used for a linear mic array. 
      // Using an uneven number of lags (so we have a zero lag), first determine the
      // range of angles in our wedge. Use this to calculate the vertex coordinates.
      //
      // Issue: for different baselines, a fixed number of lags will correspond to a
      // different range of angles! A short baseline covers a given range of angles
      // with fewer lags, or conversely a given range of lags covers a larger angle
      // range for a shorter baseline.
      // So, if we use the baseline lengths as a given, we can calculate the supported
      // lag range for that baseline. We can compare that to the lag range available,
      // and pick our displayed range of angles accordingly.
      // For 59 lags, spanning from -29 to 29, and using a sound speed of 343 m/s,
      // with a sampling frequency of 46875 Hz, the range of lags covers the full
      // range of angles for baselines up to 21.2 cm.
      
      // Completely different idea: I could also use a similar visualisation as the
      // one I use in the interferometry Shadertoy page. It shows antennas in a 2D
      // plane, with nested hyperbolae highlighting where the source (also shown)
      // can possibly lie based on the phase differences measured per baseline.
      // With this shader, I can colour each pixel by the correlated value for that
      // particular location/lagbin. This also allows me to use 2D microphone arrays,
      // as long as I limit it to considering a single plane only.
      // For this, I need to edit the shader rather than the code for the vertices.
      // I need to add the microphone positions as uniforms, as well as the speed of
      // sound and the sample rate.
      
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
      int width, height, nrChannels;
      unsigned char *data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
  
      unsigned char pixels[3 * 128 * 64];
      for (int i = 0; i < 3 * 128 * 64; i++) {
          pixels[i] = 0;
      }
  
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
      glGenerateMipmap(GL_TEXTURE_2D);
  
      stbi_image_free(data);
  
      unsigned int lagvals[15][128];
      for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 128; j++) {
          lagvals[i][j] = 0;
        }
      }
  
      int stdminvals[15] = {100000000,
  	                    100000000,
  	      	            100000000,
  		            100000000,
  		            100000000,
  		            100000000,
  		            100000000,
  		            100000000,
  		            100000000,
  		            100000000,
  		            100000000,
  		            100000000,
  		            100000000,
  		            100000000,
  		            100000000};
      // For use in peak tracking		         
      int stdmaxvals[15] = {0,
  	                    0,
  		            0,
  		            0,
  		            0,
  		            0,
  		            0,
  		            0,
  		            0,
  		            0,
  		            0,
  		            0,
  		            0,
  		            0,
  		            0};
      int minvals[15];
      int maxvals[15];
      int ranges[15];
      int maxbin[15];
  
      // render loop
      // -----------
      while (!glfwWindowShouldClose(window))
      {
        bytes_available = socket.available();
        while (bytes_available > 0) {
	  //std::cout << std::endl;
          //std::cout << "Bytes in RX buffer: " << bytes_available << std::endl;
          size_t len = socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);
	  //for (int i = 0; i < 4; i++) {
          //  std::cout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(uint8_t)recv_buf.data()[i] << " ";
  	  //}
          bytes_available = socket.available();

	  int baseline = -1;

	  if ((unsigned int)(uint8_t)recv_buf.data()[0] == 68 &&
	      (unsigned int)(uint8_t)recv_buf.data()[1] == 67 && 
	      (unsigned int)(uint8_t)recv_buf.data()[2] == 66 && 
	      (unsigned int)(uint8_t)recv_buf.data()[3] == 65) {
            baseline = 0;
	    std::cout << "B1 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 72 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 71 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 70 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 69) {
            baseline = 1;
	    std::cout << "B2 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 76 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 75 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 74 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 73) {
            baseline = 2;
	    std::cout << "B3 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 80 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 79 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 78 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 77) {
            baseline = 3;
	    std::cout << "B4 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 84 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 83 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 82 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 81) {
            baseline = 4;
	    std::cout << "B5 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 88 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 87 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 86 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 85) {
            baseline = 5;
	    std::cout << "B6 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 57 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 57 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 57 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 57) {
            baseline = 6;
	    std::cout << "B7 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 49 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 49 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 49 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 49) {
            baseline = 7;
	    std::cout << "B8 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 50 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 50 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 50 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 50) {
            baseline = 8;
	    std::cout << "B9 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 51 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 51 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 51 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 51) {
            baseline = 9;
	    std::cout << "B10 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 52 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 52 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 52 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 52) {
            baseline = 10;
	    std::cout << "B11 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 53 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 53 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 53 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 53) {
            baseline = 11;
	    std::cout << "B12 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 54 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 54 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 54 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 54) {
            baseline = 12;
	    std::cout << "B13 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 55 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 55 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 55 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 55) {
            baseline = 13;
	    std::cout << "B14 ";
	  } else if ((unsigned int)(uint8_t)recv_buf.data()[0] == 56 &&
	             (unsigned int)(uint8_t)recv_buf.data()[1] == 56 && 
	             (unsigned int)(uint8_t)recv_buf.data()[2] == 56 && 
	             (unsigned int)(uint8_t)recv_buf.data()[3] == 56) {
            baseline = 14;
	    std::cout << "B15 ";
	  }
      
          if (baseline != -1) {
	    minvals[baseline] = stdminvals[baseline];
  	    maxvals[baseline] = stdmaxvals[baseline];
	    std::cout << baseline << " ";
	    //if (selectedBaseline == baseline) { // FOR DEBUGGING
	    if (true) { // FOR DEBUGGING
  	      for (int j = 3; j < 128; j++) {
  	        lagvals[baseline][j] = 0;
  	        lagvals[baseline][j] = lagvals[baseline][j] | (unsigned int)(uint8_t)recv_buf.data()[j * 4 + 3] << 24;
  	        lagvals[baseline][j] = lagvals[baseline][j] | (unsigned int)(uint8_t)recv_buf.data()[j * 4 + 2] << 16;
  	        lagvals[baseline][j] = lagvals[baseline][j] | (unsigned int)(uint8_t)recv_buf.data()[j * 4 + 1] << 8;
  	        lagvals[baseline][j] = lagvals[baseline][j] | (unsigned int)(uint8_t)recv_buf.data()[j * 4 + 0];
	        //std::cout << lagvals[baseline][j] << " ";
  	        if (lagvals[baseline][j] > maxvals[baseline]) {
    	          maxvals[baseline] = lagvals[baseline][j];
  	          maxbin[baseline] = j;
  	        } 
  	        if (lagvals[baseline][j] < minvals[baseline] && lagvals[baseline][j] != 0) minvals[baseline] = lagvals[baseline][j];
  	      }
	    } else {
  	      for (int j = 3; j < 128; j++) {
  	        lagvals[baseline][j] = 0;
	        minvals[baseline] = 0;
	        maxvals[baseline] = 100;
	        maxbin[baseline] = 0;
  	      } 
	    }
	    //std::cout << minvals[baseline] << " " << maxvals[baseline] << std::endl;
  	    ranges[baseline] = maxvals[baseline] - minvals[baseline];
	  }
	}

	std::cout << std::endl;

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
  	
  	for (int i = 0; i < 15; i++) {
  	  if (ranges[i] < 10000) ranges[i] = 10000;
          for (int j = 3; j < 128; j++) {
	    //std::cout << lagvals[i][j] << " " << minvals[i] << " " << ranges[i] << " ";
	    //std::cout.flush();
	    //std::cout << 255 * (lagvals[i][j] - minvals[i]) / (ranges[i]) << " ";
	    //std::cout.flush();
            for (int k = 0; k < 4; k++) {
  	      // baseline i, lag j, pixel row k. Skip first 2 rows of texture.
  	      // We use 4 pixelrows per baseline.
  	      
                // Experiment to see if we can just track the peak
  	      if (peakMode) {
                //int pv = 255 * (lagvals[i][j] - minvals[i]) / (ranges[i]);
  	        if (j == maxbin[i]) {
  	          pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3] = (unsigned char)(255);
  	          pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 1] = 0;
  	          pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 2] = 0;
  	        } else {
  	          pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3] = 0;
  	          pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 1] = 0;
  	          pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 2] = 0;
  	        }
              } else {
  	        // Use normal, full lag functions here
                int pv = 255 * (lagvals[i][j] - minvals[i]) / (ranges[i]);
  	        pv < 0 ? pv = 0 : pv = pv;
  	        pv > 255 ? pv = 255 : pv = pv;
	        //if (k == 0) std::cout << std::setw(3) << pv << " ";
  	        pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3] = (unsigned char)pv;
  	        pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 1] = 0;
  	        pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 2] = 0;
  	      }
  	    }
  	  }
	  //std::cout << std::endl;
        }
  
  	// Upload the updated texture to the GPU
  	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
  	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
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

    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
	selectedBaseline = 0;

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	selectedBaseline = 1;

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	selectedBaseline = 2;

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	selectedBaseline = 3;

    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
	selectedBaseline = 4;

    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
	selectedBaseline = 5;

    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
	selectedBaseline = 6;

    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
	selectedBaseline = 7;

    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS)
	selectedBaseline = 8;

    if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS)
	selectedBaseline = 9;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	selectedBaseline = 10;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	selectedBaseline = 11;

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	selectedBaseline = 12;

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
	selectedBaseline = 13;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	selectedBaseline = 14;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

