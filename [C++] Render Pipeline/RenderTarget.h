#pragma once

#include "Base.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"

BEGIN(Engine)

// 렌더 타겟 래퍼
// - RTV(Render target view): gpu draw 목적
// - SRV(Shadow resource view): sampling 목적
// - Texture2D: 실제 GPU 리소스
class CRenderTarget final : public CBase
{
private:
	CRenderTarget(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
public:
	virtual ~CRenderTarget() = default;

public:
	ComPtr<ID3D11RenderTargetView> Get_RTV() const {
		return m_pRTV;
	}
    ComPtr<ID3D11ShaderResourceView> Get_SRV() const {
        return m_pSRV;
    }
    const ComPtr<ID3D11Texture2D>& Get_Texture2D() const {
        return m_pTexture2D;
    }

public:	
	HRESULT Initialize(_uint iSizeX, _uint iSizeY, DXGI_FORMAT ePixelFormat, const _float4& vClearColor);
	void Clear();
	HRESULT Bind_ShaderResource(shared_ptr<CShader> pShader, const _char* pConstantName);
	HRESULT Copy(ComPtr<ID3D11Texture2D> pOut);

    _uint Get_Width() { return m_iWidth; }
    _uint Get_Height() { return m_iHeight; }

#ifdef _DEBUG
public:
	HRESULT Ready_Debug(_float fX, _float fY, _float fSizeX, _float fSizeY);
	HRESULT Render(shared_ptr<CVIBuffer_Rect> pVIBuffer, shared_ptr<CShader> pShader);

#endif

private:
	ComPtr<ID3D11Device>		m_pDevice = { nullptr };
	ComPtr<ID3D11DeviceContext>	m_pContext = { nullptr };

	ComPtr<ID3D11Texture2D>			m_pTexture2D = { nullptr };
	ComPtr<ID3D11RenderTargetView>	 m_pRTV = { nullptr };
	ComPtr<ID3D11ShaderResourceView> m_pSRV = { nullptr };

	_float4						m_vClearColor = {};
    _uint m_iWidth, m_iHeight;

#ifdef _DEBUG
private:
	_float4x4					m_WorldMatrix{}; // 디버그용 월드 행렬

#endif

public:
	static shared_ptr<CRenderTarget> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, _uint iSizeX, _uint iSizeY, DXGI_FORMAT ePixelFormat, const _float4& vClearColor);
};

END
