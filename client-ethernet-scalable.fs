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

#define NUMLAGS 256

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
uniform vec3 mic7pos = vec3(0.026, -0.065, 0.);
uniform vec3 mic8pos = vec3(-0.026, -0.065, 0.);
uniform int selectedMic = 0;
uniform int selectedBaseline = -1;

uniform float lagoffsets[28]= float[](NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.,NUMLAGS/2.);
uniform float ampscales[28] = float[](1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.);
uniform float ampshifts[28] = float[](0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.);


// NOTE: On retina screens the scaling is apparently different by a factor of 2:
// my laptop has an apparent window size that is twice the numbers specified below.
// To correctly calculate the middle of the window, I have to multiply the
// coords for the middle of the window (400,300 in this example) by 2.

// retinaFactor is 2. for Cinema display, or 1. for laptop display
float retinaFactor = 1.;

float windowWidth = 800.;
float windowHeight = 600.;

float pixelscale = windowHeight / retinaFactor; // Pixels per meter

float soundspeed = 343.;
float samplerate = 46875.;

float domeradius = 1.; // Use 1 m for sky dome radius for now
float PI = 3.141592654;

float chanoffset = 1.;

vec4 c01 =  vec4((chanoffset + cos(0.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 0.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 0.  * 2. * PI / 28.))/2., 1.);
vec4 c02 =  vec4((chanoffset + cos(1.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 1.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 1.  * 2. * PI / 28.))/2., 1.);
vec4 c03 =  vec4((chanoffset + cos(2.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 2.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 2.  * 2. * PI / 28.))/2., 1.);
vec4 c04 =  vec4((chanoffset + cos(3.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 3.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 3.  * 2. * PI / 28.))/2., 1.);
vec4 c05 =  vec4((chanoffset + cos(4.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 4.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 4.  * 2. * PI / 28.))/2., 1.);
vec4 c06 =  vec4((chanoffset + cos(5.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 5.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 5.  * 2. * PI / 28.))/2., 1.);
vec4 c07 =  vec4((chanoffset + cos(6.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 6.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 6.  * 2. * PI / 28.))/2., 1.);
vec4 c08 =  vec4((chanoffset + cos(7.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 7.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 7.  * 2. * PI / 28.))/2., 1.);
vec4 c09 =  vec4((chanoffset + cos(8.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 8.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 8.  * 2. * PI / 28.))/2., 1.);
vec4 c10 =  vec4((chanoffset + cos(9.  * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 9.  * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 9.  * 2. * PI / 28.))/2., 1.);
vec4 c11 =  vec4((chanoffset + cos(10. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 10. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 10. * 2. * PI / 28.))/2., 1.);
vec4 c12 =  vec4((chanoffset + cos(11. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 11. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 11. * 2. * PI / 28.))/2., 1.);
vec4 c13 =  vec4((chanoffset + cos(12. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 12. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 12. * 2. * PI / 28.))/2., 1.);
vec4 c14 =  vec4((chanoffset + cos(13. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 13. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 13. * 2. * PI / 28.))/2., 1.);
vec4 c15 =  vec4((chanoffset + cos(14. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 14. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 14. * 2. * PI / 28.))/2., 1.);
vec4 c16 =  vec4((chanoffset + cos(15. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 15. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 15. * 2. * PI / 28.))/2., 1.);
vec4 c17 =  vec4((chanoffset + cos(16. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 16. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 16. * 2. * PI / 28.))/2., 1.);
vec4 c18 =  vec4((chanoffset + cos(17. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 17. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 17. * 2. * PI / 28.))/2., 1.);
vec4 c19 =  vec4((chanoffset + cos(18. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 18. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 18. * 2. * PI / 28.))/2., 1.);
vec4 c20 =  vec4((chanoffset + cos(19. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 19. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 19. * 2. * PI / 28.))/2., 1.);
vec4 c21 =  vec4((chanoffset + cos(20. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 20. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 20. * 2. * PI / 28.))/2., 1.);
vec4 c22 =  vec4((chanoffset + cos(21. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 21. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 21. * 2. * PI / 28.))/2., 1.);
vec4 c23 =  vec4((chanoffset + cos(22. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 22. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 22. * 2. * PI / 28.))/2., 1.);
vec4 c24 =  vec4((chanoffset + cos(23. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 23. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 23. * 2. * PI / 28.))/2., 1.);
vec4 c25 =  vec4((chanoffset + cos(24. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 24. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 24. * 2. * PI / 28.))/2., 1.);
vec4 c26 =  vec4((chanoffset + cos(25. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 25. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 25. * 2. * PI / 28.))/2., 1.);
vec4 c27 =  vec4((chanoffset + cos(26. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 26. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 26. * 2. * PI / 28.))/2., 1.);
vec4 c28 =  vec4((chanoffset + cos(27. * 2. * PI / 28.))/2., (chanoffset + cos(2. * PI / 3. + 27. * 2. * PI / 28.))/2., (chanoffset + cos(4. * PI / 3. + 27. * 2. * PI / 28.))/2., 1.);

void main()
{
    // Test code to display lag texture directly
    //FragColor = texture(texture1, TexCoord);

    // Use a projected sky dome. We define some effective radius (far-field), and calculate each pixel's
    // effective x,y,z coords using that. We then calculate the distances to each mic from that dome position.

    vec3 pixelpos = vec3((gl_FragCoord.xy - vec2(windowWidth, windowHeight) / retinaFactor) / pixelscale, 0.);

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
      float lag_12 = samplerate * (length(r_mic2) - length(r_mic1)) / soundspeed;
      float lag_13 = samplerate * (length(r_mic3) - length(r_mic1)) / soundspeed;
      float lag_14 = samplerate * (length(r_mic4) - length(r_mic1)) / soundspeed;
      float lag_15 = samplerate * (length(r_mic5) - length(r_mic1)) / soundspeed;
      float lag_16 = samplerate * (length(r_mic6) - length(r_mic1)) / soundspeed;
      float lag_17 = samplerate * (length(r_mic7) - length(r_mic1)) / soundspeed;
      float lag_18 = samplerate * (length(r_mic8) - length(r_mic1)) / soundspeed;
      float lag_23 = samplerate * (length(r_mic3) - length(r_mic2)) / soundspeed;
      float lag_24 = samplerate * (length(r_mic4) - length(r_mic2)) / soundspeed;
      float lag_25 = samplerate * (length(r_mic5) - length(r_mic2)) / soundspeed;
      float lag_26 = samplerate * (length(r_mic6) - length(r_mic2)) / soundspeed;
      float lag_27 = samplerate * (length(r_mic7) - length(r_mic2)) / soundspeed;
      float lag_28 = samplerate * (length(r_mic8) - length(r_mic2)) / soundspeed;
      float lag_34 = samplerate * (length(r_mic4) - length(r_mic3)) / soundspeed;
      float lag_35 = samplerate * (length(r_mic5) - length(r_mic3)) / soundspeed;
      float lag_36 = samplerate * (length(r_mic6) - length(r_mic3)) / soundspeed;
      float lag_37 = samplerate * (length(r_mic7) - length(r_mic3)) / soundspeed;
      float lag_38 = samplerate * (length(r_mic8) - length(r_mic3)) / soundspeed;
      float lag_45 = samplerate * (length(r_mic5) - length(r_mic4)) / soundspeed;
      float lag_46 = samplerate * (length(r_mic6) - length(r_mic4)) / soundspeed;
      float lag_47 = samplerate * (length(r_mic7) - length(r_mic4)) / soundspeed;
      float lag_48 = samplerate * (length(r_mic8) - length(r_mic4)) / soundspeed;
      float lag_56 = samplerate * (length(r_mic6) - length(r_mic5)) / soundspeed;
      float lag_57 = samplerate * (length(r_mic7) - length(r_mic5)) / soundspeed;
      float lag_58 = samplerate * (length(r_mic8) - length(r_mic5)) / soundspeed;
      float lag_67 = samplerate * (length(r_mic7) - length(r_mic6)) / soundspeed;
      float lag_68 = samplerate * (length(r_mic8) - length(r_mic6)) / soundspeed;
      float lag_78 = samplerate * (length(r_mic8) - length(r_mic7)) / soundspeed;

      float scaleoffset = -0.2;
      float totalscale = 1.;
      if (selectedBaseline == -1) totalscale = 1./28.;
      vec4 brightness =     ((max((texture(texture1, vec2(float(lag_12 +  lagoffsets[0]) / float(NUMLAGS), 1.  / 64.)).r - ampshifts[ 0]) * ampscales[  0], scaleoffset)  * c01 +
                              max((texture(texture1, vec2(float(lag_13 +  lagoffsets[1]) / float(NUMLAGS), 3.  / 64.)).r - ampshifts[ 1]) * ampscales[  1], scaleoffset)  * c02 +
                              max((texture(texture1, vec2(float(lag_14 +  lagoffsets[2]) / float(NUMLAGS), 5.  / 64.)).r - ampshifts[ 2]) * ampscales[  2], scaleoffset)  * c03 +
                              max((texture(texture1, vec2(float(lag_15 +  lagoffsets[3]) / float(NUMLAGS), 7.  / 64.)).r - ampshifts[ 3]) * ampscales[  3], scaleoffset)  * c04 +
                              max((texture(texture1, vec2(float(lag_16 +  lagoffsets[4]) / float(NUMLAGS), 9.  / 64.)).r - ampshifts[ 4]) * ampscales[  4], scaleoffset)  * c05 +
                              max((texture(texture1, vec2(float(lag_17 +  lagoffsets[5]) / float(NUMLAGS), 11. / 64.)).r - ampshifts[ 5]) * ampscales[  5], scaleoffset)  * c06 +
                              max((texture(texture1, vec2(float(lag_18 +  lagoffsets[6]) / float(NUMLAGS), 13. / 64.)).r - ampshifts[ 6]) * ampscales[  6], scaleoffset)  * c07 +
                              max((texture(texture1, vec2(float(lag_23 +  lagoffsets[7]) / float(NUMLAGS), 15. / 64.)).r - ampshifts[ 7]) * ampscales[  7], scaleoffset)  * c08 +
                              max((texture(texture1, vec2(float(lag_24 +  lagoffsets[8]) / float(NUMLAGS), 17. / 64.)).r - ampshifts[ 8]) * ampscales[  8], scaleoffset)  * c09 +
                              max((texture(texture1, vec2(float(lag_25 +  lagoffsets[9]) / float(NUMLAGS), 19. / 64.)).r - ampshifts[ 9]) * ampscales[  9], scaleoffset)  * c10 +
                              max((texture(texture1, vec2(float(lag_26 + lagoffsets[10]) / float(NUMLAGS), 21. / 64.)).r - ampshifts[10]) * ampscales[ 10], scaleoffset)  * c11 +
                              max((texture(texture1, vec2(float(lag_27 + lagoffsets[11]) / float(NUMLAGS), 23. / 64.)).r - ampshifts[11]) * ampscales[ 11], scaleoffset)  * c12 +
                              max((texture(texture1, vec2(float(lag_28 + lagoffsets[12]) / float(NUMLAGS), 25. / 64.)).r - ampshifts[12]) * ampscales[ 12], scaleoffset)  * c13 +
                              max((texture(texture1, vec2(float(lag_34 + lagoffsets[13]) / float(NUMLAGS), 27. / 64.)).r - ampshifts[13]) * ampscales[ 13], scaleoffset)  * c14 +
                              max((texture(texture1, vec2(float(lag_35 + lagoffsets[14]) / float(NUMLAGS), 29. / 64.)).r - ampshifts[14]) * ampscales[ 14], scaleoffset)  * c15 +
                              max((texture(texture1, vec2(float(lag_36 + lagoffsets[15]) / float(NUMLAGS), 31. / 64.)).r - ampshifts[15]) * ampscales[ 15], scaleoffset)  * c16 +
                              max((texture(texture1, vec2(float(lag_37 + lagoffsets[16]) / float(NUMLAGS), 33. / 64.)).r - ampshifts[16]) * ampscales[ 16], scaleoffset)  * c17 +
                              max((texture(texture1, vec2(float(lag_38 + lagoffsets[17]) / float(NUMLAGS), 35. / 64.)).r - ampshifts[17]) * ampscales[ 17], scaleoffset)  * c18 +
                              max((texture(texture1, vec2(float(lag_45 + lagoffsets[18]) / float(NUMLAGS), 37. / 64.)).r - ampshifts[18]) * ampscales[ 18], scaleoffset)  * c19 +
                              max((texture(texture1, vec2(float(lag_46 + lagoffsets[19]) / float(NUMLAGS), 39. / 64.)).r - ampshifts[19]) * ampscales[ 19], scaleoffset)  * c20 +
                              max((texture(texture1, vec2(float(lag_47 + lagoffsets[20]) / float(NUMLAGS), 41. / 64.)).r - ampshifts[20]) * ampscales[ 20], scaleoffset)  * c21 +
                              max((texture(texture1, vec2(float(lag_48 + lagoffsets[21]) / float(NUMLAGS), 43. / 64.)).r - ampshifts[21]) * ampscales[ 21], scaleoffset)  * c22 +
                              max((texture(texture1, vec2(float(lag_56 + lagoffsets[22]) / float(NUMLAGS), 45. / 64.)).r - ampshifts[22]) * ampscales[ 22], scaleoffset)  * c23 +
                              max((texture(texture1, vec2(float(lag_57 + lagoffsets[23]) / float(NUMLAGS), 47. / 64.)).r - ampshifts[23]) * ampscales[ 23], scaleoffset)  * c24 +
                              max((texture(texture1, vec2(float(lag_58 + lagoffsets[24]) / float(NUMLAGS), 49. / 64.)).r - ampshifts[24]) * ampscales[ 24], scaleoffset)  * c25 +
                              max((texture(texture1, vec2(float(lag_67 + lagoffsets[25]) / float(NUMLAGS), 51. / 64.)).r - ampshifts[25]) * ampscales[ 25], scaleoffset)  * c26 +
                              max((texture(texture1, vec2(float(lag_68 + lagoffsets[26]) / float(NUMLAGS), 53. / 64.)).r - ampshifts[26]) * ampscales[ 26], scaleoffset)  * c27 +
                              max((texture(texture1, vec2(float(lag_78 + lagoffsets[27]) / float(NUMLAGS), 55. / 64.)).r - ampshifts[27]) * ampscales[ 27], scaleoffset)  * c28)) * totalscale;
      FragColor = brightness;

    } else {
      FragColor = vec4(0.5, 0.5, 0.5, 1.);
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
