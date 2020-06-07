#version 330 core

layout(location = 0) in vec3 vPosition1;
layout(location = 1) in vec3 vNormal;
layout(location = 3) in vec2 vTexCoord;
layout(location = 4) in vec4 vBoneIDs;				// BC Part 2
layout(location = 5) in vec4 vBoneWeights;			// BC Part 2

uniform mat4 boneTransforms[32];	    			// BC Part 2  (was 64 - no of bones in any mesh is now limited to 32)
uniform mat4 ModelView;
uniform mat4 Projection;

out vec2 texCoord;
out vec4 position;   // BC-6 added
out vec4 normal;	 // BC-6 added

void main()
{
	// BC Part 2
	mat4 boneTransform = vBoneWeights[0] * boneTransforms[int(vBoneIDs[0])] +
                         vBoneWeights[1] * boneTransforms[int(vBoneIDs[1])] +
						 vBoneWeights[2] * boneTransforms[int(vBoneIDs[2])] +
						 vBoneWeights[3] * boneTransforms[int(vBoneIDs[3])];

	vec4 PosL			= boneTransform* vec4(vPosition1, 1.0); 	// vertices for lighting
	vec4 NormalL		= boneTransform* vec4(vNormal  ,  0.0);  	// normals for lighting 

	//vec4 PosL = vec4(vPosition1  , 1.0); 		// vertices for lighting
	//vec4 NormalL = vec4(vNormal  , 0.0);  	// normals for lighting 
	
    gl_Position = Projection * ModelView * PosL;     	// vertices after projection
	texCoord 	= vTexCoord;
	
	// these are for lighting and be interpolated per fragment by the fragment shader
	// This ensures the light will be recalculated per pixel/fragment.
	position 	= ModelView*PosL;
	normal 		= ModelView*NormalL;  
	// note that normal transformation has to be adjusted 
	// if scale is not same in all dimensions.
    // If not uniform scale, then the scaling has to be reversed before transforming normal	
}
