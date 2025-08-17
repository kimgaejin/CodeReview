#include "RenderTarget.h"

#include "VIBuffer_Rect.h"
#include "Shader.h"

CRenderTarget::CRenderTarget(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: m_pDevice { pDevice }
	, m_pContext { pContext }
{
}

HRESULT CRenderTarget::Initialize(_uint iSizeX, _uint iSizeY, DXGI_FORMAT ePixelFormat, const _float4& vClearColor)
{
	D3D11_TEXTURE2D_DESC			TextureDesc{};

	TextureDesc.Width = iSizeX;
	TextureDesc.Height = iSizeY;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = ePixelFormat;

	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.SampleDesc.Count = 1;

	TextureDesc.Usage = D3D11_USAGE_DEFAULT;	
	TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, nullptr, m_pTexture2D.GetAddressOf())))
		return E_FAIL;

	if (FAILED(m_pDevice->CreateRenderTargetView(m_pTexture2D.Get(), nullptr, m_pRTV.GetAddressOf())))
		return E_FAIL;

	if (FAILED(m_pDevice->CreateShaderResourceView(m_pTexture2D.Get(), nullptr, m_pSRV.GetAddressOf())))
		return E_FAIL;

	m_vClearColor = vClearColor;

    m_iWidth = iSizeX;
    m_iHeight = iSizeY;

	return S_OK;
}

void CRenderTarget::Clear()
{
	m_pContext->ClearRenderTargetView(m_pRTV.Get(), (_float*)&m_vClearColor);
}

HRESULT CRenderTarget::Bind_ShaderResource(shared_ptr<CShader> pShader, const _char* pConstantName)
{
	return pShader->Bind_SRV(pConstantName, m_pSRV);
}

HRESULT CRenderTarget::Copy(ComPtr<ID3D11Texture2D> pOut)
{
	if (nullptr == m_pTexture2D)
		return E_FAIL;

	m_pContext->CopyResource(pOut.Get(), m_pTexture2D.Get());

	return S_OK;
}

#ifdef _DEBUG

HRESULT CRenderTarget::Ready_Debug(_float fX, _float fY, _float fSizeX, _float fSizeY)
{
	D3D11_VIEWPORT              ViewportDesc{};

	_uint       iNumViewports = { 1 };

	m_pContext->RSGetViewports(&iNumViewports, &ViewportDesc);	

	XMStoreFloat4x4(&m_WorldMatrix, 
		XMMatrixScaling(fSizeX, fSizeY, 1.f) * XMMatrixTranslation(fX - ViewportDesc.Width * 0.5f, -fY + ViewportDesc.Height * 0.5f, 0.f));

	return S_OK;
}

HRESULT CRenderTarget::Render(shared_ptr<CVIBuffer_Rect> pVIBuffer, shared_ptr<CShader> pShader)
{
	if (FAILED(pShader->Bind_Matrix("g_WorldMatrix", &m_WorldMatrix)))
		return E_FAIL;

	if (FAILED(pShader->Bind_SRV("g_RenderTargetTexture", m_pSRV)))
		return E_FAIL;

	pShader->Begin(0);

	pVIBuffer->Input_Assembler();	

	return pVIBuffer->Render();
}

#endif

shared_ptr<CRenderTarget> CRenderTarget::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, _uint iSizeX, _uint iSizeY, DXGI_FORMAT ePixelFormat, const _float4& vClearColor)
{
	auto pInstance = shared_ptr<CRenderTarget>(new CRenderTarget(pDevice, pContext));

	if (FAILED(pInstance->Initialize(iSizeX, iSizeY, ePixelFormat, vClearColor)))
	{
        assert(0); // Failed to Created : CRenderTarget
        return shared_ptr<CRenderTarget>();
	}

	return pInstance;
}
