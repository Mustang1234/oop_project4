#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include<cmath>

//#include <stdio.h>
//#include <iostream>
//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

#define M_RADIUS 0.05   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define COR 0.01
#define MIN_SPEED 0.5
#define GRAVITY  6 * FPS * FPS
#define FRACTION 4 * FPS * FPS
#define SPEEDUP 3 * FPS
#define ZOOM_MAX 10.0f
#define ZOOM_MIN 0.01f
#define PLAYERHEIGHT 2.0f
#define ENEMYSIZE 0.6f
#define WALKSPEED 0.015f
#define LOOKAROUNDSPEED 0.3f
#define MAP_SIZE 30
#define WORLD_SIZE 2
#define WALL_HEIGHT 6
#define BULLETSPEED 400.0f


IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;
static int FPS = 0;

const D3DXCOLOR			headHit[2] = { D3DCOLOR_XRGB(192, 32,  0),D3DCOLOR_XRGB(128, 128,  0) };
const D3DXCOLOR			bodyHit[2] = { D3DCOLOR_XRGB(192, 16, 16),D3DCOLOR_XRGB(128,  64, 64) };

// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------

double pos_x = 0;
double pos_z = 0;
double target_x = 1.0f;
double target_y = 0.0f;
double target_z = 0.0f;
int wall_num;
int enemy_num;
int my_life;
bool my_shoot = false;

// There are four balls
// initialize the position (coordinate) of each ball (ball0 ~ ball7)
// initialize the color of each ball (ball0 ~ ball1)

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------

D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

D3DXVECTOR3 pos(0.0f, PLAYERHEIGHT, 0.0f);
D3DXVECTOR3 target(1.0f, PLAYERHEIGHT, 0.0f);
D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
	double					center_x, center_y, center_z;
	double                  m_radius;
	double					m_velocity_x;
	double					m_velocity_z;
	double					m_velocity_y;

public:
	CSphere(double radius=M_RADIUS) {
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_radius = radius;
		m_velocity_x = 0;
		m_velocity_z = 0;
		m_velocity_y = 0;
		m_pSphereMesh = NULL;
	}
	~CSphere(void) {}

public:

	bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE) {
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
			return false;
		return true;
	}

	void destroy(void) {
		if (m_pSphereMesh != NULL) {
			m_pSphereMesh->Release();
			m_pSphereMesh = NULL;
		}
	}

	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld) {
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
	}

	void ballUpdate(double timeDelta) {
		const double tX = center_x + m_velocity_x * timeDelta;
		const double tY = center_y + m_velocity_y * timeDelta;
		const double tZ = center_z + m_velocity_z * timeDelta;
		setCenter(tX, tY, tZ);
	}

	void setPower(double vx, double vz) {
		m_velocity_x = vx;
		m_velocity_y = 0;
		m_velocity_z = vz;
	}

	void setPowerY(double vx, double vy, double vz) {
		m_velocity_x = vx;
		m_velocity_y = vy;
		m_velocity_z = vz;
	}

	void setCenter(double x, double y, double z) {
		D3DXMATRIX m;
		center_x = x;	center_y = y;	center_z = z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	double getRadius(void)  const { return (double)(m_radius); }
	const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	D3DXVECTOR3 getCenter(void) const {
		D3DXVECTOR3 org(center_x, center_y, center_z);
		return org;
	}

private:
	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pSphereMesh;

};

// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

	double					m_x;
	double					m_y;
	double					m_z;
	double                  m_width;
	double					m_height;
	double                  m_depth;

public:
	CWall(void) {
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_width = 0;
		m_height = 0;
		m_depth = 0;
		m_pBoundMesh = NULL;
	}
	~CWall(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, double ix, double iz, double iwidth, double iheight, double idepth, D3DXCOLOR color = d3d::WHITE) {
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		m_width = iwidth;
		m_height = iheight;
		m_depth = idepth;

		if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
			return false;
		return true;
	}

	void setColor(D3DXCOLOR color) {
		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
	}

	void destroy(void) {
		if (m_pBoundMesh != NULL) {
			m_pBoundMesh->Release();
			m_pBoundMesh = NULL;
		}
	}

	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld) {
		if (NULL == pDevice) return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball) {
		D3DXVECTOR3 ball_center = ball.getCenter();
		double ball_radius = ball.getRadius();
		if (ball_center.z + ball_radius > m_z - m_depth / 2 &&
			ball_center.z - ball_radius < m_z + m_depth / 2 &&
			ball_center.y + ball_radius > m_y - m_height / 2 &&
			ball_center.y - ball_radius < m_y + m_height / 2 &&
			ball_center.x + ball_radius > m_x - m_width / 2 &&
			ball_center.x - ball_radius < m_x + m_width / 2) return true;
		return false;
	}

	void setPosition(double x, double y, double z) {
		D3DXMATRIX m;
		this->m_x = x;
		this->m_y = y;
		this->m_z = z;

		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

private:
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
	CLight(void) {
		static DWORD i = 0;
		m_index = i++;
		D3DXMatrixIdentity(&m_mLocal);
		::ZeroMemory(&m_lit, sizeof(m_lit));
		m_pMesh = NULL;
		m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		m_bound._radius = 0.0f;
	}
	~CLight(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, double radius = 0.1f) {
		if (NULL == pDevice)
			return false;
		if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
			return false;

		m_bound._center = lit.Position;
		m_bound._radius = radius;

		m_lit.Type = lit.Type;
		m_lit.Diffuse = lit.Diffuse;
		m_lit.Specular = lit.Specular;
		m_lit.Ambient = lit.Ambient;
		m_lit.Position = lit.Position;
		m_lit.Direction = lit.Direction;
		m_lit.Range = lit.Range;
		m_lit.Falloff = lit.Falloff;
		m_lit.Attenuation0 = lit.Attenuation0;
		m_lit.Attenuation1 = lit.Attenuation1;
		m_lit.Attenuation2 = lit.Attenuation2;
		m_lit.Theta = lit.Theta;
		m_lit.Phi = lit.Phi;
		return true;
	}
	void destroy(void) {
		if (m_pMesh != NULL) {
			m_pMesh->Release();
			m_pMesh = NULL;
		}
	}
	bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld) {
		if (NULL == pDevice)
			return false;

		D3DXVECTOR3 pos(m_bound._center);
		D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
		D3DXVec3TransformCoord(&pos, &pos, &mWorld);
		m_lit.Position = pos;

		pDevice->SetLight(m_index, &m_lit);
		pDevice->LightEnable(m_index, TRUE);
		return true;
	}

	void draw(IDirect3DDevice9* pDevice) {
		if (NULL == pDevice)
			return;
		D3DXMATRIX m;
		D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
		pDevice->SetTransform(D3DTS_WORLD, &m);
		pDevice->SetMaterial(&d3d::WHITE_MTRL);
		m_pMesh->DrawSubset(0);
	}

	D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
	DWORD               m_index;
	D3DXMATRIX          m_mLocal;
	D3DLIGHT9           m_lit;
	ID3DXMesh* m_pMesh;
	d3d::BoundingSphere m_bound;
};

// -----------------------------------------------------------------------------
// CEnemy class definition
// -----------------------------------------------------------------------------

class CEnemy {
public:
	CEnemy(void) {}
	CEnemy(int z, int x) {
		x_pos = (x - MAP_SIZE / 2) * WORLD_SIZE + WORLD_SIZE / 2;
		z_pos = (MAP_SIZE / 2 - z) * WORLD_SIZE - WORLD_SIZE / 2;

		body.create(Device, -1, -1, ENEMYSIZE, PLAYERHEIGHT * 0.85, ENEMYSIZE, d3d::CYAN);
		body.setPosition(x_pos - ENEMYSIZE / 2, PLAYERHEIGHT * 0.425, z_pos - ENEMYSIZE / 2);
		head.create(Device, -1, -1, ENEMYSIZE, PLAYERHEIGHT * 0.3, ENEMYSIZE, d3d::GREEN);
		head.setPosition(x_pos - ENEMYSIZE / 2, PLAYERHEIGHT, z_pos - ENEMYSIZE / 2);
		bullet.create(Device, d3d::RED);
		bullet.setCenter(x_pos, PLAYERHEIGHT * 0.75, z_pos);
		shoot = false;
		alive = true;
		life = 3;
	}
	~CEnemy(void) {}
public:

	void draw(IDirect3DDevice9** pDevice, const D3DXMATRIX& mWorld) {
		body.draw(*pDevice, mWorld);
		head.draw(*pDevice, mWorld);
		bullet.draw(*pDevice, mWorld);
	}

	void Update(double timeDelta, CWall* g_legowalls, CSphere my_bullet) {
		for (int k = 0; k < wall_num; k++) {
			if (g_legowalls[k].hasIntersected(bullet)) shoot = false;
		}
		D3DXVECTOR3 bullet_center = bullet.getCenter();
		double bullet_radius = bullet.getRadius();
		if (bullet_center.z + bullet_radius > pos_z - ENEMYSIZE / 2 &&
			bullet_center.z - bullet_radius < pos_z + ENEMYSIZE / 2 &&
			bullet_center.y + bullet_radius < PLAYERHEIGHT &&
			bullet_center.y - bullet_radius > 0 &&
			bullet_center.x + bullet_radius > pos_x - ENEMYSIZE / 2 &&
			bullet_center.x - bullet_radius < pos_x + ENEMYSIZE / 2) {
			my_life--;
			shoot = false;
			if (my_life <= 0)exit(0);
		}
		if (!shoot) {
			bullet.setPower(0, 0);
			bullet.setCenter(x_pos, PLAYERHEIGHT * 0.75, z_pos);
			double x_power = pos_x - x_pos;
			double z_power = pos_z - z_pos;
			double bullet_radius = sqrt(pow(x_power, 2) + pow(z_power, 2));
			bullet.setPower(BULLETSPEED * x_power / bullet_radius, BULLETSPEED * z_power / bullet_radius);
			shoot = true;
		}
		bullet.ballUpdate(timeDelta);
	}

	void hasHit(CSphere& my_bullet) {
		bool isHeadShot=false;
		if (body.hasIntersected(my_bullet) || (isHeadShot=head.hasIntersected(my_bullet))) {
			hit();
			if (isHeadShot) alive = false;
			my_shoot = false;
		}
	}

	void hit() {
		life--;
		head.setColor(headHit[life - 1]);
		body.setColor(bodyHit[life - 1]);
		if (life <= 0) alive = false;
	}

	bool isAlive() { return alive; }

	D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(x_pos, 0.0f, z_pos); }

private:
	double x_pos, z_pos;
	CWall body, head;
	CSphere bullet;
	bool shoot;
	int life;
	bool alive;
};

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

char map[MAP_SIZE][MAP_SIZE + 1] = {
	"111111111111111111111111111111",
	"100000000000000000000000000001",
	"100000000000000000000000000001",
	"100F000000000000000e0000000001",
	"10000000000000000000e000000001",
	"100000000000000000000000000001",
	"111111111111111111111111000001",
	"100000000000000000000000000001",
	"100000000000000000000000000001",
	"100000000000000000000000000001",
	"10000000000000000000e000000001",
	"100000000e00000000000000000001",
	"100000111111111110000000000001",
	"100000100000000000000000000001",
	"10000010000000000e000000000001",
	"1000e010000e000000000000000001",
	"100000100000000000000000000001",
	"100000111111111111111111111111",
	"100000000000000000000000000001",
	"100000000000000000000000000001",
	"100000000000000e00000000000001",
	"1000000000000000000000000e0001",
	"100000000000000000000000000001",
	"111111111111111111111111000001",
	"100000000000000000000000000001",
	"100000000000000000000000000001",
	"100P00000000000000000000000001",
	"100000000000000000000000000001",
	"100000000000000000000000000001",
	"111111111111111111111111111111"
};

bool make_map(CWall** g_legowalls, CWall* g_legoFlag) {
	int wall_count = 0;
	for (int k = 0; k < MAP_SIZE * MAP_SIZE; k++) if (map[(int)(k / MAP_SIZE)][(k % MAP_SIZE)] == '1')wall_count++;
	*g_legowalls = (CWall*)malloc(sizeof(CWall) * wall_count);
	wall_num = wall_count;
	for (int k = 0; k < MAP_SIZE * MAP_SIZE; k++) {
		if (map[(int)(k / MAP_SIZE)][(k % MAP_SIZE)] == '1')
			if ((*g_legowalls)[--wall_count].create(Device, -1, -1, WORLD_SIZE, WALL_HEIGHT, WORLD_SIZE, d3d::WHITE))
				(*g_legowalls)[wall_count].setPosition((k % MAP_SIZE) * WORLD_SIZE - (MAP_SIZE - 1) * WORLD_SIZE / 2, WALL_HEIGHT / 2, (MAP_SIZE - (int)(k / MAP_SIZE)) * WORLD_SIZE - (MAP_SIZE + 1) * WORLD_SIZE / 2);
			else return false;
		else if (map[(int)(k / MAP_SIZE)][(k % MAP_SIZE)] == 'F') 
			if ((*g_legoFlag).create(Device, -1, -1, WORLD_SIZE, WALL_HEIGHT, WORLD_SIZE, d3d::YELLOW))
				(*g_legoFlag).setPosition((k % MAP_SIZE) * WORLD_SIZE - (MAP_SIZE - 1) * WORLD_SIZE / 2, WALL_HEIGHT / 2, (MAP_SIZE - (int)(k / MAP_SIZE)) * WORLD_SIZE - (MAP_SIZE + 1) * WORLD_SIZE / 2);
			else return false;
		else if (map[(int)(k / MAP_SIZE)][(k % MAP_SIZE)] == 'P') {
			pos_x = (MAP_SIZE / 2 - (int)(k / MAP_SIZE)) * WORLD_SIZE - WORLD_SIZE / 2;
			pos_z = (k % MAP_SIZE - MAP_SIZE / 2) * WORLD_SIZE + WORLD_SIZE / 2;
		}
	}
	return true;
}

void locate_enemy(CEnemy** g_enemy) {
	int enemy_count = 0;
	for (int k = 0; k < MAP_SIZE * MAP_SIZE; k++) if (map[(int)(k / MAP_SIZE)][(k % MAP_SIZE)] == 'e')enemy_count++;
	*g_enemy = (CEnemy*)malloc(sizeof(CEnemy) * enemy_count);
	enemy_num = enemy_count;
	for (int k = 0; k < MAP_SIZE * MAP_SIZE; k++) {
		if (map[(int)(k / MAP_SIZE)][(k % MAP_SIZE)] == 'e')
			(*g_enemy)[--enemy_count] = CEnemy((int)(k / MAP_SIZE), (k % MAP_SIZE));
	}
}

bool goable(double pos_x, double pos_z) {
	if (map[(int)(MAP_SIZE / 2 - (pos_z + 0.2) / 2)][(int)(MAP_SIZE / 2 + (pos_x + 0.2) / 2)] == '1' ||
		map[(int)(MAP_SIZE / 2 - (pos_z + 0.2) / 2)][(int)(MAP_SIZE / 2 + (pos_x - 0.2) / 2)] == '1' ||
		map[(int)(MAP_SIZE / 2 - (pos_z - 0.2) / 2)][(int)(MAP_SIZE / 2 + (pos_x + 0.2) / 2)] == '1' ||
		map[(int)(MAP_SIZE / 2 - (pos_z - 0.2) / 2)][(int)(MAP_SIZE / 2 + (pos_x - 0.2) / 2)] == '1')return false;
	return true;
}

bool win() {
	if (map[(int)(MAP_SIZE / 2 - (pos_z + 0.2) / 2)][(int)(MAP_SIZE / 2 + (pos_x + 0.2) / 2)] == 'F' ||
		map[(int)(MAP_SIZE / 2 - (pos_z + 0.2) / 2)][(int)(MAP_SIZE / 2 + (pos_x - 0.2) / 2)] == 'F' ||
		map[(int)(MAP_SIZE / 2 - (pos_z - 0.2) / 2)][(int)(MAP_SIZE / 2 + (pos_x + 0.2) / 2)] == 'F' ||
		map[(int)(MAP_SIZE / 2 - (pos_z - 0.2) / 2)][(int)(MAP_SIZE / 2 + (pos_x - 0.2) / 2)] == 'F')return true;
	return false;
}

void destroyAllLegoBlock(void) {}


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------

CWall	g_legoPlane;
CWall	g_legoCeiling;
CWall	g_legoFlag;
CWall* g_legowalls;
CEnemy* g_enemy;
CSphere my_bullet;
CSphere aim_point= CSphere(0.001f);
CLight	g_light;

// initialization
bool Setup() {
	D3DXMatrixIdentity(&g_mWorld);
	D3DXMatrixIdentity(&g_mView);
	D3DXMatrixIdentity(&g_mProj);

	ShowCursor(false);

	if (!make_map(&g_legowalls,&g_legoFlag)) return false;

	locate_enemy(&g_enemy);

	// create plane and set the position
	if (!g_legoPlane.create(Device, -1, -1, WORLD_SIZE * MAP_SIZE, 0.5f, WORLD_SIZE * MAP_SIZE, d3d::WHITE)) return false;
	g_legoPlane.setPosition(0, 0, 0);

	if (!g_legoCeiling.create(Device, -1, -1, WORLD_SIZE * MAP_SIZE, 0.5f, WORLD_SIZE * MAP_SIZE, d3d::WHITE)) return false;
	g_legoCeiling.setPosition(0, WALL_HEIGHT, 0);

	if (!my_bullet.create(Device, d3d::BLACK)) return false;
	my_bullet.setCenter(pos_x, PLAYERHEIGHT * 0.75, pos_z); 

	if (!aim_point.create(Device, d3d::BLUE)) return false;
	aim_point.setCenter(pos_x, PLAYERHEIGHT * 0.75, pos_z);

	// light setting 
	D3DLIGHT9 lit;
	::ZeroMemory(&lit, sizeof(lit));
	lit.Type = D3DLIGHT_POINT;
	lit.Diffuse = d3d::WHITE;
	lit.Specular = d3d::WHITE * (float)(WORLD_SIZE * MAP_SIZE / 5);
	lit.Ambient = d3d::WHITE * (float)(WORLD_SIZE * MAP_SIZE / 5);
	lit.Position = D3DXVECTOR3(0.0f, WORLD_SIZE * MAP_SIZE / 2, 0.0f);
	lit.Range = WORLD_SIZE * MAP_SIZE * 10;
	lit.Attenuation0 = 0.0f;
	lit.Attenuation1 = 0.9f;
	lit.Attenuation2 = 0.0f;
	if (!g_light.create(Device, lit)) return false;

	// Position and aim the camera.
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);

	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4, (double)Width / (double)Height, 0.1f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

	// Set render states.
	Device->SetRenderState(D3DRS_LIGHTING, TRUE);
	Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	g_light.setLight(Device, g_mWorld);

	my_life = 3;

	return true;
}

void Cleanup(void) {
	g_legoPlane.destroy();
	for (int i = 0; i < MAP_SIZE * MAP_SIZE; i++) {
		g_legowalls[i].destroy();
	}
	destroyAllLegoBlock();
	g_light.destroy();
}

// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta) {
	SetCursorPos(500, 300);
	// Position and aim the camera.
	int i = 0;
	int j = 0;
	if (FPS == 0) {
		if (timeDelta != 0) {
			for (int k = 1; true; k++) {
				double f = (timeDelta * (double)pow(10, k));
				if (f >= 1) {
					if (f < 7.01 && f > 6.99) FPS = 0.0003 * pow(10, k);
					break;
				}
			}
		}
	}
	else {
		if (Device) {
			Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
			Device->BeginScene();

			// draw plane, walls, and spheres
			g_legoPlane.draw(Device, g_mWorld);
			if (g_legoPlane.hasIntersected(my_bullet)) my_shoot = false;

			g_legoFlag.draw(Device, g_mWorld);

			if (g_legoCeiling.hasIntersected(my_bullet)) my_shoot = false;

			for (i = 0; i < wall_num; i++) {
				g_legowalls[i].draw(Device, g_mWorld);
				if (g_legowalls[i].hasIntersected(my_bullet)) my_shoot = false;
			}
			for (i = 0; i < enemy_num; i++) {
				if (g_enemy[i].isAlive()) {
					g_enemy[i].Update(timeDelta, g_legowalls, my_bullet);
					g_enemy[i].draw(&Device, g_mWorld);
					if (my_shoot)g_enemy[i].hasHit(my_bullet);
				}
			}

			
			aim_point.setCenter(pos_x+target_x*0.125, PLAYERHEIGHT + target_y*0.125, pos_z + target_z*0.125);
			aim_point.draw(Device, g_mWorld);

			g_light.draw(Device);

			if (!my_shoot) {
				my_bullet.setCenter(pos_x, PLAYERHEIGHT * 0.75, pos_z);
				my_bullet.setPowerY(0,0,0);
			}
			my_bullet.ballUpdate(timeDelta);
			my_bullet.draw(Device, g_mWorld);

			if (win())exit(0);

			Device->EndScene();
			Device->Present(0, 0, 0, 0);
			Device->SetTexture(0, NULL);

			double radius = sqrt(pow(target_x, 2) + pow(target_z, 2));
			double next_x = 0;
			double next_z = 0;
			if (::GetAsyncKeyState(0x77) || ::GetAsyncKeyState(0x57)) {//w
				next_x += target_x / radius;
				next_z += target_z / radius;
			}
			if (::GetAsyncKeyState(0x73) || ::GetAsyncKeyState(0x53)) {//s
				next_x -= target_x / radius;
				next_z -= target_z / radius;
			}
			if (::GetAsyncKeyState(0x61) || ::GetAsyncKeyState(0x41)) {//a
				next_x -= target_z / radius;
				next_z += target_x / radius;
			}
			if (::GetAsyncKeyState(0x64) || ::GetAsyncKeyState(0x44)) {//d
				next_x += target_z / radius;
				next_z -= target_x / radius;
			}
			if (::GetAsyncKeyState(0x20)) {//sp
			}

			double next_radius = sqrt(pow(next_x, 2) + pow(next_z, 2));
			next_x *= WALKSPEED / next_radius;
			next_z *= WALKSPEED / next_radius;
			if (goable(pos_x + next_x, pos_z + next_z)) {
				pos_x += next_x;
				pos_z += next_z;
			}

			target = D3DXVECTOR3(pos_x + target_x, PLAYERHEIGHT + target_y, pos_z + target_z);
			pos = D3DXVECTOR3(pos_x, PLAYERHEIGHT, pos_z);

		}

	}

	// Position and aim the camera.
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);

	// Set render states.
	Device->SetRenderState(D3DRS_LIGHTING, TRUE);
	Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static bool wire = false;
	static bool isReset = true;
	//static int old_h = 492;
	//static int old_v = 269;
	//static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

	switch (msg) {
	case WM_DESTROY: {
		::PostQuitMessage(0);
		break;
	}
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_ESCAPE:
			::DestroyWindow(hwnd);
			break;

		case VK_RETURN:
			if (NULL != Device) {
				wire = !wire;
				Device->SetRenderState(D3DRS_FILLMODE,
					(wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
			}
			break;
		}
		break;
	}

	case WM_MOUSEMOVE: {
		int new_h = LOWORD(lParam);
		int new_v = HIWORD(lParam);
		double dh;//horizontal
		double dv;//vertical

		dh = (492 - new_h) * 0.001f;
		dv = (269 - new_v) * 0.001f;

		double cos_target = target_x;
		double sin_target = target_z;
		double cos_dh = cos(dh * LOOKAROUNDSPEED);
		double sin_dh = sin(dh * LOOKAROUNDSPEED);
		target_x = (cos_target * cos_dh - sin_target * sin_dh);
		target_z = (sin_target * cos_dh + cos_target * sin_dh);
		double sin_target_up = target_y;
		double cos_target_up = sqrt(1 - pow(target_y, 2));
		double sin_dv_up = sin(dv * LOOKAROUNDSPEED);
		double cos_dv_up = cos(dv * LOOKAROUNDSPEED);
		target_y = sin_target_up * cos_dv_up + cos_target_up * sin_dv_up;
		double target_radius = sqrt(pow(target_x, 2) + pow(target_y, 2) + pow(target_z, 2));
		target_x /= target_radius;
		target_y /= target_radius;
		target_z /= target_radius;


		if (LOWORD(wParam)) {
			if (LOWORD(wParam) & MK_LBUTTON) {
				if (!my_shoot) {
					my_bullet.setCenter(pos_x + target_x * 0.5, PLAYERHEIGHT + target_y * 0.5, pos_z + target_z * 0.5);
					my_bullet.setPowerY(target_x * BULLETSPEED, target_y * BULLETSPEED, target_z * BULLETSPEED);
					my_shoot = true;
				}
			}
			break;
		}
	}
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	srand(static_cast<unsigned int>(time(NULL)));

	if (!d3d::InitD3D(hinstance, Width, Height, true, D3DDEVTYPE_HAL, &Device)) {
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup()) {
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}
