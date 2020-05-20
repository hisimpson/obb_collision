#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <assert.h>
#include "dxfunc.h"

#define SVertexType D3DFVF_XYZ | D3DFVF_DIFFUSE

//vertex struct  
struct SVertex
{
	float x, y, z;
	DWORD color; //diffuse color
};


struct OBB {
    D3DXVECTOR3 c; //중심점 좌표
    D3DXVECTOR3 u[3]; //XYZ 좌표축의 회전을 나타내는 방향 벡터
    D3DXVECTOR3 r; //좌표축의 반지름
};

struct VERTEX
{
    D3DXVECTOR3 position, normal;
    //float tu, tv;
};

int TestOBBOBB(OBB *a, OBB *b);

void CreateOBB(OBB* obb, LPD3DXMESH pMesh, D3DXMATRIX& mat)
{
    // 최대 값, 최소값의 초기 값 설정
    D3DXVECTOR3 max = D3DXVECTOR3( -10000.0f, -10000.0f, -10000.0f);
    D3DXVECTOR3 min = D3DXVECTOR3( 10000.0f, 10000.0f, 10000.0f);

    //메쉬의 정점 데이터를 얻어온다.
	VERTEX * vertexBuffer = NULL;
    pMesh->LockVertexBuffer( 0, (void **)&vertexBuffer);
    // 최대 값, 최소값 구하기
	int num = (int)pMesh->GetNumVertices();
    for (int i = 0; i < num; i ++)
    {
        D3DXVECTOR3& pos = vertexBuffer[i].position;
        if (pos.x <min.x) min.x = pos.x;
        if (pos.x> max.x) max.x = pos.x;
        if (pos.y <min.y) min.y = pos.y;
        if (pos.y> max.y) max.y = pos.y;
        if (pos.z <min.z) min.z = pos.z;
        if (pos.z> max.z) max.z = pos.z;
    }
    pMesh->UnlockVertexBuffer();

    // 중심점 구하기
	D3DXVECTOR3 worldPos = D3DXVECTOR3( mat._41, mat._42, mat._43);
    obb->c = (min + max) * 0.5f + worldPos;

    // 방향 벡터 구하기
    obb->u[0] = D3DXVECTOR3( mat._11, mat._12, mat._13);
    obb->u[1] = D3DXVECTOR3( mat._21, mat._22, mat._23);
    obb->u[2] = D3DXVECTOR3( mat._31, mat._32, mat._33);

    //반지름 구하기
    obb->r.x = fabsf( max.x - min.x) * 0.5f;
    obb->r.y = fabsf( max.y - min.y) * 0.5f;
    obb->r.z = fabsf( max.z - min.z) * 0.5f;
}

extern LPDIRECT3DDEVICE9 g_dxDevice;
bool CreateBox(float w, float h, float d, int color, ID3DXMesh **mesh)
{
	assert(g_dxDevice != NULL);
	assert(mesh != NULL);
	
	ID3DXMesh *tempMesh; // Temp D3D mesh object

	// Create the sphere
	if(D3DXCreateBox(g_dxDevice, w, h, d, &tempMesh, NULL) != D3D_OK)
		return false;
	
	// Flag for how to create the D3D mesh.  We want the vertex buffer and index
	// buffer memory to be managed by DirectX	
	DWORD flag = D3DXMESH_VB_MANAGED | D3DXMESH_IB_MANAGED;

	// Copy the sphere, converting to our FVF 
	if(tempMesh->CloneMeshFVF(flag, SVertexType, g_dxDevice, mesh) != D3D_OK)
		return false;

	SVertex *v;

	// Lock the vertex data of our box
	if((*mesh)->LockVertexBuffer(0, (void**)&v) != D3D_OK)
	{	
		(*mesh)->Release();
			return false;
	}

	// Set the box's color
	for(unsigned int i = 0; i < (*mesh)->GetNumVertices(); ++i)
		v[i].color = color;

	// Unlock the vertex data
	(*mesh)->UnlockVertexBuffer();

	tempMesh->Release(); // Free up the temporary mesh
	return true;
}

OBB g_obb1;
OBB g_obb2;
LPD3DXMESH g_boxMesh1 = NULL;
LPD3DXMESH g_boxMesh2 = NULL;

void ReleaseOBB()
{
	_RELEASE_<LPD3DXMESH>(g_boxMesh1);
	_RELEASE_<LPD3DXMESH>(g_boxMesh2);
}

void ReadObb(LPD3DXMESH pMesh, D3DXMATRIX& mat, LPD3DXMESH pMesh1, D3DXMATRIX& mat1)
{
	ReleaseOBB();

	CreateOBB(&g_obb1, pMesh, mat);
	CreateOBB(&g_obb2, pMesh1, mat1);
	
	OBB* obb = &g_obb1;
	CreateBox(obb->r.x*2.0f, obb->r.y*2.0f, obb->r.z*2.0f, 0xff0000, &g_boxMesh1);
	obb = &g_obb2;
	CreateBox(obb->r.x*2.0f, obb->r.y*2.0f, obb->r.z*2.0f, 0xffff, &g_boxMesh2);

	if(TestOBBOBB(&g_obb1, &g_obb2))
		printf("TestOBBOBB is collision\r\n");
	else
		printf("TestOBBOBB is none collision\r\n");
}

void SetObbToMatrix(OBB& obb, D3DXMATRIX& mat)
{
	mat._41 = obb.c.x;
	mat._42 = obb.c.y;
	mat._43 = obb.c.z;

	mat._11 = obb.u[0].x; 
	mat._12 = obb.u[0].y;
	mat._13 = obb.u[0].z;

	mat._21 = obb.u[1].x; 
	mat._22 = obb.u[1].y;
	mat._23 = obb.u[1].z;

	mat._31 = obb.u[2].x; 
	mat._32 = obb.u[2].y;
	mat._33 = obb.u[2].z;
}

void RenderOBB()
{
	D3DXMATRIX	matWorld;
	D3DXMatrixIdentity(&matWorld);

	SetObbToMatrix(g_obb1, matWorld);
	g_dxDevice->SetTransform(D3DTS_WORLD, &matWorld);
	g_boxMesh1->DrawSubset(0);

	SetObbToMatrix(g_obb2, matWorld);
	g_dxDevice->SetTransform(D3DTS_WORLD, &matWorld);
	g_boxMesh2->DrawSubset(0);
}

int TestOBBOBB(OBB *a, OBB *b)
{
    const float EPSILON = float(1.175494e-37);

    float R[3][3], AbsR[3][3];
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            R[i][j] = D3DXVec3Dot(&a->u[i], &b->u[j]);
            AbsR[i][j] = fabsf(R[i][j]) + EPSILON;
        }
    }
        
    D3DXVECTOR3 t = b->c - a->c;
    t = D3DXVECTOR3(D3DXVec3Dot(&t, &a->u[0]), D3DXVec3Dot(&t, &a->u[1]), D3DXVec3Dot(&t, &a->u[2]));
        
    //축L=A0, L=A1, L=A2판정
    float ra, rb;

    for(int i = 0; i < 3; i++)
    {
        ra = a->r[i];
        rb = b->r[0] * AbsR[i][0] + b->r[1] * AbsR[i][1] + b->r[2] * AbsR[i][2];
        if(fabsf(t[i]) > ra + rb)return 0;
    }
    //축L=B0, L=B1, L=B2판정
    for(int i = 0; i < 3; i++)
    {
        ra = a->r[0] * AbsR[0][i] + a->r[1] * AbsR[1][i] + a->r[2] * AbsR[2][i];
        rb = b->r[i];
        if(fabsf(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb)
			return 0;
    }

    //축L=A0 X B0판정
    ra = a->r[1] * AbsR[2][0] + a->r[2] * AbsR[1][0];
    rb = b->r[1] * AbsR[0][2] + b->r[2] * AbsR[0][1];
    if(fabsf(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb)
		return 0;

    //축L=A0 X B1판정
    ra = a->r[1] * AbsR[2][1] + a->r[2] * AbsR[1][1];
    rb = b->r[0] * AbsR[0][2] + b->r[2] * AbsR[0][0];
    if(fabsf(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb)
		return 0;

    //축L=A0 X B2판정
    ra = a->r[1] * AbsR[2][2] + a->r[2] * AbsR[1][2];
    rb = b->r[0] * AbsR[0][1] + b->r[1] * AbsR[0][0];
    if(fabsf(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb)
		return 0;

    //축L=A1 X B0판정
    ra = a->r[0] * AbsR[2][0] + a->r[2] * AbsR[0][0];
    rb = b->r[1] * AbsR[1][2] + b->r[2] * AbsR[1][1];
    if(fabsf(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb)
		return 0;

    //축L=A1 X B1판정
    ra = a->r[0] * AbsR[2][1] + a->r[2] * AbsR[0][1];
    rb = b->r[0] * AbsR[1][2] + b->r[2] * AbsR[1][0];
    if(fabsf(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb)
		return 0;

    //축L=A1 X B2판정
    ra = a->r[0] * AbsR[2][2] + a->r[2] * AbsR[0][2];
    rb = b->r[0] * AbsR[1][1] + b->r[1] * AbsR[1][0];
    if(fabsf(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb)
		return 0;

    //축L=A2 X B0판정
    ra = a->r[0] * AbsR[1][0] + a->r[1] * AbsR[0][0];
    rb = b->r[1] * AbsR[2][2] + b->r[2] * AbsR[2][1];
    if(fabsf(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb)
		return 0;

    //축L=A2 X B1판정
    ra = a->r[0] * AbsR[1][1] + a->r[1] * AbsR[0][1];
    rb = b->r[0] * AbsR[2][2] + b->r[2] * AbsR[2][0];
    if(fabsf(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb)
		return 0;

    //축L=A2 X B2판정
    ra = a->r[0] * AbsR[1][2] + a->r[1] * AbsR[0][2];
    rb = b->r[0] * AbsR[2][1] + b->r[1] * AbsR[2][0];
    if(fabsf(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb)
		return 0;

    return 1;
}