#include "PlayState.h"
#include "PauseState.h"
#include "GameOverState.h"

#include "Shapes/OgreBulletCollisionsConvexHullShape.h"
#include "Shapes/OgreBulletCollisionsTrimeshShape.h"    
#include "Utils/OgreBulletCollisionsMeshToShapeConverter.h"
#include "Shapes/OgreBulletCollisionsSphereShape.h"
#include "OgreBulletCollisionsRay.h"

using namespace std;

template<> PlayState* Ogre::Singleton<PlayState>::msSingleton = 0;

OgreBulletDynamics::RigidBody *b = NULL;

void
PlayState::enter ()
{
  _root = Ogre::Root::getSingletonPtr();

  //GameManager::getSingletonPtr()->_mainTrack = GameManager::getSingletonPtr()->_pTrackManager->load("BGGame.ogg");
  //GameManager::getSingletonPtr()->_mainTrack->play();

  // Se recupera el gestor de escena y la cámara.----------------
  _sceneMgr = _root->getSceneManager("SceneManager");
  _camera = _sceneMgr->createCamera("PlayCamera");
  _viewport = _root->getAutoCreatedWindow()->addViewport(_camera);
  _sceneMgr->setAmbientLight(Ogre::ColourValue(0.4, 0.4, 0.4));
  
   
  //Camara--------------------
  _camera->setPosition(Ogre::Vector3(10,10,10));
  _camera->lookAt(Ogre::Vector3(0,0,0));
  _camera->setNearClipDistance(5);
  _camera->setFarClipDistance(10000);
  //-----------------------------
  
  _numEntities = 0;    // Numero de Shapes instanciadas
  _timeLastObject = 0; // Tiempo desde que se añadio el ultimo objeto

  // Creacion del modulo de debug visual de Bullet ------------------
  _debugDrawer = new OgreBulletCollisions::DebugDrawer();
  _debugDrawer->setDrawWireframe(true);  
  SceneNode *node = _sceneMgr->getRootSceneNode()->createChildSceneNode("debugNode", Vector3::ZERO);
  node->attachObject(static_cast <SimpleRenderable *>(_debugDrawer));
  //-----------------------------------------------------------------

  // Creacion del mundo (definicion de los limites y la gravedad)----
  AxisAlignedBox worldBounds = AxisAlignedBox (Vector3 (-10000, -10000, -10000), Vector3 (10000,  10000,  10000));
  Vector3 gravity = Vector3(0, -9.8, 0);

  _world = new OgreBulletDynamics::DynamicsWorld(_sceneMgr,worldBounds, gravity);
  _world->setDebugDrawer (_debugDrawer);
  _world->setShowDebugShapes(true);  // Muestra los collision shapes
  //-----------------------------------------------------------------

  //Creacion de los elementos iniciales del mundo---
  CreateInitialWorld();
  //------------------------------------------------
  
  //Creo la interfaz---
  createGUI();
  //-------------------

  
  _exitGame = false;
}

void
PlayState::exit ()
{
  //Salgo del estado------------------------------
  _sceneMgr->clearScene();
  _sceneMgr->destroyCamera("PlayCamera");
  _root->getAutoCreatedWindow()->removeAllViewports();
  //--------------------------------------------

  //Limpio la interfaz---------------------------
  CEGUI::Window* sheet=CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
  
  //---------------------------------------------

  //GameManager::getSingletonPtr()->_mainTrack->unload();
}

void
PlayState::pause()
{
}

void
PlayState::resume()
{
  
}

void 
PlayState::CreateInitialWorld() {
  
  //Suelo Infinito NO TOCAR---------------------------------
  Plane plane1(Vector3(0,1,0), 0);    // Normal y distancia
  MeshManager::getSingleton().createPlane("p1",
  ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane1,
  800, 450, 1, 1, true, 1, 20, 20, Vector3::UNIT_Z);
  //--------------------------------------------------------

  //Suelo Grafico-----------------------------------------------
  SceneNode* _groundNode = _sceneMgr->createSceneNode("ground");
  Entity* _groundEnt = _sceneMgr->createEntity("planeEnt", "p1");
  _groundEnt->setMaterialName("Ground");
  _groundNode->attachObject(_groundEnt);
  _sceneMgr->getRootSceneNode()->addChild(_groundNode);
  //------------------------------------------------------------
 	
  // Creamos forma de colision para el plano ----------------------- 
  OgreBulletCollisions::CollisionShape *Shape;
  Shape = new OgreBulletCollisions::StaticPlaneCollisionShape
    (Ogre::Vector3(0,1,0), 0);   // Vector normal y distancia
  OgreBulletDynamics::RigidBody *rigidBodyPlane = new 
    OgreBulletDynamics::RigidBody("rigidBodyPlane", _world);

  // Creamos la forma estatica (forma, Restitucion, Friccion) ------
  rigidBodyPlane->setStaticShape(Shape, 0.1, 0.8); 

  // Anadimos los objetos Shape y RigidBody ------------------------
  _shapes.push_back(Shape);  
  _bodies.push_back(rigidBodyPlane);
  
  //SkyBox-------------------------------------
  _sceneMgr->setSkyBox(true, "MaterialSkybox");
  //-------------------------------------------
  
  //LUCES------------------------------------------------
  Light* _sunLight = _sceneMgr->createLight("SunLight");
  _sunLight->setPosition(200, 200, 200);
  _sunLight->setType(Light::LT_SPOTLIGHT);
  _sunLight->setDiffuseColour(.20, .20, 0);
  _sunLight->setSpotlightRange(Degree(30), Degree(50));

  Vector3 _dir= -_sunLight->getPosition().normalisedCopy();
  _sunLight->setDirection(_dir);


  Light* _directionalLight = _sceneMgr->createLight("DirectionalLight");
  _directionalLight->setType(Ogre::Light::LT_DIRECTIONAL);
  //directionalLight->setDiffuseColour(.60, .60, 0);
  _directionalLight->setDirection(Ogre::Vector3(0, -1, 1));

  //-----------------------------------------------------

  //CUBO PRUEBA PJ----------------------------------------
  Entity *entity = _sceneMgr->createEntity("EntCube", "cube.mesh");
  SceneNode *node = _sceneMgr->getRootSceneNode()->createChildSceneNode("SNCube");
  node->attachObject(entity);

  Vector3 size = Vector3::ZERO; 
  Vector3 position = Vector3(5,5,5);
 
  OgreBulletCollisions::StaticMeshToShapeConverter *trimeshConverter = NULL; 
  OgreBulletCollisions::CollisionShape *bodyShape = NULL;
  OgreBulletDynamics::RigidBody *rigidBody = NULL;

  AxisAlignedBox boundingB = entity->getBoundingBox();
  size = boundingB.getSize(); 
  size /= 2.0f;   // El tamano en Bullet se indica desde el centro
  bodyShape = new OgreBulletCollisions::BoxCollisionShape(size);
  

  rigidBody = new OgreBulletDynamics::RigidBody("rigidBodyCUBE", _world);

  rigidBody->setShape(node, bodyShape,
         0.0 /* Restitucion */, 0.6 /* Friccion */,
         5.0 /* Masa */, position /* Posicion inicial */,
         Quaternion::IDENTITY /* Orientacion */);

  rigidBody->setAngularVelocity(Vector3(0,0,0));

  //rigidBody->setLinearVelocity(
  //   _camera->getDerivedDirection().normalisedCopy() * 7.0); 

  b=rigidBody;

  // Anadimos los objetos a las deques
  _shapes.push_back(bodyShape);   
  _bodies.push_back(rigidBody);

}

/*void 
PlayState::AddStaticObject(Vector3 pos,Quaternion rot) {
  
  Entity* entAim = _sceneMgr->createEntity("diana.mesh");
  SceneNode* nodeAim = _sceneMgr->createSceneNode("Target"+StringConverter::toString(_numEntities)+"SN");
  nodeAim->attachObject(entAim);
  _sceneMgr->getRootSceneNode()->addChild(nodeAim);
  
  _targets.push_back(nodeAim);

  OgreBulletCollisions::StaticMeshToShapeConverter *trimeshConverter2 = new 
  OgreBulletCollisions::StaticMeshToShapeConverter(entAim);
 
  OgreBulletCollisions::TriangleMeshCollisionShape *Trimesh2 = trimeshConverter2->createTrimesh();

  OgreBulletDynamics::RigidBody *rigidObject2 = new 
    OgreBulletDynamics::RigidBody("RigidBodydiana"+StringConverter::toString(_numEntities), _world);
  rigidObject2->setShape(nodeAim, Trimesh2, 0.5, 0.5, 0,pos,//4º Posicion inicial 
       rot); //El 5º parametro es la gravedad

  delete trimeshConverter2;

  _numEntities++;

  // Anadimos los objetos a las deques---
  _shapes.push_back(Trimesh2);   
  _bodies.push_back(rigidObject2); 
  //-------------------------------------
  

}

void 
PlayState::AddDynamicObject() {
  _timeLastObject = 0.25;   // Segundos para anadir uno nuevo... 

  Vector3 position = (_camera->getDerivedPosition() + _camera->getDerivedDirection().normalisedCopy() * 10);
 
  Entity *entity = _sceneMgr->createEntity("Arrow" + StringConverter::toString(_numEntities), "arrow.mesh");
  SceneNode *node = _sceneMgr->getRootSceneNode()->createChildSceneNode("Arrow" + StringConverter::toString(_numEntities)+"SN");
  node->attachObject(entity);

  //Obtengo la rotacion de la camara para lanzar la flecha correctamente--------------------
  Vector3 aux=_camera->getDerivedDirection().normalisedCopy(); //Este es el vector direccion
  Vector3 src = node->getOrientation() * Ogre::Vector3::UNIT_X;//Punto donde aparece la flecha
  Quaternion quat = src.getRotationTo(aux);//Rotacion
  //------------------------------------------------------------------------------------------
  
  //Colision por Box----------------------------------------------
  OgreBulletCollisions::StaticMeshToShapeConverter *trimeshConverter = NULL;
  OgreBulletCollisions::CollisionShape *boxShape = NULL;
  trimeshConverter = new OgreBulletCollisions::StaticMeshToShapeConverter(entity);
  
  AxisAlignedBox boundingB = entity->getBoundingBox();
  Vector3 size = Vector3(2.0, 0.15, 0.15);//Hacer mas pequeña
  boxShape = new OgreBulletCollisions::BoxCollisionShape(size);

  OgreBulletDynamics::RigidBody *rigidBox = new OgreBulletDynamics::RigidBody("rigidBodyArrow" + StringConverter::toString(_numEntities), _world);
  
  rigidBox->setShape(node, boxShape,0.8 , 0.5 ,50.0 , position ,quat );

  //Compruebo que a fuerza no sea mas que la fuerza maxima permitida---
  if(_force>_maxForce){
    _force=_maxForce;
  }
  //-------------------------------------------------------------------

  rigidBox->setLinearVelocity(_camera->getDerivedDirection().normalisedCopy() * _force); 

  _numEntities++;

  // Anadimos los objetos a las deques---
  _shapes.push_back(boxShape);   
  _bodies.push_back(rigidBox); 
  //-------------------------------------
   

}*/

bool
PlayState::frameStarted
(const Ogre::FrameEvent& evt)
{ 
  _deltaT = evt.timeSinceLastFrame;

  _world->stepSimulation(_deltaT); // Actualizar simulacion Bullet
  _timeLastObject -= _deltaT;

  //Deteccion Colisones---
  DetectCollisionAim();
  //----------------------

  //Actualizo GUI---
  updateGUI();
  //----------------

  
  

  return true;
}

void PlayState::DetectCollisionAim() {
  //Colisiones------------------------------
  btCollisionWorld *bulletWorld = _world->getBulletCollisionWorld();
  int numManifolds = bulletWorld->getDispatcher()->getNumManifolds();

  for (int i=0;i<numManifolds;i++) {
    btPersistentManifold* contactManifold = 
      bulletWorld->getDispatcher()->getManifoldByIndexInternal(i);
    btCollisionObject* obA = 
      (btCollisionObject*)(contactManifold->getBody0());
    btCollisionObject* obB = 
      (btCollisionObject*)(contactManifold->getBody1());
    
    //Compruebo con todas dianas-----------------------------
    /*for(unsigned int i=0;i<_targets.size();i++){
      Ogre::SceneNode* drain = _targets[i];

      OgreBulletCollisions::Object *obDrain = _world->findObject(drain);
      OgreBulletCollisions::Object *obOB_A = _world->findObject(obA);
      OgreBulletCollisions::Object *obOB_B = _world->findObject(obB);

      if ((obOB_A == obDrain) || (obOB_B == obDrain)) {
        Ogre::SceneNode* node = NULL;
        if ((obOB_A != obDrain) && (obOB_A)) {
          node = obOB_A->getRootNode(); delete obOB_A;
        }
        else if ((obOB_B != obDrain) && (obOB_B)) {
          node = obOB_B->getRootNode(); delete obOB_B;
        }
        if (node) {
          cout << "Nodo que colisiona: " << node->getName() << "\n" << endl;
          
          //Creo una flecha en la misma posicion y rotacion---
          Entity *ent = _sceneMgr->createEntity("Arrow" + StringConverter::toString(_numEntities), "arrow.mesh");
          SceneNode *nod = _sceneMgr->getRootSceneNode()->createChildSceneNode("Arrow" + StringConverter::toString(_numEntities)+"SN");
          nod->attachObject(ent);
          nod->setPosition(node->getPosition());
          nod->setOrientation(node->getOrientation());
          _numEntities++;
          //--------------------------------------------------
          
          //Aumento la puntuacion-----------------------------
          _punt+=100;
          //-------------------------------------------------
          
          //Muestro lbl de impacto---------------------------
          _impact=true;
          //-------------------------------------------------

          //Cargo y reproduzco el sonido---
          GameManager::getSingletonPtr()->_simpleEffect = GameManager::getSingletonPtr()->_pSoundFXManager->load("ArrowImpact.wav");
          GameManager::getSingletonPtr()->_simpleEffect->play();
          //-------------------------------
          _sceneMgr->getRootSceneNode()->removeAndDestroyChild (node->getName());
        }
      }
    }
    //------------------------------------------------

     */
  }
  //----------------------------------------  
}
bool
PlayState::frameEnded
(const Ogre::FrameEvent& evt)
{

  _deltaT = evt.timeSinceLastFrame;
  _world->stepSimulation(_deltaT); // Actualizar simulacion Bullet

  //Salir del juego--
  if (_exitGame)
    return false;
  //-----------------
  return true;
}

void
PlayState::keyPressed
(const OIS::KeyEvent &e)
{

  // Tecla p --> PauseState.-------
  if (e.key == OIS::KC_P) {
    pushState(PauseState::getSingletonPtr());
  }
  //-----------------

  // Tecla p --> GameOverState.-------
  if (e.key == OIS::KC_G) {
    changeState(GameOverState::getSingletonPtr());
  }
  //-----------------

  //Movimiento CUBO---------------
  if (e.key == OIS::KC_SPACE) {
    b->enableActiveState ();
    b->setLinearVelocity(Vector3(0,10,0));
  }
  if (e.key == OIS::KC_UP) {
    b->setLinearVelocity(Vector3(5,0,0));
  }
  if (e.key == OIS::KC_DOWN) {
    b->setLinearVelocity(Vector3(-5,0,0));
  }
  if (e.key == OIS::KC_LEFT) {
    b->setLinearVelocity(Vector3(0,0,-5));
  }
  if (e.key == OIS::KC_RIGHT) {
    b->setLinearVelocity(Vector3(0,0,5));
  }
  //-------------------------------- 


  //CEGUI--------------------------
  CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(static_cast<CEGUI::Key::Scan>(e.key));
  CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(e.text);
  //-------------------------------
}

void
PlayState::keyReleased
(const OIS::KeyEvent &e)
{
  
  //Recargo flechas---------------
  if (e.key == OIS::KC_R) {
    
  }
  //------------------------------

  //Salgo del juego---------------
  if (e.key == OIS::KC_ESCAPE) {
    _exitGame = true;
  }
  //-------------------------------
  
  //CEGUI--------------------------
  CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp(static_cast<CEGUI::Key::Scan>(e.key));
  //-------------------------------
}

void
PlayState::mouseMoved
(const OIS::MouseEvent &e)
{

  //Movimiento camara-----------------------------------------------------------------------------
  float rotx = e.state.X.rel * _deltaT * -1;
  float roty = e.state.Y.rel * _deltaT * -1;
  _camera->yaw(Radian(rotx));
  _camera->pitch(Radian(roty));
  //----------------------------------------------------------------------------------------------

  //CEGUI--------------------------
  CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseMove(e.state.X.rel, e.state.Y.rel);
  //-------------------------------
}

void
PlayState::mousePressed
(const OIS::MouseEvent &e, OIS::MouseButtonID id)
{
  //CEGUI--------------------------
  CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(convertMouseButton(id));
  //-------------------------------
  
}

void
PlayState::mouseReleased
(const OIS::MouseEvent &e, OIS::MouseButtonID id)
{
  //CEGUI--------------------------
  CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(convertMouseButton(id));
  //-------------------------------

  
}

PlayState*
PlayState::getSingletonPtr ()
{
return msSingleton;
}

PlayState&
PlayState::getSingleton ()
{ 
  assert(msSingleton);
  return *msSingleton;
}

CEGUI::MouseButton PlayState::convertMouseButton(OIS::MouseButtonID id)//METODOS DE CEGUI
{
  //CEGUI--------------------------
  CEGUI::MouseButton ceguiId;
  switch(id)
    {
    case OIS::MB_Left:
      ceguiId = CEGUI::LeftButton;
      break;
    case OIS::MB_Right:
      ceguiId = CEGUI::RightButton;
      break;
    case OIS::MB_Middle:
      ceguiId = CEGUI::MiddleButton;
      break;
    default:
      ceguiId = CEGUI::LeftButton;
    }
  return ceguiId;
  //------------------------------
}


void
PlayState::createGUI()
{
  // Interfaz Intro--------------------
  CEGUI::Window* sheet=CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();

  
}

void
PlayState::updateGUI()
{

  CEGUI::Window* sheet=CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
  
}