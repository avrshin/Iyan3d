//
//  SceneManager.cpp
//  SGEngine2
//
//  Created by Harishankar on 15/11/14.
//  Copyright (c) 2014 Smackall Games Pvt Ltd. All rights reserved.
//
#include "SceneManager.h"

#ifdef IOS
#import "TargetConditionals.h"
#elif ANDROID
//#include "../../../../../../../../../Android/Sdk/ndk-bundle/platforms/android-21/arch-arm/usr/include/android/log.h"
#endif

DEVICE_TYPE common::deviceType = OPENGLES2;
SceneManager::SceneManager(float width,float height,float screenScale,DEVICE_TYPE type,string bundlePath,void *renderView){

    device = type;
    displayWidth = width;
    displayHeight = height;
    this->screenScale = screenScale;
    this->bundlePath = bundlePath;
    #ifdef ANDROID
    renderMan = new OGLES2RenderManager(width,height,screenScale);
    common::deviceType = OPENGLES2;
    #elif IOS
    if(type == OPENGLES2){
        renderMan = new OGLES2RenderManager(width,height,screenScale);
        common::deviceType = OPENGLES2;
    }else if(type == METAL){
        #if !(TARGET_IPHONE_SIMULATOR)
            renderMan = (RenderManager*)initMetalRenderManager(renderView,width,height,screenScale);
            common::deviceType = METAL;
        #endif
    }
    #endif
    mtlManger = new MaterialManager(type);
    renderTargetIndex = 0;
}

SceneManager::~SceneManager(){
    RemoveAllNodes();
    RemoveAllTextures();
    if(mtlManger)
        delete mtlManger;
    if(renderMan)
        delete renderMan;
}

void SceneManager::RemoveAllTextures()
{
    for(int i = 0; i < textures.size(); i++){
        if(textures[i])
            delete textures[i];
    }
    textures.clear();

}

void SceneManager::setDisplayResolution(int width,int height){
    displayWidth = width;
    displayHeight = height;
}

void SceneManager::AddNode(shared_ptr<Node> node,MESH_TYPE meshType){
#ifndef UBUNTU
    renderMan->createVertexAndIndexBuffers(node,meshType);
#endif
    nodes.push_back(node);
}
void SceneManager::RemoveNode(shared_ptr<Node> node){
    int sameNodeIdcount = 0;
    for(int i = 0;i < nodes.size();i++){
        if(nodes[i]->getID() == node->getID()){
            sameNodeIdcount++;
        }
    }
    if(sameNodeIdcount > 1)
        Logger::log(ERROR,"SceneManager::RemoveNode","Node id repeats");
    for(int i = 0;i < nodes.size();i++){
        if(nodes[i]->getID() == node->getID()){
            nodes[i]->detachFromParent();
            nodes[i]->detachAllChildren();
            if(nodes[i])
                nodes[i].reset();
            
            nodes.erase(nodes.begin() + i);
            break;
        }
    }
    //node.reset();
}
void SceneManager::RemoveTexture(Texture *texture){
    for(int i = 0;i < textures.size();i++){
        if(textures[i] == texture){
            if(device == OPENGLES2){
                if(((OGLTexture*)textures[i])->OGLTextureName == ((OGLTexture*)texture)->OGLTextureName){
                    delete textures[i];
                    textures.erase(textures.begin() + i);
                    break;
                }
            }else if(device == METAL){
                // TODO: Implement Texture removal for metal
                if(textures[i] == texture)
                    delete textures[i];
                textures.erase(textures.begin() + i);
                break;
            }
        }
    }
}
void SceneManager::RemoveAllNodes(){
    for(int i = 0;i < nodes.size();i++){
        if (nodes[i])
        {
            nodes[i]->detachFromParent();
            nodes[i]->detachAllChildren();
            nodes[i].reset();
            nodes.erase(nodes.begin() + i);
        }
        }
    nodes.clear();
}
bool SceneManager::PrepareDisplay(int width,int height,bool clearColorBuf,bool clearDepthBuf,bool isDepthPass,Vector4 color){
    renderTargetIndex++;
    return renderMan->PrepareDisplay(width,height,clearColorBuf,clearDepthBuf,isDepthPass,color);
}
void SceneManager::Render(bool isRTT)
{
    vector<int> nodeIndex;
    for (int i = 0;i < nodes.size();i++){
        if(nodes[i]->type <= NODE_TYPE_CAMERA || nodes[i]->getVisible() == false)
            continue;
        if(isTransparentCallBack(nodes[i]->getID(),nodes[i]->callbackFuncName)){
            nodeIndex.push_back(i);
            continue;
        }
        RenderNode(isRTT, i);
    }
    for (int i = 0;i < nodeIndex.size();i++){
        int nodeId = nodeIndex[i];
        if(nodes[nodeId]->type <= NODE_TYPE_CAMERA || nodes[nodeId]->getVisible() == false || (nodes[nodeId]->getID() >= 600000 && nodes[nodeId]->getID() < 600010))
            continue;
        RenderNode(isRTT, nodeId);
    }
    nodeIndex.clear();
}
void SceneManager::EndDisplay()
{
    renderTargetIndex = 0;
    renderMan->endDisplay();
}
void SceneManager::clearDepthBuffer(){
    renderMan->setUpDepthState(CompareFunctionLessEqual,false,true); // ToDo change in depthstate for each render, may need
}
void SceneManager::RenderNode(bool isRTT, int index,bool clearDepthBuffer,METAL_DEPTH_FUNCTION func,bool changeDepthState)
{
    if(!nodes[index])
        return;
    
    if(clearDepthBuffer){
		#ifndef UBUNTU
        	if(device == OPENGLES2)
        		glClear(GL_DEPTH_BUFFER_BIT);
		#endif
    }
    if(device == METAL) {
        renderMan->setUpDepthState(func,true,clearDepthBuffer); // ToDo change in depthstate for each render,  need optimisation
    }
    nodes[index]->update();
    Mesh* meshToRender = dynamic_pointer_cast<MeshNode>(nodes[index])->getMesh();
    
    for(int meshBufferIndex = 0; meshBufferIndex < meshToRender->getMeshBufferCount(); meshBufferIndex++) {
        if(!renderMan->PrepareNode(nodes[index], meshBufferIndex, index))
            return;
        ShaderCallBackForNode(nodes[index]->getID(),nodes[index]->material->name,nodes[index]->callbackFuncName);
        renderMan->Render(nodes[index],isRTT, index,meshBufferIndex);
    }
}
void SceneManager::setDepthTest(bool enable){
	#ifndef UBUNTU
    	if(enable)
    		glEnable(GL_DEPTH_TEST);
    	else
    		glDisable(GL_DEPTH_TEST);
	#endif
}
void SceneManager::setShaderState(int nodeIndex){
    if(nodes[nodeIndex]->type <= NODE_TYPE_CAMERA)
         return;
    if(device == OPENGLES2){
        
    }else if(device == METAL){
        MTLPipelineStateCallBack(nodeIndex);
    }
}
void SceneManager::setActiveCamera(shared_ptr<Node> node){
    for(int i = 0;i < nodes.size();i++){
        if(nodes[i]->type <= NODE_TYPE_CAMERA)
            (dynamic_pointer_cast<CameraNode>(nodes[i]))->setActive(false);
    }
#ifndef UBUNTU
    renderMan->setActiveCamera(dynamic_pointer_cast<CameraNode>(node));
#endif
}
shared_ptr<CameraNode> SceneManager::getActiveCamera(){
    return renderMan->getActiveCamera();
}
Texture* SceneManager::loadTexture(string textureName,string filePath,TEXTURE_DATA_FORMAT format,TEXTURE_DATA_TYPE type)
{
    Texture *newTex = NULL;
    #ifdef ANDROID
    newTex = new OGLTexture();
    #elif IOS
    if(device == OPENGLES2)
        newTex = new OGLTexture();
    else if(device == METAL){
        #if !(TARGET_IPHONE_SIMULATOR)
            newTex = (Texture*)initMTLTexture();
        #endif
    }
    #endif

#ifdef UBUNTU
    newTex = new DummyTexture();
#endif
    newTex->loadTexture(textureName,filePath,format,type);
    textures.push_back(newTex);

    return newTex;
}

Texture* SceneManager::loadTextureFromVideo(string videoFileName,TEXTURE_DATA_FORMAT format,TEXTURE_DATA_TYPE type)
{
    Texture *newTex = NULL;
#ifdef ANDROID
    newTex = new OGLTexture();
#elif IOS
    if(device == OPENGLES2)
        newTex = new OGLTexture();
    else if(device == METAL){
#if !(TARGET_IPHONE_SIMULATOR)
        newTex = (Texture*)initMTLTexture();
#endif
    }
#endif
    
#ifdef UBUNTU
    newTex = new DummyTexture();
#endif
    newTex->loadTextureFromVideo(videoFileName,format,type);
    textures.push_back(newTex);
    
    return newTex;
}


shared_ptr<MeshNode> SceneManager::createNodeFromMesh(Mesh* mesh,string callbackFuncName,MESH_TYPE meshType,int matIndex){
    shared_ptr<MeshNode> node = make_shared<MeshNode>(); //shared_ptr<MeshNode>(new MeshNode());
    if(matIndex != NOT_EXISTS && mtlManger->materials->size())
        node->setMaterial(getMaterialByIndex(matIndex));
    node->mesh = mesh;
    node->needsVertexNormal = true;
    node->callbackFuncName = callbackFuncName;
    if(node->mesh)
    	node->mesh->Commit();
    AddNode(node,meshType);
    return node;
}
shared_ptr<AnimatedMeshNode> SceneManager::createAnimatedNodeFromMesh(AnimatedMesh* mesh,string callbackFuncName,rig_type rigType ,MESH_TYPE meshType){
    shared_ptr<AnimatedMeshNode> node = make_shared<AnimatedMeshNode>();
    node->setMesh(mesh , rigType);
    node->needsVertexNormal = true;
    node->needsUV1 = true;
    node->callbackFuncName = callbackFuncName;
    node->mesh->Commit();
    AddNode(node,meshType);
    return node;
}
shared_ptr<ParticleManager> SceneManager::createParticlesFromMesh(Mesh* mesh,string callBackFuncName,MESH_TYPE meshType,int matIndex)
{
    shared_ptr<ParticleManager> node = make_shared<ParticleManager>();
    if(matIndex != NOT_EXISTS && mtlManger->materials->size())
        node->setMaterial(getMaterialByIndex(matIndex));
    node->mesh = mesh;
    node->needsVertexNormal = true;
    node->callbackFuncName = callBackFuncName;
    if(node->mesh)
        node->mesh->Commit();
    AddNode(node,meshType);
    return node;
}

shared_ptr<CameraNode> SceneManager::createCameraNode(string callBackFuncName){
    shared_ptr<CameraNode> cam = make_shared<CameraNode>();
    cam->callbackFuncName = callBackFuncName;
    AddNode(cam,MESH_TYPE_LITE);
    return cam;
}
shared_ptr<PlaneMeshNode> SceneManager::createPlaneNode(string callBackFuncName , float aspectRatio){
    shared_ptr<PlaneMeshNode> plane(new PlaneMeshNode(aspectRatio));// = make_shared<PlaneMeshNode>(aspectRatio);
    plane->callbackFuncName = callBackFuncName;
    plane->mesh->Commit();
    AddNode(plane,MESH_TYPE_LITE);
    return plane;
}
shared_ptr<SGCircleNode> SceneManager::createCircleNode(int totVertices,int radius,string callBackFuncName){
    shared_ptr<SGCircleNode> node = shared_ptr<SGCircleNode>(new SGCircleNode(totVertices,radius));
    node->callbackFuncName = callBackFuncName;
    node->mesh->Commit();
    AddNode(node,MESH_TYPE_LITE);
    return node;
}

shared_ptr<LightNode> SceneManager::createLightNode(Mesh *mesh, string callBackFuncName) {
    shared_ptr<LightNode> light(new LightNode());
    light->callbackFuncName = callBackFuncName;
    light->mesh = mesh;
    light->mesh->Commit();
    AddNode(light,MESH_TYPE_LITE);
    return light;
}

void SceneManager::draw2DImage(Texture *texture,Vector2 originCoord,Vector2 endCoord,bool isBGImage,Material *material,bool isRTT)
{
    renderMan->useMaterialToRender(material);
    int textureValue = (device == OPENGLES2) ? ((OGLTexture*)texture)->OGLTextureName : 0;
    setPropertyValue(material,"texture1",&textureValue,DATA_TEXTURE_2D,1,true,0,NOT_EXISTS,texture);
    renderMan->draw2DImage(texture,originCoord,endCoord,isBGImage,material,isRTT);
}
void SceneManager::draw3DLine(Vector3 start , Vector3 end , Vector3 color , Material *material,int mvpUniParamIndex,int vertexColorUniParamIndex,int transparencyUniParamIndex)
{
    renderMan->useMaterialToRender(material);
    float vertColor[3] = {color.x,color.y,color.z};
    float transparency = 1.0;
    getActiveCamera()->update();
    Mat4 mat = getActiveCamera()->getProjectionMatrix() * getActiveCamera()->getViewMatrix();
    setPropertyValue(material,"mvp",mat.pointer(),DATA_FLOAT_MAT4,16,false,mvpUniParamIndex);
    setPropertyValue(material,"perVertexColor",&vertColor[0],DATA_FLOAT_VEC3,3,false,vertexColorUniParamIndex);
    setPropertyValue(material,"transparency",&transparency,DATA_FLOAT,1,false,transparencyUniParamIndex);
    renderMan->draw3DLine(start,end,material);
}

void SceneManager::draw3DLines(vector<Vector3> vPositions, Vector3 color, Material *material, int mvpUniParamIndex,int vertexColorUniParamIndex,int transparencyUniParamIndex)
{
    renderMan->useMaterialToRender(material);
    float vertColor[3] = {color.x,color.y,color.z};
    float transparency = 1.0;
    getActiveCamera()->update();
    Mat4 mat = getActiveCamera()->getProjectionMatrix() * getActiveCamera()->getViewMatrix();
    setPropertyValue(material,"mvp",mat.pointer(),DATA_FLOAT_MAT4,16,false,mvpUniParamIndex);
    setPropertyValue(material,"perVertexColor",&vertColor[0],DATA_FLOAT_VEC3,3,false,vertexColorUniParamIndex);
    setPropertyValue(material,"transparency",&transparency,DATA_FLOAT,1,false,transparencyUniParamIndex);
    renderMan->draw3DLines(vPositions, material);
}

void SceneManager::setPropertyValue(Material *material,string name,float* values,DATA_TYPE type,unsigned short count, bool isFragmentData, u16 paramIndex,int nodeIndex,Texture *tex,int userValue){
    shared_ptr<Node> nod;
    if(nodeIndex != NOT_EXISTS) nod = nodes[nodeIndex];
    if(nodeIndex == NOT_EXISTS && device == METAL){
        renderMan->bindDynamicUniform(material,name,values,type,count,paramIndex,nodeIndex,tex,isFragmentData);
    }else{
        short uIndex = material->setPropertyValue(name,values,type,count,paramIndex,nodeIndex,renderTargetIndex);
        renderMan->BindUniform(material,nod,uIndex,isFragmentData,userValue);
    }
}
void SceneManager::setPropertyValue(Material *material,string name,int* values,DATA_TYPE type,unsigned short count, bool isFragmentData,u16 paramIndex,int nodeIndex,Texture *tex,int userValue){
    shared_ptr<Node> nod;
    if(nodeIndex != NOT_EXISTS) nod = nodes[nodeIndex];
    if(nodeIndex == NOT_EXISTS && device == METAL){
        renderMan->bindDynamicUniform(material,name,values,type,count,paramIndex,nodeIndex,tex,isFragmentData);
    }else{
        short uIndex = material->setPropertyValue(name,values,type,count,paramIndex,nodeIndex,renderTargetIndex);
        renderMan->BindUniform(material,nod,uIndex,isFragmentData,userValue);
    }
}
AnimatedMesh* SceneManager::LoadMesh(string filePath){
    return CSGRMeshFileLoader::LoadMesh(filePath,device);
}
short SceneManager::LoadShaders(string materialName,string vShaderName,string fShaderName,bool isDepthPass){
    return mtlManger->CreateMaterial(materialName,vShaderName,fShaderName,isDepthPass);
}
Material* SceneManager::getMaterialByIndex(int index){
	if(index < (int)(*mtlManger->materials).size())
		return (*mtlManger->materials)[index];
	else
		return 0;
}
Material* SceneManager::getMaterialByName(string name){
    for(int i = 0 ; i < mtlManger->materials->size();i++){
        if((*mtlManger->materials)[i]->name.compare(name) == 0)
            return (*mtlManger->materials)[i];
    }
    Logger::log(ERROR,"SceneManager::getMaterialByName","No Material with the" + name + "exists");
    return NULL;
}
int SceneManager::getNodeIndexByID(int id){
    for(int i = 0;i < nodes.size();i++){
        if(nodes[i]->getID() == id)
            return i;
    }
    Logger::log(ERROR, "SceneManager:getNodeIndexByID","Id not Exits");
    return NULL;
}
Texture* SceneManager::createRenderTargetTexture(string textureName , TEXTURE_DATA_FORMAT format, TEXTURE_DATA_TYPE texelType, int width, int height){
    textures.push_back(renderMan->createRenderTargetTexture(textureName,format,texelType,width,height));
    
    return textures[textures.size()-1];
}
void SceneManager::setRenderTarget(Texture *renderTexture,bool clearBackBuffer,bool clearZBuffer,bool isDepthPass,Vector4 color)
{
    if(renderTexture)
        renderTargetIndex++;
    renderMan->setRenderTarget(renderTexture,clearBackBuffer,clearZBuffer,isDepthPass,color);
}
Vector3 SceneManager::getPixelColor(Vector2 touchPosition,Texture* texture,Vector4 bgColor)
{
    Vector4 color = renderMan->getPixelColor(touchPosition,texture);
    return Vector3(color.x,color.y,color.z);
}
void SceneManager::writeImageToFile(Texture *texture, char* filePath, IMAGE_FLIP flipType)
{
    renderMan->writeImageToFile(texture, filePath , flipType);
}
void SceneManager::setFrameBufferObjects(uint32_t fb,uint32_t cb,uint32_t db){
	#ifndef UBUNTU
    	((OGLES2RenderManager*)renderMan)->setFrameBufferObjects(fb,cb,db);
	#endif
}
shared_ptr<EmptyNode> SceneManager::addEmptyNode(){
    shared_ptr<EmptyNode> eNode = make_shared<EmptyNode>(); //shared_ptr<EmptyNode>(new EmptyNode());
    AddNode(eNode);
    return eNode;
}
void SceneManager::updateVertexBuffer(int nodeIndex){
	#ifndef UBUNTU
		if(device == OPENGLES2)
			dynamic_pointer_cast<OGLNodeData>(nodes[nodeIndex]->nodeData)->removeVertexBuffers();
		for(int i = 0; i < dynamic_pointer_cast<MeshNode>(nodes[nodeIndex])->getMesh()->getMeshBufferCount(); i++)
			renderMan->createVertexBuffer(nodes[nodeIndex],i);
	#endif
}


