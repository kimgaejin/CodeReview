#pragma once

#include "Client_Defines.h"
#include "Engine_Defines.h"
#include "SkeletalModel.h"
#include "Enemy_Desc.h"
#include "StateEvent.h"

BEGIN(Client)

class CCharacter;
struct Character_Desc;
struct Damage_Desc;

// State 데이터의 해석기
// - 새로운 상태의 정의/전이(교체)는 데이터(StateEvent/FrameEvent)로만 확장
class CState
{
protected:
	// 수명은 CCharacter에서 보장, CSkeletalModel이 없으면 생성 불가
	CState(weak_ptr<CCharacter> _owner, weak_ptr<CSkeletalModel> _model);
public:
	~CState();
	static shared_ptr<CState> Create(weak_ptr<CCharacter> _owner, weak_ptr<CSkeletalModel> _model, StateEvent* _stateEvent);

public:

	virtual void Initialize();
	virtual void On_Updated(float deltaTime, Character_Desc* charDesc);
	virtual void Control(float deltaTime, Character_Desc* charDesc);

	// 모든 상태에서 바로 전이가 가능한 상태들의 진입점
	bool Set(Character_Desc* charDesc, string name, _uint _priority = 0);
	bool Evade(Enemy_Desc _enemy, Character_Desc* charDesc);
	bool SwitchIn(Enemy_Desc _enemy, Character_Desc* charDesc);
	bool SwitchOut(Character_Desc* charDesc);
	bool AssultAid(Character_Desc* charDesc);
	bool Hited(Character_Desc* charDesc, weak_ptr<Damage_Desc> _dmgDesc);
	bool Parry(Character_Desc* charDesc);
	bool Parried(Character_Desc* charDesc);
	bool Ultimate(Character_Desc* charDesc);
	bool Died(Character_Desc* charDesc);
	bool Groggy(Character_Desc* charDesc);

protected:
	weak_ptr<CCharacter> Get_Owner();
	weak_ptr<CSkeletalModel> Get_Model();

private:
	void RegistKeyInput(char* keyInputRegister);
	bool Set_NextState(string& stateName, _uint priority = 0);

private:
	// 레퍼런스 캐시
	weak_ptr<CCharacter> m_pOwner;
	weak_ptr<CSkeletalModel> m_pModel;
	
	// 현재 실행중인 데이터
	StateEvent* m_pStateEvent = {};
	vector<bool> m_vFrameExecuted;

	// 상태 전이 우선 순위
	const _uint EVADE_PRIORITY = 100;
	const _uint SWITCH_PRIORITY = 200;
	const _uint PARRY_PRIORITY = 220;
	const _uint ASSULTAID_PRIORITY = 250;
	const _uint ULTIMATE_PRIORITY = 270;
	const _uint DIE_PRIORITY = 300;
};

END