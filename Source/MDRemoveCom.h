#include <Urho3D/Math/StringHash.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/LogicComponent.h>

namespace ZombieDolls {
	class MDRemoveCom : public Urho3D::LogicComponent
	{
		URHO3D_OBJECT(MDRemoveCom, Urho3D::LogicComponent);
	public:
		MDRemoveCom(Urho3D::Context* context)
			: Urho3D::LogicComponent(context)
		{
			SetUpdateEventMask(Urho3D::USE_UPDATE);
		}
		void Update(float timeStep) override;
		/// Register object factory and attributes.
		static void RegisterObject(Urho3D::Context* context);

		void SetCountNum(int i) { mdCountNum_ = i; }
	private:
		int mdCount_ = 0;
		int mdCountNum_ = 100;
	};
}
