#version 330 core

uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;	// moved from vertex shader
uniform vec4 Light1Position;  									// moved from vertex shader
uniform vec4 Light2Direction;									// BC-8 added for light 2
uniform vec4 Light3Position;									// BC-9 added for light 3 (spotlight)
uniform vec4 SpotConeDirection;									// BC-9 added for light 3 (spotlight)
uniform vec3 Light1col;											// BC-8 added for attn calcs in shader
uniform vec3 Light2col;											// BC-8 added for attn calcs in shader
uniform vec3 Light3col;											// BC-9 added for attn calcs in shader
uniform float SpotBeamWidth;									// BC-10 added for light 3 (spotlight)
uniform float lightModel;										// Phong =1,Binn Phong = 2
uniform float Shininess;  										// moved from vertex shader
uniform int Outline;   											// is 1 for outlined draw and 0 otherwise

in vec2 texCoord;  	// The third coordinate is always 0.0 and is discarded
in vec4 position;  	// transformed and interpolated position from vertex shader
in vec4 normal;		// transformed and interpolated normal from vertex shader

uniform sampler2D texture;
uniform float texScale;  // BC-x
uniform mat4 ModelView;

void main()
{
// BC-6 all this moved from vertex to fragment shader
// The transformations are done in the vertex shader
// the light is recalculated pixel by pixel based on interpolated point and normal data

    // Calculate key vectors for light calculations
	// L1 is point light
	// L2 is directional light
	// L3 is spot light (point light with narrow beam)
	vec3 L1 = normalize(Light1Position.xyz - position.xyz);// vector from fragment to fixed light 1
	vec3 L2 = normalize(Light2Direction.xyz);  			   // const direction of the directional light 2
	vec3 L3 = normalize(Light3Position.xyz - position.xyz);// vector from fragment to fixed spotlight light 3
	// Direction to the eye/camera (camera always at origin in camera/eye coords)
    vec3 E = normalize(-position.xyz );  // needed for specular reflections

	// Normal vector
	vec3 N = normalize(normal.xyz);   ; // interpolated and normalised Normal at this fragment
	
	// Calculate attenuation
	float D1 = length(Light1Position.xyz - position.xyz); 	// Distance from light1 to fragment
	float D2 = .001;										// Assume no attenuation of directional light
	float D3 = length(Light3Position.xyz - position.xyz);	// Distance from light3 to fragment
	float a = 0.2;  
	float b = 0.3;
	float attenuation1 = 1/(1 + a*D1 + b*D1*D1);// this seems to be common in openGL
	float attenuation2 = 3; //(1 + a*D2 + b*D2*D2);// Artificially forced to near unity for no attenuation
	float attenuation3 = 1/(1 + a*D3 + b*D3*D3);// this seems to be common in openGL
	
    // Compute terms in the illumination equation
	
	// First calculate the diffuse term
    float Kd1 = max( dot(L1, N), 0.0 );  		// removes negative cosines 
	float Kd2 = max( dot(L2, N), 0.0 );			// in this case L2 is a constant direction
	float Kd3 = max( dot(L3, N), 0.0 );  		// spotlight is non-directional with limited beam width
	//if (Kd3 < .95) attenuation3 = 0;
	float spotCos = dot(L3, normalize(SpotConeDirection.xyz));
	if(spotCos < cos(SpotBeamWidth)) attenuation3 = 0;  // no light outside the beam angle
	else attenuation3*= pow(spotCos,10);
	
	// then compute specular reflection
	vec3 R1,R2,R3;
	float Ks1,Ks2,Ks3;
	if (lightModel <= 1.1)
		{ // Case -1 - Use Phong model - exact reflection vectors
		vec3 I1 = -L1;  // Incident vector = - Light Vector
		vec3 I2 = -L2;
		vec3 I3 = -L3;
		// calculate exact reflected vector for each light
		R1 = normalize( (I1)-2*dot(N,I1)*N );  
		R1 = normalize( (I2)-2*dot(N,I2)*N );
		R3 = normalize( (I3)-2*dot(N,I3)*N );    
	}
	else {// Case 2 - Use Blinn Phong half angles (better)
		R1 = normalize(L1 + E ); 
		R2 = normalize(L2 + E );
		R3 = normalize(L3 + E );  
	}
 	Ks1 = pow( max(dot(N, R1), 0.0), Shininess );
	Ks2 = pow( max(dot(N, R2), 0.0), Shininess );
	Ks3 = pow( max(dot(N, R3), 0.0), Shininess );
	
	// BC-7 make each light white for specular calculations
	float White1 = (Light1col.x,Light1col.y,Light1col.z)/3;
	float White2 = (Light2col.x,Light2col.y,Light2col.z)/3;
	float White3 = (Light3col.x,Light3col.y,Light3col.z)/3;

	if (Kd1 <= 0) Ks1 = 0;
	if (Kd2 <= 0) Ks2 = 0;
	if (Kd3 <= 0) Ks3 = 0;

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.2, 0.2, 0.2);
	
	vec4 color;		// BC-6 added
    color.rgb = globalAmbient;
	color.rgb += attenuation1*(AmbientProduct*Light1col+Kd1*DiffuseProduct*Light1col+Ks1*SpecularProduct*White1);
	color.rgb += attenuation2*(AmbientProduct*Light2col+Kd2*DiffuseProduct*Light2col+Ks2*SpecularProduct*White2);
	//color.rgb += attenuation2*(Kd2*DiffuseProduct*Light2col+Ks2*SpecularProduct*White2);
	color.rgb += attenuation3*(AmbientProduct*Light3col+Kd3*DiffuseProduct*Light3col+Ks3*SpecularProduct*White3);
    color.a = 1.0;
	
// BC-6 - end of code moved from vertex shader

    // Either draw normally or use a single colour for highlighted edges 
	if (Outline < 1)
		gl_FragColor = color * texture2D( texture, texCoord * texScale );
	else 
		gl_FragColor = vec4(1.0,1.0,0.0,1.0);
}
