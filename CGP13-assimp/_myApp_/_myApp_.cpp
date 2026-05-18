// sb7.h 헤더 파일을 포함시킨다.
#include <sb7.h>
#include <vmath.h>
#include <shader.h>
#include <vector>

#include "SimpleModel.h"
#include "Model.h"
#include "Animation.h"
#include "Animator.h"
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
		titan.loadModel("model/walkingtitan.gltf");

		titanWalk = new Animation("model/walkingtitan.gltf", &titan);
		titanAnimator = new Animator(titanWalk);

		// player 초기 위치
		playerPos = vmath::vec3(0.0f, 0.0f, 20.0f);
		playerYaw = 180.0f;
		playerScale = 1.0f;

		// 마우스 커서 안 보이게 하기
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	// 애플리케이션 끝날 때 호출된다.
	virtual void shutdown()
	{
		glDeleteProgram(shader_program[0]);
		delete titanAnimator;
		delete titanWalk;
		titanAnimator = nullptr;
		titanWalk = nullptr;
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

		if (playerMoving && titanAnimator)
		{
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
		// playerYaw = cameraYaw;


		// 3인칭 카메라 위치
		vmath::vec3 eye;
		vmath::vec3 center;
		vmath::vec3 up(0.0f, 1.0f, 0.0f);

		if (isFirstPerson)
		{
			// 1인칭: 플레이어 눈 위치
			float eyeHeight = 1.65f;

			eye = playerPos + vmath::vec3(0.0f, eyeHeight, 0.0f);
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
		bg.draw(shader_program[0]); // VAO, Texture 바인드 안 해도 됨 -> render()에서 draw() 하나만 작성하면 됨

		if (titanAnimator)
		{
			glUniform1i(glGetUniformLocation(shader_program[0], "useAnimation"), playerMoving ? 1 : 0);

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

		titan.draw(shader_program[0]);

		// 1인칭, 3인칭 변환 시 렌더 모델 스위치
		if (!isFirstPerson)
		{
			vmath::mat4 model_titan = vmath::translate(playerPos[0], playerPos[1], playerPos[2]) * vmath::rotate(playerYaw, 0.0f, 1.0f, 0.0f) * vmath::scale(playerScale);
			glUniformMatrix4fv(glGetUniformLocation(shader_program[0], "model"), 1, GL_FALSE, model_titan);
			titan.draw(shader_program[0]);
		}
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

		// 1인칭의 경우 위아래를 더 많이 볼 수 있도록
		if (isFirstPerson)
		{
			if (cameraPitch > 60.0f)
				cameraPitch = 60.0f;

			if (cameraPitch < -60.0f)
				cameraPitch = -60.0f;
		}
		else
		{
			if (cameraPitch > 60.0f)
				cameraPitch = 60.0f;

			if (cameraPitch < -30.0f)
				cameraPitch = -30.0f;
		}
	}

	// 플레이어 이동 함수
	void updatePlayer(float dt)
	{
		playerMoving = false;
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

			playerPos += moveDir * playerSpeed * dt;
			playerMoving = true;

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


private:
	GLuint shader_program[3]; // VAO, VBO 관련 멤버변수 필요없음 -> Model.h에 다 있기 때문
	SimpleModel bg;
	Model titan;

	// titan status
	vmath::vec3 playerPos = vmath::vec3(0.0f, 0.0f, 0.0f);
	float playerYaw = 0.0f;
	float playerSpeed = 16.0f;
	float playerScale = 1.0f;

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

	Animation* titanWalk = nullptr;
	Animator* titanAnimator = nullptr;
	bool playerMoving = false;
};
// DECLARE_MAIN의 하나뿐인 인스턴스
DECLARE_MAIN(my_application)