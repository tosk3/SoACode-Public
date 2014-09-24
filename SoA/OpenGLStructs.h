#pragma once
#include "Constants.h"
#include "Keg.h"

// 4324 53
struct ColorVertex {
public:
    f32v3 position;
    ubyte color[4];
};

enum class BlendType {
    BLEND_TYPE_REPLACE,
    BLEND_TYPE_ADD,
    BLEND_TYPE_SUBTRACT,
    BLEND_TYPE_MULTIPLY
};
KEG_ENUM_DECL(BlendType);

// Size: 32 Bytes
struct BlockVertex {
public:
    struct vPosition {  //3 bytes  << 1
        ubyte x;
        ubyte y;
        ubyte z;
    } position;
    ubyte textureType; //4   

    //   10 = overlay 
    //  100 = animated overlay 
    // 1000 = normal map
    //10000 = specular map
    ubyte tex[2]; //6 
    ubyte animationLength; //7
    ubyte blendMode; //8

    ubyte textureAtlas; //9
    ubyte overlayTextureAtlas; //10
    ubyte textureIndex; //11
    ubyte overlayTextureIndex; //12

    ubyte textureWidth; //13
    ubyte textureHeight; //14
    ubyte overlayTextureWidth; //15;
    ubyte overlayTextureHeight; //16

    ubyte color[4]; //20
    ubyte overlayColor[3]; //23
    ubyte pad2; //24

    ubyte lampColor[3]; //27
    ubyte sunlight; //28

    sbyte normal[3]; //31
    sbyte merge; //32
};

struct LiquidVertex {
public:
    // TODO: x and z can be bytes?
    f32v3 position; //12
    GLubyte tex[2]; //14
    GLubyte textureUnit; //15 
    GLubyte textureIndex; //16
    GLubyte color[4]; //20
    GLubyte lampColor[3]; //23
    GLubyte sunlight; //24
};

struct TerrainVertex {
public:
    f32v3 location; //12
    f32v2 tex; //20
    f32v3 normal; //32
    GLubyte color[4]; //36
    GLubyte slopeColor[4]; //40
    GLubyte beachColor[4]; //44
    GLubyte textureUnit;
    GLubyte temperature;
    GLubyte rainfall;
    GLubyte specular; //48
};

struct PhysicsBlockVertex {
public:
    ui8 position[3]; //3
    ui8 blendMode; //4
    ui8 tex[2]; //6
    ui8 pad1[2]; //8

    ui8 textureAtlas; //9
    ui8 overlayTextureAtlas; //10
    ui8 textureIndex; //11
    ui8 overlayTextureIndex; //12

    ui8 textureWidth; //13
    ui8 textureHeight; //14
    ui8 overlayTextureWidth; //15
    ui8 overlayTextureHeight; //16

    i8 normal[3]; //19
    i8 pad2; //20
};

struct Face {
public:
    Face(i32 facen, i32 f1, i32 f2, i32 f3, i32 t1, i32 t2, i32 t3, i32 m) : facenum(facen) {
        vertexs[0] = f1;
        vertexs[1] = f2;
        vertexs[2] = f3;
        texcoord[0] = t1;
        texcoord[1] = t2;
        texcoord[2] = t3;
        mat = m;
        isQuad = 0;
    }
    Face(i32 facen, i32 f1, i32 f2, i32 f3, i32 f4, i32 t1, i32 t2, i32 t3, i32 t4, i32 m) : facenum(facen) {
        vertexs[0] = f1;
        vertexs[1] = f2;
        vertexs[2] = f3;
        vertexs[3] = f4;
        texcoord[0] = t1;
        texcoord[1] = t2;
        texcoord[2] = t3;
        texcoord[3] = t4;
        mat = m;
        isQuad = 1;
    }

    i32 facenum;
    bool isQuad;
    i32 vertexs[4];
    i32 texcoord[4];
    i32 mat;
};

struct Material {
public:
    Material(const char* na, f32 a, f32 n, f32 ni2, f32* d, f32* am,
        f32* s, i32 il, i32 t);

    std::string name;
    f32 alpha, ns, ni;
    f32 dif[3], amb[3], spec[3];
    i32 illum;
    i32 texture;
};

struct TexCoord {
public:
    TexCoord(f32 a, f32 b);

    f32 u, v;
};
