
//
// Copyright (c) 2017-2022 the rbfx project.
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

#pragma once

#include <Urho3D/Engine/StateManager.h>
#include <Urho3D/UI/SplashScreen.h>
#include <Urho3D/Plugins/PluginManager.h>

#include <string>
#include <vector>

#include "GameState.h"

using Urho3D::Vector2;
using Urho3D::Vector3;
using Urho3D::Quaternion;
using Urho3D::StringHash;
using Urho3D::VariantMap;
using Urho3D::SharedPtr;

namespace MonsterDolls
{
	struct SampleInformation
	{
		/// Title of the sample.
		ea::string name_;
		/// Type id of sample application.
		StringHash type_;
	};

	class GameApp : public Urho3D::Application
	{
		// Enable type information.
		URHO3D_OBJECT(GameApp, Urho3D::Application);

	public:
		/// Construct.
		explicit GameApp(Urho3D::Context* context);

		/// Setup before engine initialization. Modifies the engine parameters.
		void Setup() override;
		/// Setup after engine initialization. Creates the logo, console & debug HUD.
		void Start() override;
		/// Cleanup after the main loop. Called by Application.
		void Stop() override;

		/// Return command line arguments.
		const ea::vector<ea::string>& GetArgs() const { return commandLineArgs_; }

		Urho3D::ApplicationState* GetMenuState() const { return startupScreen_; }

	private:
		///
		void OnClickSample(VariantMap& args);
		///
		void OnKeyPress(VariantMap& args);
		///
		void OnFrameStart();
		void OnCloseCurrentSample();
		/// Start execution of specified sample.
		void StartSample(StringHash sampleType);
		///
		SharedPtr<Urho3D::ApplicationState> startupScreen_;
		///
		SharedPtr<Urho3D::UIElement> startButtonHolder_;
		/// Logo sprite.
		SharedPtr<Urho3D::Sprite> logoSprite_;
		///
		bool isClosing_ = false;
		/// Array of sample command line args. Use STL for compatibility with CLI.
		std::vector<std::string> commandLineArgsTemp_; // TODO: Get rid of it
		ea::vector<ea::string> commandLineArgs_;

		/// Generic Serializable inspector.
		/// @{
		SharedPtr<Urho3D::Scene> inspectorNode_;
		bool oldMouseVisible_{};
		Urho3D::MouseMode oldMouseMode_{};
		/// @}
	};
}
