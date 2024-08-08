//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "Urho3D/Precompiled.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Input/FreeFlyController.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Graphics/CustomGeometry.h>
#include <Urho3D/Physics/KinematicCharacterController.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include "Urho3D/IK/IKSolver.h"
#include <Urho3D/IK/IKEvents.h>

#include "CreateRagdoll.h"
#include "Ragdolls.h"
#include "Mover.h"
#include "MDRemoveCom.h"

#include <Urho3D/DebugNew.h>


// Create animated models
const float MODEL_MOVE_SPEED = 3.0f;
const BoundingBox bounds(Vector3(-20.0f, 0.0f, -15.0f), Vector3(20.0f, 0.0f, 20.0f));

using namespace MonsterDolls;

Ragdolls::Ragdolls(Context* context)
	: Sample(context)
	, drawDebug_(false)
{
	// Register an object factory for our custom CreateRagdoll component so that we can create them to scene nodes
	if (!context->IsReflected<CreateRagdoll>())
		context->AddFactoryReflection<CreateRagdoll>();

	//skeletalAnimation_ = new SkeletalAnimation(context);
	// Register an object factory for our custom Mover3D component so that we can create them to scene nodes
	if (!context->IsReflected<Mover3D>())
		context->AddFactoryReflection<Mover3D>();

	if (!context->IsReflected<MDRemoveCom>())
		context->AddFactoryReflection<MDRemoveCom>();
}

void Ragdolls::Start()
{
	// Execute base class startup
	Sample::Start();

	// Create the scene content
	CreateScene();

	// Create the UI content
	CreateInstructions();

	// Setup the viewport for displaying the scene
	SetupViewport();

	// Hook up to the frame update and render post-update events
	SubscribeToEvents();

	// Set the mouse mode to use in the sample
	SetMouseMode(MM_RELATIVE);
	SetMouseVisible(false);
}

void Ragdolls::CreateScene()
{
	auto* cache = GetSubsystem<ResourceCache>();

	scene_ = new Scene(context_);

	// Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
	// Create a physics simulation world with default parameters, which will update at 60fps. Like the Octree must
	// exist before creating drawable components, the PhysicsWorld must exist before creating physics components.
	// Finally, create a DebugRenderer component so that we can draw physics debug geometry
	scene_->CreateComponent<Octree>();
	scene_->CreateComponent<PhysicsWorld>();
	scene_->CreateComponent<DebugRenderer>();

	// Create a Zone component for ambient lighting & fog control
	Node* zoneNode = scene_->CreateChild("Zone");
	auto* zone = zoneNode->CreateComponent<Zone>();
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
	zone->SetAmbientColor(Color(0.1f, 0.0f, 0.0f));
	zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
	zone->SetFogStart(50.0f);
	zone->SetFogEnd(300.0f);

	// Create a directional light to the world. Enable cascaded shadows on it
	Node* lightNode = scene_->CreateChild("DirectionalLight");
	lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
	auto* light = lightNode->CreateComponent<Light>();
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
	// Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
	light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

	// Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
	   // illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
	   // generate the necessary 3D texture coordinates for cube mapping
	Node* skyNode = scene_->CreateChild("Sky");
	skyNode->SetScale(500.0f); // The scale actually does not matter
	auto* skybox = skyNode->CreateComponent<Skybox>();
	skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

	{
		// Create a floor object, 500 x 500 world units. Adjust position so that the ground is at zero Y
		Node* floorNode = scene_->CreateChild("Floor");
		floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
		floorNode->SetScale(Vector3(500.0f, 1.0f, 500.0f));
		auto* floorObject = floorNode->CreateComponent<StaticModel>();
		floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		floorObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

		// Make the floor physical by adding RigidBody and CollisionShape components
		auto* body = floorNode->CreateComponent<RigidBody>();
		// We will be spawning spherical objects in this sample. The ground also needs non-zero rolling friction so that
		// the spheres will eventually come to rest
		body->SetRollingFriction(0.15f);
		auto* shape = floorNode->CreateComponent<CollisionShape>();
		// Set a box shape of size 1 x 1 x 1 for collision. The shape will be scaled with the scene node scale, so the
		// rendering and physics representation sizes should match (the box model is also 1 x 1 x 1.)
		shape->SetBox(Vector3::ONE);
	}

	// Create the camera. Limit far clip distance to match the fog. Note: now we actually create the camera node outside
	// the scene, because we want it to be unaffected by scene load / save
	// 
	//cameraNode_ = new Node(context_);
	cameraNode_ = scene_->CreateChild("Camera");

	shakeComponent_ = cameraNode_->CreateComponent<ShakeComponent>();
	shakeComponent_->SetTraumaPower(1.0f);
	shakeComponent_->SetTraumaFalloff(2.0f);
	shakeComponent_->SetTimeScale(10.0f);
	shakeComponent_->SetShiftRange(Vector3(0.0f, 0.5f, 0.0f));
	shakeComponent_->SetRotationRange(Vector3(0.0f, 0.5f, 0.0f));

	cameraNode_->CreateComponent<FreeFlyController>();
	auto* camera = cameraNode_->CreateComponent<Camera>();
	camera->SetFarClip(300.0f);

	// Set an initial position for the camera scene node above the floor
	cameraNode_->SetPosition(Vector3(0.0f, 2.0f, -20.0f));

	CreateModels();

	gunNode_ = cameraNode_->CreateChild("Gun Node");
	gunNode_->SetPosition(Vector3(0.0f, -0.2f, 0.5f));
	auto* model = gunNode_->CreateComponent<StaticModel>();
	model->SetModel(cache->GetResource<Model>("Models/ar style gun.fbx.d/Models/ar15.mdl"));
	model->SetCastShadows(true);

	auto q = Quaternion(-90.0f, 90.0f, 90.0f);
	gunNode_->SetRotation(q);
	gunNode_->SetScale(25.0f);

	shapeNode_ = cameraNode_->CreateChild("Shape Node");
	shapeNode_->SetPosition(Vector3(0.1f, -0.4f, 30.0f));
	auto* model2 = shapeNode_->CreateComponent<StaticModel>();
	model2->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));
	model2->SetMaterial(new Material(context_));
	model2->GetMaterial()->SetShaderParameter("MatEmissiveColor", Color(1, 0, 0));
	model2->SetCastShadows(true);
	shapeNode_->SetRotation(q);
	shapeNode_->SetScale(Vector3(0.05f, 50.0f, 0.05f));
}

void Ragdolls::CreateModels()
{
	auto* cache = GetSubsystem<ResourceCache>();

	if (!zombiesNode_)
		zombiesNode_ = scene_->CreateChild("Zombie");
	else
		zombiesNode_->RemoveAllChildren();

	for (int x = -5, i = 0; x <= 5; ++x, i++)
	{
		std::string name = "Zombie_" + std::to_string(i);
		Node* modelNode = zombiesNode_->CreateChild(name.c_str());

		float X = x * 4.0f;
		float Y = 14 + Random(5.9f);
		float phi = std::atan(X / Y);

		modelNode->SetPosition(Vector3(X, 0.0f, Y));
		modelNode->SetRotation(Quaternion(0.0f, 180.0f * (1.0f + 0.4f * phi / float(M_PI)), 0.0f));

		auto* modelObject = modelNode->CreateComponent<AnimatedModel>();
		//Model* model_running = cache->GetResource<Model>("Models/Zombie Running.fbx.d/Models/Ch10.mdl");
		Model* model_running = cache->GetResource<Model>("Models/Jack.mdl");
		modelObject->SetModel(model_running);
		modelObject->SetCastShadows(true);
		// Set the model to also update when invisible to avoid staying invisible when the model should come into
		// view, but does not as the bounding box is not updated
		modelObject->SetUpdateInvisible(true);

		// Create a rigid body and a collision shape. These will act as a trigger for transforming the
		// model into a ragdoll when hit by a moving object
		auto* body = modelNode->CreateComponent<RigidBody>();
		// The Trigger mode makes the rigid body only detect collisions, but impart no forces on the
		// colliding objects
		body->SetTrigger(true);
		auto* shape = modelNode->CreateComponent<CollisionShape>();
		// Create the capsule shape with an offset so that it is correctly aligned with the model, which
		// has its origin at the feet
		shape->SetCapsule(0.7f, 2.0f, Vector3(0.0f, 1.0f, 0.0f));

		// Create an AnimationState for a walk animation. Its time position will need to be manually updated to advance the
		// animation, The alternative would be to use an AnimationController component which updates the animation automatically,
		// but we need to update the model's position manually in any case
		auto* animation_running = cache->GetResource<Animation>("Models/Jack_Walk.ani");
		//Animation* animation_running = cache->GetResource<Animation>("Models/Zombie Running.fbx.d/Animations/mixamo.com.ani");
		const float startTime = Random(animation_running->GetLength());
		auto animationController = modelNode->CreateComponent<AnimationController>();
		animationController->PlayNewExclusive(AnimationParameters{ animation_running }.Looped().Time(startTime));

		// Create our custom Mover3D component that will move & animate the model during each frame's update
		auto* mover = modelNode->CreateComponent<Mover3D>();
		Vector3 v{ MODEL_MOVE_SPEED * tan(phi) * 0.1f, 0, MODEL_MOVE_SPEED };
		mover->SetParameters(v, bounds, this);

		// Create a custom component that reacts to collisions and creates the ragdoll
		auto* crd = modelNode->CreateComponent<CreateRagdoll>();
		crd->SetRagdolls(this);
	}
}

void Ragdolls::CreateInstructions()
{
	auto* cache = GetSubsystem<ResourceCache>();
	auto* ui = GetSubsystem<UI>();

	// Construct new Text object, set string to display and font to use
	/*
	auto* instructionText = GetUIRoot()->CreateChild<Text>();
	instructionText->SetText(
		"Use WASD keys and mouse/touch to move\n"
		"LMB to spawn physics objects\n"
		"F5 to save scene, F7 to load\n"
		"Space to toggle physics debug geometry"
	);
	instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
	// The text has multiple rows. Center them in relation to each other
	instructionText->SetTextAlignment(HA_CENTER);

	// Position the text relative to the screen center
	instructionText->SetHorizontalAlignment(HA_CENTER);
	instructionText->SetVerticalAlignment(VA_CENTER);
	instructionText->SetPosition(0, GetUIRoot()->GetHeight() / 4);
	*/
}

void Ragdolls::SetupViewport()
{
	auto* renderer = GetSubsystem<Renderer>();

	// Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
	SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
	SetViewport(0, viewport);
}

void Ragdolls::MoveCamera(float timeStep)
{
	// Do not move if the UI has a focused element (the console)
	if (GetSubsystem<UI>()->GetFocusElement())
		return;

	auto* input = GetSubsystem<Input>();

	// "Shoot" a physics object with left mousebutton
	if (input->GetMouseButtonPress(MOUSEB_LEFT))
		SpawnObject();

	// Check for loading / saving the scene
	if (input->GetKeyPress(KEY_F5))
	{
		File saveFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/Ragdolls.xml", FILE_WRITE);
		scene_->SaveXML(saveFile);
	}
	if (input->GetKeyPress(KEY_F7))
	{
		File loadFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/Ragdolls.xml", FILE_READ);
		scene_->LoadXML(loadFile);
	}

	// Toggle physics debug geometry with space
	if (input->GetKeyPress(KEY_SPACE))
		drawDebug_ = !drawDebug_;
}

void Ragdolls::SpawnObject()
{
	auto* cache = GetSubsystem<ResourceCache>();

	Node* boxNode = scene_->CreateChild("Sphere");
	boxNode->SetPosition(cameraNode_->GetPosition());
	boxNode->SetRotation(cameraNode_->GetRotation());
	boxNode->SetScale(0.25f);
	auto* boxObject = boxNode->CreateComponent<StaticModel>();
	boxObject->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
	boxObject->SetMaterial(cache->GetResource<Material>("Materials/StoneSmall.xml"));
	boxObject->SetCastShadows(true);

	auto* body = boxNode->CreateComponent<RigidBody>();
	body->SetMass(1.0f);
	body->SetRollingFriction(0.15f);
	auto* shape = boxNode->CreateComponent<CollisionShape>();
	shape->SetSphere(1.0f);

	const float OBJECT_VELOCITY = 20.0f;

	// Set initial velocity for the RigidBody based on camera forward vector. Add also a slight up component
	// to overcome gravity better
	body->SetLinearVelocity(cameraNode_->GetRotation() * Vector3(0.0f, 0.0f, 7.0f) * OBJECT_VELOCITY);

	PlaySoundEffect("SmallExplosion.wav");

	shakeComponent_->AddTrauma(1.0f);
}

void Ragdolls::SubscribeToEvents()
{
	// Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request
	// debug geometry
	SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(Ragdolls, HandlePostRenderUpdate));
}

void Ragdolls::Update(float timeStep)
{
	// Move the camera, scale movement with time step
	MoveCamera(timeStep);
}

void Ragdolls::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
	// If draw debug mode is enabled, draw physics debug geometry. Use depth test to make the result easier to interpret
	if (drawDebug_)
		scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
}

void Ragdolls::CreateKicking()
{
	for (auto ptr : zombiesNode_->GetChildren()) {
		auto* cache = GetSubsystem<ResourceCache>();
		Model* model_running = cache->GetResource<Model>("Models/MeleeAttack.fbx.d/Models/Ch36.mdl");
		Animation* animation_running = cache->GetResource<Animation>("Models/MeleeAttack.fbx.d/Animations/mixamo.com.ani");

		auto* modelObject = ptr->GetComponent<AnimatedModel>();
		modelObject->SetModel(model_running);

		auto animationController = ptr->GetComponent<AnimationController>();
		animationController->PlayNewExclusive(AnimationParameters{ animation_running }.Looped().Time(0));
	}
}
