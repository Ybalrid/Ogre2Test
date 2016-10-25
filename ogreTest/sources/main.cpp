#include <iostream>
#include <string>
#include <memory>

#include<Ogre.h>

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

	auto camera = smgr->createCamera("MyCamera");
	auto cameraNode = smgr->getRootSceneNode()->createChildSceneNode();
	cameraNode->attachObject(camera);

	pause();

	return EXIT_SUCCESS;
}