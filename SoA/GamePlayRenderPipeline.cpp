#include "stdafx.h"
#include "GamePlayRenderPipeline.h"

#include <Vorb/graphics/GLStates.h>

#include "ChunkGridRenderStage.h"
#include "ChunkMemoryManager.h"
#include "ChunkMeshManager.h"
#include "CutoutVoxelRenderStage.h"
#include "DevHudRenderStage.h"
#include "Errors.h"
#include "GameSystem.h"
#include "HdrRenderStage.h"
#include "LiquidVoxelRenderStage.h"
#include "MTRenderState.h"
#include "MeshManager.h"
#include "NightVisionRenderStage.h"
#include "OpaqueVoxelRenderStage.h"
#include "Options.h"
#include "PauseMenu.h"
#include "PauseMenuRenderStage.h"
#include "PdaRenderStage.h"
#include "PhysicsBlockRenderStage.h"
#include "SkyboxRenderStage.h"
#include "SoAState.h"
#include "SpaceSystem.h"
#include "SpaceSystemRenderStage.h"
#include "TransparentVoxelRenderStage.h"
#include "soaUtils.h"

#define DEVHUD_FONT_SIZE 32

GamePlayRenderPipeline::GamePlayRenderPipeline() :
    m_drawMode(GL_FILL) {
    // Empty
}

void GamePlayRenderPipeline::init(const ui32v4& viewport, const SoaState* soaState, const App* app,
                                  const PDA* pda,
                                  SpaceSystem* spaceSystem,
                                  GameSystem* gameSystem,
                                  const PauseMenu* pauseMenu) {
    // Set the viewport
    _viewport = viewport;

    m_soaState = soaState;

    // Grab mesh manager handle
    _meshManager = soaState->chunkMeshManager.get();

    // Check to make sure we aren't leaking memory
    if (_skyboxRenderStage != nullptr) {
        pError("Reinitializing GamePlayRenderPipeline without first calling destroy()!");
    }

    // Construct framebuffer
    _hdrFrameBuffer = new vg::GLRenderTarget(_viewport.z, _viewport.w);
    _hdrFrameBuffer->init(vg::TextureInternalFormat::RGBA16F, graphicsOptions.msaa, vg::TextureFormat::RGBA, vg::TexturePixelType::HALF_FLOAT).initDepth();
    if (graphicsOptions.msaa > 0) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

    // Make swap chain
    _swapChain = new vg::RTSwapChain<2>(_viewport.z, _viewport.w);
    _swapChain->init(vg::TextureInternalFormat::RGBA8);
    _quad.init();

    // Get window dimensions
    f32v2 windowDims(_viewport.z, _viewport.w);

    m_spaceCamera.setAspectRatio(windowDims.x / windowDims.y);
    m_voxelCamera.setAspectRatio(windowDims.x / windowDims.y);

    // Set up shared params
    const vg::GLProgramManager* glProgramManager = soaState->glProgramManager.get();
    _gameRenderParams.glProgramManager = glProgramManager;

    // Init render stages
    _skyboxRenderStage = new SkyboxRenderStage(glProgramManager->getProgram("Texture"), &m_spaceCamera);
    //_physicsBlockRenderStage = new PhysicsBlockRenderStage(&_gameRenderParams, _meshManager->getPhysicsBlockMeshes(), glProgramManager->getProgram("PhysicsBlock"));
    _opaqueVoxelRenderStage = new OpaqueVoxelRenderStage(&_gameRenderParams);
    _cutoutVoxelRenderStage = new CutoutVoxelRenderStage(&_gameRenderParams);
    _chunkGridRenderStage = new ChunkGridRenderStage(&_gameRenderParams);
    _transparentVoxelRenderStage = new TransparentVoxelRenderStage(&_gameRenderParams);
    _liquidVoxelRenderStage = new LiquidVoxelRenderStage(&_gameRenderParams);
    _devHudRenderStage = new DevHudRenderStage("Fonts\\chintzy.ttf", DEVHUD_FONT_SIZE, app, windowDims);
    _pdaRenderStage = new PdaRenderStage(pda);
    _pauseMenuRenderStage = new PauseMenuRenderStage(pauseMenu);
    _nightVisionRenderStage = new NightVisionRenderStage(glProgramManager->getProgram("NightVision"), &_quad);
    _hdrRenderStage = new HdrRenderStage(glProgramManager, &_quad, &m_voxelCamera);
    m_spaceSystemRenderStage = new SpaceSystemRenderStage(ui32v2(windowDims),
                                                          spaceSystem,
                                                          gameSystem,
                                                          nullptr, &m_spaceCamera,
                                                          &m_farTerrainCamera,
                                                          GameManager::textureCache->addTexture("Textures/selector.png").id);

    loadNightVision();
    // No post-process effects to begin with
    _nightVisionRenderStage->setIsVisible(false);
    _chunkGridRenderStage->setIsVisible(false);
}

void GamePlayRenderPipeline::setRenderState(const MTRenderState* renderState) {
    m_renderState = renderState;
}

GamePlayRenderPipeline::~GamePlayRenderPipeline() {
    destroy();
}

void GamePlayRenderPipeline::render() {
    const GameSystem* gameSystem = m_soaState->gameSystem.get();
    const SpaceSystem* spaceSystem = m_soaState->spaceSystem.get();

    updateCameras();
    // Set up the gameRenderParams
    _gameRenderParams.calculateParams(m_spaceCamera.getPosition(), &m_voxelCamera,
                                      _meshManager, false);
    // Bind the FBO
    _hdrFrameBuffer->use();
  
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // worldCamera passes
    _skyboxRenderStage->draw();
    glPolygonMode(GL_FRONT_AND_BACK, m_drawMode);
    m_spaceSystemRenderStage->setRenderState(m_renderState);
    m_spaceSystemRenderStage->draw();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // Clear the depth buffer so we can draw the voxel passes
    glClear(GL_DEPTH_BUFFER_BIT);

    if (m_voxelsActive) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ZERO);
        glPolygonMode(GL_FRONT_AND_BACK, m_drawMode);
        _opaqueVoxelRenderStage->draw();
       // _physicsBlockRenderStage->draw();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        _cutoutVoxelRenderStage->draw();

        auto& voxcmp = gameSystem->voxelPosition.getFromEntity(m_soaState->playerEntity).parentVoxelComponent;
        _chunkGridRenderStage->setChunks(spaceSystem->m_sphericalVoxelCT.get(voxcmp).chunkMemoryManager);
        _chunkGridRenderStage->draw();
        _liquidVoxelRenderStage->draw();
        _transparentVoxelRenderStage->draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Post processing
    _swapChain->reset(0, _hdrFrameBuffer, graphicsOptions.msaa > 0, false);

    // TODO: More Effects
    if (_nightVisionRenderStage->isVisible()) {
        _nightVisionRenderStage->draw();
        _swapChain->swap();
        _swapChain->use(0, false);
    }

    // Draw to backbuffer for the last effect
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(_hdrFrameBuffer->getTextureTarget(), _hdrFrameBuffer->getTextureDepthID());
    _hdrRenderStage->draw();

    // UI
    _devHudRenderStage->draw();
    _pdaRenderStage->draw();
    _pauseMenuRenderStage->draw();

    // Check for errors, just in case
    checkGlError("GamePlayRenderPipeline::render()");
}

void GamePlayRenderPipeline::destroy() {
    // Make sure everything is freed here!
    delete _skyboxRenderStage;
    _skyboxRenderStage = nullptr;


    delete _opaqueVoxelRenderStage;
    _opaqueVoxelRenderStage = nullptr;

    delete _cutoutVoxelRenderStage;
    _cutoutVoxelRenderStage = nullptr;

    delete _transparentVoxelRenderStage;
    _transparentVoxelRenderStage = nullptr;

    delete _liquidVoxelRenderStage;
    _liquidVoxelRenderStage = nullptr;

    delete _devHudRenderStage;
    _devHudRenderStage = nullptr;

    delete _pdaRenderStage;
    _pdaRenderStage = nullptr;

    delete _pauseMenuRenderStage;
    _pauseMenuRenderStage = nullptr;

    delete _nightVisionRenderStage;
    _nightVisionRenderStage = nullptr;

    delete _hdrRenderStage;
    _hdrRenderStage = nullptr;

    _hdrFrameBuffer->dispose();
    delete _hdrFrameBuffer;
    _hdrFrameBuffer = nullptr;

    _swapChain->dispose();
    delete _swapChain;
    _swapChain = nullptr;

    _quad.dispose();
}

void GamePlayRenderPipeline::cycleDevHud(int offset /* = 1 */) {
    _devHudRenderStage->cycleMode(offset);
}

void GamePlayRenderPipeline::toggleNightVision() {
    if (!_nightVisionRenderStage->isVisible()) {
        _nightVisionRenderStage->setIsVisible(true);
        _nvIndex = 0;
        _nightVisionRenderStage->setParams(&_nvParams[_nvIndex]);
    } else {
        _nvIndex++;
        if (_nvIndex >= _nvParams.size()) {
            _nightVisionRenderStage->setIsVisible(false);
        } else {
            _nightVisionRenderStage->setParams(&_nvParams[_nvIndex]);
        }
    }
}
void GamePlayRenderPipeline::loadNightVision() {
    _nightVisionRenderStage->setIsVisible(false);

    _nvIndex = 0;
    _nvParams.clear();

    vio::IOManager iom;
    const cString nvData = iom.readFileToString("Data/NightVision.yml");
    if (nvData) {
        Array<NightVisionRenderParams> arr;
        keg::YAMLReader reader;
        reader.init(nvData);
        keg::Node node = reader.getFirst();
        Keg::Value v = Keg::Value::array(0, Keg::Value::custom("NightVisionRenderParams", 0, false));
        Keg::evalData((ui8*)&arr, &v, node, reader, Keg::getGlobalEnvironment());
        for (i32 i = 0; i < arr.getLength(); i++) {
            _nvParams.push_back(arr[i]);
        }
        reader.dispose();
        delete[] nvData;
    }
    if (_nvParams.size() < 1) {
        _nvParams.push_back(NightVisionRenderParams::createDefault());
    }
}

void GamePlayRenderPipeline::toggleChunkGrid() {
    _chunkGridRenderStage->setIsVisible(!_chunkGridRenderStage->isVisible());
}

void GamePlayRenderPipeline::cycleDrawMode() {
    switch (m_drawMode) {
    case GL_FILL:
        m_drawMode = GL_LINE;
        break;
    case GL_LINE:
        m_drawMode = GL_POINT;
        break;
    case GL_POINT:
    default:
        m_drawMode = GL_FILL;
        break;
    }
}

void GamePlayRenderPipeline::updateCameras() {
    const GameSystem* gs = m_soaState->gameSystem.get();
    const SpaceSystem* ss = m_soaState->spaceSystem.get();

    float sNearClip = 20.0f; ///< temporary until dynamic clipping plane works

    // Get the physics component
    auto& phycmp = gs->physics.getFromEntity(m_soaState->playerEntity);
    if (phycmp.voxelPositionComponent) {
        auto& vpcmp = gs->voxelPosition.get(phycmp.voxelPositionComponent);
        m_voxelCamera.setClippingPlane(0.2f, 10000.0f);
        m_voxelCamera.setPosition(vpcmp.gridPosition.pos);
        m_voxelCamera.setOrientation(vpcmp.orientation);
        m_voxelCamera.update();

        // Far terrain camera is exactly like voxel camera except for clip plane
        m_farTerrainCamera = m_voxelCamera;
        m_farTerrainCamera.setClippingPlane(1.0f, 100000.0f);
        m_farTerrainCamera.update();

        m_voxelsActive = true;
        sNearClip = 0.05;
    } else {
        m_voxelsActive = false;
    }
    // Player is relative to a planet, so add position if needed
    auto& spcmp = gs->spacePosition.get(phycmp.spacePositionComponent);
    if (spcmp.parentGravityId) {
        auto& parentSgCmp = ss->m_sphericalGravityCT.get(spcmp.parentGravityId);
        auto& parentNpCmp = ss->m_namePositionCT.get(parentSgCmp.namePositionComponent);
        // TODO(Ben): Could get better precision by not using + parentNpCmp.position here?
        m_spaceCamera.setPosition(m_renderState->spaceCameraPos + parentNpCmp.position);
    } else {
        m_spaceCamera.setPosition(m_renderState->spaceCameraPos);
    }
    //printVec("POSITION: ", spcmp.position);
    m_spaceCamera.setClippingPlane(sNearClip, 999999999.0f);
   
    m_spaceCamera.setOrientation(m_renderState->spaceCameraOrientation);
    m_spaceCamera.update();
}