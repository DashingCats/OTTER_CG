/*
Week 11 Tutorial Sample - Created for INFR 2310 at Ontario Tech.
(c) Atiya Nova and Samantha Stahlke 2020
*/

#include "NOU/App.h"
#include "NOU/Input.h"
#include "NOU/Entity.h"
#include "NOU/CCamera.h"
#include "NOU/CMeshRenderer.h"
#include "NOU/GLTFLoader.h"

#include "CSkinnedMeshRenderer.h"
#include "CAnimator.h"
#include "GLTFLoaderSkinning.h"

#include "Logging.h"
#include "GLM/gtx/matrix_decompose.hpp"
#include "imgui.h"

#include <memory>

using namespace nou;

int main()
{
	App::Init("Week 11 Tutorial - Blending", 600, 600);
	App::SetClearColor(glm::vec4(0.2f, 0.0f, 0.4f, 1.0f));

	App::InitImgui();

	//Loading shaders...
	auto v_texLit = std::make_unique<Shader>("shaders/texturedlit.vert", GL_VERTEX_SHADER);
	auto f_texLit = std::make_unique<Shader>("shaders/texturedlit.frag", GL_FRAGMENT_SHADER);

	ShaderProgram prog_texLit = ShaderProgram({ v_texLit.get(), f_texLit.get() });

	auto v_skinning = std::make_unique<Shader>("shaders/gpuskinning.vert", GL_VERTEX_SHADER);
	auto f_skinning = std::make_unique<Shader>("shaders/gpuskinning.frag", GL_FRAGMENT_SHADER);

	ShaderProgram prog_skinning = ShaderProgram({ v_skinning.get(), f_skinning.get() });

	//Loading textures...
	Texture2D jointTex("textures/joint_texture.png");
	Texture2D plainTex("textures/plain_texture.png");

	//Setting up materials...
	Material boiMat(prog_skinning);
	boiMat.AddTexture("albedo", plainTex);

	Material jointMat(prog_texLit);
	jointMat.AddTexture("albedo", jointTex);

	//Load a basic box mesh for drawing our skeleton.
	auto boxMesh = std::make_unique<Mesh>();
	GLTF::LoadMesh("models/box/BoxTextured.gltf", *boxMesh);

	//Load our skinned boy mesh (in base pose).
	auto boiMesh = std::make_unique<SkinnedMesh>();
	GLTF::LoadSkinnedMesh("models/boi/Base.gltf", *boiMesh);

	//Load all our animations.
	auto idleAnim = std::make_unique<SkeletalAnim>();
	GLTF::LoadAnimation("models/boi/Idle.gltf", *(idleAnim.get()));

	auto headAnim = std::make_unique<SkeletalAnim>();
	GLTF::LoadAnimation("models/boi/HeadShake.gltf", *(headAnim.get()));

	auto walkAnim = std::make_unique<SkeletalAnim>();
	GLTF::LoadAnimation("models/boi/Walk.gltf", *(walkAnim.get()));


	headAnim->Keep({ 13 });
	headAnim->MakeDiffWith(boiMesh->m_skeleton);

	//Make our camera...
	Entity camEntity = Entity::Create();
	auto& cam = camEntity.Add<CCamera>(camEntity);
	cam.Perspective(60.0f, 1.0f, 0.1f, 100.0f);
	camEntity.transform.m_pos = glm::vec3(0.0f, 0.0f, 2.0f);

	//Make an entity for the boi...
	Entity boiEntity = Entity::Create();
	//Skinned mesh renderer.
	boiEntity.Add<CSkinnedMeshRenderer>(boiEntity, *boiMesh, boiMat);
	boiEntity.transform.m_pos = glm::vec3(0.0f, -0.75f, 0.0f);
	boiEntity.transform.m_scale = glm::vec3(0.5f, 0.5f, 0.5f);
	//Skeletal animator.
	auto& skinnedAnimator = boiEntity.Add<CAnimator>(boiEntity);
	//TODO: Add idle animation as blend tree root.
	skinnedAnimator.GetBlendtree()->Insert(*idleAnim);

	//TODO: Add additive head nod animation to blend tree.
	SkeletalAnimNode* addNodeB = skinnedAnimator.GetBlendtree()->Insert(*walkAnim, SkeletalAnimNode::BlendMode::BLEND, 1.0f);
	SkeletalAnimNode* addNode = skinnedAnimator.GetBlendtree()->Insert(*headAnim, SkeletalAnimNode::BlendMode::ADD, 1.0f);


	//Make an entity for drawing our debug skeleton (just a box at each joint).
	Entity jointEntity = Entity::Create();
	jointEntity.Add<CMeshRenderer>(jointEntity, *boxMesh, jointMat);
	//Parented to the entity our skeleton is attached to.
	jointEntity.transform.SetParent(&(boiEntity.transform));
	jointEntity.transform.m_scale = glm::vec3(0.05f, 0.05f, 0.05f);
	
	//To spin our boi every frame. 
	float anglePerSecond = 30.0f;

	App::Tick();

	while (!App::IsClosing() && !Input::GetKey(GLFW_KEY_ESCAPE))
	{
		App::FrameStart();

		float deltaTime = App::GetDeltaTime();

		camEntity.Get<CCamera>().Update();

		//Spin de boi.
		float degreeSpin = anglePerSecond * deltaTime;
		boiEntity.transform.m_rotation = glm::angleAxis(glm::radians(degreeSpin), glm::vec3(0.0f, 1.0f, 0.0f)) * boiEntity.transform.m_rotation;
		boiEntity.transform.RecomputeGlobal();

		//Update the animator, and draw the boi.
		boiEntity.Get<CAnimator>().Update(deltaTime);
		boiEntity.Get<CSkinnedMeshRenderer>().Draw();

		//As a debug utility/demo: Draw our joints.
		glDisable(GL_DEPTH_TEST); 

		auto& jointTransform = jointEntity.transform;

		glm::vec3 scale, skew;
		glm::vec4 persp;

		const Skeleton& skeleton = boiEntity.Get<CSkinnedMeshRenderer>().GetSkeleton();

		for (auto& joint : skeleton.m_joints)
		{
			glm::decompose(joint.m_global,
						   scale,
						   jointTransform.m_rotation,
						   jointTransform.m_pos,
						   skew,
						   persp);

			jointTransform.RecomputeGlobal();
			jointEntity.Get<CMeshRenderer>().Draw();
		}
		
		glEnable(GL_DEPTH_TEST);

		//Simple controls for blending.
		App::StartImgui();

		//TODO: Add controls for strength of our additive nod animation.
		static bool panel = true;
		static float addParam = 0.0f;
		static float addParamB = 0.0f;

		ImGui::Begin("RIP - Ducky", &panel, ImVec2(300,300));

		ImGui::SliderFloat("Blend Strength", &addParamB, 0.0f, 1.0f);
		addNodeB->SetBlendParam(addParamB);
		ImGui::SliderFloat("Head Shake", &addParam, 0.0f, 1.0f);
		addNode->SetBlendParam(addParam);

		
		
		
		ImGui::End();

		App::EndImgui();

		App::SwapBuffers();
	}

	App::Cleanup();

	return 0;
}