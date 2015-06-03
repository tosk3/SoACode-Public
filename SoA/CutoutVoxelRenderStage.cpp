#include "stdafx.h"
#include "CutoutVoxelRenderStage.h"

#include <Vorb/graphics/GLProgram.h>
#include "Camera.h"
#include "Chunk.h"
#include "ChunkMeshManager.h"
#include "ChunkRenderer.h"
#include "GameRenderParams.h"
#include "MeshManager.h"
#include "SoaOptions.h"
#include "RenderUtils.h"
#include "ShaderLoader.h"

CutoutVoxelRenderStage::CutoutVoxelRenderStage(const GameRenderParams* gameRenderParams) :
    m_gameRenderParams(gameRenderParams) {
    // Empty
}

void CutoutVoxelRenderStage::render() {
    ChunkMeshManager* cmm = m_gameRenderParams->chunkMeshmanager;
    const std::vector <ChunkMesh *>& chunkMeshes = cmm->getChunkMeshes();
    if (chunkMeshes.empty()) return;

    const f64v3& position = m_gameRenderParams->chunkCamera->getPosition();

    if (!m_program.isCreated()) {
        m_program = ShaderLoader::createProgramFromFile("Shaders/BlockShading/standardShading.vert",
                                                             "Shaders/BlockShading/cutoutShading.frag");
    }
    m_program.use();
    m_program.enableVertexAttribArrays();

    glUniform1f(m_program.getUniform("lightType"), m_gameRenderParams->lightActive);

    glUniform3fv(m_program.getUniform("eyeNormalWorldspace"), 1, &(m_gameRenderParams->chunkCamera->getDirection()[0]));
    glUniform1f(m_program.getUniform("fogEnd"), m_gameRenderParams->fogEnd);
    glUniform1f(m_program.getUniform("fogStart"), m_gameRenderParams->fogStart);
    glUniform3fv(m_program.getUniform("fogColor"), 1, &(m_gameRenderParams->fogColor[0]));
    glUniform3fv(m_program.getUniform("lightPosition_worldspace"), 1, &(m_gameRenderParams->sunlightDirection[0]));
    glUniform1f(m_program.getUniform("specularExponent"), soaOptions.get(OPT_SPECULAR_EXPONENT).value.f);
    glUniform1f(m_program.getUniform("alphaMult"), soaOptions.get(OPT_SPECULAR_INTENSITY).value.f * 0.3f);

    // Bind the block textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, blockPack.textureInfo.id);

    glUniform1f(m_program.getUniform("dt"), (GLfloat)bdt);

    glUniform1f(m_program.getUniform("sunVal"), m_gameRenderParams->sunlightIntensity);

    glUniform1f(m_program.getUniform("alphaMult"), 1.0f);

    float blockAmbient = 0.000f;
    glUniform3f(m_program.getUniform("ambientLight"), blockAmbient, blockAmbient, blockAmbient);
    glUniform3fv(m_program.getUniform("lightColor"), 1, &(m_gameRenderParams->sunlightColor[0]));

    glUniform1f(m_program.getUniform("fadeDistance"), ChunkRenderer::fadeDist);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Chunk::vboIndicesID);

    glLineWidth(3);

    glDisable(GL_CULL_FACE);

    f64v3 cpos;

    static GLuint saveTicks = SDL_GetTicks();
    bool save = 0;
    if (SDL_GetTicks() - saveTicks >= 60000) { //save once per minute
        save = 1;
        saveTicks = SDL_GetTicks();
    }

    ChunkMesh *cm;

    for (int i = chunkMeshes.size() - 1; i >= 0; i--) {
        cm = chunkMeshes[i];

        if (cm->inFrustum) {
            ChunkRenderer::drawCutout(cm, m_program, position,
                                                 m_gameRenderParams->chunkCamera->getViewProjectionMatrix());
        }
    }
    glEnable(GL_CULL_FACE);
    m_program.disableVertexAttribArrays();
    m_program.unuse();
}

