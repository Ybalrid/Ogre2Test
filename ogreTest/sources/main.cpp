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

int main(void)
{
	cout << "Hello world!\n";
	auto quitThread = thread(quitOnPose);
	quitThread.detach();

	//Create root
	auto root = make_unique<Ogre::Root>("plugins.cfg", "ogre.cfg", "Ogre.log");
	//Ogre::LogManager::getSingleton().setLogDetail(Ogre::LoggingLevel::LL_BOREME);

	//Configure root
	root->showConfigDialog();

	//Create an automatic window
	auto window = root->initialise(true);

	//Create a scene manager that use X threads
	auto smgr = root->createSceneManager(Ogre::ST_GENERIC, SMGR_WORKERS, Ogre::INSTANCING_CULLING_THREADED);

	auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
	resourceGroupManager->addResourceLocation("./media/", "FileSystem", RG_MYRG);

	auto hlmsManager = root->getHlmsManager();
	if (!hlmsManager)
	{
		cerr << "hlms manager is null?\n";
	}

	Ogre::String hlmsFolder = "./HLMS/";

	//Define the shader library to use for HLMS
	Ogre::ArchiveVec library;
	auto archiveLibrary = Ogre::ArchiveManager::getSingletonPtr()->load(hlmsFolder + "Hlms/Common/" + SL, "FileSystem", true);
	library.push_back(archiveLibrary);

	//Define "unlit" and "PBS" (physics based shader) HLMS
	auto archiveUnlit = Ogre::ArchiveManager::getSingletonPtr()->load(hlmsFolder + "Hlms/Unlit/" + SL, "FileSystem", true);
	auto archivePbs = Ogre::ArchiveManager::getSingletonPtr()->load(hlmsFolder + "Hlms/Pbs/" + SL, "FileSystem", true);
	auto hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit(archiveUnlit, &library);
	auto hlmsPbs = OGRE_NEW Ogre::HlmsPbs(archivePbs, &library);
	hlmsManager->registerHlms(hlmsUnlit);
	hlmsManager->registerHlms(hlmsPbs);

	resourceGroupManager->initialiseAllResourceGroups();

	//Create and attach a camera to the scene manager
	auto camera = smgr->createCamera("MyCamera");
	/*auto cameraNode = smgr->getRootSceneNode()->createChildSceneNode();
	cameraNode->attachObject(camera);*/

	auto compositorManager = root->getCompositorManager2();

	const Ogre::IdString workspaceName{ "MyWorkspace" };

	if (!compositorManager->hasWorkspaceDefinition(workspaceName))
		compositorManager->createBasicWorkspaceDef(workspaceName, Ogre::ColourValue::Green);

	auto workspace = compositorManager->addWorkspace(smgr, window, camera, workspaceName, true);

	auto v1mesh = Ogre::v1::MeshManager::getSingleton().load("athene.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

	Ogre::MeshPtr v2mesh = Ogre::MeshManager::getSingleton().createManual("athene.mesh V2", RG_MYRG);
	v2mesh->importV1(v1mesh.get(), true, true, true);
	v1mesh->unload();
	v1mesh.setNull();

	auto a3dObject = smgr->createItem(v2mesh);
	auto objectNode = smgr->getRootSceneNode()->createChildSceneNode();
	objectNode->attachObject(a3dObject);
	objectNode->setScale(0.1, 0.1, 0.1);

	camera->setPosition({ 0, 0, 10 });

	while (run)
	{
		Ogre::WindowEventUtilities::messagePump();
		root->renderOneFrame();
	}

	return EXIT_SUCCESS;
}