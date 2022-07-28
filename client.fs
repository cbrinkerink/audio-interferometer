#version 330 core

// This fragment shader is to be expanded: we will include microphone positions,
// and calculate the lag value for every baseline per pixel. We then look up the
// corresponding correlation value for that baseline and multiply all the values
// together. This should give us a 2D sound map.
// To be done:
// - Add and initialise 2D microphone positions as uniforms
// - Rework texture1 so that it contains lag values in a clear order
// - Add code to calculate lag value for every baseline
// - Translate lag to texture coords for every baseline

out vec4 FragColor;
in vec3 ourColor;
in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;

// Microphone positions
// Offset from mic1: 0.073, 0.065
vec3 mic1pos = vec3(-0.073, -0.065, 0.);
vec3 mic2pos = vec3(0.073, -0.065, 0.);
vec3 mic3pos = vec3(-0.038, 0.065, 0.);
vec3 mic4pos = vec3(0.04, 0.065, 0.);

float pixelscale = 600.; // Pixels per meter
// NOTE: On retina screens this is apparently different by a factor of 2:
// my laptop has an apparent window size that is twice the numbers here.
// To correctly calculate the middle of the window, I have to multiply the
// coords for the middle of the window (400,300 in this example) by 2.
float windowWidth = 800.;
float windowHeight = 600.;
float soundspeed = 343.;
float samplerate = 46875.;

float domeradius = 1.; // Use 1 m for sky dome radius for now

void main()
{
    // Test code to display lag texture directly
    //FragColor = texture(texture1, TexCoord);

    // Use a projected sky dome. We define some effective radius (far-field), and calculate each pixel's
    // effective x,y,z coords using that. We then calculate the distances to each mic from that dome position.

    vec3 pixelpos = vec3((gl_FragCoord.xy - vec2(windowWidth, windowHeight)) / pixelscale, 0.);

    if (length(pixelpos) <= domeradius) {
      pixelpos.z = sqrt(domeradius * domeradius - pixelpos.x * pixelpos.x - pixelpos.y * pixelpos.y);

      vec3 r_mic1 = pixelpos - mic1pos;
      vec3 r_mic2 = pixelpos - mic2pos;
      vec3 r_mic3 = pixelpos - mic3pos;
      vec3 r_mic4 = pixelpos - mic4pos;

      // Baseline 1-2
      float d_12 = length(r_mic2) - length(r_mic1);
      int lag_12 = int(round(samplerate * d_12 / soundspeed));
      // Baseline 2-3
      float d_23 = length(r_mic3) - length(r_mic2);
      int lag_23 = int(round(samplerate * d_23 / soundspeed));
      // Baseline 3-1
      float d_31 = length(r_mic1) - length(r_mic3);
      int lag_31 = int(round(samplerate * d_31 / soundspeed));
      // Baseline 1-4
      float d_14 = length(r_mic4) - length(r_mic1);
      int lag_14 = int(round(samplerate * d_14 / soundspeed));
      // Baseline 2-4
      float d_24 = length(r_mic4) - length(r_mic2);
      int lag_24 = int(round(samplerate * d_24 / soundspeed));
      // Baseline 3-4
      float d_34 = length(r_mic4) - length(r_mic3);
      int lag_34 = int(round(samplerate * d_34 / soundspeed));

      vec4 brightness = 
                         (texture(texture1, vec2(float(lag_12 + 32) / 64., 3./16.)).r * vec4(1., 0., 0., 0.4) +
                          texture(texture1, vec2(float(lag_23 + 32) / 64., 5./16.)).r * vec4(0., 1., 0., 0.4) +
                          texture(texture1, vec2(float(lag_31 + 32) / 64., 7./16.)).r * vec4(0., 0., 1., 0.4) +
                          texture(texture1, vec2(float(lag_14 + 32) / 64., 9./16.)).r * vec4(0.7, 0.7, 0., 0.4) +
                          texture(texture1, vec2(float(lag_24 + 32) / 64., 11./16.)).r * vec4(0.7, 0., 0.7, 0.4) +
                          texture(texture1, vec2(float(lag_34 + 32) / 64., 13./16.)).r * vec4(0., 0.7, 0.7, 0.4))/2.4;
      FragColor = brightness;

      if (length(r_mic1) < 0.005 || length(r_mic2) < 0.005 || length(r_mic3) < 0.005 || length(r_mic4) < 0.005) {
          FragColor = vec4(0., 0., 1., 1.);
      }
    } else {
      FragColor = vec4(0., 0., 0., 1.);
    }

    if (gl_FragCoord.y < 60. && gl_FragCoord.y >= 50.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 3./16.));
    } else if (gl_FragCoord.y < 50. && gl_FragCoord.y >= 40.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 5./16.));
    } else if (gl_FragCoord.y < 40. && gl_FragCoord.y >= 30.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 7./16.));
    } else if (gl_FragCoord.y < 30. && gl_FragCoord.y >= 20.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 9./16.));
    } else if (gl_FragCoord.y < 20. && gl_FragCoord.y >= 10.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 11./16.));
    } else if (gl_FragCoord.y < 10.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 13./16.));
    }
}
