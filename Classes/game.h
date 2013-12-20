//
//  game.h
//  cutrope
//
//  Created by Kowalski on 12/17/13.
//
//

#ifndef __cutrope__game__
#define __cutrope__game__
//unlock test below to see debug draw
//#define _debug
#include <iostream>
#include "cocos2d.h"
#include "defination.h"
#include "Box2D.h"
#include "GLES-Render.h"
#include "cocos-ext.h"
#define ptm_rto 40
USING_NS_CC_EXT;
USING_NS_CC;

class win:public CCLayer{
public:
    CREATE_FUNC(win);
    static CCScene* scene(CCString *st){
        CCScene *sc=CCScene::create();
        win *w=win::create();
        w->setString(st);
        sc->addChild(w);
        return sc;
    }
    virtual bool init(){
        if(!CCLayer::init()){
            return false;
        }
        return true;
    }
    void setString(CCString *str){
        CCLabelTTF *l=CCLabelTTF::create(str->getCString(), "Marker Felt", 33);
        l->setPosition(ccp(1164.0f*.5f, 640.0f*.5f));
        this->addChild(l);
    }
};

class B2BODY :public CCObject{
public:
    b2Body* m_b2body;
};

class game :public CCLayer,b2ContactListener{
public:
    
    typedef enum{
        type_rope=1,
        type_suger=2,
        type_touch=3,
        type_moster=4
    } type_type;
    
    b2World *world;
    GLESDebugDraw *debug_draw;
    b2Body *suger;
    b2Body *touchPoint;
    CCSpriteBatchNode *chains;
    CCArray *bodys_need_destory;
    
    CREATE_FUNC(game);
    static CCScene* scene(){
        CCScene *sc=CCScene::create();
        game *ly=game::create();
        sc->addChild(ly);
        return sc;
    }
    
    
    virtual bool init(){
        if (!CCLayer::create()) {
            return false;
        }
        bodys_need_destory=CCArray::createWithCapacity(3);
        bodys_need_destory->retain();
        chains=CCSpriteBatchNode::create(res_png_rope);
        this->addChild(chains,100,1);
#ifndef _debug
        loadBackgroundImage();
#endif
        scheduleUpdate();
        //regsiter to touchDispatcher################=========-------
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
        //init b2world################=========-------
        b2Vec2 graverty=b2Vec2(0,-10);
        bool allow_sleep=true;
        world=new b2World(graverty);
        world->SetAllowSleeping(allow_sleep);
        //setup debug draw################=========-------
#ifdef _debug
        debug_draw=new GLESDebugDraw(ptm_rto);
        debug_draw->SetFlags(
                             b2Draw::e_aabbBit
                             |b2Draw::e_centerOfMassBit
                             |b2Draw::e_jointBit
                             |b2Draw::e_pairBit
                             |b2Draw::e_shapeBit
                             );
        world->SetDebugDraw(debug_draw);
#endif
        world->SetContactListener(this);
        //add suger and bind to Sprite################=========-------
        CCSprite *sp=CCSprite::create(res_png_suger);
        sp->setTag(type_suger);
        this->addChild(sp);
        sp->setScale(0.583);
        b2BodyDef obj_bdy_def;
        obj_bdy_def.type=b2_dynamicBody;
        obj_bdy_def.position.Set(4.0f, 4.8f);
        b2Body *obj_bdy=world->CreateBody(&obj_bdy_def);
        obj_bdy->SetGravityScale(0.5);
        suger=obj_bdy;
        //obj_bdy->SetLinearDamping(0.5);
        b2PolygonShape obj_shp;
        obj_shp.SetAsBox(1, 1);
        obj_bdy->CreateFixture(&obj_shp, 0.001);
        // obj_bdy->SetAngularVelocity(-6.283f);
        obj_bdy->SetUserData(sp);
        
        //add monster################=========-------
        b2BodyDef monster_body_def;
        monster_body_def.type=b2_staticBody;
        monster_body_def.position.Set(1136/2/ptm_rto,80/ptm_rto);
        b2Body *monster_body=world->CreateBody(&monster_body_def);
        b2PolygonShape monster_shape;
        monster_shape.SetAsBox(0.5, 0.5);
        b2FixtureDef monster_fix_def;
        monster_fix_def.isSensor=true;
        monster_fix_def.shape=&monster_shape;
        monster_body->CreateFixture(&monster_fix_def);
        CCSprite *monster=CCSprite::create(res_png_monster);
        monster->setScale(80.0f/312.0f);
        monster->setTag(type_moster);
        this->addChild(monster);
        monster_body->SetUserData(monster);
        
        //add touch################=========-------
        b2BodyDef touch_body_def;
        touch_body_def.type=b2_staticBody;
        touch_body_def.position.Set(0,-10);
        touch_body_def.bullet=true;
        touch_body_def.fixedRotation=true;
        touchPoint=world->CreateBody(&touch_body_def);
        b2CircleShape touch_shape;
        touch_shape.m_radius=0.25f;
        touchPoint->CreateFixture(&touch_shape, 0);
        CCMotionStreak *m=CCMotionStreak::create(0.25f, 3, 40, ccc3(255, 255, 255), res_png_touch);
        m->setTag(type_touch);
        this->addChild(m);
        touchPoint->SetUserData(m);
        
        //create chains
        createChain(b2Vec2(345.0f/ptm_rto,500.0f/ptm_rto));
        createChain(b2Vec2(827.0f/ptm_rto,500.0f/ptm_rto));
        
        return true;
    }
    //wall cat:0x1 mask:0xe
    //suger cat:0x2 mask:0xd
    //rope cat:0x4 mask:0x3
    //monster cat:0x8 mask:0x2
    void loadBackgroundImage(){
        CCSprite *bg=CCSprite::create(res_png_background);
        bg->setAnchorPoint(ccp(.5f, .5f));
        bg->setPosition(ccp(1136/2, 640/2));
        this->addChild(bg,0,-10);
        
        CCSize sz=CCDirector::sharedDirector()->getWinSize();
        CCSize contentSz=bg->getContentSize();
        
        bg->setScaleX(sz.width/contentSz.width);
        bg->setScaleY(sz.height/contentSz.height);
    }
    
    virtual void update(float delta){
        world->Step(delta, 16, 16);
        for(b2Body *b= world->GetBodyList();b;b=b->GetNext()){
            CCNode *sp=(CCNode*)b->GetUserData();
            if (sp) {
                sp->setPosition(ccp(b->GetPosition().x*ptm_rto,b->GetPosition().y*ptm_rto));
                sp->setRotation(-CC_RADIANS_TO_DEGREES(b->GetAngle()));
            }
            
        }
        //safe release body
       
            CCObject *bb;
            CCARRAY_FOREACH(bodys_need_destory, bb){
                B2BODY *bodyContainer=(B2BODY*)bb;
                b2Body *b=bodyContainer->m_b2body;
                CCNode *node=(CCNode*)b->GetUserData();
                b->SetUserData(0);
                node->removeFromParent();
                world->DestroyBody(b);
            }
            bodys_need_destory->removeAllObjects();
        
        //check if loose
        if(suger->GetPosition().y<-3){
            CCScene *w=win::scene(CCString::create("You Loose (x_X)"));
            CCDirector::sharedDirector()->replaceScene(w);
        }
    }
    
    virtual void draw(){
        CCNode::draw();
        //Open GL
#ifdef _debug
        glDisable(GL_TEXTURE_2D);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        world->DrawDebugData();
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
    }
    virtual bool ccTouchBegan(CCTouch *pTouch, CCEvent *pEvent){
        touchPoint->SetActive(true);
        return true;
    }
    virtual void ccTouchMoved(CCTouch *pTouch, CCEvent *pEvent){
        b2Vec2 touch=toMiters(pTouch->getLocation());
        touchPoint->SetTransform(touch, touchPoint->GetAngle());
        //        b2Vec2 local=suger->GetPosition();
        //        b2Vec2 r=touch-local;
        //        r.Normalize();
        //        r.operator*=(1000);
        //        suger->ApplyForceToCenter(r);
    }
    virtual void ccTouchEnded(CCTouch *pTouch, CCEvent *pEvent){
        touchPoint->SetActive(false);
    }
    virtual void ccTouchCancelled(CCTouch *pTouch, CCEvent *pEvent){
        touchPoint->SetActive(false);
    }
    b2Vec2 toMiters(CCPoint p){
        return b2Vec2(p.x/ptm_rto,p.y/ptm_rto);
    }
    
    virtual void BeginContact(b2Contact* contact) {
        b2Body *bA=contact->GetFixtureA()->GetBody();
        CCNode *nA=(CCNode*)bA ->GetUserData();
        b2Body *bB=contact->GetFixtureB()->GetBody();
        CCNode *nB=(CCNode*) bB->GetUserData();
        //CCLog("contact %i - %i",nA->getTag(),nB->getTag());
        
        //marke body for destory while end this loop
        if (nA->getTag()==type_rope&&nB->getTag()==type_touch) {
            //world->DestroyBody(bA);
            B2BODY *b=new B2BODY();
            b->m_b2body=bA;
            b->autorelease();
            bodys_need_destory->addObject(b);
        }else  if(nB->getTag()==type_rope&&nA->getTag()==type_touch){
            nB->removeFromParent();
            //world->DestroyBody(bB);
            B2BODY *b=new B2BODY;
            b->m_b2body=bB;
            b->autorelease();
            bodys_need_destory->addObject(b);
        }
        
        if((nB->getTag()==type_suger&&nA->getTag()==type_moster)||(nA->getTag()==type_suger&&nB->getTag()==type_moster)){
            //CCLog("you Win");
            CCScene *w=win::scene(CCString::create("You Win (^-^)"));
            CCDirector::sharedDirector()->replaceScene(w);
            
            
        }
    }
    //create a chain connect to the suger
    void createChain(b2Vec2 point){
        b2BodyDef p;
        p.type=b2_staticBody;
        p.position.Set(point.x, point.y);
        b2Body *p_b=world->CreateBody(&p);
        b2Body *lastBody=p_b;
        
        for(float i=0;i<10;i++){
            b2BodyDef chain_body_def;
            chain_body_def.type=b2_dynamicBody;
            b2PolygonShape chain_shape_def;
            chain_shape_def.SetAsBox(0.05f, 0.5);
            b2Body *chain_body=world->CreateBody(&chain_body_def);
            chain_body->CreateFixture(&chain_shape_def, 0.001f);
            
            CCSprite *cb_spr=CCSprite::create(res_png_rope);
            cb_spr->setScaleX(4/cb_spr->getContentSize().width);
            cb_spr->setScaleY(40/cb_spr->getContentSize().height);
            cb_spr->setTag(type_rope);
            chains->addChild(cb_spr);
            chain_body->SetUserData(cb_spr);
            
            if (i==9) {
                chain_body=suger;
            }
            
            b2RevoluteJointDef joint_def;
            joint_def.bodyA=lastBody;
            joint_def.localAnchorA=b2Vec2(0, 0.4);
            joint_def.bodyB=chain_body;
            joint_def.localAnchorB=b2Vec2(0, -0.4);
            world->CreateJoint(&joint_def);
            lastBody=chain_body;
        }
    }
    
    ~game(){
        CC_SAFE_DELETE(world);
        CC_SAFE_DELETE(debug_draw);
        CC_SAFE_RELEASE_NULL(bodys_need_destory);
    }
};
#endif /* defined(__cutrope__game__) */
