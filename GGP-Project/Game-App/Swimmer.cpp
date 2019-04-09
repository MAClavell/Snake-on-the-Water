#include "Swimmer.h"
#include "ExtendedMath.h"

//Buoyancy consts
#define MASS 0.5f
#define GRAVITY 9.81f
#define FLUID_DENSITY 2.0f
#define DRAG_COEFF 1.05f
#define AIR_DENSITY 0.1225f

using namespace DirectX;

//Snake follow logic from:
//https://github.com/rimij405/ggp-smij/blob/Unity-Prototype/Prototype/Boat-Snake-Prototype/Assets/Scripts/BoatFollower.cs

Swimmer::Swimmer(Mesh* mesh, Material* material, std::string name)
	: Entity(mesh, material, name)
{
	//Create buffers
	bufferLength = (int)ceil(lagSeconds * MAX_FPS);
	positionBuffer = new XMFLOAT3[bufferLength];
	timeBuffer = new float[bufferLength];

	//Set default vals
	swmrState = SwimmerState::Entering;
	this->leader = nullptr;
	positionBuffer[0] = positionBuffer[1] = DirectX::XMFLOAT3(0, 0, 0);
	timeBuffer[0] = timeBuffer[1] = timer = 0;
	velocity = 0;
	acceleration = 0;

	oldestIndex = 0;
	newestIndex = 1;
}

Swimmer::~Swimmer()
{
	//Delete buffers
	delete[] positionBuffer;
	delete[] timeBuffer;
}

void Swimmer::Update(float deltaTime)
{
	ApplyBuoyancy(deltaTime);
	switch (swmrState)
	{
		case SwimmerState::Entering:
			swmrState = SwimmerState::Floating;
			break;

		case SwimmerState::Floating:
			Float(deltaTime);
			break;

		case SwimmerState::Joining:
			Join(deltaTime);
			break;

		case SwimmerState::Following:
			Follow(deltaTime);
			break;

		case SwimmerState::Docking:
			
			break;

		case SwimmerState::Hitting:

			break;
	}	
}

// --------------------------------------------------------
// Run this swimmer's entering behaviour
//---------------------------------------------------------
void Swimmer::Enter(float deltaTime)
{

}

// --------------------------------------------------------
// Apply buoyancy to the swimmer
//---------------------------------------------------------
void Swimmer::ApplyBuoyancy(float deltaTime)
{
	//printf("%9.6f\n", GetPosition().y);
	Collider* col = GetCollider();
	XMFLOAT3 position = GetPosition();
	XMFLOAT3 halves = col->GetHalfSize();

	//Thanks Khan once again
	//https://www.khanacademy.org/science/physics/fluids/buoyant-force-and-archimedes-principle/a/buoyant-force-and-archimedes-principle-article
	//Calculate displaced volume and buoyancy
	float hTop = position.y + halves.y > 0 ? 0 : position.y + halves.y;
	float hBot = position.y - halves.y > 0 ? 0 : position.y - halves.y;
	float area = (2 * halves.x) * (2 * halves.z);
	float watDisplaced = area * -(hBot - hTop);
	float buoyancy = FLUID_DENSITY * GRAVITY * watDisplaced;

	//https://www.grc.nasa.gov/WWW/K-12/airplane/falling.html
	float drag = 0;
	if(position.y > 0)
	{
		//Calculate air drag if we are above the water
		drag = DRAG_COEFF * AIR_DENSITY * ((velocity * velocity * area) / 2);
	}
	else
	{
		//Calculate fluid drag if we are below the water
		drag = DRAG_COEFF * FLUID_DENSITY * ((velocity * velocity * area) / 2);
	}

	//Apply bouyancy and gravity to acceleration
	acceleration += buoyancy / MASS;
	acceleration += -GRAVITY;

	//Add acceleration to velocity
	velocity += (acceleration * deltaTime);

	//Add drag based on velocity's direction
	if(velocity < 0)
		velocity += (drag * deltaTime);
	else velocity -= (drag * deltaTime);

	//Add velocity to position
	position.y += velocity * deltaTime;

	//Reset
	acceleration = 0;

	SetPosition(position);
}

// Run this swimmer's floating behaviour
void Swimmer::Float(float deltaTime)
{
	// Rotate when idle.
	XMFLOAT3 nextRotation = this->GetRotation();
	nextRotation.x += 5 * deltaTime;
	this->SetRotation(nextRotation);
}

// Update the swimmer's buffers for snake movement
XMFLOAT3 Swimmer::GetTrailPos(float deltaTime)
{
	// Update internal timer.
	timer += deltaTime;

	// Insert newest position into our cache.
	// If the cache is full, overwrite the latest sample.
	int newIndex = (newestIndex + 1) % bufferLength;
	if (newIndex != oldestIndex)
		newestIndex = newIndex;

	positionBuffer[newestIndex] = leader->GetPosition();
	timeBuffer[newestIndex] = timer;

	// Skip ahead in the buffer to the segment containing our target time.
	float targetTime = timer - lagSeconds;
	int nextIndex;
	while (timeBuffer[nextIndex = (oldestIndex + 1) % bufferLength] < targetTime)
		oldestIndex = nextIndex;
	float span = timeBuffer[nextIndex] - timeBuffer[oldestIndex];
	float progress = 0;
	if (span > 0)
	{
		progress = (targetTime - timeBuffer[oldestIndex]) / span;
	}

	//Lerp to find the position
	XMFLOAT3 lerp;
	XMStoreFloat3(&lerp, XMVectorLerp(XMLoadFloat3(&(positionBuffer[oldestIndex])), XMLoadFloat3(&(positionBuffer[nextIndex])), progress));
	return lerp;
}

// Run this swimmer's joining behaviour
void Swimmer::Join(float deltaTime)
{
	XMFLOAT3 trailPos = GetTrailPos(deltaTime);
	float dist = ExtendedMath::DistanceFloat3(trailPos, GetPosition());
	if (dist > 0.1f)
	{
		XMFLOAT3 lerp;
		XMStoreFloat3(&lerp, XMVectorScale(XMVector3Normalize(
			XMLoadFloat3(&trailPos) - XMLoadFloat3(&GetPosition())), 5 * deltaTime)
		);
		MoveAbsolute(lerp);
	}
	//XMFLOAT3 trailPos = GetTrailPos(deltaTime);
	//float dist = ExtendedMath::DistanceFloat3(GetPosition(), trailPos);
	//if (dist > 0.25f)
	//{
	//	XMFLOAT3 lerp;
	//	XMStoreFloat3(&lerp, XMVectorLerp(
	//			XMLoadFloat3(&GetPosition()), XMLoadFloat3(&trailPos), 3 * deltaTime)
	//	);
	//	SetPosition(lerp);
	//}
	else
	{
		swmrState = SwimmerState::Following;
		printf("hit\n");
	}
}

// Run this swimmer's following behaviour
void Swimmer::Follow(float deltaTime)
{
	// Interpolate between the two samples on either side of our target time.
	XMFLOAT3 newPos = GetTrailPos(deltaTime);
	SetPosition(newPos);
}

// Check if the boat is in the following state
bool Swimmer::IsFollowing()
{
	return this->enabled && (swmrState == SwimmerState::Following);
}

// Set Swimmer to follow a game object.
void Swimmer::JoinTrail(Entity* newLeader)
{
	swmrState = SwimmerState::Joining;
	this->leader = newLeader;
	positionBuffer[0] = positionBuffer[1] = leader->GetPosition();
}

// Set Swimmer's state for when the Boat hits something
void Swimmer::StartHit()
{
	swmrState = SwimmerState::Hitting;
}

// Set Swimmer's state for when the Boat is docking the swimmers
void Swimmer::StartDock()
{
	swmrState = SwimmerState::Docking;
}
