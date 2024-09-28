

typedef struct Particle {
	ParticleFlags flags;
	Vector4 col;
	Vector2 size;
	Vector2 pos;
	Vector2 velocity;
	Vector2 acceleration;
	float friction;
	float64 end_time;
	float fade_out_vel_range;
	bool immortal;
	float identifier;
	ParticleKind kind;
} Particle;
Particle particles[2048] = {0};
int particle_cursor = 0;

Particle* particle_new() {
	Particle* p = &particles[particle_cursor];
	particle_cursor += 1;
	if (particle_cursor >= (sizeof(particles) / sizeof(particles[0]))) {
		particle_cursor = 0;
	}
	if (p->flags & PARTICLE_FLAGS_valid) {
		log_warning("too many particles, overwriting existing");
	}
	p->flags |= PARTICLE_FLAGS_valid;
	return p;
}

void particle_clear(Particle* p) {
	memset(p, 0, sizeof(Particle));
}