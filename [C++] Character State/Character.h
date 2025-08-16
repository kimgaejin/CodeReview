#pragma once

#include "Component.h"
#include "GameObject.h"
#include "State.h"
#include "Enemy_Desc.h"
#include "Character_Desc.h"
#include "State_Param.h"

BEGIN(Client)

// 캐릭터의 데이터를 들고있고, 즉시 호출해야하는 함수 제공
// - 상태(State) 전환과 입력/이동/전투 상호작용을 한 곳에서 중재
// - 상태 데이터(StateEvent)는 CState가 해석, 캐릭터는 교체/우선순위/파라미터만 관리
class CCharacter : public CComponent
{
	COMPONENT_BODY(CCharacter)
private:
	CCharacter(weak_ptr<CGameObject> _owner);
public:
	virtual ~CCharacter();
	static weak_ptr<CCharacter> Create(weak_ptr<CGameObject> _owner, weak_ptr<CSkeletalModel> _model, string characterTag);

public:
	virtual void On_Updated(_float fTimeDelta) override;
	virtual void On_LateUpdated(_float fTimeDelta) override;
	virtual HRESULT Initialize() override;

public:
	Character_Desc& Get_DescRef() { return m_desc; }
	
	// 상태 전환
	void Set_State(shared_ptr<CState> _state, _uint _priority = 0, shared_ptr<STATE_PARAM> _param = nullptr);
	void Set_StateImmediately(shared_ptr<CState> _state, shared_ptr<STATE_PARAM> _param = nullptr);

	// 입력 처리
	void Control(float deltaTime);

	void MoveTo(_vector worldVec);
	void MoveToFront(float value);
	void MoveToBack(float value);

	void LookAtPos(_fvector vAt);
	void LookAtLerp(_fvector vTarget, _float fDeltaTime, _float fRotateSpeed);
	void LookAtLerpWorld(_fvector vTarget, _float fDeltaTime, _float fRotateSpeed);

	void Set_Visible(bool type);
	void Set_CameraLookVec(_vector vec);
	_vector Get_CameraLookVec();
	void Set_Position(_float3 position);
	_float3 Get_Position();
	_float Get_HeadPosition();
	_float3 Get_FocusPosition(float focus);
	_float3 Get_LookAt();
	void Set_Target(weak_ptr<CCharacter> target);
	weak_ptr<CCharacter> Get_Target();
	int Damaged(weak_ptr<Damage_Desc> _dmgDesc);
	void Spawn();
	void Clear_Assulted() { m_desc.bIsAssulted = false; }

	// Change State를 거치지 않고, Level/Controller 에서 직접 호출하는 함수
	void Evade(Enemy_Desc enemy);
	void SwitchIn(Enemy_Desc enemy);
	void SwitchOut();
	void AssultAid();
	void Parry();
	void Parried();
	void Ultimate();
	void Hit(weak_ptr<Damage_Desc> _dmgDesc, int& damaged);
	void Wait();

	// Desc 기반 쿼리
	int Get_Team() const { return m_desc.iTeam; }
	float Get_CameraHeight() const { return m_desc.CameraHeight * 0.1f; }
	float Get_Atk() const { return static_cast<float>(m_desc.Attack); }
	float Get_Impact() const { return static_cast<float>(m_desc.Impact); }
	bool Is_Dead() const { return m_desc.bIsDead; }
	bool Is_Groggyable() const { return m_desc.iCurImpacted >= m_desc.iMaxImpacted; }
	bool Is_Spawned() const { return m_desc.bIsSpawned; }
	bool Is_Assulted() const { return m_desc.bIsAssulted; }
	void Req_EndGame();

	weak_ptr<CSkeletalModel> Get_ModelForEditor() { return m_pModel; }

private:

private:
	// 컴포넌트 캐시
	weak_ptr<CTransform> m_pTransform;
	weak_ptr<CSkeletalModel> m_pModel;

	// 상태 관련 포인터 정보
	shared_ptr<CState> m_state;
	shared_ptr<CState> m_nextState;
	shared_ptr<STATE_PARAM> m_nextParam;
	_uint m_nextStatePriority = {};

	// 캐릭터 런타임 데이터
	Character_Desc m_desc;
	_vector m_pCameraLookVec = {};
};

END
