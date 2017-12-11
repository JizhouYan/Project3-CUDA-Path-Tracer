﻿#pragma once

#include "intersections.h"
 
// CHECKITOUT
/**
 * Computes a cosine-weighted random direction in a hemisphere.
 * Used for diffuse lighting.
 */
__host__ __device__
glm::vec3 calculateRandomDirectionInHemisphere(
        glm::vec3 normal, thrust::default_random_engine &rng) {
    thrust::uniform_real_distribution<float> u01(0, 1);

    float up = sqrt(u01(rng)); // cos(theta)
    float over = sqrt(1 - up * up); // sin(theta)
    float around = u01(rng) * TWO_PI;

    // Find a direction that is not the normal based off of whether or not the
    // normal's components are all equal to sqrt(1/3) or whether or not at
    // least one component is less than sqrt(1/3). Learned this trick from
    // Peter Kutz.

    glm::vec3 directionNotNormal;
    if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(1, 0, 0);
    } else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(0, 1, 0);
    } else {
        directionNotNormal = glm::vec3(0, 0, 1);
    }

    // Use not-normal direction to generate two perpendicular directions
    glm::vec3 perpendicularDirection1 =
        glm::normalize(glm::cross(normal, directionNotNormal));
    glm::vec3 perpendicularDirection2 =
        glm::normalize(glm::cross(normal, perpendicularDirection1));

    return up * normal
        + cos(around) * over * perpendicularDirection1
        + sin(around) * over * perpendicularDirection2;
}

/**
 * Scatter a ray with some probabilities according to the material properties.
 * For example, a diffuse surface scatters in a cosine-weighted hemisphere.
 * A perfect specular surface scatters in the reflected ray direction.
 * In order to apply multiple effects to one surface, probabilistically choose
 * between them.
 * 
 * The visual effect you want is to straight-up add the diffuse and specular
 * components. You can do this in a few ways. This logic also applies to
 * combining other types of materials (such as refractive).
 * 
 * - Always take an even (50/50) split between a each effect (a diffuse bounce
 *   and a specular bounce), but divide the resulting color of either branch
 *   by its probability (0.5), to counteract the chance (0.5) of the branch
 *   being taken.
 *   - This way is inefficient, but serves as a good starting point - it
 *     converges slowly, especially for pure-diffuse or pure-specular.
 * - Pick the split based on the intensity of each material color, and divide
 *   branch result by that branch's probability (whatever probability you use).
 *
 * This method applies its changes to the Ray parameter `ray` in place.
 * It also modifies the color `color` of the ray in place.
 *
 * You may need to change the parameter list for your purposes!
 */
__host__ __device__
void scatterRay(
PathSegment & pathSegment,
glm::vec3 intersect,
glm::vec3 normal,
const Material &m,
thrust::default_random_engine &rng) {
	thrust::uniform_real_distribution<float> u01(0, 1);
	float prob = u01(rng);
	//my reference:http://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf
	if (prob > 1 - m.hasRefractive && prob < 1) { //transmissive dominate
		float indexRatio;
        float theta = (180.0f / PI) * acos(glm::dot(pathSegment.ray.direction, normal) / (glm::length(pathSegment.ray.direction) * glm::length(normal)));
		if (theta >= 90.0f) {  
			indexRatio = 1.f / m.indexOfRefraction;
		} else {
			indexRatio = m.indexOfRefraction;
	 	}
		float R0 = (1 - indexRatio) / (1 + indexRatio) * (1 - indexRatio) / (1 + indexRatio);
		// Schlick’s approximation of the Fresnel equation
		float RSchlick = R0 + (1.0f - R0) * glm::pow(1.0f - glm::abs(glm::dot(normal, pathSegment.ray.direction)), 5);
		if (RSchlick < prob) {  // refraction
			pathSegment.ray.direction = glm::normalize(glm::refract(pathSegment.ray.direction, normal, indexRatio));
		} else { // reflection
			pathSegment.ray.direction = glm::normalize(glm::reflect(pathSegment.ray.direction, normal));
		}
		pathSegment.ray.origin = intersect + 1e-3f * (glm::normalize(pathSegment.ray.direction));
		pathSegment.color *= m.color * m.specular.color;

	} else if (m.hasReflective == 1) { //directly goes to perfect specular
		pathSegment.ray.direction = pathSegment.ray.direction - 2.0f * normal * (glm::dot(pathSegment.ray.direction, normal));
		pathSegment.ray.origin = intersect + 1e-3f * (glm::normalize(pathSegment.ray.direction));
		pathSegment.color *= m.color * m.specular.color;

	} else if (prob > (1 - m.hasRefractive - m.hasReflective) && prob < (1 - m.hasRefractive)) { //reflection dominate & diffuse combined
		if (0.5f * m.hasReflective < u01(rng)) { //50% percent
			pathSegment.ray.direction = pathSegment.ray.direction - 2.0f * normal * (glm::dot(pathSegment.ray.direction, normal));
			pathSegment.ray.origin = intersect + 1e-3f * (glm::normalize(pathSegment.ray.direction));
			pathSegment.color *= m.color * m.specular.color;
		} else { // diffuse
			pathSegment.ray.direction = calculateRandomDirectionInHemisphere(normal, rng);
			pathSegment.ray.origin = intersect + 1e-3f * (glm::normalize(pathSegment.ray.direction));
			pathSegment.color *= m.color;
		}

	} else { 
		pathSegment.ray.direction = calculateRandomDirectionInHemisphere(normal, rng);
		pathSegment.ray.origin = intersect + 1e-3f * (glm::normalize(pathSegment.ray.direction));
		pathSegment.color *= m.color;
	} 
	    pathSegment.remainingBounces -= 1;
}
//**********my original non-frensel refraction************//
		//glm::vec3 refractionDirection;
		//float theta;
		////get the angel between ray.direction and the normal.
		////refer to pbrt & adam's slides
		//theta = (180.0f / PI) * acos(glm::dot(pathSegment.ray.direction, normal) / (glm::length(pathSegment.ray.direction) * glm::length(normal)));
		////For the incident vector I and surface normal N, and the ratio of indices of refraction eta, return the refraction vector.
		//if (theta >= 90.0f) {
		//	refractionDirection = glm::refract(pathSegment.ray.direction, normal, 1.0f / m.indexOfRefraction);
		//} else{
		//	refractionDirection = glm::refract(pathSegment.ray.direction, -normal, m.indexOfRefraction);
		//}
		//pathSegment.ray.direction = refractionDirection;
		//pathSegment.ray.origin = intersect + glm::normalize(pathSegment.ray.direction) * 1e-3f;
		//pathSegment.color *= m.color * m.specular.color;


