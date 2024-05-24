#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#include "Material.glsl"

layout(binding = 4) readonly buffer VertexArray { float Vertices[]; };
layout(binding = 5) readonly buffer IndexArray { uint Indices[]; };
layout(binding = 6) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 7) readonly buffer OffsetArray { uvec2[] Offsets; };
layout(binding = 8) uniform sampler2D[] TextureSamplers;

#include "Scatter.glsl"
#include "Vertex.glsl"

const float pi = 3.1415926535897932384626433832795;

hitAttributeEXT vec2 HitAttributes;
rayPayloadInEXT RayPayload Ray;

vec2 Mix(vec2 a, vec2 b, vec2 c, vec3 barycentrics)
{
	return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3 Mix(vec3 a, vec3 b, vec3 c, vec3 barycentrics) 
{
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

RayPayload ScatterSimple(const Material m, const vec3 direction, const vec3 normal, const vec2 texCoord, const float t, inout uint seed, in uint bounce)
{
	const bool isScattered = dot(direction, normal) < 0;
	const vec4 texColor = m.DiffuseTextureId >= 0 ? texture(TextureSamplers[nonuniformEXT(m.DiffuseTextureId)], texCoord) : vec4(1);
	vec4 colorAndDistance = vec4(m.Diffuse.rgb * texColor.rgb, t);
	vec4 scatter = vec4( AlignWithNormal( RandomInHemiSphere1(seed), normal), isScattered ? 1 : 0);
	float pdf = 1.0;
	vec4 emitColor = vec4(0);
	
	if( m.MaterialModel == MaterialDiffuseLight )
	{
	    if(isScattered)
		{
			emitColor = vec4(15);
			colorAndDistance.w = -1;
		}
	}
	
	// half probability to scatter to light
	if( RandomFloat(seed) < 0.5 )
	{
		// scatter to light
		vec3 lightpos = vec3( mix( 213, 343, RandomFloat(seed)) ,555, mix( -213, -343, RandomFloat(seed)) );
		vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
		vec3 tolight = lightpos - worldPos;
		float dist = length(tolight);
		tolight = tolight / dist;
		
		scatter.xyz = tolight;
		float light_pdf = dist * dist / (abs(scatter.y) * 130 * 130);
		pdf = 1.0f / light_pdf;
	}
	
	return RayPayload(colorAndDistance, emitColor, scatter, vec4(normal, m.Fuzziness), seed,0,bounce,pdf);
}

void main()
{
	// Get the material.
	const uvec2 offsets = Offsets[gl_InstanceCustomIndexEXT];
	const uint indexOffset = offsets.x;
	const uint vertexOffset = offsets.y;
	const Vertex v0 = UnpackVertex(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 0]);
	const Vertex v1 = UnpackVertex(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 1]);
	const Vertex v2 = UnpackVertex(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 2]);
	const Material material = Materials[v0.MaterialIndex];

	// Compute the ray hit point properties.
	const vec3 barycentrics = vec3(1.0 - HitAttributes.x - HitAttributes.y, HitAttributes.x, HitAttributes.y);
	const vec3 normal = normalize(Mix(v0.Normal, v1.Normal, v2.Normal, barycentrics));
	const vec2 texCoord = Mix(v0.TexCoord, v1.TexCoord, v2.TexCoord, barycentrics);

	Ray = ScatterSimple(material, gl_WorldRayDirectionEXT, normal, texCoord, gl_HitTEXT, Ray.RandomSeed, Ray.BounceCount);
}
