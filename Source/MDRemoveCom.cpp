#include <Urho3D/Core/Context.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Scene/Serializable.h>

#include "MDRemoveCom.h"

using namespace MonsterDolls;

void MDRemoveCom::Update(float timeStep)
{
	if (mdCount_++ > mdCountNum_)
		GetNode()->Remove();
}

void MDRemoveCom::RegisterObject(Context* context)
{
	context->AddFactoryReflection<MDRemoveCom>();
	//URHO3D_ATTRIBUTE("MD Count", int, mdCount_, 0, AM_DEFAULT);
	//URHO3D_ATTRIBUTE("MD Count Num", int, mdCountNum_, 100, AM_DEFAULT);
}
