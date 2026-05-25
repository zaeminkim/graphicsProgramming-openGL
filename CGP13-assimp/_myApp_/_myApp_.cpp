// sb7.h 헤더 파일을 포함시킨다.
#include <sb7.h>
#include <vmath.h>
#include <shader.h>
#include <vector>

#include "SimpleModel.h"
#include "Model.h"
#include "Animation.h"
#include "Animator.h"
#include "Collision.h"
#include <glm/gtc/type_ptr.hpp>

// sb7::application을 상속받는다.
class my_application : public sb7::application
{
public:
	// 쉐이더 프로그램 컴파일한다.
	GLuint compile_shader(const char* vs_file, const char* fs_file)
	{
		// 버텍스 쉐이더를 생성하고 컴파일한다.
		GLuint vertex_shader = sb7::shader::load(vs_file, GL_VERTEX_SHADER);

		// 프래그먼트 쉐이더를 생성하고 컴파일한다.
		GLuint fragment_shader = sb7::shader::load(fs_file, GL_FRAGMENT_SHADER);

		// 프로그램을 생성하고 쉐이더를 Attach시키고 링크한다.
		GLuint program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, fragment_shader);
		glLinkProgram(program);

		// 이제 프로그램이 쉐이더를 소유하므로 쉐이더를 삭제한다.
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		return program;
	}

	// 애플리케이션 초기화 수행한다.
	virtual void startup()
	{
		// 쉐이더 프로그램 컴파일 및 연결
		shader_program[0] = compile_shader("simple_phong_vs.glsl", "simple_phong_fs.glsl");

		//stbi_set_flip_vertically_on_load(true);

		// 객체 정의 : bg --------------------------------------------------
		// 위치와 컬러, 텍스처 좌표를 정의한다.
		GLfloat bg_pos[] = {
			// 첫번째 삼각형
			-20.0f, 0, 20.0f,
			20.0f, 0, 20.0f,
			20.0f, 0, -20.0f,
			// 두번째 삼각형
			-20.0f, 0, 20.0f,
			20.0f, 0, -20.0f,
			-20.0f, 0, -20.0f
		};
		GLfloat bg_tex[] = {
			0.0f, 0.0f,
			40.0f, 0.0f,
			40.0f, 40.0f,

			0.0f, 0.0f,
			40.0f, 40.0f,
			0.0f, 40.0f
		};
		GLfloat bg_norm[] = {
			0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f,

			0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f
		};


		bg.init();
		bg.setupMesh(6, bg_pos, bg_tex, bg_norm);
		bg.loadDiffuseMap("wall.jpg");

		titan.init();
		titan.loadModel("model/fatTitan.gltf");

		corps.init();
		corps.loadModel("corps/scene.gltf");

		house.init();
		house.loadModel("house/scene.gltf");
		// house model의 AABB 계산
		houseLocalBox = CalculateModelLocalAABB(house);
		AddBuilding(vmath::vec3(0.0f, 0.0f, 0.0f), 0.01f);
		AddBuilding(vmath::vec3(15.0f, 0.0f, 5.0f), 0.01f);
		AddBuilding(vmath::vec3(-12.0f, 0.0f, -8.0f), 0.01f);
		AddBuilding(vmath::vec3(8.0f, 0.0f, -18.0f), 0.012f);

		// 애니메이션 출력하기
		titanIdle = new Animation("model/fatTitan.gltf", &titan, "Idle");
		titanWalk = new Animation("model/fatTitan.gltf", &titan, "Walk");
		titanAttack = new Animation("model/fatTitan.gltf", &titan, "Attack");
		titanJump = new Animation("model/fatTitan.gltf", &titan, "Jump");

		titanAnimator = new Animator(titanIdle);

		//// player 초기 위치
		//playerPos = vmath::vec3(0.0f, 0.0f, 20.0f);
		//playerYaw = 180.0f;
		//playerScale = 1.0f;

		// 마우스 커서 안 보이게 하기
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	// 애플리케이션 끝날 때 호출된다.
	virtual void shutdown()
	{
		glDeleteProgram(shader_program[0]);

		delete titanAnimator;
		delete titanIdle;
		delete titanWalk;
		delete titanAttack;
		delete titanJump;

		titanAnimator = nullptr;
		titanIdle = nullptr;
		titanWalk = nullptr;
		titanAttack = nullptr;
		titanJump = nullptr;
	}

	virtual void render(double currentTime)
	{
		if (lastTime == 0.0)
		{
			lastTime = currentTime;
		}

		deltaTime = static_cast<float>(currentTime - lastTime);
		lastTime = currentTime;

		updatePlayer(deltaTime);
		updateJump(deltaTime);

		//// playerMoving : 이동할 때에만 걷기 애니메이션 출력 -> 이동 중일 때만 애니메이션 시간이 증가함
		//if (playerMoving && titanAnimator)
		//{
		//	titanAnimator->UpdateAnimation(deltaTime);
		//}

		// 애니메이션 시간 업데이트 방식 수정하기
		if (titanAnimator)
		{
			updateAnimationState();
			titanAnimator->UpdateAnimation(deltaTime);
		}

		const GLfloat skyColor[] = { 0.55f, 0.75f, 0.95f, 1.0f };
		glClearBufferfv(GL_COLOR, 0, skyColor);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glViewport(0, 0, info.windowWidth, info.windowHeight);

		// 카메라 매트릭스 계산 ------------------------------
		// 플레이어 이동 시점
		float yawRad = cameraYaw * 3.141592f / 180.0f;
		float pitchRad = cameraPitch * 3.141592f / 180.0f;

		vmath::vec3 cameraDir = vmath::vec3(
			cosf(pitchRad) * sinf(yawRad),
			sinf(pitchRad),
			cosf(pitchRad) * cosf(yawRad)
		);

		// 플레이어가 카메라 방향을 바라보게 함
		//playerYaw = cameraYaw;


		// 3인칭 카메라 위치
		vmath::vec3 eye;
		vmath::vec3 center;
		vmath::vec3 up(0.0f, 1.0f, 0.0f);

		if (isFirstPerson)
		{
			// 1인칭: 플레이어 눈 위치
			float eyeHeight = 3.7f;
			float eyeForwardOffset = 0.15f;

			float yawOnlyRad = playerYaw * 3.141592f / 180.0f;

			vmath::vec3 modelForward = vmath::vec3(
				sinf(yawOnlyRad),
				0.0f,
				cosf(yawOnlyRad)
			);

			eye = playerPos
				+ vmath::vec3(0.0f, eyeHeight, 0.0f)
				+ modelForward * eyeForwardOffset;

			center = eye + cameraDir * 5.0f;
		}
		else
		{
			// 3인칭: 플레이어 뒤쪽 카메라
			vmath::vec3 target = playerPos + vmath::vec3(0.0f, 1.5f, 0.0f);
			eye = target - cameraDir * thirdPersonDistance + vmath::vec3(0.0f, thirdPersonHeight, 0.0f);
			center = target + cameraDir * 5.0f;
		}
		vmath::mat4 lookAt = vmath::lookat(eye, center, up);


		float fov = 50.f;
		vmath::mat4 projM = vmath::perspective(fov, (float)info.windowWidth / info.windowHeight, 0.1f, 1000.0f);

		// 라이팅 설정 ---------------------------------------
		vmath::vec3 lightPos = vmath::vec3(100.0f, 200.0f, 100.0f);
		vmath::vec3 lightColor(1.0f, 1.0f, 1.0f);
		vmath::vec3 viewPos = eye;



		// 객체 그리기 ---------------------------------------
		glUseProgram(shader_program[0]);

		glUniformMatrix4fv(glGetUniformLocation(shader_program[0], "projection"), 1, GL_FALSE, projM);
		glUniformMatrix4fv(glGetUniformLocation(shader_program[0], "view"), 1, GL_FALSE, lookAt);

		glUniform3fv(glGetUniformLocation(shader_program[0], "viewPos"), 1, viewPos);

		vmath::vec3 lightAmbient(0.55f, 0.55f, 0.55f);
		vmath::vec3 lightDiffuse(0.85f, 0.82f, 0.75f);
		vmath::vec3 lightSpecular(0.35f, 0.35f, 0.35f);
		glUniform3fv(glGetUniformLocation(shader_program[0], "light.direction"), 1, lightPos);
		glUniform3fv(glGetUniformLocation(shader_program[0], "light.ambient"), 1, lightAmbient);
		glUniform3fv(glGetUniformLocation(shader_program[0], "light.diffuse"), 1, lightDiffuse);
		glUniform3fv(glGetUniformLocation(shader_program[0], "light.specular"), 1, lightSpecular);

		// 1.0f = 현실의 1m 로 가정하고 draw()
		vmath::mat4 model_bg = vmath::translate(0.0f, 0.0f, 0.0f) * vmath::scale(5.5f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program[0], "model"), 1, GL_FALSE, model_bg);
		glUniform1i(glGetUniformLocation(shader_program[0], "useAnimation"), 0);
		bg.draw(shader_program[0]); // VAO, Texture 바인드 안 해도 됨 -> render()에서 draw() 하나만 작성하면 됨

		//vmath::mat4 model_house = vmath::translate(0.0f, 0.0f, 0.0f) * vmath::scale(0.01f);
		//vmath::mat4 model_house = vmath::translate(housePos[0], housePos[1], housePos[2]) * vmath::scale(houseScale);
		//glUniformMatrix4fv(glGetUniformLocation(shader_program[0], "model"), 1, GL_FALSE, model_house);
		//glUniform1i(glGetUniformLocation(shader_program[0], "useAnimation"), 0);
		//house.draw(shader_program[0]);

		glUniform1i(glGetUniformLocation(shader_program[0], "useAnimation"), 0);

		for (const BuildingInstance& building : buildings)
		{
			vmath::mat4 model_house = vmath::translate(building.position[0], building.position[1], building.position[2]) * vmath::scale(building.scale);
			glUniformMatrix4fv(glGetUniformLocation(shader_program[0], "model"), 1, GL_FALSE, model_house);
			house.draw(shader_program[0]);
		}


		if (titanAnimator)
		{
			glUniform1i(glGetUniformLocation(shader_program[0], "useAnimation"), 1);

			const auto& transforms = titanAnimator->GetFinalBoneMatrices();
			for (int i = 0; i < transforms.size(); ++i)
			{
				std::string name = "finalBonesMatrices[" + std::to_string(i) + "]";
				glUniformMatrix4fv(
					glGetUniformLocation(shader_program[0], name.c_str()),
					1,
					GL_FALSE,
					glm::value_ptr(transforms[i])
				);
			}
		}
		else
		{
			glUniform1i(glGetUniformLocation(shader_program[0], "useAnimation"), 0);
		}

		//corps.draw(shader_program[0]);

		//// 1인칭, 3인칭 변환 시 렌더 모델 스위치
		//if (!isFirstPerson)
		//{
		//	vmath::mat4 model_titan = vmath::translate(playerPos[0], playerPos[1], playerPos[2]) * vmath::rotate(playerYaw, 0.0f, 1.0f, 0.0f) * vmath::scale(playerScale);
		//	glUniformMatrix4fv(glGetUniformLocation(shader_program[0], "model"), 1, GL_FALSE, model_titan);
		//	titan.draw(shader_program[0]);
		//}

		vmath::mat4 model_titan = vmath::translate(playerPos[0], playerPos[1], playerPos[2]) * vmath::rotate(playerYaw, 0.0f, 1.0f, 0.0f) * vmath::scale(playerScale);
		glUniformMatrix4fv(glGetUniformLocation(shader_program[0], "model"), 1, GL_FALSE, model_titan);
		titan.draw(shader_program[0]);
	}

	void onResize(int w, int h)
	{
		sb7::application::onResize(w, h);
	}

	// 키보드 입력 처리
	void onKey(int key, int action)
	{
		bool pressed = (action != GLFW_RELEASE);

		if (key == GLFW_KEY_UP || key == GLFW_KEY_W)
			keyUp = pressed;

		if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)
			keyDown = pressed;

		if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A)
			keyLeft = pressed;

		if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D)
			keyRight = pressed;

		// V 키로 1인칭 / 3인칭 전환
		if (key == GLFW_KEY_V && action == GLFW_PRESS)
		{
			isFirstPerson = !isFirstPerson;

			if (isFirstPerson)
				cameraPitch = 0.0f;
			else
				cameraPitch = -10.0f;
		}

		// F 키로 Attack
		if (key == GLFW_KEY_F)
		{
			if (action == GLFW_PRESS && !attackKeyPressed)
			{
				attackRequested = true;
				attackKeyPressed = true;
			}
			else if (action == GLFW_RELEASE)
			{
				attackKeyPressed = false;
			}
		}

		// Spacebar 키로 Jump
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
			if (!isJumping && !isAttacking)
			{
				isJumping = true;
				verticalVelocity = jumpPower;

				currentAnimState = TitanAnimState::Jump;
			}
		}
	}

	// 마우스로 조절 입력 처리
	void onMouseMove(int x, int y)
	{
		if (firstMouse)
		{
			lastMouseX = x;
			lastMouseY = y;
			firstMouse = false;
		}

		int xoffset = lastMouseX - x;
		int yoffset = lastMouseY - y;

		lastMouseX = x;
		lastMouseY = y;

		cameraYaw += xoffset * mouseSensitivity;
		cameraPitch += yoffset * mouseSensitivity;

		
		if (cameraPitch > 60.0f)
			cameraPitch = 60.0f;

		if (cameraPitch < -30.0f)
			cameraPitch = -30.0f;
	}

	void updateJump(float dt)
	{
		if (isJumping)
		{
			verticalVelocity += gravity * dt;
			playerPos[1] += verticalVelocity * dt;

			if (playerPos[1] <= groundY)
			{
				playerPos[1] = groundY;
				verticalVelocity = 0.0f;
				isJumping = false;
			}
		}
	}

	// 플레이어 이동 함수
	void updatePlayer(float dt)
	{
		playerMoving = false;

		// F키 (공격 시)에는 이동 금지
		if (isAttacking || attackRequested) 
			return;
		

		float yawRad = cameraYaw * 3.141592f / 180.0f;

		vmath::vec3 forward = vmath::vec3(
			sinf(yawRad),
			0.0f,
			cosf(yawRad)
		);

		vmath::vec3 right = vmath::vec3(
			cosf(yawRad),
			0.0f,
			-sinf(yawRad)
		);

		vmath::vec3 moveDir = vmath::vec3(0.0f, 0.0f, 0.0f);

		if (keyUp)
			moveDir += forward;

		if (keyDown)
			moveDir -= forward;

		if (keyRight)
			moveDir -= right;

		if (keyLeft)
			moveDir += right;

		float len = sqrtf(
			moveDir[0] * moveDir[0] +
			moveDir[2] * moveDir[2]
		);

		if (len > 0.0f)
		{
			moveDir[0] /= len;
			moveDir[2] /= len;

			vmath::vec3 oldPos = playerPos;
			vmath::vec3 desiredPos = playerPos + moveDir * playerSpeed * dt;

			bool moved = false;

			// X축 이동 먼저 검사
			vmath::vec3 tryX = vmath::vec3(desiredPos[0], oldPos[1], oldPos[2]);

			if (!CheckStaticCollision(GetTitanCircle(tryX)))
			{
				playerPos[0] = tryX[0];
				moved = true;
			}

			// Z축 이동 따로 검사
			vmath::vec3 tryZ = vmath::vec3(playerPos[0], oldPos[1], desiredPos[2]);

			if (!CheckStaticCollision(GetTitanCircle(tryZ)))
			{
				playerPos[2] = tryZ[2];
				moved = true;
			}

			playerMoving = moved;


			// 이동 중에는 플레이어가 이동 방향을 보게 하기
			if (isFirstPerson)
			{
				playerYaw = cameraYaw;
			}
			else
			{
				playerYaw = atan2f(moveDir[0], moveDir[2]) * 180.0f / 3.141592f;
			}
		}

		// 맵 바깥으로 나가지 못하게
		float mapLimit = 100.0f;

		if (playerPos[0] < -mapLimit) playerPos[0] = -mapLimit;
		if (playerPos[0] > mapLimit) playerPos[0] = mapLimit;
		if (playerPos[2] < -mapLimit) playerPos[2] = -mapLimit;
		if (playerPos[2] > mapLimit) playerPos[2] = mapLimit;
	}

	// 애니메이션 상태 결정 함수 
	void updateAnimationState()
	{
		// 공격 중이면 공격 애니메이션이 끝날 때까지 유지
		if (isAttacking)
		{
			if (titanAnimator->IsAnimationFinished())
			{
				isAttacking = false;
			}
			else
			{
				currentAnimState = TitanAnimState::Attack;
			}
		}

		// 공격 중이 아닐 때만 다른 상태 판단
		if (!isAttacking)
		{
			if (attackRequested)
			{
				currentAnimState = TitanAnimState::Attack;
				attackRequested = false;
				isAttacking = true;
			}
			else if (isJumping)
			{
				currentAnimState = TitanAnimState::Jump;
			}
			else if (playerMoving)
			{
				currentAnimState = TitanAnimState::Walk;
			}
			else
			{
				currentAnimState = TitanAnimState::Idle;
			}
		}

		// 상태가 같으면 같은 애니메이션을 다시 시작하지 않음
		if (currentAnimState == previousAnimState)
			return;

		// 조건에 따른 애니메이션 출력
		switch (currentAnimState)
		{
		case TitanAnimState::Idle:
			titanAnimator->PlayAnimation(titanIdle, true);
			break;

		case TitanAnimState::Walk:
			titanAnimator->PlayAnimation(titanWalk, true);
			break;

		case TitanAnimState::Attack:
			titanAnimator->PlayAnimation(titanAttack, false);
			break;
		
		case TitanAnimState::Jump:
			titanAnimator->PlayAnimation(titanJump, false);
			break;
		}

		previousAnimState = currentAnimState;
	}

	enum class TitanAnimState
	{
		Idle,
		Walk,
		Attack,
		Jump
	};

	struct BuildingInstance
	{
		vmath::vec3 position;
		float scale;
		AABB worldBox;
	};

	// 3d 모델에서 자동으로 AABB 계산하는 함수
	AABB CalculateModelLocalAABB(const Model& model)
	{
		AABB box;

		box.min = vmath::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		box.max = vmath::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (const auto& mesh : model.meshes)
		{
			for (const auto& vertex : mesh.vertices)
			{
				const vmath::vec3& p = vertex.Position;

				if (p[0] < box.min[0]) box.min[0] = p[0];
				if (p[1] < box.min[1]) box.min[1] = p[1];
				if (p[2] < box.min[2]) box.min[2] = p[2];

				if (p[0] > box.max[0]) box.max[0] = p[0];
				if (p[1] > box.max[1]) box.max[1] = p[1];
				if (p[2] > box.max[2]) box.max[2] = p[2];
			}
		}
		return box;
	}

	AABB TransformAABB_TranslateScale(
		const AABB& localBox,
		const vmath::vec3& position,
		float scale
	)
	{
		AABB worldBox;

		worldBox.min = position + localBox.min * scale;
		worldBox.max = position + localBox.max * scale;

		return worldBox;
	}


	Circle GetTitanCircle(const vmath::vec3& pos)
	{
		return MakeCircle(
			pos,
			titanCollisionRadius,
			titanCollisionHeight
		);
	}

	bool CheckStaticCollision(const Circle& circle)
	{
		for (const AABB& box : staticColliders)
		{
			if (CheckCircleAABB(circle, box))
				return true;
		}

		return false;
	}

	void AddBuilding(const vmath::vec3& position, float scale)
	{
		BuildingInstance building;
		building.position = position;
		building.scale = scale;
		building.worldBox = TransformAABB_TranslateScale(
			houseLocalBox,
			position,
			scale
		);

		buildings.push_back(building);
		staticColliders.push_back(building.worldBox);
	}

private:
	GLuint shader_program[3]; // VAO, VBO 관련 멤버변수 필요없음 -> Model.h에 다 있기 때문
	SimpleModel bg;
	Model titan, house, corps;

	// titan status
	vmath::vec3 playerPos = vmath::vec3(0.0f, 0.0f, 20.0f);
	float playerYaw = 0.0f;
	float playerSpeed = 16.0f;
	float playerScale = 1.5f;

	// key input status
	bool keyUp = false;
	bool keyDown = false;
	bool keyLeft = false;
	bool keyRight = false;

	// 플레이어 이동은 프레임마다 진행
	double lastTime = 0.0f;
	float deltaTime = 0.0f;

	// mouse camera
	float cameraYaw = 180.0f;
	float cameraPitch = -10.0f;

	bool firstMouse = true;
	int lastMouseX = 0;
	int lastMouseY = 0;

	float mouseSensitivity = 0.15f;
	float thirdPersonDistance = 6.0f;
	float thirdPersonHeight = 2.5f;

	// 1인칭 전환 변수
	bool isFirstPerson = false;

	Animation* titanIdle = nullptr;
	Animation* titanWalk = nullptr;
	Animation* titanAttack = nullptr;
	Animation* titanJump = nullptr;

	Animator* titanAnimator = nullptr;

	TitanAnimState currentAnimState = TitanAnimState::Idle;
	TitanAnimState previousAnimState = TitanAnimState::Idle;

	bool playerMoving = false;

	bool attackRequested = false;
	bool attackKeyPressed = false;
	bool isAttacking = false;

	bool isJumping = false;
	float verticalVelocity = 0.0f;
	float gravity = -55.0f;
	float jumpPower = 16.0f;
	float groundY = 0.0f;

	AABB houseLocalBox;
	std::vector<BuildingInstance> buildings;

	std::vector<AABB> staticColliders;

	float titanCollisionRadius = 1.2f;
	float titanCollisionHeight = 4.0f;

	struct CorpsSoldier
	{
		vmath::vec3 position;
		vmath::vec3 velocity;

		float yaw = 0.0f;
		float scale = 1.0f;

		float radius = 0.4f;
		float height = 1.8f;

		bool alive = true;

		bool grappling = false;
		vmath::vec3 hookPoint = vmath::vec3(0.0f, 0.0f, 0.0f);

		float maxSpeed = 28.0f;
		float grappleAccel = 60.0f;
		float chaseAccel = 18.0f;
	};

	std::vector<CorpsSoldier> corpsList;

};
// DECLARE_MAIN의 하나뿐인 인스턴스
DECLARE_MAIN(my_application)