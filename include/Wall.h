#ifndef Wall_H
#define Wall_H

#include <Ogre.h>
#include <OIS/OIS.h>

#include "GameState.h"

#include <CEGUI.h>
#include <RendererModules/Ogre/Renderer.h>

#include <OgreOverlaySystem.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayManager.h>

#include <OgreBulletDynamicsRigidBody.h>
#include <Shapes/OgreBulletCollisionsStaticPlaneShape.h>
#include <Shapes/OgreBulletCollisionsBoxShape.h>

#include "GameEntity.h"

using namespace Ogre;
using namespace OgreBulletCollisions;
using namespace OgreBulletDynamics;

enum WallType {
  LeftWall,
  RightWall,
  BackWall,
  FrontWall,
  Floor,
};

class Wall : public GameEntity{
 public:
  Wall() {}
  Wall(WallType type);
  Wall(Ogre::SceneNode* sNode, OgreBulletDynamics::RigidBody* rigBody, WallType type);
  ~Wall();

  WallType getType();
  void setType(WallType type);

  Vector3 getSpawnPosition();
  void setSpawnPosition(Vector3 New_Position);

 protected:
  WallType _type;
  Vector3 _spawnPosition;
};

#endif
