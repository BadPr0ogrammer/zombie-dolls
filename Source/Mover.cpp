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
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/GraphicsEvents.h>

#include "Mover.h"
#include "Ragdolls.h"
#include "CreateRagdoll.h"
#include "MDRemoveCom.h"

#include <Urho3D/DebugNew.h>

#include <EASTL/string.h>
#include <string>

using namespace MonsterDolls;

Mover3D::Mover3D(Context* context) :
	LogicComponent(context),
	moveSpeed_{ 0.0f, 0.0f, 0.0f }
{
	// Only the scene update event is needed: unsubscribe from the rest for optimization
	SetUpdateEventMask(USE_UPDATE);
}

void Mover3D::SetParameters(const Vector3& moveSpeed, const BoundingBox& bounds, Ragdolls* ragdolls)
{
	moveSpeed_ = moveSpeed;
	bounds_ = bounds;
	ragdolls_ = ragdolls;
}

void Mover3D::Update(float timeStep)
{
	// If in risk of going outside the plane, rotate the model right
	Vector3 pos = node_->GetPosition();
	if (pos.z_ > bounds_.min_.z_ && pos.z_ < bounds_.max_.z_)
		// node_->Yaw(rotationSpeed_ * timeStep);
		node_->Translate(GetMoveSpeed() * timeStep);
	else {
		ragdolls_->PlaySoundEffect("BigExplosion.wav");
		ragdolls_->CreateKicking();

		auto* mdRemoveCom = node_->CreateComponent<MDRemoveCom>();
		mdRemoveCom->SetCountNum(200);

		node_->RemoveComponent<Mover3D>();
	}
}
