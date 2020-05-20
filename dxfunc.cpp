#include <windows.h>
#include <time.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h> 
#include "resource.h"

#include "dxfunc.h"

#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "winmm.lib")

//윈도우즈창에서 콘솔창 띄우기
#pragma comment(linker , "/entry:WinMainCRTStartup /subsystem:console")

#define		PI					(3.1415926)
#define		DEGtoRAD(x)			(float)((float)x * PI / 180.0f)

//버텍스 구조체와 타입 선언
struct CUSTOMVERTEX {
	float x, y, z; 
	float nx, ny, nz;
	DWORD color; 
	float tu, tv;
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)

LPDIRECT3DTEXTURE9 g_pTexture = NULL;


extern HWND             g_hDlg;
LPDIRECT3D9             g_d3d = NULL; 
LPDIRECT3DDEVICE9       g_dxDevice = NULL; 
LPDIRECT3DVERTEXBUFFER9 g_VertexBuffer = NULL; 
D3DPRESENT_PARAMETERS	g_d3dpp; 
bool g_bIsFullScreen = false;

LPD3DXMESH g_teapotMesh1 = NULL;
LPD3DXMESH g_teapotMesh2 = NULL;


//초기값은 워닝에러를 막기위해서 넣었을뿐 크게 의미가 없다.
D3DDISPLAYMODE  g_d3ddm = {640, 480, 60, D3DFMT_A8R8G8B8};

//main.cpp의 변수
extern HWND   g_hRenderWnd;

D3DXVECTOR3 g_pos = D3DXVECTOR3(2, 0, 0);
D3DXVECTOR3 g_dir = D3DXVECTOR3(1, 0, 0);

float g_angle2 = 0.0f;

float g_move_unitys = 0.1f;

void ReadObb(LPD3DXMESH pMesh, D3DXMATRIX& mat, LPD3DXMESH pMesh1, D3DXMATRIX& mat1);
void RenderOBB();
void ReleaseOBB();

float INV_PI = 180.0f / 3.141592f;
float GetAngle(float x, float y)
{
    float degree = atan2(y, x) * INV_PI;
    degree = fmodf(degree + 360, 360);
	//printf("GetAngle: %.02f\r\n", degree);
    return degree;
}

HRESULT InitD3D(HWND hWnd)
{
	ZeroMemory (&g_d3dpp, sizeof(g_d3dpp));

    if( NULL == ( g_d3d = Direct3DCreate9( D3D_SDK_VERSION ) ) )
        return E_FAIL;

	//버텍스 처리 결정
	D3DCAPS9 caps;
	D3DDEVTYPE deviceType = D3DDEVTYPE_HAL;
	g_d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, deviceType, &caps);
	int vertexProcessing = 0;
	if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
		vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	if( FAILED( g_d3d->GetAdapterDisplayMode (D3DADAPTER_DEFAULT, &g_d3ddm)))
			return E_FAIL;

    g_d3dpp.Windowed = !g_bIsFullScreen;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = g_d3ddm.Format;
	g_d3dpp.hDeviceWindow              = g_hRenderWnd;
	g_d3dpp.EnableAutoDepthStencil     = true; 
	g_d3dpp.AutoDepthStencilFormat     = D3DFMT_D24S8;

    if(FAILED(g_d3d->CreateDevice (D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, 
								   vertexProcessing, 
								   &g_d3dpp, &g_dxDevice)))
	{
		return E_FAIL;
	}

	//g_dxDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
	//g_dxDevice->SetRenderState( D3DRS_AMBIENT, 0xffffffff );
	
	g_dxDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	g_dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	g_dxDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME); 
	return S_OK;
}

//-----------------------------------------------------------------------------
HRESULT D3DUtil_SetProjectionMatrix( D3DMATRIX& mat, FLOAT fFOV, FLOAT fAspect,
                                     FLOAT fNearPlane, FLOAT fFarPlane )
{
    if( fabs(fFarPlane-fNearPlane) < 0.01f )
        return E_INVALIDARG;
    if( fabs(sin(fFOV/2)) < 0.01f )
        return E_INVALIDARG;

    FLOAT w = fAspect * ( cosf(fFOV/2)/sinf(fFOV/2) );
    FLOAT h =   1.0f  * ( cosf(fFOV/2)/sinf(fFOV/2) );
    FLOAT Q = fFarPlane / ( fFarPlane - fNearPlane );

    ZeroMemory( &mat, sizeof(D3DMATRIX) );
    mat._11 = w;
    mat._22 = h;
    mat._33 = Q;
    mat._34 = 1.0f;
    mat._43 = -Q*fNearPlane;

    return S_OK;
}
//-----------------------------------------------------------------------------
void SetupCamera(HWND hWnd)
{
    D3DXMATRIX   matProj;
	D3DXMATRIX	 matCameraView;
	D3DXVECTOR3  cameraTarget;
    D3DXVECTOR3  cameraPos;

	// Set where camera is viewing from
	cameraPos.x = 0.0f;
	cameraPos.y = 10.0f;
	cameraPos.z = -10.0f;
	// Set what camera is looking at
	cameraTarget.x = 0.0f;
	cameraTarget.y = 0.0f;
	cameraTarget.z = 0.0f;

	// Setup Camera View
	D3DXMatrixLookAtLH( &matCameraView, 
						&cameraPos, 
						&cameraTarget, 
						&D3DXVECTOR3( 0.0f, 1.0f, 0.0f ) );
	// Tell the device to use the camera view for the viewport
	g_dxDevice->SetTransform( D3DTS_VIEW, &matCameraView );

	// Setup Projection 
	RECT rect;
	GetClientRect(hWnd, &rect);
	float width = (float)rect.right, height = (float)rect.bottom;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, width/height, 1.0f, 100.0f );

	// Tell the device to use the above matrix for projection
	g_dxDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}

void DestroyDirect()
{
	ReleaseOBB();
	_RELEASE_<LPDIRECT3DTEXTURE9>(g_pTexture);
	_RELEASE_<LPDIRECT3DVERTEXBUFFER9>(g_VertexBuffer);
	_RELEASE_<LPDIRECT3DDEVICE9>(g_dxDevice);
	_RELEASE_<LPDIRECT3D9>(g_d3d);
}

bool RenderInit()
{
	float	fSizeX = 2.0f;
	float	fSizeY = 2.0f;
	
	CUSTOMVERTEX vertex[4];

	// Create the geometry
	vertex[0].x = -fSizeX/2;
	vertex[0].y = -fSizeY/2;
	vertex[0].z = 0.0f;
	vertex[0].nx = 0.0f;
	vertex[0].ny = 0.0f;
	vertex[0].nz = -1.0f;
	vertex[0].color = 0xffffff;
	vertex[0].tu = 0.0f;
    vertex[0].tv = 1.0f;
	
	vertex[1].x = -fSizeX/2;
	vertex[1].y = fSizeY/2;
	vertex[1].z = 0.0f;
	vertex[1].nx = 0.0f;
	vertex[1].ny = 0.0f;
	vertex[1].nz = -1.0f;
	vertex[1].color = 0xffffff;
	vertex[1].tu = 0.0f;
    vertex[1].tv = 0.0f;

	vertex[2].x = fSizeX/2;
	vertex[2].y = -fSizeY/2;
	vertex[2].z = 0.0f;
	vertex[2].nx = 0.0f;
	vertex[2].ny = 0.0f;
	vertex[2].nz = -1.0f;
	vertex[2].color = 0xffffff;
	vertex[2].tu = 1.0f;
    vertex[2].tv = 1.0f;
	
	vertex[3].x = fSizeX/2;
	vertex[3].y = fSizeY/2;
	vertex[3].z = 0.0f;
	vertex[3].nx = 0.0f;
	vertex[3].ny = 0.0f;
	vertex[3].nz = -1.0f;
	vertex[3].color = 0xffffff;
	vertex[3].tu = 1.0f;
    vertex[3].tv = 0.0f;
	
	g_dxDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &g_VertexBuffer, NULL);
	
	VOID* pVertices;
	g_VertexBuffer->Lock (0, sizeof(vertex), &pVertices, 0);
    memcpy (pVertices, vertex, sizeof(vertex));
    g_VertexBuffer->Unlock();

	D3DXCreateTeapot(g_dxDevice, &g_teapotMesh1, NULL);
	D3DXCreateTeapot(g_dxDevice, &g_teapotMesh2, NULL);

	return true;
}

HRESULT ResetDeviceObjects()
{
	HRESULT hr;

    // Release all vidmem objects
    //if( FAILED( hr = InvalidateDeviceObjects() ) )
    //    return hr;

	// Reset the device
	if( FAILED( hr = g_dxDevice->Reset( &g_d3dpp ) ) )
		return hr;

	return S_OK;
}

void Update()
{
	if (GetAsyncKeyState(VK_UP) & 0x8000)
		g_pos += (g_dir*g_move_unitys);
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		g_pos += (-g_dir*g_move_unitys);
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		float angle = GetAngle(g_dir.x, -g_dir.z);
		angle += 1.0f;
		float radian = D3DXToRadian(angle);

		D3DXMATRIX mat;
		D3DXMatrixRotationY(&mat, radian);
		g_dir = D3DXVECTOR3(1, 0, 0);
		D3DXVec3TransformCoord(&g_dir, &g_dir, &mat);
	}
	if (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000)
	{
		g_angle2 += 1.0f;
	}
}

HRESULT RenderLoop()
{
    HRESULT hr;

    // Test the cooperative level to see if it's okay to render
    if( FAILED( hr = g_dxDevice->TestCooperativeLevel() ) )
    {
        // If the device was lost, do not render until we get it back
        if( D3DERR_DEVICELOST == hr )
            return S_OK;

        // Check if the device needs to be resized.
        if( D3DERR_DEVICENOTRESET == hr )
        {
            // If we are windowed, read the desktop mode and use the same format for
            // the back buffer
            if( !g_bIsFullScreen )
            {
				g_d3d->GetAdapterDisplayMode (D3DADAPTER_DEFAULT, &g_d3ddm);
                g_d3dpp.BackBufferFormat = g_d3ddm.Format;
            }

            if( FAILED( hr = ResetDeviceObjects() ) )
                return hr;
        }
        return hr;
    }

    // Render the scene as normal
    if( FAILED( hr = Render3D() ) )
        return hr;

	return S_OK;
}

HRESULT Render3D ()
{
	D3DXMATRIX	matWorld;
	D3DXMatrixIdentity(&matWorld);

	//렌드링
	HRESULT hr;
    g_dxDevice->Clear (0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB (0, 0, 0), 1.0f, 0);
    g_dxDevice->BeginScene ();

	//draw rectangle
	D3DXMatrixTranslation(&matWorld, 0, 0, 0);
	g_dxDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_dxDevice->SetFVF (D3DFVF_CUSTOMVERTEX);
	g_dxDevice->SetStreamSource(0, g_VertexBuffer, 0, sizeof(CUSTOMVERTEX));
    g_dxDevice->DrawPrimitive (D3DPT_TRIANGLESTRIP, 0, 2);

	D3DXMATRIX	matRot, matTrans;
	float angle = GetAngle(g_dir.x, -g_dir.z);
	D3DXMatrixRotationY(&matRot, DEGtoRAD(angle));
	D3DXMatrixTranslation(&matTrans, g_pos.x, g_pos.y, g_pos.z);
	matWorld = matRot*matTrans;

	//오른쪽 주전자 렌더링
	g_dxDevice->SetTransform(D3DTS_WORLD, &matWorld);
	g_teapotMesh1->DrawSubset(0);

	//왼쪽 주전자의 회전축의 정보를 대화상자에서 가져온다.
	D3DXVECTOR3 dir2 = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	char retString[MAX_PATH];
	GetDlgItemText(g_hDlg, IDC_XAXIS, retString, MAX_PATH);
	dir2.x = (float)atof(retString);
	GetDlgItemText(g_hDlg, IDC_YAXIS, retString, MAX_PATH);
	dir2.y = (float)atof(retString);
	GetDlgItemText(g_hDlg, IDC_ZAXIS, retString, MAX_PATH);
	dir2.z = (float)atof(retString);

	//왼쪽 주전자 렌더링
	D3DXMATRIX	matWorld2, matRot2, matTrans2;
	D3DXMatrixRotationAxis(&matRot2, &dir2,  DEGtoRAD(g_angle2));
	D3DXMatrixTranslation(&matTrans2, -2, 0, 0);
	matWorld2 = matRot2*matTrans2;
	g_dxDevice->SetTransform(D3DTS_WORLD, &matWorld2);
	g_teapotMesh2->DrawSubset(0);

	//주전자 메쉬에서 OBB 대화상자를 만들고 렌더링한다.
	ReadObb(g_teapotMesh1, matWorld, g_teapotMesh2, matWorld2);
	RenderOBB();

    g_dxDevice->EndScene ();
    hr = g_dxDevice->Present (NULL, NULL, NULL, NULL);

	return hr;
}