#pragma once

#include "Client_Defines.h"

BEGIN(Client)

// 캐릭터의 런타임 정보
// - CState가 읽고 쓰며, CCharacter가 수명/초기화 관리
struct Character_Desc
{
	// ===== 기본 능력치 ===== 
	int curLevel;
	int upgradeLevel;
	int CameraHeight = 10; // 캐릭터 신장

	int MaxHp;
	int CurHp;
	int MaxEnergy;
	int CurEnergy;
	int Attack;
	int Defense;
	int Impact; // 충격력
	int CritDamage; // 치명타 피해
	int CritRate; // 치명타 확률
	int AnomalyProficiency; // 이상 장악력
	int AnomalyMastery; // 이상 마스터리
	int Penetration; // 관통률

	// ===== 장비/스킬 데이터 ===== 
	int weaponData;
	int diskData1;
	int diskData2;
	int diskData3;
	int diskData4;
	int diskData5;
	int diskData6;

	int upgradeCore; // 핵심 스킬(패시브)
	int upgradeNormal; // 일반 스킬
	int upgradeDodge; // 회피 스킬
	int upgradeSupport; // 지원 스킬
	int upgradeSpecial; // 특수 스킬
	int upgradeCombo; // 궁극기 및 콤보

	// ===== 연출용 데이터 ===== 
	_float3 personalColor ; // 캐릭터 이펙트 컬러

	// ===== 메타 데이터 ===== 
	string characterName; // harumasa
	string characterKoreanName; // 아사바 하루마사 *한국어는 length가 제대로 안될테니 조심*
	string department; // 대공동 특별행동부 제6과
	string rank; // S, A
	string attribute; // 전기, 물리 등
	string combatStyle; // 강공, 격파 등

	// ===== 상태 이름(key) ===== 
	string strState_Idle = "";
	string strState_Enter = "";
	string strState_Evade_F = "";
	string strState_Evade_B = "";
	string strState_SwitchIn = "";
	string strState_SwitchOut = "";
	string strState_AssultAid = "";
	string strState_DodgeCounter = "";
	string strState_Die = "";
	string strState_Hit_H_F = "";
	string strState_Hit_H_B = "";
	string strState_Hit_Shake = "";
	string strState_Attack_Special = "";
	string strState_Wait = "";
	string strState_Parry = "";
	string strState_Parried = "";

	// ===== 아이콘/애니메이션 경로(key) ===== 
	string strIconRoleGeneral = "";
	string strIconRoleCircle = "";
	string strIconRoleSelect = "";
	string strIconInterKnotRole = "";

	string strUltimateAnimationKey = "";

	// ===== 런타임 상태 ===== 
	float fMovementSpeed = 5.0f;
	float fEventSpeed = {}; // state event에 의해 제어되는 속도
	int iCurFrame = {}; // AnimationStuido 에서는 frame 기준으로 배치하나, 인게임에서는 fCurTime 으로만 검사함
	float fCurTime = {};
	bool bIsMoveable = {};
	bool bIsDodging = {};
	bool bIsHitable = { true };
	bool bIsGroggy;
	bool Is_Groggyable() { return bIsGroggy || iMaxImpacted <= iCurImpacted; }
	int iAssultChance;
	bool bIsDead;
	bool bIsSpawned;
	bool bIsAssulted;
	int iTeam = 0;
	int iCost = 0;
	float fDodgeTimer;
	int iCurImpacted = 0;
	int iMaxImpacted = 1000;
	float fLastTelegraphTime;
	_char perFrameKeyInput[16] = {};
	_char keyInputRegister[16] = {};

	// ===== 런타임 레퍼런스 ===== 
	weak_ptr<CCharacter> targetLock;
	weak_ptr<CTexture> IconRoleGeneralTexture;
	weak_ptr<CTexture> IconRoleCircleTexture;
	weak_ptr<CTexture> IconRoleSelectTexture;
	weak_ptr<CTexture> IconRoleInterKnotRoleTexture;

	// 상태 진입 시 공통 초기화
	void On_ChangedState()
	{
		fMovementSpeed = 5.0f;
		bIsMoveable = false;
		bIsHitable = true;
		bIsGroggy = false;
		iCurFrame = 0;
		fCurTime = 0;
		fEventSpeed = 0;
		memset(perFrameKeyInput, 0, sizeof(perFrameKeyInput));
		memset(keyInputRegister, 0, sizeof(keyInputRegister));
		targetLock.reset();
	}
};

END