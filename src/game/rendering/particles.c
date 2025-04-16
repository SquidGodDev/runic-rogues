#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/rendering/drawables.h"
#include "game/rendering/particles.h"
#include "game/rendering/textures.h"

#define FRAME_TIME 0.016f // Seconds
#define FRAME_COUNT 21

Particle particles[MAX_PARTICLE_COUNT] = {0};

void initParticles(void) {
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		particles[i].active = 0;
	}
}

void spawnParticle(int x, int y) {
	int newParticleIndex = -1;
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		if (!particles[i].active) {
			newParticleIndex = i;
			break;
		}
	}

	if (newParticleIndex == -1) {
		return;
	}

	particles[newParticleIndex] = (struct Particle) {
		x, // x
		y, // y
		FRAME_TIME, // animationTime
		FRAME_COUNT, // frameCount
		0, // animationFrame
		1  // active
	};
}

void updateParticles(float deltaTime) {
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		if (!particles[i].active) {
			continue;
		}

		Particle curParticle = particles[i];
		
		curParticle.animationTime -= deltaTime;
		if (curParticle.animationTime <= 0) {
			curParticle.animationTime += FRAME_TIME;
			curParticle.animationFrame += 1;
			if (curParticle.animationFrame >= curParticle.frameCount) {
				curParticle.active = 0;
				particles[i] = curParticle;
				continue;
			}
		}

		particles[i] = curParticle;

		addDrawableToRenderQueue(curParticle.x, curParticle.y, PARTICLE, i, projectileBreakTexture);
	}
}