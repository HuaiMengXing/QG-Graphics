#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;



GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_IndexCount(),	
	m_CurrFrame(),
	m_CurrMode(ShowMode::WoodCrate),
	m_VSConstantBuffer(),
	m_PSConstantBuffer()
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	if (!InitEffect())
		return false;

	if (!InitResource())
		return false;

	// 初始化鼠标，键盘不需要
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);

	return true;
}

void GameApp::OnResize()
{
	assert(m_pd2dFactory);
	assert(m_pdwriteFactory);
	// 释放D2D的相关资源
	m_pColorBrush.Reset();
	m_pd2dRenderTarget.Reset();

	D3DApp::OnResize();

	// 为D2D创建DXGI表面渲染目标
	ComPtr<IDXGISurface> surface;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = m_pd2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, m_pd2dRenderTarget.GetAddressOf());
	surface.Reset();

	if (hr == E_NOINTERFACE)
	{
		OutputDebugStringW(L"\n警告：Direct2D与Direct3D互操作性功能受限，你将无法看到文本信息。现提供下述可选方法：\n"
			L"1. 对于Win7系统，需要更新至Win7 SP1，并安装KB2670838补丁以支持Direct2D显示。\n"
			L"2. 自行完成Direct3D 10.1与Direct2D的交互。详情参阅："
			L"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			L"3. 使用别的字体库，比如FreeType。\n\n");
	}
	else if (hr == S_OK)
	{
		// 创建固定颜色刷和文本格式
		HR(m_pd2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			m_pColorBrush.GetAddressOf()));
		HR(m_pdwriteFactory->CreateTextFormat(L"宋体", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20, L"zh-cn",
			m_pTextFormat.GetAddressOf()));
	}
	else
	{
		// 报告异常问题
		assert(m_pd2dRenderTarget);
	}
	
}

void GameApp::UpdateScene(float dt)
{

	Keyboard::State state = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(state);	

	// 键盘切换模式
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1) && m_CurrMode != ShowMode::WoodCrate)
	{
		// 播放木箱动画
		m_CurrMode = ShowMode::WoodCrate;
		m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout3D.Get());
		auto meshData = Geometry::CreateBox();
		ResetMesh(meshData);
		m_pd3dImmediateContext->VSSetShader(m_pVertexShader3D.Get(), nullptr, 0);
		m_pd3dImmediateContext->PSSetShader(m_pPixelShader3D.Get(), nullptr, 0);
		//m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate[0].GetAddressOf());
		
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2) && m_CurrMode != ShowMode::FireAnim)
	{
		m_CurrMode = ShowMode::FireAnim;
		m_CurrFrame = 0;
		m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout2D.Get());
		auto meshData = Geometry::Create2DShow();
		ResetMesh(meshData);
		m_pd3dImmediateContext->VSSetShader(m_pVertexShader2D.Get(), nullptr, 0);
		m_pd3dImmediateContext->PSSetShader(m_pPixelShader2D.Get(), nullptr, 0);
		m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pFireAnims[0].GetAddressOf());
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3) && m_CurrMode != ShowMode::WoodCrate1)
	{
		// 播放木箱动画
		m_CurrMode = ShowMode::WoodCrate1;
		m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout3D.Get());
		auto meshData = Geometry::CreateBox();
		ResetMesh(meshData);
		m_pd3dImmediateContext->VSSetShader(m_pVertexShader3D.Get(), nullptr, 0);
		m_pd3dImmediateContext->PSSetShader(m_pPixelShader3D.Get(), nullptr, 0);
		m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate1[0].GetAddressOf());
	}

	//键盘切换灯光类型
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::W))
	{
		if (m_PSConstantBuffer.numPointLight == 1)
			m_PSConstantBuffer.numPointLight = 0;
		else
			m_PSConstantBuffer.numPointLight = 1;

		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
		memcpy_s(mappedData.pData, sizeof(PSConstantBuffer), &m_PSConstantBuffer, sizeof(PSConstantBuffer));
		m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);//更新一下常量缓冲区
	}

	if (m_CurrMode == ShowMode::WoodCrate)
	{
		static float cubePhi = 0.0f, cubeTheta = 0.0f;
		// 获取鼠标状态
		Mouse::State mouseState = m_pMouse->GetState();
		Mouse::State lastMouseState = m_MouseTracker.GetLastState();
		// 获取键盘状态
		Keyboard::State keyState = m_pKeyboard->GetState();
		Keyboard::State lastKeyState = m_KeyboardTracker.GetLastState();

		if (keyState.IsKeyDown(Keyboard::Q))
		{
			// 更新鼠标按钮状态跟踪器，仅当鼠标按住的情况下才进行移动
			m_MouseTracker.Update(mouseState);
			m_KeyboardTracker.Update(keyState);
			if (mouseState.leftButton == true && m_MouseTracker.leftButton == m_MouseTracker.HELD)
			{
				cubeTheta -= (mouseState.x - lastMouseState.x) * 0.01f;
				cubePhi -= (mouseState.y - lastMouseState.y) * 0.01f;
			}
			if (keyState.IsKeyDown(Keyboard::E))
				cubePhi += dt * 2;
			if (keyState.IsKeyDown(Keyboard::S))
				cubePhi -= dt * 2;
			if (keyState.IsKeyDown(Keyboard::A))
				cubeTheta += dt * 2;
			if (keyState.IsKeyDown(Keyboard::D))
				cubeTheta -= dt * 2;
			XMMATRIX W = XMMatrixRotationY(cubeTheta) * XMMatrixRotationX(cubePhi);
			m_VSConstantBuffer.world = XMMatrixTranspose(W);
			m_VSConstantBuffer.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
		}
		else
		{
			static float phi = 0.0f, theta = 0.0f;
			phi += 0.0001f, theta += 0.00015f;
			XMMATRIX W = XMMatrixRotationX(phi) * XMMatrixRotationY(theta);
			m_VSConstantBuffer.world = XMMatrixTranspose(W);
			m_VSConstantBuffer.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
		}

		static float phi1 = 0.0f;
		phi1 += 0.001f;
		//位置偏移，所以先移动再旋转，然后移回来
		XMMATRIX texMat = XMMatrixTranslation(-0.5f, -0.5f, 0.0f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f);//这个不旋转
		m_VSConstantBuffer.texMat = XMMatrixTranspose(texMat); //纹理旋转


		// 更新常量缓冲区，让立方体转起来
		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[0].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
		memcpy_s(mappedData.pData, sizeof(VSConstantBuffer), &m_VSConstantBuffer, sizeof(VSConstantBuffer));
		m_pd3dImmediateContext->Unmap(m_pConstantBuffers[0].Get(), 0);
	}
	else if (m_CurrMode == ShowMode::WoodCrate1)
	{
	
		static float cubePhi = 0.0f, cubeTheta = 0.0f;
		// 获取鼠标状态
		Mouse::State mouseState = m_pMouse->GetState();
		Mouse::State lastMouseState = m_MouseTracker.GetLastState();
		// 获取键盘状态
		Keyboard::State keyState = m_pKeyboard->GetState();
		Keyboard::State lastKeyState = m_KeyboardTracker.GetLastState();

		if (keyState.IsKeyDown(Keyboard::Q))
		{
			// 更新鼠标按钮状态跟踪器，仅当鼠标按住的情况下才进行移动
			m_MouseTracker.Update(mouseState);
			m_KeyboardTracker.Update(keyState);
			if (mouseState.leftButton == true && m_MouseTracker.leftButton == m_MouseTracker.HELD)
			{
				cubeTheta -= (mouseState.x - lastMouseState.x) * 0.01f;
				cubePhi -= (mouseState.y - lastMouseState.y) * 0.01f;
			}
			if (keyState.IsKeyDown(Keyboard::E))
				cubePhi += dt * 2;
			if (keyState.IsKeyDown(Keyboard::S))
				cubePhi -= dt * 2;
			if (keyState.IsKeyDown(Keyboard::A))
				cubeTheta += dt * 2;
			if (keyState.IsKeyDown(Keyboard::D))
				cubeTheta -= dt * 2;
			XMMATRIX W = XMMatrixRotationY(cubeTheta) * XMMatrixRotationX(cubePhi);
			m_VSConstantBuffer.world = XMMatrixTranspose(W);
			m_VSConstantBuffer.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
		}
		else
		{
			static float phi = 0.0f, theta = 0.0f;
			phi += 0.0001f, theta += 0.00015f;
			XMMATRIX W = XMMatrixRotationX(phi) * XMMatrixRotationY(theta);
			m_VSConstantBuffer.world = XMMatrixTranspose(W);
			m_VSConstantBuffer.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
		}




		static float phi1 = 0.0f;
		phi1 += 0.001f;
		//位置偏移，所以先移动再旋转，然后移回来
		XMMATRIX texMat = XMMatrixTranslation(-0.5f, -0.5f, 0.0f) * XMMatrixRotationZ(phi1) * XMMatrixTranslation(0.5f, 0.5f, 0.0f);
		m_VSConstantBuffer.texMat = XMMatrixTranspose(texMat); //纹理旋转

		
		// 更新常量缓冲区，让立方体转起来
		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[0].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
		memcpy_s(mappedData.pData, sizeof(VSConstantBuffer), &m_VSConstantBuffer, sizeof(VSConstantBuffer));
		m_pd3dImmediateContext->Unmap(m_pConstantBuffers[0].Get(), 0);
	}
	else if (m_CurrMode == ShowMode::FireAnim)
	{
		// 用于限制在1秒60帧
		static float totDeltaTime = 0;

		totDeltaTime += dt;
		if (totDeltaTime > 1.0f / 60)
		{
			totDeltaTime -= 1.0f / 60;
			m_CurrFrame = (m_CurrFrame + 1) % 120;
			m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pFireAnims[m_CurrFrame].GetAddressOf());
		}		
	}
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	// 绘制几何模型
	if (m_CurrMode == ShowMode::WoodCrate)
	{
		m_pd3dImmediateContext->PSSetShaderResources(1, 1, m_pWoodCrate1[1].GetAddressOf());//因为混合问题，需要一个白色背景
		
		m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate[0].GetAddressOf());
		m_pd3dImmediateContext->DrawIndexed(m_IndexCount/6, 0, 0);

		m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate[1].GetAddressOf());
		m_pd3dImmediateContext->DrawIndexed(m_IndexCount / 6, (m_IndexCount / 6)*1, 0);

		m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate[2].GetAddressOf());
		m_pd3dImmediateContext->DrawIndexed(m_IndexCount / 6, (m_IndexCount / 6) * 2, 0);

		m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate[3].GetAddressOf());
		m_pd3dImmediateContext->DrawIndexed(m_IndexCount / 6, (m_IndexCount / 6) * 3, 0);

		m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate[4].GetAddressOf());
		m_pd3dImmediateContext->DrawIndexed(m_IndexCount / 6, (m_IndexCount / 6) * 4, 0);

		m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate[5].GetAddressOf());
		m_pd3dImmediateContext->DrawIndexed(m_IndexCount / 6, (m_IndexCount / 6) * 5, 0);

	}
	else if (m_CurrMode == ShowMode::WoodCrate1)
	{
		m_pd3dImmediateContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
		m_pd3dImmediateContext->PSSetSamplers(1, 1, m_pSamplerState.GetAddressOf());
		m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate1[0].GetAddressOf());
		m_pd3dImmediateContext->PSSetShaderResources(1, 1, m_pWoodCrate1[2].GetAddressOf());

		//m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate1[0].GetAddressOf());
		m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);
	}
	else
	{
		m_pd3dImmediateContext->PSSetShaderResources(1, 1, m_pWoodCrate1[1].GetAddressOf());//因为混合问题，需要一个白色背景
		m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);
	}

	//
	// 绘制Direct2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		static const WCHAR* textStr = L"切换显示: 1-木箱(3D) 2-火焰(2D) 3-旋转木箱(3D)\n"
			L"          W-关灯\n"
			L"      按住Q-鼠标";
		m_pd2dRenderTarget->DrawTextW(textStr, (UINT)wcslen(textStr), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		static const WCHAR* textStrName = L"2022QG工作室图形组第三次小组培训\n"
			L"姓名：张明裕\n"
			L"作业完成日期：2022年04月08日\n";
		m_pd2dRenderTarget->DrawTextW(textStrName, (UINT)wcslen(textStrName), m_pTextFormat.Get(),
			D2D1_RECT_F{ 950.0f, 0.0f, 2800.0f, 2800.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}


bool GameApp::InitEffect()
{
	ComPtr<ID3DBlob> blob;

	// 创建顶点着色器(2D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_2D_VS.cso", L"HLSL\\Basic_2D_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader2D.GetAddressOf()));
	// 创建顶点布局(2D)
	HR(m_pd3dDevice->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout2D.GetAddressOf()));

	// 创建像素着色器(2D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_2D_PS.cso", L"HLSL\\Basic_2D_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader2D.GetAddressOf()));

	// 创建顶点着色器(3D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_3D_VS.cso", L"HLSL\\Basic_3D_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader3D.GetAddressOf()));
	// 创建顶点布局(3D)
	HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout3D.GetAddressOf()));

	// 创建像素着色器(3D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_3D_PS.cso", L"HLSL\\Basic_3D_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader3D.GetAddressOf()));

	return true;
}

bool GameApp::InitResource()
{
	// 初始化网格模型并设置到输入装配阶段
	auto meshData = Geometry::CreateBox();
	ResetMesh(meshData);

	// ******************
	// 设置常量缓冲区描述
	//
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.ByteWidth = sizeof(VSConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	// 新建用于VS和PS的常量缓冲区
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = sizeof(PSConstantBuffer);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[1].GetAddressOf()));

	// ******************
	// 初始化纹理和采样器状态
	//

	// 初始化木箱纹理
	//CreateDDSTextureFromFile([In]D3D设备,[In]dds图片文件名,[Out]输出一个指向资源接口类的指针,
	//                         [Out]输出一个指向着色器资源视图的指针,忽略，忽略)
	
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Tex\\one.dds", nullptr, m_pWoodCrate[0].GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Tex\\two.dds", nullptr, m_pWoodCrate[1].GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Tex\\three.dds", nullptr, m_pWoodCrate[2].GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Tex\\four.dds", nullptr, m_pWoodCrate[3].GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Tex\\five.dds", nullptr, m_pWoodCrate[4].GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Tex\\six.dds", nullptr, m_pWoodCrate[5].GetAddressOf()));

	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Tex\\flarealpha.dds", nullptr, m_pWoodCrate1[2].GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Tex\\flare.dds", nullptr, m_pWoodCrate1[0].GetAddressOf()));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Tex\\s.dds", nullptr, m_pWoodCrate1[1].GetAddressOf())); //混合问题，需要一个白色纹理
	// 初始化火焰纹理
	WCHAR strFile[40];
	m_pFireAnims.resize(120);
	for (int i = 1; i <= 120; ++i)
	{
		//函数wsprintf()将一系列的字符和数值输入到缓冲区
		//wsprintf(输出缓冲区,格式字符串, 需输出的参数)
		wsprintf(strFile, L"..\\Tex\\FireAnim\\Fire%03d.bmp", i);
		HR(CreateWICTextureFromFile(m_pd3dDevice.Get(), strFile, nullptr, m_pFireAnims[static_cast<size_t>(i) - 1].GetAddressOf()));
	}
		
	// 初始化采样器状态
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	//sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;// 所选过滤器
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	//寻址模式 改为通过将每个不在[0,1]2区间内的(u,v)映射为程序员指定的某个颜色来扩展纹理
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;// U方向寻址模式  
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;// V方向寻址模式
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;// W方向寻址模式
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;                 // 若mipmap等级低于MinLOD，则使用等级MinLOD。最小允许设为0
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX; // 若mipmap等级高于MaxLOD，则使用等级MaxLOD。必须比MinLOD大  
	//CreateSamplerState([In]采样器状态描述,[Out]输出的采样器)
	HR(m_pd3dDevice->CreateSamplerState(&sampDesc, m_pSamplerState.GetAddressOf()));

	
	// ******************
	// 初始化常量缓冲区的值
	//

	// 初始化用于VS的常量缓冲区的值
	m_VSConstantBuffer.world = XMMatrixIdentity();			
	m_VSConstantBuffer.view = XMMatrixTranspose(XMMatrixLookAtLH(
		XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f),
		XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	));
	m_VSConstantBuffer.proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 1.0f, 1000.0f));
	m_VSConstantBuffer.worldInvTranspose = XMMatrixIdentity();
	
	// 初始化用于PS的常量缓冲区的值
	// 这里只使用一盏点光来演示
	m_PSConstantBuffer.pointLight[0].position = XMFLOAT3(0.0f, 0.0f, -10.0f);
	m_PSConstantBuffer.pointLight[0].ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_PSConstantBuffer.pointLight[0].diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	m_PSConstantBuffer.pointLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_PSConstantBuffer.pointLight[0].att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	m_PSConstantBuffer.pointLight[0].range = 25.0f;
	m_PSConstantBuffer.numDirLight = 0;
	m_PSConstantBuffer.numPointLight = 1;
	m_PSConstantBuffer.numSpotLight = 0;
	// 初始化材质
	m_PSConstantBuffer.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_PSConstantBuffer.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_PSConstantBuffer.material.specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 5.0f);
	// 注意不要忘记设置此处的观察位置，否则高亮部分会有问题
	m_PSConstantBuffer.eyePos = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);

	// 更新PS常量缓冲区资源
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(PSConstantBuffer), &m_PSConstantBuffer, sizeof(PSConstantBuffer));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);

	// ******************
	// 给渲染管线各个阶段绑定好所需资源
	// 设置图元类型，设定输入布局
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout3D.Get());
	// 默认绑定3D着色器
	m_pd3dImmediateContext->VSSetShader(m_pVertexShader3D.Get(), nullptr, 0);
	// VS常量缓冲区对应HLSL寄存于b0的常量缓冲区
	m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
	// PS常量缓冲区对应HLSL寄存于b1的常量缓冲区
	//PSSetConstantBuffers([In]起始槽索引，对应HLSL的register(t*),[In]着色器资源视图数目,[In]着色器资源视图数组)
	m_pd3dImmediateContext->PSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
	// 像素着色阶段设置好采样器
	//PSSetSamplers([In]起始槽索引,[In]采样器状态数目,[In]采样器数组)
	m_pd3dImmediateContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
	//m_pd3dImmediateContext->PSSetSamplers(1, 1, m_pSamplerState.GetAddressOf());
	//m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate1[0].GetAddressOf());
	//m_pd3dImmediateContext->PSSetShaderResources(1, 1, m_pWoodCrate1[1].GetAddressOf());
	m_pd3dImmediateContext->PSSetShader(m_pPixelShader3D.Get(), nullptr, 0);


	
	// ******************
	// 设置调试对象名
	//
	D3D11SetDebugObjectName(m_pVertexLayout2D.Get(), "VertexPosTexLayout");
	D3D11SetDebugObjectName(m_pVertexLayout3D.Get(), "VertexPosNormalTexLayout");
	D3D11SetDebugObjectName(m_pConstantBuffers[0].Get(), "VSConstantBuffer");
	D3D11SetDebugObjectName(m_pConstantBuffers[1].Get(), "PSConstantBuffer");
	D3D11SetDebugObjectName(m_pVertexShader2D.Get(), "Basic_2D_VS");
	D3D11SetDebugObjectName(m_pVertexShader3D.Get(), "Basic_3D_VS");
	D3D11SetDebugObjectName(m_pPixelShader2D.Get(), "Basic_2D_PS");
	D3D11SetDebugObjectName(m_pPixelShader3D.Get(), "Basic_3D_PS");
	D3D11SetDebugObjectName(m_pSamplerState.Get(), "SSLinearWrap");

	return true;
}
template<class VertexType>
bool GameApp::ResetMesh(const Geometry::MeshData<VertexType>& meshData)
{
	// 释放旧资源
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();



	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * sizeof(VertexType);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));

	// 输入装配阶段的顶点缓冲区设置
	UINT stride = sizeof(VertexType);			// 跨越字节数
	UINT offset = 0;							// 起始偏移量

	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);



	// 设置索引缓冲区描述
	m_IndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(DWORD) * m_IndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = meshData.indexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
	// 输入装配阶段的索引缓冲区设置
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);



	// 设置调试对象名
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), "IndexBuffer");

	return true;
}
