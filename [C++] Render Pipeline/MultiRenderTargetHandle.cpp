#include "MultiRenderTargetHandle.h"

#include "GameInstance.h"

bool CMultiRenderTargetHandle::g_isBegun = false;

CMultiRenderTargetHandle::CMultiRenderTargetHandle(const _wstring& strTargetTag, ComPtr<ID3D11DepthStencilView> pDSV, _bool isClear)
{
    m_pGameInstance = CGameInstance::GetInstance();
    m_pDevice = m_pGameInstance->Get_Device();
    m_pContext = m_pGameInstance->Get_Context();

    Begin(strTargetTag, pDSV, isClear);
}

CMultiRenderTargetHandle::~CMultiRenderTargetHandle()
{
    End();
}

shared_ptr<CMultiRenderTargetHandle> CMultiRenderTargetHandle::Create(const _wstring& strTargetTag, ComPtr<ID3D11DepthStencilView> pDSV, bool isClear)
{
    return shared_ptr<CMultiRenderTargetHandle>(new CMultiRenderTargetHandle(strTargetTag, pDSV, isClear));
}

void CMultiRenderTargetHandle::Begin(const _wstring& strTargetTag, ComPtr<ID3D11DepthStencilView> pDSV, _bool isClear)
{
    // 렌더링 과정에서 중복 실행은 허용하지 않음
    if (g_isBegun)
    {
        assert(0);
        return;
    }

    vector<shared_ptr<CRenderTarget>>* pMRTs = m_pGameInstance->Find_MRT(strTargetTag);
    if (nullptr == pMRTs)
    {
        assert(0); // MRT is null.
        return;
    }
    g_isBegun = true;

    // MRT 에서 사용할 
    ID3D11RenderTargetView* tmpRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ComPtr<ID3D11DepthStencilView> tmpDSV;
    m_pContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, tmpRTVs, tmpDSV.GetAddressOf());

    m_iSlotCount = 0;
    m_pBackBuffers.resize(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
    for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
        m_pBackBuffers[i] = tmpRTVs[i];
        if (tmpRTVs[i]) m_iSlotCount = i + 1;
    }
    m_pDSV = tmpDSV;

    ID3D11RenderTargetView* newRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

    _uint n = 0;
    for (auto& rt : *pMRTs) {
        if (isClear) rt->Clear();
        newRTVs[n++] = rt->Get_RTV().Get();
    }

    ID3D11DepthStencilView* newDSV = pDSV ? pDSV.Get() : m_pDSV.Get();
    m_pContext->OMSetRenderTargets(n, newRTVs, newDSV);
}

void CMultiRenderTargetHandle::End()
{
    if (!g_isBegun)
    {
        assert(0); // Begin() 짝 없이 호출된, 잘못된 상황
        return;
    }

    // gpu 상태 돌려주기
    ID3D11RenderTargetView* rtvPtrs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    for (UINT i = 0; i < m_iSlotCount; ++i)
        rtvPtrs[i] = m_pBackBuffers[i].Get();

    m_pContext->OMSetRenderTargets(m_iSlotCount, rtvPtrs, m_pDSV.Get());

    for (auto& buffer : m_pBackBuffers) buffer.Reset();
    m_pBackBuffers.clear();
    m_pDSV.Reset();

    g_isBegun = false;
}
