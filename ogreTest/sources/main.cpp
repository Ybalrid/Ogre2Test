#include <iostream>
#include <string>
#include <memory>

#include<Ogre.h>
#include <Compositor\OgreCompositorManager2.h>
#include <Compositor\OgreCompositorWorkspace.h>
//Change the number of threads Ogre should use here
constexpr const size_t SMGR_WORKERS{ 4 };

using namespace std;

void pause()
{
	cout << "Press RETURN to continue" << endl;
	cin.get();
}

int main(void)
{
	cout << "Hello world!\n";

	//Create root
	auto root = make_unique<Ogre::Root>("plugins.cfg", "ogre.cfg", "Ogre.log");

	//Configure root
	root->showConfigDialog();

	//Create an automatic window
	auto window = root->initialise(true);

	//Create a scene manager that use X threads
	auto smgr = root->createSceneManager(Ogre::ST_GENERIC, SMGR_WORKERS, Ogre::INSTANCING_CULLING_THREADED);

	auto renderSystem = root->getRenderSystem();

	//Create and attach a camera to the scene manager
	auto camera = smgr->createCamera("MyCamera");
	//auto cameraNode = smgr->getRootSceneNode()->createChildSceneNode();
	//cameraNode->attachObject(camera);

	camera->setPosition({ 0, 0, 10 });

	auto compositorManager = root->getCompositorManager2();

	const Ogre::IdString workspaceName{ "MyWorkspace" };

	if (!compositorManager->hasWorkspaceDefinition(workspaceName))
		compositorManager->createBasicWorkspaceDef(workspaceName, Ogre::ColourValue::Green);

	auto workspace = compositorManager->addWorkspace(smgr, window, camera, workspaceName, true);

	while (!window->isClosed())
	{
		Ogre::WindowEventUtilities::messagePump();
		root->renderOneFrame();
	}

	return EXIT_SUCCESS;
}