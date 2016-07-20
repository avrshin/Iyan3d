//
//  OBJMeshFileLoader.cpp
//  SGEngine2
//
//  Created by Vivek shivam on 30/03/2015.
//  Copyright (c) 2014 Smackall Games Pvt Ltd. All rights reserved.
//

#include "OBJMeshFileLoader.h"
#include "../../Iyan3dEngineFiles/HeaderFiles/Constants.h"

#ifdef  ANDROID
//#include "../../../../../../../../../../Android/Sdk/ndk-bundle/platforms/android-21/arch-x86_64/usr/include/string.h"
//#include "../../../../../../../../../../Android/Sdk/ndk-bundle/platforms/android-21/arch-x86_64/usr/include/math.h"
#endif

static const u32 WORD_BUFFER_LENGTH = 512;

OBJMeshFileLoader::OBJMeshFileLoader()
{
}

OBJMeshFileLoader::~OBJMeshFileLoader()
{
}

bool OBJMeshFileLoader::isALoadableFileExtension(const string& filename) const
{
    return true;
}

Mesh* OBJMeshFileLoader::createMesh(string filepath, int& status, DEVICE_TYPE device)
{
    VertMap.clear();
    Mesh* objMesh = new Mesh();
    ifstream file(filepath, ios::ate);
    if (!file.is_open())
        Logger::log(ERROR, "OBJLoader", "File not available" + filepath);
    
    file.seekg(0, file.end);
    int length = (int)file.tellg();
    file.seekg(0, file.beg);
    
    const long filesize = length;
    if (!filesize) {
        status = OBJ_FILE_READ_ERROR;
        return NULL;
    }
    
    vector<Vector3> positionBuffer;
    vector<Vector3> normalsBuffer;
    vector<Vector2> textureCoordBuffer;
    
    u32 smoothingGroup = 0;
    
    string relPath = " ";
    printf("\n filesize %d %d", length, file.end);
    c8* buf = new c8[filesize];
    memset(buf, 0, filesize);
    file.read((char*)buf, filesize);
    const c8* const bufEnd = buf + filesize;
    
    const c8* bufPtr = buf;
    string grpName, mtlName;
    bool mtlChanged = false;
    bool useGroups = false;
    bool useMaterials = false;
    while (bufPtr != bufEnd) {
        switch (bufPtr[0]) {
            case 'm':
            {
                if (useMaterials) {
                    c8 name[WORD_BUFFER_LENGTH];
                    bufPtr = goAndCopyNextWord(name, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
                    //readMTL(name, relPath);
                }
            } break;
                
            case 'v':
                switch (bufPtr[1]) {
                    case ' ':
                    {
                        Vector3 vec;
                        bufPtr = readVec3(bufPtr, vec, bufEnd);
                        positionBuffer.push_back(vec);
                    } break;
                        
                    case 'n':
                    {
                        Vector3 vec;
                        bufPtr = readVec3(bufPtr, vec, bufEnd);
                        normalsBuffer.push_back(vec);
                    } break;
                        
                    case 't':
                    {
                        Vector2 vec;
                        bufPtr = readUV(bufPtr, vec, bufEnd);
                        textureCoordBuffer.push_back(vec);
                    } break;
                }
                break;
                
            case 'g':
            {
                c8 grp[WORD_BUFFER_LENGTH];
                bufPtr = goAndCopyNextWord(grp, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
                if (useGroups) {
                    if (0 != grp[0])
                        grpName = grp;
                    else
                        grpName = "default";
                }
                mtlChanged = true;
            } break;
                
            case 's':
            {
                c8 smooth[WORD_BUFFER_LENGTH];
                bufPtr = goAndCopyNextWord(smooth, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
                if (string("off") == smooth)
                    smoothingGroup = 0;
                else
                    smoothingGroup = strtoul10(smooth);
            } break;
                
            case 'u':
            {
                c8 matName[WORD_BUFFER_LENGTH];
                bufPtr = goAndCopyNextWord(matName, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
                mtlName = matName;
                mtlChanged = true;
            }
                break;
                
            case 'f':
            {
                c8 vertexWord[WORD_BUFFER_LENGTH];
                vertexData v;
                const string wordBuffer = copyLine(bufPtr, bufEnd);
                const c8* linePtr = wordBuffer.c_str();
                const c8* const endPtr = linePtr + wordBuffer.size();
                
                vector<unsigned int> faceCorners;
                
                linePtr = goNextWord(linePtr, endPtr);
                while (0 != linePtr[0]) {
                    int Idx[3];
                    Idx[1] = Idx[2] = -1;
                    
                    u32 wlength = copyWord(vertexWord, linePtr, WORD_BUFFER_LENGTH, endPtr);
                    retrieveVertexIndices(vertexWord, Idx, vertexWord + wlength + 1, (unsigned int)positionBuffer.size(), (unsigned int)textureCoordBuffer.size(), (unsigned int)normalsBuffer.size());
                    v.vertPosition = positionBuffer[Idx[0]];
                    if (-1 != Idx[1])
                        v.texCoord1 = textureCoordBuffer[Idx[1]];
                    else
                        v.texCoord1 = Vector2(0.0f, 0.0f);
                    if (-1 != Idx[2])
                        v.vertNormal = normalsBuffer[Idx[2]];
                    else
                        v.vertNormal = Vector3(0.0f, 0.0f, 0.0f);

                    unsigned int vertId = 0;
                    string posStr = to_string(v.vertPosition.x) + to_string(v.vertPosition.y) + to_string(v.vertPosition.z);
                    string tcoordStr = posStr + to_string(v.texCoord1.x) + to_string(v.texCoord1.y);
                    std::hash<std::string> hash_fn;
                    std::size_t str_hash = hash_fn(tcoordStr);
                    if (VertMap.find(str_hash) != VertMap.end())
                        vertId = VertMap.find(str_hash)->second;
                    else {
                        objMesh->addVertex(&v);
                        vertId = objMesh->getVerticesCount() - 1;
                        VertMap.insert(std::pair<size_t, int>(str_hash, vertId));
                        int verticesCount = (int)objMesh->getVerticesCount();
                        if (verticesCount > OBJ_MAX_VERTICES_ALLOWED) {
                            delete objMesh;
                            status = OBJ_CROSSED_MAX_VERTICES_LIMIT;
                            VertMap.clear();
                            delete[] buf;
                            file.close();
                            return NULL;
                        }
                    }
                    faceCorners.push_back(vertId);
                    linePtr = goNextWord(linePtr, endPtr);
                }

                for (u32 i = 1; i < faceCorners.size() - 1; ++i) {
                    objMesh->addToIndicesArray(faceCorners[i + 1]);
                    objMesh->addToIndicesArray(faceCorners[i]);
                    objMesh->addToIndicesArray(faceCorners[0]);
                }
                faceCorners.clear();
            } break;
                
            case '#':
            default:
                break;
        }
        bufPtr = goNextLine(bufPtr, bufEnd);
    }
    VertMap.clear();
    delete[] buf;
    objMesh->recalculateNormals();
    file.close();
    return objMesh;
}

const c8* OBJMeshFileLoader::readVec3(const c8* bufPtr, Vector3& vec, const c8* const bufEnd)
{
    const u32 WORD_BUFFER_LENGTH = 256;
    c8 wordBuffer[WORD_BUFFER_LENGTH];
    
    bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
    vec.x = -stof(wordBuffer);
    bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
    vec.y = stof(wordBuffer);
    bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
    vec.z = stof(wordBuffer);
    return bufPtr;
}

const c8* OBJMeshFileLoader::readUV(const c8* bufPtr, Vector2& vec, const c8* const bufEnd)
{
    const u32 WORD_BUFFER_LENGTH = 256;
    c8 wordBuffer[WORD_BUFFER_LENGTH];
    
    bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
    vec.x = stof(wordBuffer);
    bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
    vec.y = 1 - stof(wordBuffer);

    return bufPtr;
}

const c8* OBJMeshFileLoader::readBool(const c8* bufPtr, bool& tf, const c8* const bufEnd)
{
    const u32 BUFFER_LENGTH = 8;
    c8 tfStr[BUFFER_LENGTH];
    
    bufPtr = goAndCopyNextWord(tfStr, bufPtr, BUFFER_LENGTH, bufEnd);
    tf = strcmp(tfStr, "off") != 0;
    return bufPtr;
}

const c8* OBJMeshFileLoader::goFirstWord(const c8* buf, const c8* const bufEnd, bool acrossNewlines)
{
    if (acrossNewlines) {
        while ((buf != bufEnd) && isspace(*buf))
            ++buf;
    } else {
        while ((buf != bufEnd) && isspace(*buf) && (*buf != '\n'))
            ++buf;
    }
    
    return buf;
}

const c8* OBJMeshFileLoader::goNextWord(const c8* buf, const c8* const bufEnd, bool acrossNewlines)
{
    while ((buf != bufEnd) && !isspace(*buf))
        ++buf;
    
    return goFirstWord(buf, bufEnd, acrossNewlines);
}

const c8* OBJMeshFileLoader::goNextLine(const c8* buf, const c8* const bufEnd)
{
    while (buf != bufEnd) {
        if (*buf == '\n' || *buf == '\r')
            break;
        ++buf;
    }
    return goFirstWord(buf, bufEnd);
}

u32 OBJMeshFileLoader::copyWord(c8* outBuf, const c8* const inBuf, u32 outBufLength, const c8* const bufEnd)
{
    if (!outBufLength)
        return 0;
    if (!inBuf) {
        *outBuf = 0;
        return 0;
    }
    
    u32 i = 0;
    while (inBuf[i]) {
        if (isspace(inBuf[i]) || &(inBuf[i]) == bufEnd)
            break;
        ++i;
    }
    
    u32 length = min(i, outBufLength - 1);
    for (u32 j = 0; j < length; ++j)
        outBuf[j] = inBuf[j];
    
    outBuf[length] = 0;
    return length;
}

string OBJMeshFileLoader::copyLine(const c8* inBuf, const c8* bufEnd)
{
    if (!inBuf)
        return string();
    
    const c8* ptr = inBuf;
    while (ptr < bufEnd) {
        if (*ptr == '\n' || *ptr == '\r')
            break;
        ++ptr;
    }
    return string(inBuf, (u32)(ptr - inBuf + ((ptr < bufEnd) ? 1 : 0)));
}

const c8* OBJMeshFileLoader::goAndCopyNextWord(c8* outBuf, const c8* inBuf, u32 outBufLength, const c8* bufEnd)
{
    inBuf = goNextWord(inBuf, bufEnd, false);
    copyWord(outBuf, inBuf, outBufLength, bufEnd);
    return inBuf;
}

int OBJMeshFileLoader::strtol10(const char* in, const char** out)
{
    if (!in) {
        if (out)
            *out = in;
        return 0;
    }
    
    const bool negative = ('-' == *in);
    if (negative || ('+' == *in))
        ++in;
    
    const u32 unsignedValue = strtoul10(in, out);
    if (unsignedValue > (u32)INT_MAX) {
        if (negative)
            return (int)INT_MIN;
        else
            return (int)INT_MAX;
    } else {
        if (negative)
            return -((int)unsignedValue);
        else
            return (int)unsignedValue;
    }
}

bool OBJMeshFileLoader::retrieveVertexIndices(c8* vertexData, int* idx, const c8* bufEnd, u32 vbsize, u32 vtsize, u32 vnsize)
{
    c8 word[16] = "";
    const c8* p = goFirstWord(vertexData, bufEnd);
    u32 idxType = 0;
   
    u32 i = 0;
    while (p != bufEnd) {
        if ((isdigit(*p)) || (*p == '-'))
            word[i++] = *p;

        else if (*p == '/' || *p == ' ' || *p == '\0') {
            word[i] = '\0';
            idx[idxType] = strtol10(word);
            if (idx[idxType] < 0) {
                switch (idxType) {
                    case 0:
                        idx[idxType] += vbsize;
                        break;
                    case 1:
                        idx[idxType] += vtsize;
                        break;
                    case 2:
                        idx[idxType] += vnsize;
                        break;
                }
            }
            else
                idx[idxType] -= 1;
            
            word[0] = '\0';
            i = 0;
            
            if (*p == '/') {
                if (++idxType > 2)
                    idxType = 0;
            } else {
                while (++idxType < 3)
                    idx[idxType] = -1;
                ++p;
                break;
            }
        }
        
        ++p;
    }
    return true;
}

void OBJMeshFileLoader::cleanUp()
{
}

u32 OBJMeshFileLoader::strtoul10(const char* in, const char** out)
{
    if (!in) {
        if (out)
            *out = in;
        return 0;
    }
    
    bool overflow = false;
    u32 unsignedValue = 0;
    while ((*in >= '0') && (*in <= '9')) {
        const u32 tmp = (unsignedValue * 10) + (*in - '0');
        if (tmp < unsignedValue) {
            unsignedValue = (u32)0xffffffff;
            overflow = true;
        }
        if (!overflow)
            unsignedValue = tmp;
        ++in;
    }
    
    if (out)
        *out = in;
    
    return unsignedValue;
}
