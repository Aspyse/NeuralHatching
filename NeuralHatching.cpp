// NeuralHatching.cpp : Defines the entry point for the application.
//
#include <iostream>
#include "Editor.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	Editor* editor;
	editor = new Editor;

	if (editor->Initialize())
		editor->Run();

	editor->Shutdown();
	delete editor;
	editor = 0;

	return 0;
}
