#include <iostream>
#include <string>
#include <memory>
#include <atomic>
#include <thread>

#include <Ogre.h>
#include <OgreMesh2.h>
#include <OgreMeshManager2.h>
#include <OgreItem.h>
#include <OgreCamera.h>
#include <OgreHlms.h>
#include <OgreHlmsCommon.h>
#include <OgreHlmsManager.h>
#include <OgreLight.h>
#include <Hlms/Pbs/OgreHlmsPbs.h>
#include <Hlms/Unlit/OgreHlmsUnlit.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>

//Change the number of threads Ogre should use here
constexpr const size_t SMGR_WORKERS{ 4 };

//Since we are loading resources, we can define our own resource group.
constexpr const char* const RG_MYRG{ "MY_RESOURCE_GROUP" };

//Define the shading language for the HighLevelMaterialSystem.
//I only want to use the OpenGL render system, so I'm just going to hardcode it as GLSL
constexpr const char* const SL{ "GLSL" };

using namespace std;

void pause()
{
	cout << "Press RETURN to continue" << endl;
	cin.get();
}

atomic<bool> run = true;
void quitOnPose(void)
{
	cin.get();
	run = false;
}

Ogre::Quaternion animation(float sec)
{
	return Ogre::Quaternion{ Ogre::Radian(Ogre::Math::PI + (Ogre::Math::PI * Ogre::Math::Cos(sec))), Ogre::Vector3::UNIT_Y };
}

int main(void)
{
	cout << "Hello world!\n";
	//auto quitThread = thread(quitOnPose);quitThread.detach();

	//Create root
	auto root = make_unique<Ogre::Root>("plugins.cfg", "ogre.cfg", "Ogre.log");
	Ogre::LogManager::getSingleton().setLogDetail(Ogre::LoggingLevel::LL_BOREME);

	//Configure root
	root->showConfigDialog();

	//Create an automatic window
	auto window = root->initialise(true);

	//Create a scene manager that use X threads
	auto smgr = root->createSceneManager(Ogre::ST_GENERIC, SMGR_WORKERS, Ogre::INSTANCING_CULLING_THREADED);
	smgr->setDisplaySceneNodes(true);

	//Declare some resources
	auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
	resourceGroupManager->addResourceLocation("./media/", "FileSystem", RG_MYRG);

	//Init the HLMS

	//Get the manager
	auto hlmsManager = root->getHlmsManager();

	//Set where to find the HLMS shader library
	Ogre::String hlmsFolder = "./HLMS/";

	//Define the shader library to use for HLMS
	Ogre::ArchiveVec library;
	auto archiveLibrary = Ogre::ArchiveManager::getSingletonPtr()->load(hlmsFolder + "Hlms/Common/" + SL, "FileSystem", true);
	library.push_back(archiveLibrary);

	//Define "unlit" and "PBS" (physics based shader) HLMS
	auto archiveUnlit = Ogre::ArchiveManager::getSingletonPtr()->load(hlmsFolder + "Hlms/Unlit/" + SL, "FileSystem", true);
	auto archivePbs = Ogre::ArchiveManager::getSingletonPtr()->load(hlmsFolder + "Hlms/Pbs/" + SL, "FileSystem", true);
	auto hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit(archiveUnlit, &library);
	hlmsManager->registerHlms(hlmsUnlit);
	auto hlmsPbs = OGRE_NEW Ogre::HlmsPbs(archivePbs, &library);
	hlmsManager->registerHlms(hlmsPbs);

	//All resources initialized
	resourceGroupManager->initialiseAllResourceGroups();

	//Create and attach a camera to the scene manager
	auto camera = smgr->createCamera("MyCamera");

	//Configure the compositor for simple rendering
	auto compositorManager = root->getCompositorManager2();

	//Create a compositor workspace, and register it to the manager
	const Ogre::IdString workspaceName{ "MyWorkspace" };
	if (!compositorManager->hasWorkspaceDefinition(workspaceName))
		compositorManager->createBasicWorkspaceDef(workspaceName, Ogre::ColourValue{ 0.2f, 0.3f, 0.4f });

	//Give it the current scene, the camera from where to render, and use the window as output
	auto workspace = compositorManager->addWorkspace(smgr, window, camera, workspaceName, true);

	//Load an Ogre 1.x mesh
	auto v1mesh = Ogre::v1::MeshManager::getSingleton().load("athene.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

	//Create a Ogre 2.x mesh and convert the old one to the new format
	Ogre::MeshPtr v2mesh = Ogre::MeshManager::getSingleton().createManual("athene.mesh V2", RG_MYRG);
	v2mesh->importV1(v1mesh.get(), true, true, true);

	//Get rid of the old mesh
	v1mesh->unload();
	v1mesh.setNull();

	//Create an Item (not an Entity) from the new mesh
	auto a3dObject = smgr->createItem(v2mesh);
	auto objectNode = smgr->getRootSceneNode()->createChildSceneNode();
	objectNode->attachObject(a3dObject);
	objectNode->setScale(0.1, 0.1, 0.1);

	//Move the camera
	camera->setPosition({ 0, 10, 20 });
	camera->lookAt(objectNode->getPosition());
	camera->setNearClipDistance(0.001);
	camera->setFarClipDistance(2000);
	camera->setAutoAspectRatio(true);

	auto light = smgr->createLight();
	auto lightNode = smgr->getRootSceneNode()->createChildSceneNode();
	lightNode->attachObject(light);
	light->setType(Ogre::Light::LT_DIRECTIONAL);
	light->setDirection(Ogre::Vector3(0, -1, -0.75f).normalisedCopy());
	light->setPowerScale(Ogre::Math::PI);
	/*light->setDiffuseColour(Ogre::ColourValue::Red);
	light->setSpecularColour(Ogre::ColourValue::Red);*/

	root->getTimer()->reset();
	do
	{
		objectNode->setOrientation(animation(static_cast<float>(root->getTimer()->getMilliseconds()) / 1000.0f));
		//cout << "frame\n";
		Ogre::WindowEventUtilities::messagePump();
		if (window->isClosed()) break;
		root->renderOneFrame();
	} while (run);

	return EXIT_SUCCESS;
}