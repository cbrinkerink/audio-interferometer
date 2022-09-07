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

// Add uniforms to control lag calibrations, relative amplitudes

// texture sampler
uniform sampler2D texture1;

// Microphone positions
// Offset of mic1: 0.073, 0.065
uniform vec3 mic1pos = vec3(-0.073, -0.065, 0.);
uniform vec3 mic2pos = vec3(0.073, -0.065, 0.);
uniform vec3 mic3pos = vec3(-0.038, 0.065, 0.);
uniform vec3 mic4pos = vec3(0.04, 0.065, 0.);
uniform vec3 mic5pos = vec3(0.118, 0.285, 0.);
uniform vec3 mic6pos = vec3(0.268, 0.285, 0.);
uniform vec3 mic7pos = vec3(-0.026, -0.065, 0.);
uniform vec3 mic8pos = vec3(0.026, -0.065, 0.);
uniform int selectedMic = 0;
uniform int selectedBaseline = -1;

uniform int lagoffsets[28]= int[](64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64);
uniform float ampscales[28] = float[](1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.);
uniform int ampshifts[28] = int[](0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);

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

vec4 c01 =  vec4((1. + cos(0.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 0.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 0.  * 2. * PI / 28.))/2., 1.);
vec4 c02 =  vec4((1. + cos(1.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 1.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 1.  * 2. * PI / 28.))/2., 1.);
vec4 c03 =  vec4((1. + cos(2.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 2.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 2.  * 2. * PI / 28.))/2., 1.);
vec4 c04 =  vec4((1. + cos(3.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 3.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 3.  * 2. * PI / 28.))/2., 1.);
vec4 c05 =  vec4((1. + cos(4.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 4.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 4.  * 2. * PI / 28.))/2., 1.);
vec4 c06 =  vec4((1. + cos(5.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 5.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 5.  * 2. * PI / 28.))/2., 1.);
vec4 c07 =  vec4((1. + cos(6.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 6.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 6.  * 2. * PI / 28.))/2., 1.);
vec4 c08 =  vec4((1. + cos(7.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 7.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 7.  * 2. * PI / 28.))/2., 1.);
vec4 c09 =  vec4((1. + cos(8.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 8.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 8.  * 2. * PI / 28.))/2., 1.);
vec4 c10 = vec4((1. + cos(9.  * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 9.  * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 9.  * 2. * PI / 28.))/2., 1.);
vec4 c11 = vec4((1. + cos(10. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 10. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 10. * 2. * PI / 28.))/2., 1.);
vec4 c12 = vec4((1. + cos(11. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 11. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 11. * 2. * PI / 28.))/2., 1.);
vec4 c13 = vec4((1. + cos(12. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 12. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 12. * 2. * PI / 28.))/2., 1.);
vec4 c14 = vec4((1. + cos(13. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 13. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 13. * 2. * PI / 28.))/2., 1.);
vec4 c15 = vec4((1. + cos(14. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 14. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 14. * 2. * PI / 28.))/2., 1.);
vec4 c16 = vec4((1. + cos(15. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 15. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 15. * 2. * PI / 28.))/2., 1.);
vec4 c17 = vec4((1. + cos(16. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 16. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 16. * 2. * PI / 28.))/2., 1.);
vec4 c18 = vec4((1. + cos(17. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 17. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 17. * 2. * PI / 28.))/2., 1.);
vec4 c19 = vec4((1. + cos(18. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 18. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 18. * 2. * PI / 28.))/2., 1.);
vec4 c20 = vec4((1. + cos(19. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 19. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 19. * 2. * PI / 28.))/2., 1.);
vec4 c21 = vec4((1. + cos(20. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 20. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 20. * 2. * PI / 28.))/2., 1.);
vec4 c22 = vec4((1. + cos(21. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 21. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 21. * 2. * PI / 28.))/2., 1.);
vec4 c23 = vec4((1. + cos(22. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 22. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 22. * 2. * PI / 28.))/2., 1.);
vec4 c24 = vec4((1. + cos(23. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 23. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 23. * 2. * PI / 28.))/2., 1.);
vec4 c25 = vec4((1. + cos(24. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 24. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 24. * 2. * PI / 28.))/2., 1.);
vec4 c26 = vec4((1. + cos(25. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 25. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 25. * 2. * PI / 28.))/2., 1.);
vec4 c27 = vec4((1. + cos(26. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 26. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 26. * 2. * PI / 28.))/2., 1.);
vec4 c28 = vec4((1. + cos(27. * 2. * PI / 28.))/2., (1. + cos(2. * PI / 3. + 27. * 2. * PI / 28.))/2., (1. + cos(4. * PI / 3. + 27. * 2. * PI / 28.))/2., 1.);

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
      vec3 r_mic7 = pixelpos - mic7pos;
      vec3 r_mic8 = pixelpos - mic8pos;

      // Baseline 1-2
      int lag_12 = int(round(samplerate * (length(r_mic2) - length(r_mic1)) / soundspeed));
      int lag_13 = int(round(samplerate * (length(r_mic3) - length(r_mic1)) / soundspeed));
      int lag_14 = int(round(samplerate * (length(r_mic4) - length(r_mic1)) / soundspeed));
      int lag_15 = int(round(samplerate * (length(r_mic5) - length(r_mic1)) / soundspeed));
      int lag_16 = int(round(samplerate * (length(r_mic6) - length(r_mic1)) / soundspeed));
      int lag_17 = int(round(samplerate * (length(r_mic7) - length(r_mic1)) / soundspeed));
      int lag_18 = int(round(samplerate * (length(r_mic8) - length(r_mic1)) / soundspeed));
      int lag_23 = int(round(samplerate * (length(r_mic3) - length(r_mic2)) / soundspeed));
      int lag_24 = int(round(samplerate * (length(r_mic4) - length(r_mic2)) / soundspeed));
      int lag_25 = int(round(samplerate * (length(r_mic5) - length(r_mic2)) / soundspeed));
      int lag_26 = int(round(samplerate * (length(r_mic6) - length(r_mic2)) / soundspeed));
      int lag_27 = int(round(samplerate * (length(r_mic7) - length(r_mic2)) / soundspeed));
      int lag_28 = int(round(samplerate * (length(r_mic8) - length(r_mic2)) / soundspeed));
      int lag_34 = int(round(samplerate * (length(r_mic4) - length(r_mic3)) / soundspeed));
      int lag_35 = int(round(samplerate * (length(r_mic5) - length(r_mic3)) / soundspeed));
      int lag_36 = int(round(samplerate * (length(r_mic6) - length(r_mic3)) / soundspeed));
      int lag_37 = int(round(samplerate * (length(r_mic7) - length(r_mic3)) / soundspeed));
      int lag_38 = int(round(samplerate * (length(r_mic8) - length(r_mic3)) / soundspeed));
      int lag_45 = int(round(samplerate * (length(r_mic5) - length(r_mic4)) / soundspeed));
      int lag_46 = int(round(samplerate * (length(r_mic6) - length(r_mic4)) / soundspeed));
      int lag_47 = int(round(samplerate * (length(r_mic7) - length(r_mic4)) / soundspeed));
      int lag_48 = int(round(samplerate * (length(r_mic8) - length(r_mic4)) / soundspeed));
      int lag_56 = int(round(samplerate * (length(r_mic6) - length(r_mic5)) / soundspeed));
      int lag_57 = int(round(samplerate * (length(r_mic7) - length(r_mic5)) / soundspeed));
      int lag_58 = int(round(samplerate * (length(r_mic8) - length(r_mic5)) / soundspeed));
      int lag_67 = int(round(samplerate * (length(r_mic7) - length(r_mic6)) / soundspeed));
      int lag_68 = int(round(samplerate * (length(r_mic8) - length(r_mic6)) / soundspeed));
      // WEIRD: swap order of mics 7 and 8 here for correct behaviour...
      int lag_78 = int(round(samplerate * (length(r_mic7) - length(r_mic8)) / soundspeed));

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

                            ((texture(texture1, vec2(float(lag_12 + lagoffsets[0]) / 128., 1. / 64.)).r  * c01 +
                              texture(texture1, vec2(float(lag_13 + lagoffsets[1]) / 128., 3. / 64.)).r  * c02 +
                              texture(texture1, vec2(float(lag_14 + lagoffsets[2]) / 128., 5. / 64.)).r  * c03 +
                              texture(texture1, vec2(float(lag_15 + lagoffsets[3]) / 128., 7. / 64.)).r  * c04 +
                              texture(texture1, vec2(float(lag_16 + lagoffsets[4]) / 128., 9. / 64.)).r  * c05 +
                              texture(texture1, vec2(float(lag_17 + lagoffsets[5]) / 128., 11. / 64.)).r  * c06 +
                              texture(texture1, vec2(float(lag_18 + lagoffsets[6]) / 128., 13. / 64.)).r  * c07 +
                              texture(texture1, vec2(float(lag_23 + lagoffsets[7]) / 128., 15. / 64.)).r  * c08 +
                              texture(texture1, vec2(float(lag_24 + lagoffsets[8]) / 128., 17. / 64.)).r  * c09 +
                              texture(texture1, vec2(float(lag_25 + lagoffsets[9]) / 128., 19. / 64.)).r  * c10 +
                              texture(texture1, vec2(float(lag_26 + lagoffsets[10]) / 128., 21. / 64.)).r  * c11 +
                              texture(texture1, vec2(float(lag_27 + lagoffsets[11]) / 128., 23. / 64.)).r  * c12 +
                              texture(texture1, vec2(float(lag_28 + lagoffsets[12]) / 128., 25. / 64.)).r  * c13 +
                              texture(texture1, vec2(float(lag_34 + lagoffsets[13]) / 128., 27. / 64.)).r  * c14 +
                              texture(texture1, vec2(float(lag_35 + lagoffsets[14]) / 128., 29. / 64.)).r  * c15 +
                              texture(texture1, vec2(float(lag_36 + lagoffsets[15]) / 128., 31. / 64.)).r  * c16 +
                              texture(texture1, vec2(float(lag_37 + lagoffsets[16]) / 128., 33. / 64.)).r  * c17 +
                              texture(texture1, vec2(float(lag_38 + lagoffsets[17]) / 128., 35. / 64.)).r  * c18 +
                              texture(texture1, vec2(float(lag_45 + lagoffsets[18]) / 128., 37. / 64.)).r  * c19 +
                              texture(texture1, vec2(float(lag_46 + lagoffsets[19]) / 128., 39. / 64.)).r  * c20 +
                              texture(texture1, vec2(float(lag_47 + lagoffsets[20]) / 128., 41. / 64.)).r  * c21 +
                              texture(texture1, vec2(float(lag_48 + lagoffsets[21]) / 128., 43. / 64.)).r  * c22 +
                              texture(texture1, vec2(float(lag_56 + lagoffsets[22]) / 128., 45. / 64.)).r  * c23 +
                              texture(texture1, vec2(float(lag_57 + lagoffsets[23]) / 128., 47. / 64.)).r  * c24 +
                              texture(texture1, vec2(float(lag_58 + lagoffsets[24]) / 128., 49. / 64.)).r  * c25 +
                              texture(texture1, vec2(float(lag_67 + lagoffsets[25]) / 128., 51. / 64.)).r  * c26 +
                              texture(texture1, vec2(float(lag_68 + lagoffsets[26]) / 128., 53. / 64.)).r  * c27 +
                              texture(texture1, vec2(float(lag_78 + lagoffsets[27]) / 128., 55. / 64.)).r  * c28) / 5.);
      FragColor = brightness;

    } else {
      FragColor = vec4(0., 0., 0., 1.);
    }

    if (length(pixelpos.xy - mic1pos.xy) < 0.005) {
        if (selectedMic == 0) FragColor = vec4(1., 1., 1., 1.);
        else if (selectedBaseline == 0 ||
                 selectedBaseline == 1 ||
                 selectedBaseline == 2 ||
                 selectedBaseline == 3 ||
                 selectedBaseline == 4 ||
                 selectedBaseline == 5 ||
                 selectedBaseline == 6) FragColor = vec4(0., 1., 0., 1.);
        else FragColor = vec4(0., 0., 1., 1.);
    } else if (length(pixelpos.xy - mic2pos.xy) < 0.005) {
        if (selectedMic == 1) FragColor = vec4(1., 1., 1., 1.);
        else if (selectedBaseline == 0 ||
                 selectedBaseline == 7 ||
                 selectedBaseline == 8 ||
                 selectedBaseline == 9 ||
                 selectedBaseline == 10 ||
                 selectedBaseline == 11 ||
                 selectedBaseline == 12) FragColor = vec4(0., 1., 0., 1.);
        else FragColor = vec4(0., 0., 1., 1.);
    } else if (length(pixelpos.xy - mic3pos.xy) < 0.005) {
        if (selectedMic == 2) FragColor = vec4(1., 1., 1., 1.);
        else if (selectedBaseline == 1 ||
                 selectedBaseline == 7 ||
                 selectedBaseline == 13 ||
                 selectedBaseline == 14 ||
                 selectedBaseline == 15 ||
                 selectedBaseline == 16 ||
                 selectedBaseline == 17) FragColor = vec4(0., 1., 0., 1.);
        else FragColor = vec4(0., 0., 1., 1.);
    } else if (length(pixelpos.xy - mic4pos.xy) < 0.005) {
        if (selectedMic == 3) FragColor = vec4(1., 1., 1., 1.);
        else if (selectedBaseline == 2 ||
                 selectedBaseline == 8 ||
                 selectedBaseline == 13 ||
                 selectedBaseline == 18 ||
                 selectedBaseline == 19 ||
                 selectedBaseline == 20 ||
                 selectedBaseline == 21) FragColor = vec4(0., 1., 0., 1.);
        else FragColor = vec4(0., 0., 1., 1.);
    } else if (length(pixelpos.xy - mic5pos.xy) < 0.005) {
        if (selectedMic == 4) FragColor = vec4(1., 1., 1., 1.);
        else if (selectedBaseline == 3 ||
                 selectedBaseline == 9 ||
                 selectedBaseline == 14 ||
                 selectedBaseline == 18 ||
                 selectedBaseline == 22 ||
                 selectedBaseline == 23 ||
                 selectedBaseline == 24) FragColor = vec4(0., 1., 0., 1.);
        else FragColor = vec4(0., 0., 1., 1.);
    } else if (length(pixelpos.xy - mic6pos.xy) < 0.005) {
        if (selectedMic == 5) FragColor = vec4(1., 1., 1., 1.);
        else if (selectedBaseline == 4 ||
                 selectedBaseline == 10 ||
                 selectedBaseline == 15 ||
                 selectedBaseline == 19 ||
                 selectedBaseline == 22 ||
                 selectedBaseline == 25 ||
                 selectedBaseline == 26) FragColor = vec4(0., 1., 0., 1.);
        else FragColor = vec4(0., 0., 1., 1.);
    } else if (length(pixelpos.xy - mic7pos.xy) < 0.005) {
        if (selectedMic == 6) FragColor = vec4(1., 1., 1., 1.);
        else if (selectedBaseline == 5 ||
                 selectedBaseline == 11 ||
                 selectedBaseline == 16 ||
                 selectedBaseline == 20 ||
                 selectedBaseline == 23 ||
                 selectedBaseline == 25 ||
                 selectedBaseline == 27) FragColor = vec4(0., 1., 0., 1.);
        else FragColor = vec4(0., 0., 1., 1.);
    } else if (length(pixelpos.xy - mic8pos.xy) < 0.005) {
        if (selectedMic == 7) FragColor = vec4(1., 1., 1., 1.);
        else if (selectedBaseline == 6 ||
                 selectedBaseline == 12 ||
                 selectedBaseline == 17 ||
                 selectedBaseline == 21 ||
                 selectedBaseline == 24 ||
                 selectedBaseline == 26 ||
                 selectedBaseline == 27) FragColor = vec4(0., 1., 0., 1.);
        else FragColor = vec4(0., 0., 1., 1.);
    }

    if (gl_FragCoord.y < 84. && gl_FragCoord.y >= 81.)      FragColor = texture(texture1, vec2(TexCoord.x, 1./64.));
    else if (gl_FragCoord.y < 81. && gl_FragCoord.y >= 78.) FragColor = texture(texture1, vec2(TexCoord.x, 3./64.));
    else if (gl_FragCoord.y < 78. && gl_FragCoord.y >= 75.) FragColor = texture(texture1, vec2(TexCoord.x, 5./64.));
    else if (gl_FragCoord.y < 75. && gl_FragCoord.y >= 72.) FragColor = texture(texture1, vec2(TexCoord.x, 7./64.));
    else if (gl_FragCoord.y < 72. && gl_FragCoord.y >= 69.) FragColor = texture(texture1, vec2(TexCoord.x, 9./64.));
    else if (gl_FragCoord.y < 69. && gl_FragCoord.y >= 66.) FragColor = texture(texture1, vec2(TexCoord.x, 11./64.));
    else if (gl_FragCoord.y < 66. && gl_FragCoord.y >= 63.) FragColor = texture(texture1, vec2(TexCoord.x, 13./64.));
    else if (gl_FragCoord.y < 63. && gl_FragCoord.y >= 60.) FragColor = texture(texture1, vec2(TexCoord.x, 15./64.));
    else if (gl_FragCoord.y < 60. && gl_FragCoord.y >= 57.) FragColor = texture(texture1, vec2(TexCoord.x, 17./64.));
    else if (gl_FragCoord.y < 57. && gl_FragCoord.y >= 54.) FragColor = texture(texture1, vec2(TexCoord.x, 19./64.));
    else if (gl_FragCoord.y < 54. && gl_FragCoord.y >= 51.) FragColor = texture(texture1, vec2(TexCoord.x, 21./64.));
    else if (gl_FragCoord.y < 51. && gl_FragCoord.y >= 48.) FragColor = texture(texture1, vec2(TexCoord.x, 23./64.));
    else if (gl_FragCoord.y < 48. && gl_FragCoord.y >= 45.) FragColor = texture(texture1, vec2(TexCoord.x, 25./64.));
    else if (gl_FragCoord.y < 45. && gl_FragCoord.y >= 42.) FragColor = texture(texture1, vec2(TexCoord.x, 27./64.));
    else if (gl_FragCoord.y < 42. && gl_FragCoord.y >= 39.) FragColor = texture(texture1, vec2(TexCoord.x, 29./64.));
    else if (gl_FragCoord.y < 39. && gl_FragCoord.y >= 36.) FragColor = texture(texture1, vec2(TexCoord.x, 31./64.));
    else if (gl_FragCoord.y < 36. && gl_FragCoord.y >= 33.) FragColor = texture(texture1, vec2(TexCoord.x, 33./64.));
    else if (gl_FragCoord.y < 33. && gl_FragCoord.y >= 30.) FragColor = texture(texture1, vec2(TexCoord.x, 35./64.));
    else if (gl_FragCoord.y < 30. && gl_FragCoord.y >= 27.) FragColor = texture(texture1, vec2(TexCoord.x, 37./64.));
    else if (gl_FragCoord.y < 27. && gl_FragCoord.y >= 24.) FragColor = texture(texture1, vec2(TexCoord.x, 39./64.));
    else if (gl_FragCoord.y < 24. && gl_FragCoord.y >= 21.) FragColor = texture(texture1, vec2(TexCoord.x, 41./64.));
    else if (gl_FragCoord.y < 21. && gl_FragCoord.y >= 18.) FragColor = texture(texture1, vec2(TexCoord.x, 43./64.));
    else if (gl_FragCoord.y < 18. && gl_FragCoord.y >= 15.) FragColor = texture(texture1, vec2(TexCoord.x, 45./64.));
    else if (gl_FragCoord.y < 15. && gl_FragCoord.y >= 12.) FragColor = texture(texture1, vec2(TexCoord.x, 47./64.));
    else if (gl_FragCoord.y < 12. && gl_FragCoord.y >= 9.)  FragColor = texture(texture1, vec2(TexCoord.x, 49./64.));
    else if (gl_FragCoord.y < 9. && gl_FragCoord.y >= 6.)   FragColor = texture(texture1, vec2(TexCoord.x, 51./64.));
    else if (gl_FragCoord.y < 6. && gl_FragCoord.y >= 3.)   FragColor = texture(texture1, vec2(TexCoord.x, 53./64.));
    else if (gl_FragCoord.y < 3. && gl_FragCoord.y >= 0.)   FragColor = texture(texture1, vec2(TexCoord.x, 55./64.));
}
