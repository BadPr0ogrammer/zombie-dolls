#include <Urho3D/Math/StringHash.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/LogicComponent.h>

using namespace Urho3D;

namespace MonsterDolls {
	class MDRemoveCom : public LogicComponent
	{
		URHO3D_OBJECT(MDRemoveCom, LogicComponent);
	public:
		MDRemoveCom(Context* context)
			: LogicComponent(context)
		{
			SetUpdateEventMask(USE_UPDATE);
		}
		void Update(float timeStep) override;
		/// Register object factory and attributes.
		static void RegisterObject(Context* context);

		void SetCountNum(int i) { mdCountNum_ = i; }
	private:
		int mdCount_ = 0;
		int mdCountNum_ = 100;
	};
}
