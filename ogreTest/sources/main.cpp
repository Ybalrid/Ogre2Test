#include <iostream>
#include <string>
#include <memory>

#include <Ogre.h>
#include <OgreMesh2.h>
#include <OgreMeshManager2.h>
#include <OgreItem.h>
#include <OgreCamera.h>
#include <OgreHlms.h>
#include <OgreHlmsCommon.h>
#include <OgreHlmsManager.h>
#include <OgreSceneNode.h>
#include <OgreLight.h>
#include <Hlms/Pbs/OgreHlmsPbs.h>
#include <Hlms/Unlit/OgreHlmsUnlit.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>

//Mandatory for starting the program as a Win32 application
#ifdef _WIN32
#include <Windows.h>
#endif

//Change the number of threads Ogre should use here
constexpr const size_t SMGR_WORKERS{ 4 };

//Since we are loading resources, we can define our own resource group.
constexpr const char* const RG_MYRG{ "MY_RESOURCE_GROUP" };

//Define the shading language for the HighLevelMaterialSystem.
//I only want to use the OpenGL render system, so I'm just going to hardcode it as GLSL
//Theses text variable should be set as so and don't change
constexpr const char* const ShaderLanguage{ "GLSL" };
constexpr const char* const GL3PLUS_RENDERSYSTEM{ "OpenGL 3+ Rendering Subsystem" };

//RenderWindow configuration
Ogre::uint8 WindowAALevel{ 16 };
Ogre::Vector2 WindowPosition{ 0,0 }, WindowSize(1024, 768);
bool WindowVsync{ false }, WindowFullscreen{ false };

std::string to_string(bool b)
{
	if (b) return "true";
	return "false";
}

using namespace std;

void pauseConsole()
{
	cout << "Press RETURN to continue" << endl;
	cin.get();
}

Ogre::Quaternion animation(float sec)
{
	return Ogre::Quaternion{ Ogre::Radian(Ogre::Math::PI + (Ogre::Math::PI * Ogre::Math::Cos(sec))), Ogre::Vector3::UNIT_Y };
}

decltype(auto) loadV1mesh(Ogre::String meshName)
{
	return Ogre::v1::MeshManager::getSingleton().load(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
}

decltype(auto) asV2mesh(Ogre::String meshName, Ogre::String ResourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::String sufix = " V2",
	bool halfPos = true, bool halfTextCoords = true, bool qTangents = true)
{
	//Get the V1 mesh
	auto v1mesh = loadV1mesh(meshName);

	//Convert it as a V2 mesh
	auto mesh = Ogre::MeshManager::getSingletonPtr()->createManual(meshName + sufix, ResourceGroup);
	mesh->importV1(v1mesh.get(), halfPos, halfTextCoords, qTangents);

	//Unload the useless V1 mesh
	v1mesh->unload();
	v1mesh.setNull();

	//Return the shared pointer to the new mesh
	return mesh;
}

void declareHlmsLibrary(Ogre::String&& path)
{
#ifdef _DEBUG
	if (string(ShaderLanguage) != "GLSL" || string(Ogre::Root::getSingleton().getRenderSystem()->getName()) != "OpenGL 3+ Rendering Subsystem")
		throw std::runtime_error("This function is OpenGL only. Please use the RenderSytem_GL3+ in the Ogre configuration!");
#endif
	//Ogre::String path = path;
	auto log = [](const std::string& message) {Ogre::LogManager::getSingleton().logMessage(message); };

	//The hlmsFolder can come from a configuration file where it could be "empty" or set to "." or lacking the trailing "/"
	if (path.empty()) path = "./";
	else if (path[path.size() - 1] != '/') path += "/";

	log("The data folder path for the hlms library is : " + path);
	log("The shader language used for the hlms library is : " + string{ ShaderLanguage });

	//Get the hlmsManager (not a singleton by itself, but accessible via Root)
	auto hlmsManager = Ogre::Root::getSingleton().getHlmsManager();

	//Define the shader library to use for HLMS
	auto commonLibrary = Ogre::ArchiveVec();

	auto archiveCommonLibrary = Ogre::ArchiveManager::getSingletonPtr()->load(path + "Hlms/Common/" + ShaderLanguage, "FileSystem", true);
	auto archiveCommonLibraryAny = Ogre::ArchiveManager::getSingletonPtr()->load(path + "Hlms/Common/Any", "FileSystem", true);
	commonLibrary.push_back(archiveCommonLibrary);
	commonLibrary.push_back(archiveCommonLibraryAny);

	//Define "unlit" and "PBS" (physics based shader) HLMS
	auto archiveUnlitLibraryAny = Ogre::ArchiveManager::getSingletonPtr()->load(path + "Hlms/Unlit/Any", "FileSystem", true);
	commonLibrary.push_back(archiveUnlitLibraryAny); //Add the unlit
	auto archiveUnlit = Ogre::ArchiveManager::getSingletonPtr()->load(path + "Hlms/Unlit/" + ShaderLanguage, "FileSystem", true);
	auto hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit(archiveUnlit, &commonLibrary);
	hlmsManager->registerHlms(hlmsUnlit);

	//Remove the last one loaded...
	commonLibrary.pop_back(); //pop the unlit

	auto archivePbsLibraryAny = Ogre::ArchiveManager::getSingletonPtr()->load(path + "Hlms/Pbs/Any", "FileSystem", true);
	commonLibrary.push_back(archivePbsLibraryAny); //push the unlit
	auto archivePbs = Ogre::ArchiveManager::getSingletonPtr()->load(path + "Hlms/Pbs/" + ShaderLanguage, "FileSystem", true);
	auto hlmsPbs = OGRE_NEW Ogre::HlmsPbs(archivePbs, &commonLibrary);
	hlmsManager->registerHlms(hlmsPbs);
}

#ifdef _WIN32
int main(void);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main();
}
#endif

int main(void)
{
	auto run{ true };
	cout << "Hello world!\n";
	//auto quitThread = thread(quitOnPose);quitThread.detach();

	//Create root
	auto root = make_unique<Ogre::Root>("plugins.cfg", "ogre.cfg", "Ogre.log");
	Ogre::LogManager::getSingleton().setLogDetail(Ogre::LoggingLevel::LL_BOREME);

	//Get the OpenGL 3+ RenderSystem
	auto renderSystem = root->getRenderSystemByName(GL3PLUS_RENDERSYSTEM);

#ifdef _DEBUG
	//renderSystem should not be nullptr now
	if (!renderSystem) throw std::runtime_error("Ogre is unable to find the GL3+ RenderSystem");
#endif

	//Configure Ogre Root to render
	root->setRenderSystem(renderSystem);
	renderSystem->setConfigOption("FSAA", to_string(WindowAALevel));
	renderSystem->setConfigOption("sRGB Gamma Conversion", "Yes");

	//Init Ogre without automatically creating the render window
	root->initialise(false);

	//Put the miscellaneous configuration option the window can take in a value pair list
	auto misc = Ogre::NameValuePairList();
	misc["vsync"] = to_string(WindowVsync);
	misc["FSAA"] = to_string(WindowAALevel);
	misc["top"] = to_string(WindowPosition.x);
	misc["left"] = to_string(WindowPosition.y);

	//Manually create a window
	auto window = root->createRenderWindow("Ogre Window", WindowSize.x, WindowSize.y, WindowFullscreen, &misc);

	//Create a scene manager that use X threads
	auto smgr = root->createSceneManager(Ogre::ST_GENERIC, SMGR_WORKERS, Ogre::INSTANCING_CULLING_THREADED);
	//smgr->setDisplaySceneNodes(true);
	//smgr->setForward3D(true, 4, 4, 5, 96, 0.1f, 200);

	//Declare some resources
	auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
	resourceGroupManager->addResourceLocation("./media/", "FileSystem", RG_MYRG);
	resourceGroupManager->addResourceLocation("./media/Sinbad", "FileSystem", RG_MYRG);

	//Init the HLMS
	declareHlmsLibrary("HLMS");

	//All resources initialized
	resourceGroupManager->initialiseAllResourceGroups(true);

	//Create and attach a camera to the scene manager
	auto camera = smgr->createCamera("MyCamera");
	camera->setNearClipDistance(0.01f);
	camera->setFarClipDistance(2000);
	camera->setAutoAspectRatio(true);

	//Create a camera, detach it from the root node and attach it to a new node
/*	if (Ogre::SceneNode* cameraNode = camera->getParentSceneNode()) cameraNode->detachObject(camera);
	Ogre::SceneNode* cameraNode = smgr->getRootSceneNode()->createChildSceneNode();
	cameraNode->attachObject(camera);
	*/

	//Configure the compositor for simple rendering
	auto compositorManager = root->getCompositorManager2();

	//Create a compositor workspace, and register it to the manager
	const Ogre::String workspaceNameStr{ "MyWorkspace" };
	const Ogre::IdString workspaceName{ workspaceNameStr };
	if (!compositorManager->hasWorkspaceDefinition(workspaceName))
		compositorManager->createBasicWorkspaceDef(workspaceNameStr, Ogre::ColourValue{ 0.2f, 0.3f, 0.4f });

	//Give it the current scene, the camera from where to render, and use the window as output
	auto workspace = compositorManager->addWorkspace(smgr, window, camera, workspaceName, true);

	//Create an Item (not an Entity) from the new mesh
	auto a3dObject = smgr->createItem(asV2mesh("athene.mesh", RG_MYRG));
	auto objectNode = smgr->getRootSceneNode()->createChildSceneNode();
	objectNode->attachObject(a3dObject);
	objectNode->setScale(0.1f, 0.1f, 0.1f);
	objectNode->setPosition(5, 0, 0);

	//Create a Sinbad Item from the good old Sinbad mesh (with modified texture to use hlmsPbs)
	auto Sinbad = smgr->createItem(asV2mesh("Sinbad.mesh", RG_MYRG));
	auto SinbadNode = smgr->getRootSceneNode()->createChildSceneNode();
	SinbadNode->attachObject(Sinbad);
	SinbadNode->setScale(1.3f, 1.3f, 1.3f);
	SinbadNode->setPosition(-5, 0, 0);

	Ogre::SkeletonAnimation* danceAnim{ nullptr };
#ifdef _DEBUG
	if (Sinbad->hasSkeleton())
	{
		cerr << "skeleton!\n";
		cerr << "Sinbad animations : " << Sinbad->getSkeletonInstance()->getAnimations().size() << '\n';
#endif
		danceAnim = Sinbad->getSkeletonInstance()->getAnimation("Dance");
#ifdef _DEBUG
	}

	if (danceAnim)
	{
#endif
		danceAnim->setEnabled(true);
		danceAnim->setLoop(true);
#ifdef _DEBUG
	}
#endif
	//Move the camera
	/*cameraNode->setPosition({ 0, 5, 20 });
	cameraNode->lookAt({ 0, 0, 0 }, Ogre::Node::TransformSpace::TS_PARENT);
*/

	camera->setPosition(0, 5, 20);
	camera->lookAt(0, 0, 0);

	//define colors for the light
	Ogre::ColourValue warm{ 1, 0.75f, 0.75f }, cold{ 0.75f, 0.75f, 1 };

	//Add a light
	auto light = smgr->createLight();
	auto lightNode = smgr->getRootSceneNode()->createChildSceneNode();
	lightNode->attachObject(light);
	light->setType(Ogre::Light::LT_DIRECTIONAL);
	light->setDirection(Ogre::Vector3(-1, -1, -0.75f).normalisedCopy());
	light->setPowerScale(Ogre::Math::PI);
	light->setDiffuseColour(warm);
	light->setSpecularColour(warm);

	//Add a light
	light = smgr->createLight();
	lightNode = smgr->getRootSceneNode()->createChildSceneNode();
	lightNode->attachObject(light);
	light->setType(Ogre::Light::LT_DIRECTIONAL);
	light->setDirection(Ogre::Vector3(1, 1, -0.75f).normalisedCopy());
	light->setPowerScale(Ogre::Math::PI);
	light->setDiffuseColour(cold);
	light->setSpecularColour(cold);

	//Add a light
	auto pointlight = smgr->createLight();
	auto pointlightNode = smgr->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
	pointlightNode->attachObject(pointlight);
	pointlight->setType(Ogre::Light::LT_POINT);
	pointlightNode->setPosition(0, 3, 2);
	pointlight->setPowerScale(10 * Ogre::Math::PI);
	pointlight->setDiffuseColour(warm);
	pointlight->setSpecularColour(cold);
	pointlight->setCastShadows(true);

	Ogre::Quaternion orientation;

	Ogre::Real then, now{ 0 };
	do
	{
		then = now;
		orientation = animation(now = (static_cast<float>(root->getTimer()->getMilliseconds()) / 1000.0f));
		objectNode->setOrientation(orientation);
		SinbadNode->setOrientation(orientation);

		if (danceAnim) danceAnim->addTime(now - then);

		Ogre::WindowEventUtilities::messagePump();
		if (window->isClosed()) break;
		root->renderOneFrame();
	} while (run);

	return EXIT_SUCCESS;
}