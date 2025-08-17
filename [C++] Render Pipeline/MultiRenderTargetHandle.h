#pragma once

#include "RenderTarget.h"

BEGIN(Engine)

// MRT 바인딩을 스코프 단위로 관리하는 RAII 핸들
// - 생성자(Begin) 시점에 RTV, DSV 상태 백업
// - 소멸자(End) 에서는 내부 참조만 정리
class CMultiRenderTargetHandle
{
private:
    CMultiRenderTargetHandle(const _wstring& strTargetTag, ComPtr<ID3D11DepthStencilView> pDSV, _bool isClear);

public:
    virtual ~CMultiRenderTargetHandle();
    static shared_ptr<CMultiRenderTargetHandle> Create(const _wstring& strTargetTag, ComPtr<ID3D11DepthStencilView> pDSV = nullptr, bool isClear = true);

public:

private:
    void Begin(const _wstring& strTargetTag, ComPtr<ID3D11DepthStencilView> pDSV, _bool isClear);
    void End();

private:
    class CGameInstance* m_pGameInstance;

    ComPtr<ID3D11Device>		m_pDevice = { nullptr };
    ComPtr<ID3D11DeviceContext>	m_pContext = { nullptr };
    vector<ComPtr<ID3D11RenderTargetView>> m_pBackBuffers = { nullptr };
    ComPtr<ID3D11DepthStencilView> m_pDSV = { nullptr };
    ID3D11ShaderResourceView* m_nullSRVs[16]{ nullptr };

    _uint m_iSlotCount = {};

    static bool g_isBegun;
};

END
