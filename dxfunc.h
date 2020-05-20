#ifndef  __DXFUNC_H
#define  __DXFUNC_H

extern HRESULT InitD3D(HWND hWnd);
extern void SetupCamera(HWND hWnd);
extern void DestroyDirect();
extern bool RenderInit();
extern void Update();
extern HRESULT RenderLoop();
extern HRESULT Render3D();

extern int gMouseX;
extern int gMouseY;

#define    SCREEN_SIZEX     900
#define    SCREEN_SIZEY     600

template<class T> void _RELEASE_(T t)
{
	if( t )
	{
		t->Release();
		t = 0;
	}
}

template<class T> void _DELETE_(T t)
{
	if( t )
	{
		delete t;
		t = 0;
	}
}


#endif