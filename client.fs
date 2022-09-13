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

// 50 cm baseline is ~137 lags.

out vec4 FragColor;
in vec3 ourColor;
in vec2 TexCoord;

// Add uniforms to control lag calibrations and relative amplitudes

// texture sampler
uniform sampler2D texture1;

// Microphone positions
// Offset from mic1: 0.073, 0.065
vec3 mic1pos = vec3(-0.073, -0.065, 0.);
vec3 mic2pos = vec3(0.073, -0.065, 0.);
vec3 mic3pos = vec3(-0.038, 0.065, 0.);
vec3 mic4pos = vec3(0.04, 0.065, 0.);
vec3 mic5pos = vec3(0.118, 0.285, 0.);
vec3 mic6pos = vec3(0.268, 0.285, 0.);

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
float PI = 3.141592654;

vec4 c1 = vec4((1. + cos(0.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 0.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 0.  * 2. * PI / 15.))/2., 1.);
vec4 c2 = vec4((1. + cos(1.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 1.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 1.  * 2. * PI / 15.))/2., 1.);
vec4 c3 = vec4((1. + cos(2.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 2.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 2.  * 2. * PI / 15.))/2., 1.);
vec4 c4 = vec4((1. + cos(3.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 3.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 3.  * 2. * PI / 15.))/2., 1.);
vec4 c5 = vec4((1. + cos(4.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 4.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 4.  * 2. * PI / 15.))/2., 1.);
vec4 c6 = vec4((1. + cos(5.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 5.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 5.  * 2. * PI / 15.))/2., 1.);
vec4 c7 = vec4((1. + cos(6.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 6.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 6.  * 2. * PI / 15.))/2., 1.);
vec4 c8 = vec4((1. + cos(7.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 7.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 7.  * 2. * PI / 15.))/2., 1.);
vec4 c9 = vec4((1. + cos(8.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 8.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 8.  * 2. * PI / 15.))/2., 1.);
vec4 c10 = vec4((1. + cos(9.  * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 9.  * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 9.  * 2. * PI / 15.))/2., 1.);
vec4 c11 = vec4((1. + cos(10. * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 10. * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 10. * 2. * PI / 15.))/2., 1.);
vec4 c12 = vec4((1. + cos(11. * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 11. * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 11. * 2. * PI / 15.))/2., 1.);
vec4 c13 = vec4((1. + cos(12. * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 12. * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 12. * 2. * PI / 15.))/2., 1.);
vec4 c14 = vec4((1. + cos(13. * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 13. * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 13. * 2. * PI / 15.))/2., 1.);
vec4 c15 = vec4((1. + cos(14. * 2. * PI / 15.))/2., (1. + cos(2. * PI / 3. + 14. * 2. * PI / 15.))/2., (1. + cos(4. * PI / 3. + 14. * 2. * PI / 15.))/2., 1.);

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
      vec3 r_mic5 = pixelpos - mic5pos;
      vec3 r_mic6 = pixelpos - mic6pos;

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
      // Baseline 1-5
      float d_15 = length(r_mic5) - length(r_mic1);
      int lag_15 = int(round(samplerate * d_15 / soundspeed));
      // Baseline 2-5
      float d_25 = length(r_mic5) - length(r_mic2);
      int lag_25 = int(round(samplerate * d_25 / soundspeed));
      // Baseline 3-5
      float d_35 = length(r_mic5) - length(r_mic3);
      int lag_35 = int(round(samplerate * d_35 / soundspeed));
      // Baseline 4-5
      float d_45 = length(r_mic5) - length(r_mic4);
      int lag_45 = int(round(samplerate * d_45 / soundspeed));
      // Baseline 1-6
      float d_16 = length(r_mic6) - length(r_mic1);
      int lag_16 = int(round(samplerate * d_16 / soundspeed));
      // Baseline 2-6
      float d_26 = length(r_mic6) - length(r_mic2);
      int lag_26 = int(round(samplerate * d_26 / soundspeed));
      // Baseline 3-6
      float d_36 = length(r_mic6) - length(r_mic3);
      int lag_36 = int(round(samplerate * d_36 / soundspeed));
      // Baseline 4-6
      float d_46 = length(r_mic6) - length(r_mic4);
      int lag_46 = int(round(samplerate * d_46 / soundspeed));
      // Baseline 5-6
      float d_56 = length(r_mic6) - length(r_mic5);
      int lag_56 = int(round(samplerate * d_56 / soundspeed));

      vec4 brightness = 
                            //((texture(texture1, vec2(float(lag_12 + 33) / 64., 1/32. + 1. / 32.)).r  * c1 +
                            //  texture(texture1, vec2(float(lag_23 + 32) / 64., 1/32. + 3. / 32.)).r  * c2 +
                            //  texture(texture1, vec2(float(lag_31 + 33) / 64., 1/32. + 5. / 32.)).r  * c3 +
                            //  texture(texture1, vec2(float(lag_14 + 33) / 64., 1/32. + 7. / 32.)).r  * c4 +
                            //  texture(texture1, vec2(float(lag_24 + 32) / 64., 1/32. + 9. / 32.)).r  * c5 +
                            //  texture(texture1, vec2(float(lag_34 + 33) / 64., 1/32. + 11. / 32.)).r * c6 +
                            //  texture(texture1, vec2(float(lag_15 + 32) / 64., 1/32. + 13. / 32.)).r * c7 +
                            //  texture(texture1, vec2(float(lag_25 + 32) / 64., 1/32. + 15. / 32.)).r * c8 +
                            //  texture(texture1, vec2(float(lag_35 + 32) / 64., 1/32. + 17. / 32.)).r * c9 +
                            //  texture(texture1, vec2(float(lag_45 + 32) / 64., 1/32. + 19. / 32.)).r * c10 +
                            //  texture(texture1, vec2(float(lag_16 + 32) / 64., 1/32. + 21. / 32.)).r * c11 +
                            //  texture(texture1, vec2(float(lag_26 + 32) / 64., 1/32. + 23. / 32.)).r * c12 +
                            //  texture(texture1, vec2(float(lag_36 + 32) / 64., 1/32. + 25. / 32.)).r * c13 +
                            //  texture(texture1, vec2(float(lag_46 + 32) / 64., 1/32. + 27. / 32.)).r * c14 +
                            //  texture(texture1, vec2(float(lag_56 + 33) / 64., 1/32. + 29. / 32.)).r * c15) / 7.5);

                              //texture(texture1, vec2(float(lag_16 + 64) / 128., 1/32. + 21. / 32.)).r * c11;

                            ((texture(texture1, vec2(float(lag_12 + 65) / 128., 1/32. + 1. / 32.)).r  * c1 +
                              texture(texture1, vec2(float(lag_23 + 64) / 128., 1/32. + 3. / 32.)).r  * c2 +
                              texture(texture1, vec2(float(lag_31 + 70) / 128., 1/32. + 5. / 32.)).r  * c3 +
                              texture(texture1, vec2(float(lag_14 + 65) / 128., 1/32. + 7. / 32.)).r  * c4 +
                              texture(texture1, vec2(float(lag_24 + 67) / 128., 1/32. + 9. / 32.)).r  * c5 +
                              texture(texture1, vec2(float(lag_34 + 67) / 128., 1/32. + 11. / 32.)).r * c6 +
                              texture(texture1, vec2(float(lag_15 + 64) / 128., 1/32. + 13. / 32.)).r * c7 +
                              texture(texture1, vec2(float(lag_25 + 64) / 128., 1/32. + 15. / 32.)).r * c8 +
                              texture(texture1, vec2(float(lag_35 + 64) / 128., 1/32. + 17. / 32.)).r * c9 +
                              texture(texture1, vec2(float(lag_45 + 67) / 128., 1/32. + 19. / 32.)).r * c10 +
                              texture(texture1, vec2(float(lag_16 + 64) / 128., 1/32. + 21. / 32.)).r * c11 +
                              texture(texture1, vec2(float(lag_26 + 64) / 128., 1/32. + 23. / 32.)).r * c12 +
                              texture(texture1, vec2(float(lag_36 + 64) / 128., 1/32. + 25. / 32.)).r * c13 +
                              texture(texture1, vec2(float(lag_46 + 64) / 128., 1/32. + 27. / 32.)).r * c14 +
                              texture(texture1, vec2(float(lag_56 + 65) / 128., 1/32. + 29. / 32.)).r * c15) / 2.5);
      FragColor = brightness;

    } else {
      FragColor = vec4(0., 0., 0., 1.);
    }

    if (length(pixelpos.xy - mic1pos.xy) < 0.005 ||
        length(pixelpos.xy - mic2pos.xy) < 0.005 ||
        length(pixelpos.xy - mic3pos.xy) < 0.005 ||
        length(pixelpos.xy - mic4pos.xy) < 0.005 ||
        length(pixelpos.xy - mic5pos.xy) < 0.005 ||
        length(pixelpos.xy - mic6pos.xy) < 0.005) {
        FragColor = vec4(0., 0., 1., 1.);
    }

    if (gl_FragCoord.y < 75. && gl_FragCoord.y >= 70.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 1./32.));
    } else if (gl_FragCoord.y < 70. && gl_FragCoord.y >= 65.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 3./32.));
    } else if (gl_FragCoord.y < 65. && gl_FragCoord.y >= 60.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 5./32.));
    } else if (gl_FragCoord.y < 60. && gl_FragCoord.y >= 55.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 7./32.));
    } else if (gl_FragCoord.y < 55. && gl_FragCoord.y >= 50.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 9./32.));
    } else if (gl_FragCoord.y < 50. && gl_FragCoord.y >= 45.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 11./32.));
    } else if (gl_FragCoord.y < 45. && gl_FragCoord.y >= 40.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 13./32.));
    } else if (gl_FragCoord.y < 40. && gl_FragCoord.y >= 35.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 15./32.));
    } else if (gl_FragCoord.y < 35. && gl_FragCoord.y >= 30.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 17./32.));
    } else if (gl_FragCoord.y < 30. && gl_FragCoord.y >= 25.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 19./32.));
    } else if (gl_FragCoord.y < 25. && gl_FragCoord.y >= 20.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 21./32.));
    } else if (gl_FragCoord.y < 20. && gl_FragCoord.y >= 15.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 23./32.));
    } else if (gl_FragCoord.y < 15. && gl_FragCoord.y >= 10.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 25./32.));
    } else if (gl_FragCoord.y < 10. && gl_FragCoord.y >= 5.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 27./32.));
    } else if (gl_FragCoord.y < 5.) {
        FragColor = texture(texture1, vec2(TexCoord.x, 1./32. + 29./16.));
    }
}
