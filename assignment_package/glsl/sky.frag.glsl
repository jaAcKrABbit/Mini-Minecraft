#version 150

uniform mat4 u_ViewProj;    // We're actually passing the inverse of the viewproj
                            // from our CPU, but it's named u_ViewProj so we don't
                            // have to bother rewriting our ShaderProgram class

uniform ivec2 u_Dimensions; // Screen dimensions

uniform vec3 u_Eye; // Camera pos

uniform float u_Time;


vec3 outColor;
out vec4 out_Col;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float scalar = 0.005;

const vec3 sunColor = vec3(255, 255, 190) / 255.0;
const vec3 moonColor = vec3(244, 246, 240) / 255.0;

// Sunset palette
const vec3 sunset[5] = vec3[](vec3(255, 229, 119) / 255.0,
                              vec3(254, 192, 81) / 255.0,
                              vec3(255, 137, 103) / 255.0,
                              vec3(253, 96, 81) / 255.0,
                              vec3(0, 24, 72) / 255.0);
// Dusk palette
const vec3 dusk[5] = vec3[](vec3(144, 96, 144) / 255.0,
                            vec3(96, 72, 120) / 255.0,
                            vec3(72, 48, 120) / 255.0,
                            vec3(48, 24, 96) / 255.0,
                            vec3(0, 24, 72) / 255.0);

// Day
const vec3 day[5] = vec3[]( vec3(135, 181, 235) / 255.0,
                            vec3(135, 198, 235) / 255.0,
                            vec3(135, 206, 235) / 255.0,
                            vec3(135, 214, 235) / 255.0,
                            vec3(0, 204, 255) / 255.0);

// Night
const vec3 night[5] = vec3[](vec3(133, 89, 136) / 255.0,
                             vec3(107, 73, 132) / 255.0,
                             vec3(72, 52, 117) / 255.0,
                             vec3(43, 47, 119) / 255.0,
                             vec3(7, 11, 52) / 255.0);

float rand(vec2 p){
    return fract(sin(dot(p, vec2(127.9898, 311.7))) * 43758.5453);
}

vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 random3( vec3 p ) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                          dot(p,vec3(269.5, 183.3, 765.54)),
                          dot(p, vec3(420.69, 631.2,109.21))))
                 *43758.5453);
}

float WorleyNoise3D(vec3 p)
{
    // Tile the space
    vec3 pointInt = floor(p);
    vec3 pointFract = fract(p);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int z = -1; z <= 1; z++)
    {
        for(int y = -1; y <= 1; y++)
        {
            for(int x = -1; x <= 1; x++)
            {
                vec3 neighbor = vec3(float(x), float(y), float(z));

                // Random point inside current neighboring cell
                vec3 point = random3(pointInt + neighbor);

                // Animate the point
                point = 0.5 + 0.5 * sin(u_Time * scalar + 6.2831 * point); // 0 to 1 range

                // Compute the distance b/t the point and the fragment
                // Store the min dist thus far
                vec3 diff = neighbor + point - pointFract;
                float dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }
    return minDist;
}

float worleyFBM(vec3 uv) {
    float sum = 0;
    float freq = 4;
    float amp = 0.5;
    for(int i = 0; i < 8; i++) {
        sum += WorleyNoise3D(uv * freq) * amp;
        freq *= 2;
        amp *= 0.5;
    }
    return sum;
}

vec2 sphereToUV(vec3 p) {
    float phi = atan(p.z, p.x);
    if(phi < 0) {
        phi += TWO_PI;
    }
    float theta = acos(p.y);
    return vec2(1 - phi / TWO_PI, 1 - theta / PI);
}

vec3 uvToSunset(vec2 uv) {
    if(uv.y < 0.5) {
        return sunset[0];
    }
    else if(uv.y < 0.55) {
        return mix(sunset[0], sunset[1], (uv.y - 0.5) / 0.05);
    }
    else if(uv.y < 0.6) {
        return mix(sunset[1], sunset[2], (uv.y - 0.55) / 0.05);
    }
    else if(uv.y < 0.65) {
        return mix(sunset[2], sunset[3], (uv.y - 0.6) / 0.05);
    }
    else if(uv.y < 0.75) {
        return mix(sunset[3], sunset[4], (uv.y - 0.65) / 0.1);
    }
    return sunset[4];
}

vec3 uvToDusk(vec2 uv) {
    if(uv.y < 0.5) {
        return dusk[0];
    }
    else if(uv.y < 0.55) {
        return mix(dusk[0], dusk[1], (uv.y - 0.5) / 0.05);
    }
    else if(uv.y < 0.6) {
        return mix(dusk[1], dusk[2], (uv.y - 0.55) / 0.05);
    }
    else if(uv.y < 0.65) {
        return mix(dusk[2], dusk[3], (uv.y - 0.6) / 0.05);
    }
    else if(uv.y < 0.75) {
        return mix(dusk[3], dusk[4], (uv.y - 0.65) / 0.1);
    }
    return dusk[4];
}

vec3 uvToDay(vec2 uv) {
    if(uv.y < 0.5) {
        return day[0];
    }
    else if(uv.y < 0.55) {
        return mix(day[0], day[1], (uv.y - 0.5) / 0.05);
    }
    else if(uv.y < 0.6) {
        return mix(day[1], day[2], (uv.y - 0.55) / 0.05);
    }
    else if(uv.y < 0.65) {
        return mix(day[2], day[3], (uv.y - 0.6) / 0.05);
    }
    else if(uv.y < 0.75) {
        return mix(day[3], day[4], (uv.y - 0.65) / 0.1);
    }
    return day[4];
}

vec3 uvToNight(vec2 uv) {
//    //scattering stars
//    float star = rand(uv);
//    if (star < 0.001 && star > 0.00095) {
//        return vec3(255, 229, 247) / 255.0;
//    }
    if(uv.y < 0.5) {
        return night[0];
    }
    else if(uv.y < 0.55) {
        return mix(night[0], night[1], (uv.y - 0.5) / 0.05);
    }
    else if(uv.y < 0.6) {
        return mix(night[1], night[2], (uv.y - 0.55) / 0.05);
    }
    else if(uv.y < 0.65) {
        return mix(night[2], night[3], (uv.y - 0.6) / 0.05);
    }
    else if(uv.y < 0.75) {
        return mix(night[3], night[4], (uv.y - 0.65) / 0.1);
    }
    return night[4];
}

void main()
{
    //converting from pixel space to world space, the whole process is in ray tracing slides
    vec2 ndc = (gl_FragCoord.xy / vec2(u_Dimensions)) * 2.0 - 1.0; // -1 to 1 NDC
    vec4 p = vec4(ndc.xy, 1, 1); // Pixel at the far clip plane
    p *= 1000.0; // Times far clip plane value
    p = /*Inverse of*/ u_ViewProj * p; // Convert from unhomogenized screen to world
    // now in world space, cast a ray from eye to the pixel location
    vec3 rayDir = normalize(p.xyz - u_Eye);
    //map a ray direction to uv coordinates
    vec2 uv = sphereToUV(rayDir);
    //offset for clouds using worley noise
    vec2 offset = vec2(0.0);

    // Get a noise value in the range [-1, 1]
    // by using Worley noise as the noise basis of FBM
    offset = vec2(worleyFBM(rayDir));
    offset *= 2.0;
    offset -= vec2(1.0);
    //temp
   // offset = vec2(0, 0);


    // Compute a gradient from the bottom of the sky-sphere to the top
    vec3 dayColor = uvToDay(uv + offset * 0.1);
    vec3 nightColor = uvToNight(uv + offset * 0.1);
    vec3 sunsetColor = uvToSunset(uv + offset * 0.1);
    vec3 duskColor = uvToDusk(uv + offset * 0.1);

    // Add a glowing sun in the sky
    // rotate the sun in the sky keeping x value unchanged, y from 0 to 1, z from 1 to 0 simulating the reality
    vec3 sunDir = normalize(vec3(0, sin(u_Time * scalar), cos(u_Time * scalar)));
    float sunSize = 30;
    //angle between sun light direction and ray direction
    float sunAngle = acos(dot(rayDir, sunDir)) * 360.0 / PI;

    // Add a moon in the sky
    // moon rises from the other side of the world
    vec3 moonDir = normalize(vec3(0, -sin(u_Time * scalar), -cos(u_Time * scalar)));
    //moon size smaller than sun
    float moonSize = 5;
    float moonAngle = acos(dot(rayDir, moonDir)) * 360.0 / PI;

#define SUNSET_THRESHOLD 0.75
#define MOONSET_THRESHOLD 0.4
#define DUSK_THRESHOLD -0.1

    //day -> sunset -> dusk -> night -> dusk -> sunrise -> day
    // when sun is in the sky, blue sky
    if (sunDir.y >= 0) {
        //when it's about to sunrise and sunset
        if (sunDir.y < SUNSET_THRESHOLD) {
            float t = sunDir.y / SUNSET_THRESHOLD;
            outColor = mix(sunsetColor, dayColor, t);
        }else if (sunDir.y < 0.3) {
            float t = sunDir.y / 0.3;
            outColor = mix(duskColor, sunsetColor, t);

        }else {
            outColor = dayColor;
        }

    }
    // when it's night, when sunDir y value is negative
    else {
        //when moon is about to rise and set, transit dusk color to sunsetColor(temp faking sun rise color)
        if (moonDir.y < 0.3)
        {
            float t = moonDir.y / 0.3;
            outColor = mix(sunsetColor, duskColor, t);
        //when night is about to end, transit from night color to dusk color
        } else if (moonDir.y < MOONSET_THRESHOLD) {
            float t = (moonDir.y - 0.3) / 0.3;
            outColor = mix(duskColor, nightColor, t);
        } else {
            outColor = nightColor;
        }
    }
    // when we are looking at the sun, angle is in the range of the size(angle) of the sun
    if(sunAngle < sunSize) {
        // 7.5 is how big the center of the sun is
        if(sunAngle < 7.5) {
            outColor = sunColor;
        }
        // Corona of sun, mix with day color
        else {
            outColor = mix(sunColor, outColor, (sunAngle - 7.5) / 22.5);
        }
    }
    //when we are looking at moon (angle between raydir and moondir is within the angle of the moon)
    if(moonAngle < moonSize) {
       outColor = moonColor;
    }

    out_Col = vec4(outColor, 1);
}
