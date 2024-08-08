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
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/CommandLine.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/RenderPipeline/RenderPipeline.h>
#include <Urho3D/Resource/ResourceCache.h>
#if URHO3D_RMLUI
#include <Urho3D/RmlUI/RmlSerializableInspector.h>
#include <Urho3D/RmlUI/RmlUI.h>
#endif
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "Ragdolls.h"

#include "Rotator.h"

#include "SamplesManager.h"

using namespace MonsterDolls;

// Expands to this example's entry-point
URHO3D_DEFINE_APPLICATION_MAIN(SamplesManager);

using namespace MonsterDolls;

//namespace Urho3D{

SamplesManager::SamplesManager(Context* context) :
	Application(context)
{
}

void SamplesManager::Setup()
{
	// Modify engine startup parameters
	engineParameters_[EP_WINDOW_TITLE] = "Monster Dolls";
	engineParameters_[EP_APPLICATION_NAME] = "Monster Dolls";
	engineParameters_[EP_LOG_NAME] = "conf://MonsterDolls.log";
	engineParameters_[EP_BORDERLESS] = false;
	engineParameters_[EP_HEADLESS] = false;
	engineParameters_[EP_SOUND] = true;
	engineParameters_[EP_RESOURCE_PATHS] = "Data;CoreData;Cache";
	engineParameters_[EP_ORIENTATIONS] = "LandscapeLeft LandscapeRight Portrait";
	engineParameters_[EP_WINDOW_RESIZABLE] = true;
	engineParameters_[EP_FULL_SCREEN] = false;
#if MOBILE
	engineParameters_[EP_ORIENTATIONS] = "Portrait";
#endif
	if (!engineParameters_.contains(EP_RESOURCE_PREFIX_PATHS))
	{
		engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";..;../..";
		if (GetPlatform() == PlatformId::MacOS ||
			GetPlatform() == PlatformId::iOS)
			engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";../Resources;../..";
		else
			engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";..;../..";
	}
	engineParameters_[EP_AUTOLOAD_PATHS] = "Autoload";
}

void SamplesManager::Start()
{
	ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
	VirtualFileSystem* vfs = context_->GetSubsystem<VirtualFileSystem>();
	vfs->SetWatching(true);

	UI* ui = context_->GetSubsystem<UI>();

#if MOBILE
	// Scale UI for high DPI mobile screens
	auto dpi = GetSubsystem<Graphics>()->GetDisplayDPI();
	if (dpi.z_ >= 200)
		ui->SetScale(2.0f);
#endif

	// Parse command line arguments
	ea::transform(commandLineArgsTemp_.begin(), commandLineArgsTemp_.end(), ea::back_inserter(commandLineArgs_),
		[](const std::string& str) { return ea::string(str.c_str()); });

	// Register an object factory for our custom Rotator component so that we can create them to scene nodes
	context_->AddFactoryReflection<Rotator>();
	/// context_->AddFactoryReflection<SampleSelectionScreen>();

	inspectorNode_ = MakeShared<Scene>(context_);

	startupScreen_ = MakeShared<ApplicationState>(context_);
	startupScreen_->SetMouseMode(MM_FREE);
	startupScreen_->SetMouseVisible(true);

	context_->GetSubsystem<StateManager>()->EnqueueState(startupScreen_);

#if URHO3D_SYSTEMUI
	if (DebugHud* debugHud = context_->GetSubsystem<Engine>()->CreateDebugHud())
		debugHud->ToggleAll();
#endif
	auto* input = context_->GetSubsystem<Input>();
	SubscribeToEvent(E_RELEASED, &SamplesManager::OnClickSample);
	//// SubscribeToEvent(&sampleSelectionScreen_->dpadAdapter_, E_KEYUP, &SamplesManager::OnArrowKeyPress);
	SubscribeToEvent(input, E_KEYUP, &SamplesManager::OnKeyPress);
	SubscribeToEvent(E_SAMPLE_EXIT_REQUESTED, &SamplesManager::OnCloseCurrentSample);
	//// SubscribeToEvent(E_JOYSTICKBUTTONDOWN, &SamplesManager::OnButtonPress);
	SubscribeToEvent(E_BEGINFRAME, &SamplesManager::OnFrameStart);

	startupScreen_->GetUIRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
	IntVector2 listSize = VectorMin(IntVector2(300, 36/*600*/), ui->GetRoot()->GetSize());
	auto* layout = startupScreen_->GetUIRoot()->CreateChild<UIElement>();
	startButtonHolder_ = layout;
	layout->SetLayoutMode(LM_VERTICAL);
	layout->SetAlignment(HA_CENTER, VA_CENTER);
	layout->SetSize(listSize);
	layout->SetStyleAuto();

	context_->AddFactoryReflection<Ragdolls>();

	auto button = MakeShared<Button>(context_);
	button->SetMinHeight(30);
	button->SetStyleAuto();
	button->SetVar("SampleType", Ragdolls::GetTypeStatic());

	auto* title = button->CreateChild<Text>();
	title->SetAlignment(HA_CENTER, VA_CENTER);
	title->SetText("Start");
	title->SetFont(context_->GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 30);
	title->SetStyleAuto();

	layout->AddChild(button);

	// Get logo texture
	Texture2D* logoTexture = cache->GetResource<Texture2D>("Textures/FishBoneLogo.png");
	if (!logoTexture)
		return;

	logoSprite_ = startupScreen_->GetUIRoot()->CreateChild<Sprite>();
	logoSprite_->SetTexture(logoTexture);

	int textureWidth = logoTexture->GetWidth();
	int textureHeight = logoTexture->GetHeight();
	logoSprite_->SetScale(256.0f / textureWidth);
	logoSprite_->SetSize(textureWidth, textureHeight);
	logoSprite_->SetHotSpot(textureWidth, textureHeight);
	logoSprite_->SetAlignment(HA_RIGHT, VA_BOTTOM);
	logoSprite_->SetOpacity(0.9f);
	logoSprite_->SetPriority(-100);
}

void SamplesManager::Stop()
{
	engine_->DumpResources(true);
	GetSubsystem<StateManager>()->Reset();
}

void SamplesManager::OnClickSample(VariantMap& args)
{
	using namespace Released;
	StringHash sampleType = static_cast<UIElement*>(args[P_ELEMENT].GetPtr())->GetVar("SampleType").GetStringHash();
	if (!sampleType)
		return;

	StartSample(sampleType);
}

void SamplesManager::StartSample(StringHash sampleType)
{
	UI* ui = context_->GetSubsystem<UI>();
	ui->SetFocusElement(nullptr);

	StringVariantMap args;
	args["Args"] = GetArgs();
	context_->GetSubsystem<StateManager>()->EnqueueState(sampleType, args);
}

void SamplesManager::OnKeyPress(VariantMap& args)
{
	using namespace KeyUp;

	int key = args[P_KEY].GetInt();

	// Close console (if open) or exit when ESC is pressed
	StateManager* stateManager = GetSubsystem<StateManager>();
	auto* currentSample = dynamic_cast<Sample*>(stateManager->GetState());
	if (key == KEY_ESCAPE && (!currentSample || currentSample->IsEscapeEnabled()))
		isClosing_ = true;

#if URHO3D_RMLUI
	if (key == KEY_I)
	{
		auto* renderer = GetSubsystem<Renderer>();
		auto* input = GetSubsystem<Input>();
		Viewport* viewport = renderer ? renderer->GetViewport(0) : nullptr;
		RenderPipelineView* renderPipelineView = viewport ? viewport->GetRenderPipelineView() : nullptr;

		if (inspectorNode_->HasComponent<RmlSerializableInspector>())
		{
			inspectorNode_->RemoveComponent<RmlSerializableInspector>();

			input->SetMouseVisible(oldMouseVisible_);
			input->SetMouseMode(oldMouseMode_);
		}
		else if (renderPipelineView)
		{
			auto inspector = inspectorNode_->CreateComponent<RmlSerializableInspector>();
			inspector->Connect(renderPipelineView->GetRenderPipeline());

			oldMouseVisible_ = input->IsMouseVisible();
			oldMouseMode_ = input->GetMouseMode();
			input->SetMouseVisible(true);
			input->SetMouseMode(MM_ABSOLUTE);
		}
	}
#endif

	if (!startupScreen_->IsActive())
		return;

}

void SamplesManager::OnFrameStart()
{

	if (isClosing_)
	{
		StateManager* stateManager = context_->GetSubsystem<StateManager>();
		isClosing_ = false;
		if (stateManager->GetTargetState() != startupScreen_->GetTypeNameStatic())
		{
			Input* input = context_->GetSubsystem<Input>();
			UI* ui = context_->GetSubsystem<UI>();
			stateManager->EnqueueState(startupScreen_);
		}
		else
		{
#if URHO3D_SYSTEMUI
			if (auto* console = GetSubsystem<Console>())
			{
				if (console->IsVisible())
				{
					console->SetVisible(false);
					return;
				}
			}
#endif
#if !defined(__EMSCRIPTEN__)
			context_->GetSubsystem<Engine>()->Exit();
#endif
		}
#if URHO3D_RMLUI
		// Always close inspector
		inspectorNode_->RemoveComponent<RmlSerializableInspector>();
#endif
	}
}

void SamplesManager::OnCloseCurrentSample()
{
	isClosing_ = true;
}
