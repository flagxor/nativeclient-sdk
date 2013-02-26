// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "physics_layer.h"
#include "physics_nodes/CCPhysicsSprite.h"

// Pixels-to-meters ratio for converting screen coordinates
// to Box2D "meters".
#define PTM_RATIO 32
#define SCREEN_TO_WORLD(n) ((n) / PTM_RATIO)
#define WORLD_TO_SCREEN(n) ((n) * PTM_RATIO)
#define VELOCITY_ITERATIONS 8
#define POS_ITERATIONS 1

#define SPRITE_BATCH_NODE_TAG 99
#define MAX_SPRITES 100

#define DEFAULT_DENSITY 1.0f
#define DEFAULT_FRICTION 0.2f
#define DEFAULT_RESTITUTION 0.1f

USING_NS_CC_EXT;

bool PhysicsLayer::init() {
  if (!CCLayerColor::initWithColor(ccc4(0,0x8F,0xD8,0xD8)))
    return false;

  setTouchEnabled(true);

  InitPhysics();

  // create brush texture
  brush_ = CCSprite::create("brush.png");
  brush_->retain();
  CCSize brush_size = brush_->getContentSize();
  brush_radius_ = MAX(brush_size.height/2, brush_size.width/2);

  // script physics updates each frame
  schedule(schedule_selector(PhysicsLayer::UpdateWorld));
  return true;
}

PhysicsLayer::PhysicsLayer() :
   current_touch_id_(-1),
   render_target_(NULL),
   box2d_density_(DEFAULT_DENSITY),
   box2d_restitution_(DEFAULT_RESTITUTION),
   box2d_friction_(DEFAULT_FRICTION),
#ifdef COCOS2D_DEBUG
   debug_enabled_(false)
#endif
   {
}

PhysicsLayer::~PhysicsLayer() {
  brush_->release();
  delete box2d_world_;
#ifdef COCOS2D_DEBUG
  delete box2d_debug_draw_;
#endif
}

void PhysicsLayer::registerWithTouchDispatcher()
{
  CCDirector* director = CCDirector::sharedDirector();
  director->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
}

void PhysicsLayer::CreateRenderTarget()
{
  // create render target for shape drawing
  assert(!render_target_);
  CCSize win_size = CCDirector::sharedDirector()->getWinSize();
  render_target_ = CCRenderTexture::create(win_size.width,
                                           win_size.height,
                                           kCCTexture2DPixelFormat_RGBA8888);
  render_target_->setPosition(ccp(win_size.width / 2, win_size.height / 2));
  addChild(render_target_);
}

bool PhysicsLayer::InitPhysics() {
  b2Vec2 gravity(0.0f, -9.8f);
  box2d_world_ = new b2World(gravity);
  box2d_world_->SetAllowSleeping(true);
  box2d_world_->SetContinuousPhysics(true);

  // create the ground
  b2BodyDef groundBodyDef;
  groundBodyDef.position.Set(0, 0);
  b2Body* groundBody = box2d_world_->CreateBody(&groundBodyDef);

  CCSize win_size = CCDirector::sharedDirector()->getWinSize();
  int world_width = SCREEN_TO_WORLD(win_size.width);
  int world_height = SCREEN_TO_WORLD(win_size.height);

  // Define the ground box shape.
  b2EdgeShape groundBox;
  // bottom
  groundBox.Set(b2Vec2(0, 0), b2Vec2(world_width, 0));
  groundBody->CreateFixture(&groundBox, 0);
  // top
  groundBox.Set(b2Vec2(0, world_height), b2Vec2(world_width, world_height));
  groundBody->CreateFixture(&groundBox, 0);
  // left
  groundBox.Set(b2Vec2(0, world_height), b2Vec2(0,0));
  groundBody->CreateFixture(&groundBox, 0);
  // right
  groundBox.Set(b2Vec2(world_width, world_height), b2Vec2(world_width, 0));
  groundBody->CreateFixture(&groundBox, 0);

#ifdef COCOS2D_DEBUG
  box2d_debug_draw_ = new GLESDebugDraw(PTM_RATIO);
  box2d_world_->SetDebugDraw(box2d_debug_draw_);

  uint32 flags = 0;
  flags += b2Draw::e_shapeBit;
  flags += b2Draw::e_jointBit;
  flags += b2Draw::e_centerOfMassBit;
  //flags += b2Draw::e_aabbBit;
  //flags += b2Draw::e_pairBit;
  box2d_debug_draw_->SetFlags(flags);
#endif
  return true;
}

void PhysicsLayer::ToggleDebug() {
  debug_enabled_ = !debug_enabled_;

  // Set visibility of all children based on debug_enabled_
  CCArray* children = getChildren();
  if (!children)
    return;
  for (uint i = 0; i < children->count(); i++)
  {
    CCNode* child = static_cast<CCNode*>(children->objectAtIndex(i));
    if (child == render_target_)
      continue;
    child->setVisible(!debug_enabled_);
  }
}

void PhysicsLayer::UpdateWorld(float dt) {
   box2d_world_->Step(dt, VELOCITY_ITERATIONS, POS_ITERATIONS);
}

void PhysicsLayer::DrawPoint(CCPoint& location)
{
  render_target_->begin();
  brush_->setPosition(ccp(location.x, location.y));
  brush_->visit();
  render_target_->end();
  points_being_drawn_.push_back(location);
}

void PhysicsLayer::draw()
{
  CCLayerColor::draw();

#ifdef COCOS2D_DEBUG
  if (debug_enabled_)
  {
    ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position);
    kmGLPushMatrix();
    box2d_world_->DrawDebugData();
    kmGLPopMatrix();
  }
#endif
}

void PhysicsLayer::DrawLine(CCPoint& start, CCPoint& end)
{
  // calculate distance moved
  float distance = ccpDistance(start, end);

  // draw the brush sprite into render texture at every point between the old
  // and new cursor positions
  render_target_->begin();
  for (int i = 0; i < int(distance + 0.5); i++)
  {
    float difx = end.x - start.x;
    float dify = end.y - start.y;
    float delta = (float)i / distance;
    brush_->setPosition(
        ccp(start.x + (difx * delta), start.y + (dify * delta)));

    brush_->visit();
  }
  render_target_->end();
  points_being_drawn_.push_back(end);
}

bool PhysicsLayer::ccTouchBegan(CCTouch* touch, CCEvent* event) {
  if (current_touch_id_ != -1)
    return false;

  current_touch_id_ = touch->getID();

  if (!render_target_)
    CreateRenderTarget();

  points_being_drawn_.clear();
  CCPoint location = touch->getLocation();
  DrawPoint(location);
  return true;
}

void PhysicsLayer::ccTouchMoved(CCTouch* touch, CCEvent* event) {
  assert(touch->getID() == current_touch_id_);
  CCPoint end = touch->getLocation();
  CCPoint start = touch->getPreviousLocation();
  DrawLine(start, end);
}

void PhysicsLayer::ccTouchEnded(CCTouch* touch, CCEvent* event) {
  assert(touch->getID() == current_touch_id_);
  b2Body* body = CreatePhysicsBody();
  CCSprite* sprite = CreatePhysicsSprite(body);
  addChild(sprite);
  if (debug_enabled_)
    sprite->setVisible(false);

  // release render target (it will get recreated on next touch).
  removeChild(render_target_, true);
  render_target_ = NULL;
  current_touch_id_ = -1;
}

CCRect CalcBodyBounds(b2Body* body) {
  CCSize s = CCDirector::sharedDirector()->getWinSize();

  float minX = FLT_MAX;
  float maxX = 0;
  float minY = FLT_MAX;
  float maxY = 0;

  const b2Transform& xform = body->GetTransform();
  for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()) {
    b2Shape* shape = f->GetShape();
    if (shape->GetType() == b2Shape::e_circle)
    {
      b2CircleShape* c = static_cast<b2CircleShape*>(shape);
      b2Vec2 center = b2Mul(xform, c->m_p);
      if (center.x - c->m_radius < minX)
        minX = center.x - c->m_radius;
      if (center.x + c->m_radius > maxX)
        maxX = center.x + c->m_radius;
      if (center.y - c->m_radius < minY)
        minY = center.y - c->m_radius;
      if (center.y + c->m_radius > maxY)
        maxY = center.y + c->m_radius;
    }
    else
    {
      b2PolygonShape* poly = static_cast<b2PolygonShape*>(shape);
      int32 vertexCount = poly->m_vertexCount;

      for (int i = 0; i < vertexCount; ++i) {
        b2Vec2 vertex = b2Mul(xform, poly->m_vertices[i]);
        if (vertex.x < minX)
          minX = vertex.x;
        if (vertex.x > maxX)
          maxX = vertex.x;
        if (vertex.y < minY)
          minY = vertex.y;
        if (vertex.y > maxY)
          maxY = vertex.y;
      }
    }
  }

  maxX = WORLD_TO_SCREEN(maxX);
  minX = WORLD_TO_SCREEN(minX);
  maxY = WORLD_TO_SCREEN(maxY);
  minY = WORLD_TO_SCREEN(minY);

  float width = maxX - minX;
  float height = maxY - minY;
  float remY = s.height - maxY;
  return CCRectMake(minX, remY, width, height);
}

CCSprite* PhysicsLayer::CreatePhysicsSprite(b2Body* body)
{
  CCPhysicsSprite *sprite;

  // create a new texture based on the current contents of the
  // render target
  CCImage* image = render_target_->newCCImage();
  CCTexture2D* tex = new CCTexture2D();
  tex->initWithImage(image);
  tex->autorelease();
  delete image;

  // Find the bounds of the physics body wihh the target texture
  CCRect sprite_rect = CalcBodyBounds(body);
  sprite_rect.origin.x -= brush_radius_;
  sprite_rect.origin.y -= brush_radius_;
  sprite_rect.size.width += brush_radius_;
  sprite_rect.size.height += brush_radius_;

  CCSize s = CCDirector::sharedDirector()->getWinSize();
  CCPoint body_pos = ccp(WORLD_TO_SCREEN(body->GetPosition().x),
                         WORLD_TO_SCREEN(body->GetPosition().y));


  // Create a new sprite based on the texture
  sprite = CCPhysicsSprite::createWithTexture(tex, sprite_rect);
  sprite->setB2Body(body);
  sprite->setPTMRatio(PTM_RATIO);

  // Set the anchor point of the sprite
  float anchorX = body_pos.x - sprite_rect.origin.x;
  float anchorY = body_pos.y + sprite_rect.origin.y + sprite_rect.size.height;
  anchorY -= s.height;

  // anchor point goes from 0.0 to 1.0 with in bounds of the sprite itself.
  sprite->setAnchorPoint(ccp(anchorX / sprite_rect.size.width,
                             anchorY / sprite_rect.size.height));
  return sprite;
}

b2Body* PhysicsLayer::CreatePhysicsBody()
{
  assert(points_being_drawn_.size());
  CCPoint start_point = points_being_drawn_.front();

  assert(points_being_drawn_.size());
  CCLog("new body from %d points", points_being_drawn_.size());
  // create initial body
  b2BodyDef def;
  def.type = b2_dynamicBody;
  def.position.Set(SCREEN_TO_WORLD(start_point.x),
                   SCREEN_TO_WORLD(start_point.y));
  b2Body* body = box2d_world_->CreateBody(&def);

  const int min_box_length = brush_radius_;

  // Create an initial box the size of the brush
  AddSphereToBody(body, &start_point);
  AddSphereToBody(body, &points_being_drawn_.back());

  // Add boxes to body for every point that was drawn by the
  // user.
  // initialise endpoint to be the second item in the list
  // and iterate until it points to the final element.
  PointList::iterator iter = points_being_drawn_.begin();
  ++iter;
  for (; iter != points_being_drawn_.end(); iter++)
  {
    CCPoint end_point = *iter;
    float distance = ccpDistance(start_point, end_point);
    // if the distance between points it too small then
    // skip the current point
    if (distance < min_box_length)
    {
      if (iter != points_being_drawn_.end() - 1)
        continue;
    }
    AddLineToBody(body, start_point, end_point);
    start_point = *iter;
  }

  points_being_drawn_.clear();
  return body;
}

void PhysicsLayer::AddShapeToBody(b2Body *body, b2Shape* shape)
{
  b2FixtureDef shape_def;
  shape_def.shape = shape;
  shape_def.density = box2d_density_;
  shape_def.friction = box2d_friction_;
  shape_def.restitution = box2d_restitution_;
  body->CreateFixture(&shape_def);
}

void PhysicsLayer::AddSphereToBody(b2Body *body, CCPoint* location)
{
  b2CircleShape shape;
  shape.m_radius = SCREEN_TO_WORLD(brush_radius_);
  shape.m_p.x = SCREEN_TO_WORLD(location->x) - body->GetPosition().x;
  shape.m_p.y = SCREEN_TO_WORLD(location->y) - body->GetPosition().y;
  AddShapeToBody(body, &shape);
}

void PhysicsLayer::AddLineToBody(b2Body *body, CCPoint start, CCPoint end)
{
  float distance = ccpDistance(start, end);

  float sx = start.x;
  float sy = start.y;
  float ex = end.x;
  float ey = end.y;
  float dist_x = sx - ex;
  float dist_y = sy - ey;
  float angle = atan2(dist_y, dist_x);

  float posx = SCREEN_TO_WORLD((sx+ex)/2) - body->GetPosition().x;
  float posy = SCREEN_TO_WORLD((sy+ey)/2) - body->GetPosition().y;

  float width = SCREEN_TO_WORLD(abs(distance));
  float height = SCREEN_TO_WORLD(brush_->boundingBox().size.height);

  b2PolygonShape shape;
  shape.SetAsBox(width / 2, height / 2, b2Vec2(posx, posy), angle);
  AddShapeToBody(body, &shape);
}