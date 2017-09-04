#include "GameLayer.h"
#include "SimpleAudioEngine.h"

#define BALL_INIT_POS Point(100, 150)
#define BALL_STRETCH_LENGTH 50

#define LAUNCHER_POS1 Point( 80, 135)
#define LAUNCHER_POS2 Point(125, 140)

#define WINSIZE Director::getInstance()->getWinSize()

USING_NS_CC;
using namespace CocosDenshion;

Scene* GameLayer::createScene(int remaining, int level) {
  auto scene = Scene::createWithPhysics();
  auto layer = GameLayer::create(remaining, level);
  scene->addChild(layer);

  return scene;
  // return GameLayer::create();
}

bool GameLayer::init(int remaining, int level) {
  if(!Scene::init()) {
    return false;
  }

  _remaining = remaining;
  _level = level;

  //    auto visibleSize = Director::getInstance()->getVisibleSize();
  Vec2 origin = Director::getInstance()->getVisibleOrigin();
  auto touchListener = EventListenerTouchOneByOne::create();
  touchListener->onTouchBegan = CC_CALLBACK_2(GameLayer::onTouchBegan, this);
  touchListener->onTouchMoved = CC_CALLBACK_2(GameLayer::onTouchMoved, this);
  touchListener->onTouchEnded = CC_CALLBACK_2(GameLayer::onTouchEnded, this);
  touchListener->onTouchCancelled = CC_CALLBACK_2(GameLayer::onTouchCancelled, this);
  _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

  auto contactListener = EventListenerPhysicsContact::create();
  contactListener->onContactBegin = CC_CALLBACK_1(GameLayer::onContactBegin, this);
  _eventDispatcher->addEventListenerWithSceneGraphPriority(contactListener, this);

  auto audio = SimpleAudioEngine::getInstance();
  audio->preloadEffect("fire.mp3");
  audio->preloadEffect("hit.mp3");

  if (!audio->isBackgroundMusicPlaying()) {
      audio->playBackgroundMusic("bgm.mp3", true);
  }

  return true;
}

void GameLayer::onEnter() {
  Scene::onEnter();

  auto v = Vect(0, -98);
  auto scene = dynamic_cast<Scene*>(this->getParent());
  scene->getPhysicsWorld()->setGravity(v);
  scene->getPhysicsWorld()->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);

  createBackground();
  createGround();
  createBall();
  createBlockAndEnemy(_level);
}

void GameLayer::createBackground() {
  auto background = Sprite::create("background.png");
  background->setAnchorPoint(Point(0, 0.5));
  background->setPosition(Point(0, WINSIZE.height / 2));
  addChild(background, Z_Background, T_Background);
}

void GameLayer::createGround() {
  auto background = getChildByTag(T_Background);

  PhysicsMaterial material;
  material.density = 0.5;
  material.restitution = 0.5;
  material.friction = 0.8;

  auto body = PhysicsBody::createBox(Size(background->getContentSize().width, 50), material);
  body->setDynamic(false);

  auto node = Node::create();
  node->setAnchorPoint(Point::ANCHOR_MIDDLE);
  node->setPhysicsBody(body);
  node->setPosition(Point(background->getContentSize().width / 2, 25));
  addChild(node);
}

void GameLayer::createEnemy(cocos2d::Point position) {
  auto enemy = Sprite::create("enemy1.png");
  enemy->setPosition(position);
  enemy->setTag(T_Enemy);

  auto animation = Animation::create();
  animation->addSpriteFrameWithFile("enemy2.png");
  animation->addSpriteFrameWithFile("enemy1.png");
  animation->setDelayPerUnit(1);

  auto repeat = RepeatForever::create(Animate::create(animation));
  enemy->runAction(repeat);

  PhysicsMaterial material;
  material.density = 0.5;
  material.restitution = 0.5;
  material.friction = 0.3;

  auto body = PhysicsBody::createCircle(enemy->getContentSize().width * 0.47, material);
  body->setDynamic(true);
  body->setContactTestBitmask(0x01);
  enemy->setPhysicsBody(body);

  addChild(enemy, Z_Enemy);
}

bool GameLayer::onTouchBegan(Touch* touch, Event* unused_event) {
  bool ret = false;

  auto ball = getChildByTag(T_Ball);
  if(ball && ball->getBoundingBox().containsPoint(touch->getLocation())) {
    ball->setPosition(calculatedPosition(touch->getLocation()));
    ret = true;
  }

  return ret;
}

void GameLayer::onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* unused_event) {
  auto ball = getChildByTag(T_Ball);
  if(ball) {
    ball->setPosition(calculatedPosition(touch->getLocation()));

    auto angle = ((BALL_INIT_POS - ball->getPosition()).getAngle());
    auto pos = ball->getPosition() + Point(-25, 0).rotate(Point::forAngle(angle));

    auto gum1 = getChildByTag(T_Gum1);
    if(!gum1) {
      gum1 = Sprite::create("brown.png");
      addChild(gum1, Z_Gum1, T_Gum1);
    }

    gum1->setPosition(LAUNCHER_POS1 - (LAUNCHER_POS1 - pos) / 2);
    gum1->setRotation(CC_RADIANS_TO_DEGREES((LAUNCHER_POS1 - pos).getAngle() * -1));
    gum1->setScaleX(LAUNCHER_POS1.getDistance(pos));
    gum1->setScaleY(10);

    auto gum2 = getChildByTag(T_Gum2);
    if(!gum2) {
      gum2 = Sprite::create("brown.png");
      addChild(gum2, Z_Gum2, T_Gum2);
    }

    gum2->setPosition(LAUNCHER_POS2 - (LAUNCHER_POS2 - pos) / 2);
    gum2->setRotation(CC_RADIANS_TO_DEGREES((LAUNCHER_POS2 - pos).getAngle() * -1));
    gum2->setScaleX(LAUNCHER_POS2.getDistance(pos));
    gum2->setScaleY(10);
  }
}

void GameLayer::onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* unused_event) {
  auto ball = getChildByTag(T_Ball);
  if(ball) {
    SimpleAudioEngine::getInstance()->playEffect("fire.mp3");

    removeChildByTag(T_Gum1);
    removeChildByTag(T_Gum2);

    ball->setPosition(calculatedPosition(touch->getLocation()));

    applyingForceToBall(ball);

    scheduleUpdate();

    scheduleOnce(schedule_selector(GameLayer::failureGame), 10);
  }
}

void GameLayer::onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* unused_event) {
  onTouchEnded(touch, unused_event);
}

void GameLayer::createBall() {
  auto ball = Sprite::create("ball.png");
  ball->setPosition(BALL_INIT_POS);
  ball->setTag(T_Ball);
  ball->setZOrder(Z_Ball);

  PhysicsMaterial material;
  material.density = 0.5;
  material.restitution = 0.5;
  material.friction = 0.3;

  auto body = PhysicsBody::createCircle(ball->getContentSize().width * 0.47, material);
  body->setDynamic(false);
  body->setContactTestBitmask(0x01);
  ball->setPhysicsBody(body);

  addChild(ball);

  auto launcher1 = Sprite::create("launcher1.png");
  launcher1->setScale(0.5);
  launcher1->setPosition(Point(100, 100));
  addChild(launcher1, Z_Launcher1);

  auto launcher2 = Sprite::create("launcher2.png");
  launcher2->setScale(0.5);
  launcher2->setPosition(launcher1->getPosition());
  addChild(launcher2, Z_Launcher2);
}

Point GameLayer::calculatedPosition(Point touchPosition) {
  int distance = touchPosition.getDistance(BALL_INIT_POS);

  if (distance > BALL_STRETCH_LENGTH) {
    return BALL_INIT_POS + (touchPosition - BALL_INIT_POS) * BALL_STRETCH_LENGTH / distance;
  } else {
    return touchPosition;
  }
}

void GameLayer::applyingForceToBall(Node* ball) {
  ball->getPhysicsBody()->setDynamic(true);

  Vect force = (ball->getPosition() - BALL_INIT_POS) * -10000;
  ball->getPhysicsBody()->resetForces();
  ball->getPhysicsBody()->applyImpulse(force);
}

void GameLayer::createBlockAndEnemy(int level) {
  switch (level) {
      case 1: {
          createEnemy(Point(636, 125));

          createBlock(BlockType::Stone, Point(536, 75), 0);
          createBlock(BlockType::Stone, Point(636, 75), 0);
          createBlock(BlockType::Stone, Point(736, 75), 0);

          createBlock(BlockType::Block1, Point(586, 150), 90);
          createBlock(BlockType::Block1, Point(586, 250), 90);
          createBlock(BlockType::Block1, Point(686, 150), 90);
          createBlock(BlockType::Block1, Point(686, 250), 90);
          createBlock(BlockType::Roof, Point(636, 325), 0);

          break;
      }
      case 2: {
          createEnemy(Point(536, 125));
          createEnemy(Point(636, 125));
          createEnemy(Point(736, 125));

          createBlock(BlockType::Stone, Point(436, 75), 0);
          createBlock(BlockType::Stone, Point(536, 75), 0);
          createBlock(BlockType::Stone, Point(636, 75), 0);
          createBlock(BlockType::Stone, Point(736, 75), 0);
          createBlock(BlockType::Stone, Point(836, 75), 0);

          createBlock(BlockType::Block1, Point(486, 150), 90);
          createBlock(BlockType::Block1, Point(586, 150), 90);
          createBlock(BlockType::Block1, Point(686, 150), 90);
          createBlock(BlockType::Block1, Point(786, 150), 90);
          createBlock(BlockType::Roof, Point(536, 225), 0);
          createBlock(BlockType::Roof, Point(636, 225), 0);
          createBlock(BlockType::Roof, Point(736, 225), 0);

          break;
      }

      default: {
          createEnemy(Point(536, 125));
          createEnemy(Point(636, 125));
          createEnemy(Point(636, 245));
          createEnemy(Point(736, 125));

          createBlock(BlockType::Stone, Point(436, 75), 0);
          createBlock(BlockType::Stone, Point(536, 75), 0);
          createBlock(BlockType::Stone, Point(636, 75), 0);
          createBlock(BlockType::Stone, Point(736, 75), 0);
          createBlock(BlockType::Stone, Point(836, 75), 0);

          createBlock(BlockType::Block1, Point(486, 150), 90);
          createBlock(BlockType::Block1, Point(586, 150), 90);
          createBlock(BlockType::Block1, Point(686, 150), 90);
          createBlock(BlockType::Block1, Point(786, 150), 90);

          createBlock(BlockType::Block1, Point(536, 210), 0);
          createBlock(BlockType::Block1, Point(636, 210), 0);
          createBlock(BlockType::Block1, Point(736, 210), 0);

          createBlock(BlockType::Block1, Point(586, 270), 90);
          createBlock(BlockType::Block1, Point(686, 270), 90);

          createBlock(BlockType::Roof, Point(636, 345), 0);

          break;
      }
  }
}

void GameLayer::createBlock(BlockType type, Point position, float angle) {
  std::string fileName;

  switch (type) {
    case BlockType::Block1:
    fileName = "block1.png";
    break;

    case BlockType::Block2:
    fileName = "block2.png";
    break;

    case BlockType::Roof:
    fileName = "roof.png";
    break;

    default:
    fileName = "stone.png";
    break;
  }

  auto block = Sprite::create(fileName.c_str());
  block->setPosition(position);
  block->setRotation(angle);
  block->setTag(T_Block);

  PhysicsBody* body;

  switch(type) {
    case BlockType::Block1:
    case BlockType::Block2: {
      body = PhysicsBody::createBox(block->getContentSize(), PhysicsMaterial(0.5, 0.5, 0.3));
      body->setDynamic(true);
      body->setContactTestBitmask(0x01);
      break;
    }

    case BlockType::Roof: {
      Point points[3] = {Point(-50, -25), Point(0, 25), Point(50, -25)};
      body = PhysicsBody::createPolygon(points, 3, PhysicsMaterial(0.5, 0.5, 0.3));
      body->setDynamic(true);
      body->setContactTestBitmask(0x01);
      break;
    }

    default: {
      body = PhysicsBody::createBox(block->getContentSize(), PhysicsMaterial(0.5, 0.5, 0.3));
      body->setDynamic(false);
      break;
    }
  }

  block->setPhysicsBody(body);

  addChild(block, Z_Block);
}

bool GameLayer::onContactBegin(PhysicsContact& contact) {
    auto bodyA = contact.getShapeA()->getBody();
    auto bodyB = contact.getShapeB()->getBody();

    if(bodyA->getNode() && bodyB->getNode()) {
        PhysicsBody* ball = nullptr;

        if(bodyA->getNode()->getTag() == T_Ball) {
            ball = bodyA;
        } else if(bodyB->getNode()->getTag() == T_Ball) {
            ball = bodyB;
        }

        if(ball && ball->getVelocity().getLength() > 200) {
            contactedBall();
        }

        Node* enemy = nullptr;
        PhysicsBody* object = nullptr;

        if(bodyA->getNode()->getTag() == T_Enemy) {
            enemy = bodyA->getNode();
            object = bodyB;
        } else if(bodyB->getNode()->getTag() == T_Enemy) {
            enemy = bodyB->getNode();
            object = bodyA;
        }

        if(enemy && object->getVelocity().getLength() > 10) {
            //敵の消滅
            removingEnemy(enemy);
        }
    }

    return true;
}

void GameLayer::contactedBall() {
    auto star = Sprite::create("star.png");
    star->setPosition(getChildByTag(T_Ball)->getPosition());
    star->setScale(0);
    star->setOpacity(127);
    addChild(star, Z_Star);

    auto scale = ScaleTo::create(0.1, 1);
    auto fadeout = FadeOut::create(0.1);
    auto remove = RemoveSelf::create();
    auto sequence = Sequence::create(scale, fadeout, remove, nullptr);
    star->runAction(sequence);
}

void GameLayer::removingEnemy(Node* enemy) {
    SimpleAudioEngine::getInstance()->playEffect("hit.mp3");

    auto removingEnemy = Sprite::create("enemy1.png");
    removingEnemy->setPosition(enemy->getPosition());
    addChild(removingEnemy, Z_Enemy);

    auto sequenceForEnemy = Sequence::create(ScaleTo::create(0.3, 0), RemoveSelf::create(), nullptr);
    removingEnemy->runAction(sequenceForEnemy);

    auto fog = Sprite::create("fog1.png");
    fog->setPosition(removingEnemy->getPosition());
    addChild(fog, Z_Fog);

    auto animation = Animation::create();
    animation->addSpriteFrameWithFile("fog1.png");
    animation->addSpriteFrameWithFile("fog2.png");
    animation->addSpriteFrameWithFile("fog3.png");
    animation->setDelayPerUnit(0.15);

    auto spawn = Spawn::create(Animate::create(animation), FadeOut::create(0.45), nullptr);
    auto sequenceForFog = Sequence::create(spawn, RemoveSelf::create(), nullptr);

    fog->runAction(sequenceForFog);

    enemy->removeFromParent();
}

GameLayer* GameLayer::create(int remaining, int level) {
    GameLayer* pRet = new GameLayer();
    pRet->init(remaining, level);
    pRet->autorelease();

    return pRet;
}

void GameLayer::update(float dt) {
    auto enemy = getChildByTag(T_Enemy);
    if(!enemy) {
        unschedule(schedule_selector(GameLayer::failureGame));
        unscheduleUpdate();

        successGame();
    }
}

void GameLayer::successGame() {
    auto success = Sprite::create("success.png");
    success->setPosition(Point(WINSIZE / 2));
    success->setOpacity(0);
    addChild(success, Z_Result);

    auto fadein = FadeIn::create(0.5);
    auto delay = DelayTime::create(1.0);
    auto fadeout = FadeOut::create(0.5);
    auto func = CallFunc::create([this]() {
        int nextLevel = (_level >= 3)? 1 :_level + 1;
        auto scene = GameLayer::createScene(_remaining, nextLevel);
        Director::getInstance()->replaceScene(scene);
    });

    auto sequence = Sequence::create(fadein, delay, fadeout, func, nullptr);
    success->runAction(sequence);
}

void GameLayer::failureGame(float dt) {
    unscheduleUpdate();

    if(--_remaining <= 0) {
        auto failure = Sprite::create("failure.png");
        failure->setPosition(Point(WINSIZE / 2));
        addChild(failure, Z_Result);
    } else {
        removeChildByTag(T_Ball);
        createBall();
    }
}
