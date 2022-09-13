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

// Ugly global variables for key presses
bool peakMode = true;

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
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
    unsigned char pixels[3 * 128 * 64];
    for (int i = 0; i < 3 * 128 * 64; i++) {
        //pixels[i] = rand() % 256;
        pixels[i] = 0;
    }

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

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
    //char lookingfor[] = {68, 67, 66, 65};
    char lookingfor[] = {68, 67, 66};
    while (!synced) {
        asio::read(port, asio::buffer(&tmp, 1));
        if (tmp == lookingfor[state]) {
	    state++;
	} else {
	    state = 0;
	}
	//if (state == 4) synced = true;
	if (state == 3) synced = true;
    }
    //char tmp2[252 + 14 * 256];
    char tmp2[381 + 14 * 384];
    //asio::read(port, asio::buffer(&tmp2, 252 + 14 * 256));
    asio::read(port, asio::buffer(&tmp2, 381 + 14 * 384));

    //unsigned char buf[15 * 256];
    unsigned char buf[15 * 384];

    int lagvals[15][128];
    for (int i = 0; i < 15; i++) {
      for (int j = 0; j < 128; j++) {
        lagvals[i][j] = 0;
      }
    }

    //int stdminvals[15] = {53800000,
//	                 53800000,
//	      	         53300000,
//		         54100000,
//		         54800000,
//		         54600000,
//		         20000000,
//		         20000000,
//		         20000000,
//		         20000000,
//		         20000000,
//		         20000000,
//		         20000000,
//		         20000000,
//		         20000000};
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
    // For use in full lag function mapping
//    int stdmaxvals[6] = {4000000,
//	                 4200000,
// 	                 3700000,
//		         4500000,
//		         5200000,
//		         4800000};
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
        //asio::read(port, asio::buffer(&buf, 15 * 256));
        asio::read(port, asio::buffer(&buf, 15 * 384));
	//std::cout << uchar2hex(buf[0]) << " " << uchar2hex(buf[1]) << " " << uchar2hex(buf[2]) << " " << uchar2hex(buf[3]) << std::endl;
	//std::cout << uchar2hex(buf[0]) << " " << uchar2hex(buf[1]) << " " << uchar2hex(buf[2]) << std::endl;
	for (int i = 0; i < 15; i++) {
	  minvals[i] = stdminvals[i];
	  maxvals[i] = stdmaxvals[i];
	  cout << i << " ";
	  for (int j = 3; j < 128; j++) {
	    lagvals[i][j] = 0;
	    //lagvals[i][j] = lagvals[i][j] | buf[i * 4 * 64 + j * 4 + 3] << 24;
	    //lagvals[i][j] = lagvals[i][j] | buf[i * 4 * 64 + j * 4 + 2] << 16;
	    //lagvals[i][j] = lagvals[i][j] | buf[i * 4 * 64 + j * 4 + 1] << 8;
	    //lagvals[i][j] = lagvals[i][j] | buf[i * 4 * 64 + j * 4];
	    lagvals[i][j] = lagvals[i][j] | buf[i * 3 * 128 + j * 3 + 2] << 16;
	    lagvals[i][j] = lagvals[i][j] | buf[i * 3 * 128 + j * 3 + 1] << 8;
	    lagvals[i][j] = lagvals[i][j] | buf[i * 3 * 128 + j * 3];
	    //lagvals[i][j] = sqrt(lagvals[i][j]);
	    cout << lagvals[i][j] << " ";
	    if (lagvals[i][j] > maxvals[i]) {
		   maxvals[i] = lagvals[i][j];
		   maxbin[i] = j;
	    } 
	    if (lagvals[i][j] < minvals[i] && lagvals[i][j] != 0) minvals[i] = lagvals[i][j];
	  }
	  cout << std::endl;
	  ranges[i] = maxvals[i] - minvals[i];
	  if (ranges[i] < 100000) ranges[i] = 100000;
	  //std::cout << std::endl << i << " " << minval << " " << maxval << " " << ranges[i] << std::endl;
	}
	//std::cout << endl;

	// Texture updates here
	
	for (int i = 0; i < 15; i++) {
            for (int j = 3; j < 128; j++) {
              for (int k = 0; k < 4; k++) {
	        // baseline i, lag j, pixel row k. Skip first 2 rows of texture.
		// We use 4 lines per baseline.
		
                // Experiment to see if we can just track the peak
		if (peakMode) {
		  if (j == maxbin[i]) {
		    //pixels[3 * 64 * 2 + i * 3 * 64 * 4 + k * 64 * 3 + j * 3] = (unsigned char)(255 * (lagvals[i][j] - minvals[i]) / (ranges[i]));
		    //pixels[3 * 64 * 2 + i * 3 * 64 * 4 + k * 64 * 3 + j * 3 + 1] = 0;
		    //pixels[3 * 64 * 2 + i * 3 * 64 * 4 + k * 64 * 3 + j * 3 + 2] = 0;
		    //pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3] = (unsigned char)(255 * (lagvals[i][j] - minvals[i]) / (ranges[i]));
		    pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3] = (unsigned char)(255);
		    pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 1] = 0;
		    pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 2] = 0;
		  } else {
		    //pixels[3 * 64 * 2 + i * 3 * 64 * 4 + k * 64 * 3 + j * 3] = 0;
		    //pixels[3 * 64 * 2 + i * 3 * 64 * 4 + k * 64 * 3 + j * 3 + 1] = 0;
		    //pixels[3 * 64 * 2 + i * 3 * 64 * 4 + k * 64 * 3 + j * 3 + 2] = 0;
		    pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3] = 0;
		    pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 1] = 0;
		    pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 2] = 0;
		  }
                } else {
		  // Use normal, full lag functions here
                  int pv = 255 * (lagvals[i][j] - minvals[i]) / (ranges[i]);
		  //pv = pv - 3 * (255 - pv);
		  pv < 0 ? pv = 0 : pv = pv;
		  pv > 255 ? pv = 255 : pv = pv;
		  //pixels[3 * 64 * 2 + i * 3 * 64 * 4 + k * 64 * 3 + j * 3] = (unsigned char)pv;
		  //pixels[3 * 64 * 2 + i * 3 * 64 * 4 + k * 64 * 3 + j * 3 + 1] = 0;
		  //pixels[3 * 64 * 2 + i * 3 * 64 * 4 + k * 64 * 3 + j * 3 + 2] = 0;
		  pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3] = (unsigned char)pv;
		  pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 1] = 0;
		  pixels[3 * 128 * 2 + i * 3 * 128 * 4 + k * 128 * 3 + j * 3 + 2] = 0;
		}
	      }
	    }
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

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

