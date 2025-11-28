#include <iostream>
#include <glad/glad.h>
#include <stdlib.h>
#include "Application.h"
#include "Log.h"

#include <filesystem>
namespace fs = std::filesystem;

enum MainState
{
	CREATE = 1,
	AWAKE,
	START,
	LOOP,
	CLEAN,
	FAIL,
	EXIT
};

std::unique_ptr<Application> App = nullptr;

int main(int argc, char* argv[]) {

	fs::path cwd = fs::current_path();

	if (cwd.filename() == "build" || cwd.filename() == "Debug" || cwd.filename() == "Release")
	{
		fs::current_path(cwd.parent_path());
		LOG("Working Directory corrected to: %s", fs::current_path().string().c_str());
	}
	else if (!fs::exists("Assets"))
	{
		if (fs::exists(cwd.parent_path() / "Assets"))
		{
			fs::current_path(cwd.parent_path());
			LOG("Working Directory corrected to: %s", fs::current_path().string().c_str());
		}
	}

	LOG("Application starting ...");

	//Initializes the engine state
	Application::EngineState state = Application::EngineState::CREATE;
	int result = EXIT_FAILURE;

	while (state != Application::EngineState::EXIT)
	{
		switch (state)
		{
			// Allocate the Application --------------------------------------------
		case Application::EngineState::CREATE:
			LOG("CREATION PHASE ===============================");
			state = Application::EngineState::AWAKE;

			break;

			// Awake all modules -----------------------------------------------
		case Application::EngineState::AWAKE:
			LOG("AWAKE PHASE ===============================");
			if (Application::GetInstance().Awake() == true)
				state = Application::EngineState::START;
			else
			{
				LOG("ERROR: Awake failed");
				state = Application::EngineState::FAIL;
			}

			break;

			// Call all modules before first frame  ----------------------------
		case Application::EngineState::START:
			LOG("START PHASE ===============================");
			if (Application::GetInstance().Start() == true)
			{
				state = Application::EngineState::LOOP;
				LOG("UPDATE PHASE ===============================");
			}
			else
			{
				state = Application::EngineState::FAIL;
				LOG("ERROR: Start failed");
			}
			break;

			// Loop all modules until we are asked to leave ---------------------
		case Application::EngineState::LOOP:
			if (Application::GetInstance().Update() == false)
				state = Application::EngineState::CLEAN;
			break;

			// Cleanup allocated memory -----------------------------------------
		case Application::EngineState::CLEAN:
			LOG("CLEANUP PHASE ===============================");
			if (Application::GetInstance().CleanUp() == true)
			{
				result = EXIT_SUCCESS;
				state = Application::EngineState::EXIT;
			}
			else
				state = Application::EngineState::FAIL;

			break;

			// Exit with errors and shame ---------------------------------------
		case Application::EngineState::FAIL:
			LOG("Exiting with errors");
			result = EXIT_FAILURE;
			state = Application::EngineState::EXIT;
			break;
		}
	}

	LOG("Closing Application ===============================");

	return result;
}