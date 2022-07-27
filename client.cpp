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

#include <cmath>

using namespace std;
using namespace boost;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

std::string uchar2hex(unsigned char inchar)
{
  std::ostringstream oss (std::ostringstream::out);
  oss << std::setw(2) << std::setfill('0') << std::hex << (int)(inchar);
  return oss.str();
}

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lag functions", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
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
    Shader ourShader("basic-opengl.vs", "basic-opengl.fs"); 

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    
    // Write new routine where we draw a 'wedge', to be used for a linear mic array. 
    // Using an uneven number of lags (so we have a zero lag), first determine the
    // range of angles in our wedge. Use this to calculate the vertex coordinates.
    int numlags = 59 // zero lag is lag number 29
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
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    unsigned char *data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);

    // Experiment here with manual texture data specs
    //unsigned char pixels[] = {
    //	    255, 0, 0,  0, 255, 0,  255, 0, 0,  0, 255, 0,
    //	    0, 255, 0,  255, 0, 0,  0, 255, 0,  255, 0, 0,
    //	    255, 0, 0,  0, 255, 0,  255, 0, 0,  0, 255, 0,
    //	    0, 255, 0,  255, 0, 0,  0, 255, 0,  255, 0, 0,
    //};
    unsigned char pixels[12288];
    for (int i = 0; i < 12288; i++) {
        //pixels[i] = rand() % 256;
        pixels[i] = 0;
    }

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    //////////////////////////////////////
    // Serial connection stuff comes below
    //////////////////////////////////////

    asio::io_service io;
    asio::serial_port port(io);

    port.open("/dev/tty.usbserial-FT1V75R11");
    termios t;
    int fd = port.native_handle();

    // Set the baud rate manually here
    if (tcgetattr(fd, &t) < 0) { /* handle error */ }
    if (cfsetspeed(&t, 921600) < 0) { /* handle error */ }
    if (tcsetattr(fd, TCSANOW, &t) < 0) { /* handle error */ }

    // First, let's sync with the packet train we receive
    bool synced = false;
    char tmp;
    int state = 0;
    char lookingfor[] = {68, 67, 66, 65};
    while (!synced) {
        asio::read(port, asio::buffer(&tmp, 1));
        if (tmp == lookingfor[state]) {
	    state++;
	} else {
	    state = 0;
	}
	if (state == 4) synced = true;
    }
    char tmp2[252 + 5 * 256];
    asio::read(port, asio::buffer(&tmp2, 252 + 5 * 256));

    unsigned char buf[1536];

    int lagvals[6][64];
    for (int i = 0; i < 6; i++) {
      for (int j = 0; j < 63; j++) {
        lagvals[i][j] = 0;
      }
    }

    int minvals[6];
    int ranges[6];
    int maxbin[6];

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // bind Texture
        glBindTexture(GL_TEXTURE_2D, texture);

	// Get serial lag data, scale it, and store it
        asio::read(port, asio::buffer(&buf, 1536));
	std::cout << uchar2hex(buf[0]) << " " << uchar2hex(buf[1]) << " " << uchar2hex(buf[2]) << " " << uchar2hex(buf[3]) << std::endl;
	for (int i = 0; i < 6; i++) {
          int minval = 10000000;
	  int maxval = 0;
	  for (int j = 3; j < 63; j++) {
	    lagvals[i][j] = 0;
	    lagvals[i][j] = lagvals[i][j] | buf[i * 4 * 64 + j * 4 + 3] << 24;
	    lagvals[i][j] = lagvals[i][j] | buf[i * 4 * 64 + j * 4 + 2] << 16;
	    lagvals[i][j] = lagvals[i][j] | buf[i * 4 * 64 + j * 4 + 1] << 8;
	    lagvals[i][j] = lagvals[i][j] | buf[i * 4 * 64 + j * 4];
	    lagvals[i][j] = 2 * sqrt(lagvals[i][j]);
	    //cout << lagvals[i][j] << " ";
	    if (lagvals[i][j] > maxval) {
		   maxval = lagvals[i][j];
		   maxbin[i] = j;
	    }
	    if (lagvals[i][j] < minval && lagvals[i][j] != 0) minval = lagvals[i][j];
	  }
	  minvals[i] = minval;
	  ranges[i] = maxval - minval;
	  if (ranges[i] < 50) ranges[i] = 50;
	  //std::cout << std::endl << i << " " << minval << " " << maxval << " " << ranges[i] << std::endl;
	}
	//std::cout << endl;

	// Texture updates here
	
	for (int i = 0; i < 6; i++) {
            for (int j = 3; j < 63; j++) {
	      //std::cout << ((lagvals[i][j] - minvals[i]) * 255 / ranges[i]) << " ";
              for (int k = 0; k < 8; k++) {
	        //baseline i, lag j, pixel row k. Skip first 8 rows.
		if (j != maxbin[i]) {
		  pixels[3 * 64 * 8 + i * 3 * 64 * 8 + k * 64 * 3 + j * 3] = (unsigned char)((lagvals[i][j] - minvals[i]) * 255 / ranges[i]);
		  pixels[3 * 64 * 8 + i * 3 * 64 * 8 + k * 64 * 3 + j * 3 + 1] = 0;
		  pixels[3 * 64 * 8 + i * 3 * 64 * 8 + k * 64 * 3 + j * 3 + 2] = 0;
		} else {
		  pixels[3 * 64 * 8 + i * 3 * 64 * 8 + k * 64 * 3 + j * 3] = 0;
		  pixels[3 * 64 * 8 + i * 3 * 64 * 8 + k * 64 * 3 + j * 3 + 1] = 255;
		  pixels[3 * 64 * 8 + i * 3 * 64 * 8 + k * 64 * 3 + j * 3 + 2] = 0;
		}
	      }
	    }
	    //std::cout << std::endl;
        }

	// Upload the updated texture to the GPU
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
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

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

