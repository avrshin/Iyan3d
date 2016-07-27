
#extension GL_EXT_draw_instanced : enable

attribute vec3 vertPosition;
attribute vec3 vertNormal;
attribute vec2 texCoord1;
attribute vec3 vertTangent;
attribute vec3 vertBitangent;
attribute vec4 optionalData1;

uniform vec3 eyePos;
uniform mat4 vp, lvp, model[uniSize];
uniform vec3 eyePos;
uniform vec3 meshColor[uniSize];
uniform float hasLighting[uniSize], reflectionValue[uniSize], hasMeshColor[uniSize], uvScaleValue[uniSize], transparencyValue[uniSize];
uniform float ambientLight;

varying float vHasLighting, vReflectionValue, vHasMeshColor, vTransparencyValue, vShadowDist;
varying vec2 vTexCoord, vTexCoordBias, vReflectionCoord;
varying vec3 vMeshColor;
varying vec3 vEyeVec, vVertexPosition;
varying mat3 vTBNMatrix;

void main()
{
    int iId = gl_InstanceIDEXT;
    vHasMeshColor = hasMeshColor[iId];
    vMeshColor = (meshColor[iId].x == -1.0) ? optionalData1.xyz : meshColor[iId];
    vTexCoord = texCoord1 * uvScaleValue[iId];

    vec3 T = normalize(vec3(model[iId] * vec4(vertTangent, 0.0)));
    vec3 B = normalize(vec3(model[iId] * vec4(vertBitangent, 0.0)));
    vec3 N = normalize(vec3(model[iId] * vec4(vertNormal,  0.0)));
    vTBNMatrix = mat3(T, B, N);
    
    vVertexPosition = (model[iId] * vec4(vertPosition, 1.0)).xyz;
    vEyeVec = normalize(eyePos - vVertexPosition);
    
    if(bool(hasLighting[iId])) {
        vec4 vertexLightCoord = lvp * vVertexPosition;
        vec4 texCoords = vertexLightCoord / vertexLightCoord.w;
        vTexCoordBias = ((texCoords / 2.0) + 0.5).xy;
        
        if(vertexLightCoord.w > 0.0) {
            vShadowDist = (vertexLightCoord.z) / 5000.0;
            vShadowDist += 0.00000009;
        }
    } else {
        vShadowDist = 0.0;
        vTexCoordBias = vec2(0.0);
    }
    
    if(reflectionValue[uniSize] > 0.0) {
        vec3 r = reflect( -eyeVec, N );
        float m = 2. * sqrt(pow( r.x, 2. ) + pow( r.y, 2. ) + pow( r.z + 1., 2.0));
        vReflectionCoord = r.xy / m + .5;
        vReflectionCoord.y = -vReflectionCoord.y;
    }
    
    gl_Position = vp * vVertexPosition;
}
