#include "Renderer.h"
#include "GameObject.h"
#include "Shader.h"
#include "Model.h"
#include "UIObject.h"
#include "GameInstance.h"
#include "Engine_Enum.h"
#include "InstancedModel.h"

#include "RenderTargetHandle.h"
#include "MultiRenderTargetHandle.h"

CRenderer::CRenderer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
    
}

CRenderer::~CRenderer()
{
    for (size_t i = 0; i < RENDER_END; i++)
    {
        m_RenderObjects[i].clear();
    }

#ifdef _DEBUG
    m_renderData->DebugComponent.clear();
#endif // _DEBUG
}

HRESULT CRenderer::Initialize()
{
    // Render data Init
    m_renderData = make_shared<RenderData>();
    m_pGameInstance = CGameInstance::GetInstance();
    m_renderData->m_pGameInstance = CGameInstance::GetInstance();
    m_renderData->pContext = m_renderData->m_pGameInstance->Get_Context();
    m_renderData->pDevice = m_renderData->m_pGameInstance->Get_Device();

    D3D11_VIEWPORT              ViewportDesc{};
    _uint       iNumViewports = { 1 };
    m_renderData->pContext->RSGetViewports(&iNumViewports, &ViewportDesc);

    _uint width = static_cast<_uint>(ViewportDesc.Width);
    _uint height = static_cast<_uint>(ViewportDesc.Height);
    m_renderData->width = width;
    m_renderData->height = height;
    m_renderData->viewportSize = { ViewportDesc.Width , ViewportDesc.Height };

    // Component
    m_renderData->pVIBuffer = CVIBuffer_Rect::Create(m_renderData->pDevice, m_renderData->pContext);
    if (nullptr == m_renderData->pVIBuffer) assert(0);

    m_renderData->pShader = CShader::Create(m_renderData->pDevice, m_renderData->pContext, TEXT("../Bin/ShaderFiles/Shader_Deferred.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements);
    if (nullptr == m_renderData->pShader) assert(0);

    // Depth stencil view (DSV)
    if (FAILED(Ready_Depth_Stencil_View(width, height, m_renderData->DSV))) assert(0);
    if (FAILED(Ready_Depth_Stencil_View(width * 2, height * 2, m_renderData->UpscaleDSV))) assert(0);
    if (FAILED(Ready_Depth_Stencil_View(width / 2, height / 2, m_renderData->DownscaleDSV)))  assert(0);
    if (FAILED(Ready_Depth_Stencil_View(width / 4, height / 4, m_renderData->DoubleDownscaleDSV))) assert(0);
    if (FAILED(Ready_Depth_Stencil_View(width / 8, height / 8, m_renderData->QuaterDownscaleDSV))) assert(0);

    // Render target (RT)
    _float4 COLOR_WHITE = _float4(1.0f, 1.0f, 1.0f, 1.0f);
    _float4 COLOR_ZERO = _float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Diffuse"), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, _float4(1.f, 1.f, 1.f, 0.f)))) assert(0);
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Diffuse_Copy"), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, _float4(1.f, 1.f, 1.f, 0.f)))) assert(0);
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Normal"), width, height, DXGI_FORMAT_R16G16B16A16_UNORM, _float4(1.f, 1.f, 1.f, 1.f)))) assert(0);
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Depth"), width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, _float4(0.f, 1.f, 0.f, 0.f))))  assert(0);
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_PickPos"), width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, COLOR_ZERO))) assert(0);
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_PickPos_Copy"), width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, COLOR_ZERO))) assert(0);
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Shade"), width, height, DXGI_FORMAT_R16G16B16A16_UNORM, COLOR_ZERO))) assert(0);
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Specular"), width, height, DXGI_FORMAT_R16G16B16A16_UNORM, COLOR_ZERO))) assert(0);
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_RimLight"), width, height, DXGI_FORMAT_R16G16B16A16_UNORM, COLOR_ZERO))) assert(0);
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Mask"), width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, COLOR_ZERO))) assert(0);

	// ... 기타 렌더 타겟 생성 중략

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObject"), TEXT("Target_Diffuse")))) assert(0);
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObject"), TEXT("Target_Normal")))) assert(0);
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObject"), TEXT("Target_Depth")))) assert(0);
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObject"), TEXT("Target_PickPos")))) assert(0);
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Light"), TEXT("Target_Shade")))) assert(0);
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Light"), TEXT("Target_Specular")))) assert(0);
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Light"), TEXT("Target_RimLight")))) assert(0);

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_BackBuffer"), TEXT("Target_BackBuffer")))) assert(0);
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_BackBuffer"), TEXT("Target_BackBuffer_Sub")))) assert(0);
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_BackBuffer"), TEXT("Target_BackBuffer_Mask")))) assert(0);

	// ... 기타 멀티 렌더 타겟 생성 중략

    // Matrix
    XMStoreFloat4x4(&m_renderData->WorldMatrix, XMMatrixScaling(ViewportDesc.Width, ViewportDesc.Height, 1.f));
    XMStoreFloat4x4(&m_renderData->ViewMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&m_renderData->ProjMatrix, XMMatrixOrthographicLH(ViewportDesc.Width, ViewportDesc.Height, 0.f, 1.f));

    m_pPostProcessor = CPostProcessor::Create();

    return S_OK;
}

// Object가 RENDERGROUP 단계에서 자신을 그려달라는 요청
HRESULT CRenderer::Add_RenderObject(RENDERGROUP eRenderGroup, shared_ptr<CGameObject> pRenderObject)
{
    if (nullptr == pRenderObject || 
        eRenderGroup >= RENDER_END)
        return E_FAIL;

    m_RenderObjects[eRenderGroup].push_back(pRenderObject);

    return S_OK;
}

HRESULT CRenderer::Add_RenderObject_Front(RENDERGROUP eRenderGroup, shared_ptr<CGameObject> pRenderObject)
{
    if (nullptr == pRenderObject ||
        eRenderGroup >= RENDER_END)
        return E_FAIL;

    m_RenderObjects[eRenderGroup].push_front(pRenderObject);

    return S_OK;
}

// 현재 Active 중인 카메라 뷰를 기준으로 매 프레임 그려지는 메인 루프
HRESULT CRenderer::Draw()
{
    float deltaTime = m_pGameInstance->Get_TimeDelta();
    m_bCalledScreenshot = false; // 스크린샷이 한 프레임에 두 번 이상 불리는 것 방지용
    
	// 최적화를 위한 모드
    if (m_pGameInstance->Key_Down(DIK_F8))
    {
        m_isTrashComputer = !m_isTrashComputer;
    }

    // 이번 프레임의 Viewport size는 이곳에서 한번만 확인
    D3D11_VIEWPORT ViewportDesc{};
    _uint iNumViewports = { 1 };
    m_renderData->pContext->RSGetViewports(&iNumViewports, &ViewportDesc);
    m_vCurFrameViewport = { ViewportDesc.Width, ViewportDesc.Height };
    
    auto shader = m_renderData->pShader; // 주로 screen effect 위주 쉐이더
    auto buffer = m_renderData->pVIBuffer; // screen 용 Rect Buffer

	// skybox
    if (FAILED(Render_Priority())) return E_FAIL;

	// 그림자 관련 
    if (FAILED(Ready_Shadow())) return E_FAIL;

    if (FAILED(Render_NonBlend())) return E_FAIL;
    
    if (FAILED(Render_Blend())) return E_FAIL;

    if (FAILED(Render_Lights())) return E_FAIL;

    if (FAILED(Render_Deferred())) return E_FAIL;

    if (!m_isTrashComputer)
        m_pPostProcessor->SSAO(m_renderData);

    Composite_Shadow();

    m_pPostProcessor->Fog(m_renderData, m_fFogMin, m_fFogMax, m_fFogDensity, m_FogColor);

    if (0.0f < m_fVignetteValue)
        m_pPostProcessor->VignetteBlur(m_renderData, 300.0f, 200.0f, m_fVignetteValue);
    m_fVignetteValue -= m_pGameInstance->Get_TimeDelta();

    // WeightBlend 들어가기 전, Bloom 먼저
    if (!m_isTrashComputer)
        m_pPostProcessor->Bloom(m_renderData);

    if (FAILED(Render_Effect_WeightBlend())) return E_FAIL;
    if (FAILED(Render_Effect_WeightBlend_Resolve())) return E_FAIL;
    if (!m_isTrashComputer)
        m_pPostProcessor->Bloom_WeightBlend(m_renderData);

    bool isWorldBlur = Is_WorldBlur(deltaTime);
    if (isWorldBlur && !m_isTrashComputer)
    {
        m_pPostProcessor->LerpBlur_RenderTarget(m_renderData, TEXT("Target_BackBuffer"), TEXT("Target_BackBuffer"), m_fFullScreenBlur_Timer);
    }
    
    if (FAILED(Render_NonLight())) return E_FAIL;
    if (m_tDistortionData.fTime > 0.0f)
    {
        m_pPostProcessor->ScreenDistortion(m_renderData, m_tDistortionData);
        m_tDistortionData.fTime -= m_pGameInstance->Get_TimeDelta();
    }

    if (FAILED(Render_Name())) return E_FAIL;
    if (FAILED(Render_UI())) return E_FAIL;

#ifdef _DEBUG
    if (FAILED(Render_Debug()))
        return E_FAIL;
#endif

    if (FAILED(Render_BackBuffer())) return E_FAIL;

    return S_OK;
}

// 예시 1. 지형/지형마스크/캐릭터 를 그리는 렌더 타겟 패스
// - 렌더 큐에 모아서 순서별로 실행시켜주는 역할
HRESULT CRenderer::Render_NonBlend()
{
    auto shader = m_renderData->pShader;
    auto buffer = m_renderData->pVIBuffer;

    HRESULT hr;

	// 지형
    if (auto handle = CMultiRenderTargetHandle::Create(TEXT("MRT_GameObject")))
    {
        for (auto& pRenderObject : m_RenderObjects[RENDER_NONBLEND])
        {
            if (nullptr != pRenderObject)
            {
				// 실제로 어떤 쉐이더 패스를 사용하고, 어떤 Input assembler를 갖는지는 오브젝트가 판단
                hr = pRenderObject->Render();
            }
        }
    }

	// 스킬 바닥 등 지형 Mask
    if (FAILED(Render_Mask())) return E_FAIL;

	// 캐릭터를 제외하고 딤드 효과
    if (0 < m_fDimmedColor)
    {
        m_pGameInstance->Copy_RenderTargetToRT(TEXT("Target_Diffuse"), TEXT("Target_Diffuse_Copy"));
        if (auto handle = CRenderTargetHandle::Create(TEXT("Target_Diffuse"), nullptr, false))
        {
            m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Diffuse_Copy"), shader, "g_DiffuseTexture");
            shader->Bind_RawValue("g_fDimmedColor", &m_fDimmedColor, sizeof(_float));
            shader->Begin("Dimmed");
            buffer->Input_Assembler();
            buffer->Render();
        }
    }

	// 캐릭터
    if (auto handle = CMultiRenderTargetHandle::Create(TEXT("MRT_GameObject"), nullptr, false))
    {
        for (auto& pRenderObject : m_RenderObjects[RENDER_NONBLEND_CHARACTER])
        {
            if (nullptr != pRenderObject)
            {
                hr = pRenderObject->Render();
            }
        }
    }

    // 캐릭터 아웃라인: 클릭 등 상호작용으로 이루어지기에 매 프레임 1~3개 가량
    if (auto handle = CMultiRenderTargetHandle::Create(TEXT("MRT_GameObject"), nullptr, false))
    {
        for (auto& pRenderObject : m_RenderObjects[RENDER_NONBLEND_OUTLINE])
        {
            if (nullptr != pRenderObject)
            {
                hr = pRenderObject->Render_Outline();
                hr = pRenderObject->Render();
            }
        }
    }

    m_RenderObjects[RENDER_NONBLEND].clear();
    m_RenderObjects[RENDER_NONBLEND_CHARACTER].clear();
    m_RenderObjects[RENDER_NONBLEND_OUTLINE].clear();

    return S_OK;
}

// 예시 2. G-Buffer에 기록된 Diffuse, Normal 등 오브젝트 정보와 Shadow map을 통한 디퍼드 렌더링 과정
HRESULT CRenderer::Render_Deferred()
{
    auto shader = m_renderData->pShader;
    auto buffer = m_renderData->pVIBuffer;

    if (auto handle = CMultiRenderTargetHandle::Create(TEXT("MRT_BackBuffer"), nullptr, false))
    {
        m_renderData->Bind_DefaultMatrix();

        // RawValue 로 보내는 행렬은 전치
        vector<_matrix> lightView = m_pGameInstance->Get_CascadeLightViews();
        vector<_matrix> transLightView;
        for (auto& lv : lightView)
        {
            transLightView.push_back(XMMatrixTranspose(lv));
        }
        vector<_matrix> lightProj = m_pGameInstance->Get_CascadeLightProjs();
        vector<_matrix> transLightProj;
        for (auto& lp : lightProj)
        {
            transLightProj.push_back(XMMatrixTranspose(lp));
        }
        size_t cascadeSize = lightView.size();

        if (FAILED(shader->Bind_RawValue("g_LightViewMatrix", transLightView.data(), static_cast<_uint>(sizeof(_float4x4) * cascadeSize)))) return E_FAIL;
        if (FAILED(shader->Bind_RawValue("g_LightProjMatrix", transLightProj.data(), static_cast<_uint>(sizeof(_float4x4) * cascadeSize)))) return E_FAIL;

        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Diffuse"), shader, "g_DiffuseTexture"))) return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Shade"), shader, "g_ShadeTexture"))) return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Specular"), shader, "g_SpecularTexture"))) return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_RimLight"), shader, "g_RimTexture"))) return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), shader, "g_DepthTexture"))) return E_FAIL;

        auto cascadeSplit = m_pGameInstance->Get_CascadeSplit();
        shader->Bind_RawValue("g_cascadeSplit", cascadeSplit.data(), sizeof(_float4));

        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_CascadeLightMap0"), shader, "g_LightDepthTexture0"))) return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_CascadeLightMap1"), shader, "g_LightDepthTexture1"))) return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_CascadeLightMap2"), shader, "g_LightDepthTexture2"))) return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_CascadeLightMap3"), shader, "g_LightDepthTexture3"))) return E_FAIL;

        shader->Begin(3);
        buffer->Input_Assembler();
        buffer->Render();   
    }

    m_renderData->Unbind_AllShaderResourceSlots();
    m_pGameInstance->Copy_RenderTargetToRT(TEXT("Target_BackBuffer_Sub"), TEXT("Target_Shadow"));

    return S_OK;
}

// ... 다른 함수 세부 구현 생략
