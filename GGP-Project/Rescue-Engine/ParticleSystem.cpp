#include "ParticleSystem.h"
#include "ExtendedMath.h"

//Creates a new particle system.
ParticleSystem::ParticleSystem(ParticleSystemType type, XMFLOAT3 position, Mesh* mesh, Material* material, int particles = 100, int time = 100, float gravity = 1.0f)
{
	aliveTime = time;

	for (auto i = 0; i < particles; i++) //Creates X amount of particles
	{
		Particle* p = new Particle(mesh, material, "particle" + i);

		switch (type)
		{
			case(ParticleSystem::Splash):
				p->SetParticleTimer(aliveTime);
				p->SetParticleDirection(ExtendedMath::RandomSplashVector());
				p->SetParticleGravity(gravity);
				p->SetParticleForce(0.01f);
				p->SetParticleSlowdown(.9f);
				p->SetPosition(position);
				p->SetScale(XMFLOAT3(2.0f, 2.0f, 2.0f));
			break;
		}

		particleList.push_back(p);
	}
}

//Deletes the particle system.
ParticleSystem::~ParticleSystem()
{
	ClearParticleList();
}

//Clears the list of particles.
bool ParticleSystem::ClearParticleList()
{
	while (particleList.size() > 0)
	{
		particleList[0]->~Particle();
		particleList.erase(particleList.begin(), particleList.begin());
	}
	return true;
}

//Updates the particles in the particle list.
void ParticleSystem::Update(float deltaTime)
{
	std::cout << "There are " << particleList.size() << " particles in the particle system." << std::endl;
	for (int i = 0; i < particleList.size(); i++)
	{
		Particle* p = particleList[i];
		p->Update(deltaTime);

		if (p->GetParticleTimer() > aliveTime)
		{
			ClearParticleList();
			break;
		}
	}
}