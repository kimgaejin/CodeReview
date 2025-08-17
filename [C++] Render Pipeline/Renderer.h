#pragma once

#include "Engine_Defines.h"
#include "GameObject.h"

#include "RenderData.h"
#include "PostProcessor.h"
#include "Engine_Enum.h"

BEGIN(Engine)

class CModel;
class CInstancedModel;

// 오브젝트 들의 렌더 요청을 최종적으로 실행하는 워크플로우
// - 렌더 큐(RenderGroup)으로 들어온 요청을 매 프레임 수행
// - 렌더 타겟 패스별로 MRT 바인딩, 드로우, 포스트 프로세싱
// - Output-Merger 상태는 MRT 핸들로 스코프 단위에서 바인딩하고 복구
class CRenderer final
{
private:
	// 일부 Static Object 들이 그림자 요청을 매번 하지 않고 렌더러에 위임
	// - 주로 언덕, 풀 등 지형 데이터
	// - 수명 자체는 각 오브젝트가 소유하며, 렌더러는 오브젝트 missing 시 해당 오브젝트 렌더링을 포기함
    struct BookedShadow
    {
        MODEL_TYPE type;
        weak_ptr<CGameObject> gameObject;
        weak_ptr<CShader> shader;
        weak_ptr<CModel> model;
        weak_ptr<CInstancedModel> instanceModel; // instanceModel인 경우, 그냥 model 의 처리가 아닌 instanceModel의 처리로 됨

        BookedShadow(MODEL_TYPE _type, shared_ptr<CGameObject> _gameObject, shared_ptr<CShader> _shader, shared_ptr<CModel> _model, shared_ptr <CInstancedModel> _instanceModel)
        {
            type = _type;
            gameObject = _gameObject;
            shader = _shader;
            model = _model;
            instanceModel = _instanceModel;
        }
    };

public:
	enum RENDERGROUP { RENDER_PRIORITY, RENDER_SHADOW, RENDER_NONBLEND_CHARACTER, RENDER_NONBLEND, RENDER_NONBLEND_OUTLINE, RENDER_MASK, RENDER_NONLIGHT, RENDER_EFFECT, RENDER_PRIORITY_2D_EFFECT, RENDER_BLEND, RENDER_NAME, RENDER_SUBCAMERA_FRONT, RENDER_SUBCAMERA, RENDER_UI, RENDER_SCREENSHOT, RENDER_DEBUG, RENDER_END };

private:
	CRenderer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
public:
	virtual ~CRenderer();
	static shared_ptr<CRenderer> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);

public:
	HRESULT Initialize();
	HRESULT Add_RenderObject(RENDERGROUP eRenderGroup, shared_ptr<CGameObject> pRenderObject);
	HRESULT Add_RenderObject_Front(RENDERGROUP eRenderGroup, shared_ptr<CGameObject> pRenderObject);
	HRESULT Draw();
    void Register_Shader(RENDERGROUP group, weak_ptr<CShader> _shader);
    
    void Book_ShadowGroup(MODEL_TYPE type, shared_ptr<CGameObject> _gameObject, shared_ptr<CShader> _shader, shared_ptr<CModel> _model, shared_ptr<CInstancedModel> _InstancedModel);

public:
#ifdef _DEBUG
    HRESULT Add_DebugComponent(shared_ptr<CComponent> pDebugComponent);
#endif

public:
    void Set_Blur();
    void Set_Bloom(float threshold, float strength);
    void Set_FullScreenBlur(bool type) { m_bFullscreenBlur_enable = type; }
    void Set_SSAOBlur(bool enable, float value) { m_testSSAOBlur = value; }
    void SetFog(_float fFogMin = 50.0f, _float fFogMax = 150.0f, _float fFogDensity = 0.3f, DirectX::XMFLOAT4 fogColor = { 0.33f, 0.69f, 0.98f, 1.0f }) { m_fFogMin = fFogMin; m_fFogMax = fFogMax; m_fFogDensity = fFogDensity; m_FogColor = fogColor; }
    void Set_Dimmed(_float fColorValue) { m_fDimmedColor = fColorValue;  }
    void Set_VignetteBlur(float value) { m_fVignetteValue = value; }
    void Call_Screenshot();
    void Set_Distortion(DistortionData data) { m_tDistortionData = data; m_tDistortionData.fStartTime = m_tDistortionData.fTime; }

private:
	class CGameInstance*			m_pGameInstance = { nullptr };
	list<shared_ptr<CGameObject>>	m_RenderObjects[RENDER_END];

private:
    shared_ptr<RenderData> m_renderData;

private:
	HRESULT Render_Priority();	
	HRESULT Render_NonBlend();
	HRESULT Render_Mask();
	HRESULT Render_Lights();
	HRESULT Render_Deferred();
	HRESULT Render_BackBuffer();
	HRESULT Render_NonLight();

    HRESULT Render_Effect_WeightBlend();
    HRESULT Render_Effect_WeightBlend_Resolve();
    HRESULT Render_Effect_WeightBlend_For_Priority2DEffect();
    HRESULT Render_Effect_WeightBlend_Resolve_For_Priority2DEffect();

	HRESULT Render_Blend();
    HRESULT Render_Name();
	HRESULT Render_UI();
	HRESULT Ready_Shadow();
    HRESULT Composite_Shadow();

	HRESULT Ready_Depth_Stencil_View(_uint width, _uint height, ComPtr<ID3D11DepthStencilView>& rDSV);
    bool Is_WorldBlur(float deltaTime);

private:
    _float2 m_vCurFrameViewport;
    _float m_testSSAOBlur = 2.0f;
    vector<pair<_uint, _uint>> m_vShadowResolutions;
    vector<ComPtr<ID3D11DepthStencilView>> m_pShadowDSVs;
    vector<weak_ptr<CShader>> m_pRegisteredShaders[RENDER_END];

    _float m_fFogMin{ 50.0f }, m_fFogMax{ 150.0f }, m_fFogDensity{0.3f};
    DirectX::XMFLOAT4 m_FogColor = { 0.33f, 0.69f, 0.98f, 1.0f };
    _float m_fDimmedColor = {};

    list<shared_ptr<struct BookedShadow>> m_bookedShadowGroup;
    bool m_isTrashComputer = false;
    bool m_bCalledScreenshot = false;
    DistortionData m_tDistortionData = {};

#ifdef _DEBUG
private:
	HRESULT Render_Debug();
#endif
	
private:
    unique_ptr<CPostProcessor> m_pPostProcessor;
    float m_bFullscreenBlur_enable = false;
    float m_fFullScreenBlur_Timer = 0.0f;
    float m_fVignetteValue = 0.0f;
};

END
