#include "State.h"

#include "State_Manager.h"
#include "Character.h"
#include "Character_Desc.h"
#include "RequestHolder.h"
#include "VFX_Desc.h"
#include "FileConvertor.h"
#include <VFX_Trail.h>

// 공통 FrameEvent 방어코드: "이 프레임에서 이미 처리했는가?"
static inline bool ShouldSkipEvent(float curTime, const FrameEvent& fe, std::vector<bool>& executed, size_t idx, bool isUpdateAllFrames) {
	if (curTime < fe.time) return true;                    // 아직 이벤트 시점 전
	if (!isUpdateAllFrames && executed[idx]) return true;  // 1회성인데 이미 처리함
	executed[idx] = true;
	return false;
}


CState::CState(weak_ptr<CCharacter> _owner, weak_ptr<CSkeletalModel> _model)
{
	m_pOwner = _owner;
	m_pModel = _model;
}

CState::~CState()
{
}

shared_ptr<CState> CState::Create(weak_ptr<CCharacter> _owner, weak_ptr<CSkeletalModel> _model, StateEvent* _stateEvent)
{
	auto state = shared_ptr<CState>(new CState(_owner, _model));
	state->m_pStateEvent = _stateEvent;
	state->m_vFrameExecuted.resize(_stateEvent->frameEvents.size());
	std::fill(state->m_vFrameExecuted.begin(), state->m_vFrameExecuted.end(), false);

	return state;
}

void CState::Initialize()
{

}

void CState::On_Updated(float deltaTime, Character_Desc* charDesc)
{
	if (!m_pStateEvent)
		return;

	if (m_pStateEvent->frameLength == 0)
		return;

	// 포인터 캐시
	auto owner = m_pOwner.lock();
	auto model = m_pModel.lock();
	if (!owner || !model) return;

	const float curTime = charDesc->fCurTime;

	for (size_t i = 0; i < m_pStateEvent->frameEvents.size(); ++i)
	{
		FrameEvent& fe = m_pStateEvent->frameEvents[i];

		// frameEvent는 시간단위로 정렬되어있음
		// 단, 이전의 이벤트들은 매프레임 검사요소 때문에, 각자 분기문에서 처리함
		// ex) ChangeState intParam[4] 참조
		if (charDesc->fCurTime < fe.time )
			break;

		if (fe.type == FrameEvent::FMEVT_PLAYANIM)
		{
			const bool isUpdateAllFrames = (fe.intParam.size() >= 5) ? fe.intParam[4] != 0 : false;
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, isUpdateAllFrames)) continue;

			const float blendTime = fe.intParam[0] * 0.1f;
			const bool isLoop = fe.intParam[1] == 1;
			const bool  onlyWhenNotPlaying = fe.intParam[2] == 1;
			const bool  onlyWhenDifferent = fe.intParam[3] == 1;

			shared_ptr<CSkeletalModel> model = Get_Model().lock();
			std::string curAnim;
			const bool isPlaying = model->Is_PlayingAnimation(curAnim);
			const bool isSame = (model->Get_CurAnimName() == fe.strParam);

			// 재생 조건
			// - onlyWhenNotPlaying: 현재 재생 중이면 스킵
			// - onlyWhenDifferent : 같은 애니면 스킵
			if ((onlyWhenNotPlaying && isPlaying) || (onlyWhenDifferent && isSame)) continue;

			model->Play(fe.strParam, isLoop, blendTime);

			continue;
		}

		if (fe.type == FrameEvent::FMEVT_CHANGESTATE)
		{
			string& nextStateName = fe.strParam;
			const int usedE = fe.intParam[0];
			const int usedCost = fe.intParam[1];
			const int usedLC = fe.intParam[2];
			const int isMoveKeyDown = fe.intParam[3] == 1;
			const bool isMoveKeyUp = fe.intParam[3] == 2;
			_uint priority = 0;
			
			const bool isUpdateAllFrames = fe.intParam[4] != 0;
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, isUpdateAllFrames)) continue;

			if (0 < usedE)
			{
				if (charDesc->keyInputRegister[static_cast<int>(KEYINPUT::KEY_E)] == 0)
					continue;

				if (charDesc->CurEnergy < usedE)
					continue;

				charDesc->CurEnergy -= usedE;
				priority = 50;
			}

			if (0 < usedCost)
			{
				if (charDesc->iCost < usedCost)
					continue;

				charDesc->iCost -= usedCost;
				priority = 40;
			}

			if (0 < usedLC)
			{
				priority = 30;
				if (charDesc->keyInputRegister[static_cast<int>(KEYINPUT::KEY_LB)] == 0)
					continue;
			}
			if (isMoveKeyDown)
			{
				priority = 20;
				if (charDesc->keyInputRegister[static_cast<int>(KEYINPUT::KEY_UP)] == 0
					&& charDesc->keyInputRegister[static_cast<int>(KEYINPUT::KEY_DOWN)] == 0
					&& charDesc->keyInputRegister[static_cast<int>(KEYINPUT::KEY_LEFT)] == 0
					&& charDesc->keyInputRegister[static_cast<int>(KEYINPUT::KEY_RIGHT)] == 0
					)
					continue;
			}

			if (isMoveKeyUp)
			{
				priority = 20;
				if (charDesc->perFrameKeyInput[static_cast<int>(KEYINPUT::KEY_UP)] == 1
					|| charDesc->perFrameKeyInput[static_cast<int>(KEYINPUT::KEY_DOWN)] == 1
					|| charDesc->perFrameKeyInput[static_cast<int>(KEYINPUT::KEY_LEFT)] == 1
					|| charDesc->perFrameKeyInput[static_cast<int>(KEYINPUT::KEY_RIGHT)] == 1
					)
					continue;
			}

			Set_NextState(nextStateName, priority);
			 m_vFrameExecuted[i] = true;
			
			continue;
		}

		if (fe.type == FrameEvent::FMEVT_SETFOCUS)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			const float focusValue = fe.intParam[0] * 0.1f;
			
			shared_ptr<CCharacter> character = Get_Owner().lock();
			shared_ptr<CGameObject> gameObject = character->Get_Owner().lock();
			auto reqHolder = gameObject->Get_Component<CRequestHolder>();
			if (reqHolder.expired())
				continue;

			auto camFocusReq = make_shared<Request_CameraFocus_Desc>();
			camFocusReq->intensity = focusValue;
			reqHolder.lock()->Push(camFocusReq);

			continue;
		}

		if (fe.type == FrameEvent::FMEVT_MOVEABLE)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			charDesc->fMovementSpeed = fe.intParam[0] * 0.1f;
			charDesc->bIsMoveable = true;
			continue;
		}

		if (fe.type == FrameEvent::FMEVT_MOVEFRONT)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			charDesc->fEventSpeed = fe.intParam[0] * 0.1f;
			continue;
		}

		if (fe.type == FrameEvent::FMEVT_SHADERPROPERTY)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			const float alpha = fe.intParam[0] * 0.01f;
			const float glow = fe.intParam[1] * 0.01f;
			const float dummy1 = fe.intParam[2] * 0.01f;
			const float dummy2 = fe.intParam[3] * 0.01f;
			Get_Model().lock()->Set_MaterialsEffectParam({ alpha, glow, dummy1, dummy2 });

			continue;
		}

		if (fe.type == FrameEvent::FMEVT_SHADERPROPERTY_SLOPE)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			const float alpha = fe.intParam[0] * 0.01f;
			const float glow = fe.intParam[1] * 0.01f;
			const float dummy1 = fe.intParam[2] * 0.01f;
			const float dummy2 = fe.intParam[3] * 0.01f;
			Get_Model().lock()->Set_MaterialsEffectParam_Slope({ alpha, glow, dummy1, dummy2 });

			continue;
		}

		if (fe.type == FrameEvent::FMEVT_LOCKON)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			if (!charDesc->targetLock.expired())
			{
				shared_ptr<CCharacter> owner = Get_Owner().lock();
				shared_ptr<CCharacter> target = charDesc->targetLock.lock();
				_float3 pos = target->Get_Position();
				_vector posVec = XMLoadFloat3(&pos);
				owner->LookAtPos(posVec);
			}
			continue;
		}

		if (fe.type == FrameEvent::FMEVT_RANDSTATE)
		{
			string& nextStateName = fe.strParam;
			int iRate = fe.intParam[0];

			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			auto randValue = rand() % 100;
			if (randValue < iRate)
			{
				Set_NextState(nextStateName);
			}
			continue;

		}

		if (fe.type == FrameEvent::FMEVT_ATTACKEFFECT)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			bool isTargetPosition = fe.intParam[0] == 1;
			const float centerX = fe.intParam[1] * 0.01f; // center X
			const float centerY = fe.intParam[2] * 0.01f; // center Y
			const float centerZ = fe.intParam[3] * 0.01f; // center Z
			const float extentX = fe.intParam[4] * 0.01f; // extent X
			const float extentY = fe.intParam[5] * 0.01f; // extent Y
			const float extentZ = fe.intParam[6] * 0.01f; // extent Z
					   
			const float attackPTG = fe.intParam[7] * 0.01f; // Attack %
			const float impactPTG = fe.intParam[8] * 0.01f; // Impact %
			const int anomalyType = fe.intParam[9];			// Anomaly type
			const float anomalyPTG = fe.intParam[10] * 0.01f; // Anomaly %
			const float knockbackPower = fe.intParam[11] * 0.01f; // Knockback power
			const bool isRanged = (fe.intParam.size() < 13) ? 0 : fe.intParam[12] == 1; // is Ranged
			const bool isAssultCause = (fe.intParam.size() < 14) ? 0 : fe.intParam[13] == 1; // is assulat attack cause

			shared_ptr<CCharacter> character = Get_Owner().lock();
			shared_ptr<CGameObject> gameObject = character->Get_Owner().lock();
			auto reqHolder = gameObject->Get_Component<CRequestHolder>();
			if (reqHolder.expired())
				continue;

			auto attackReq = make_shared<Request_Spawn_AttackEffect>();
			
			auto dmgDesc = make_shared<Damage_Desc>();
			dmgDesc->isTargetPosition = isTargetPosition;
			dmgDesc->localCenter = { centerX , centerY, centerZ };
			dmgDesc->localExtent = { extentX , extentY, extentZ };

			dmgDesc->attackPTG = attackPTG;
			dmgDesc->impactPTG = impactPTG;
			dmgDesc->anomalyType = anomalyType;
			dmgDesc->anomalyPTG = anomalyPTG;
			dmgDesc->knockbackPower = knockbackPower;
			dmgDesc->isRanged = isRanged;
			dmgDesc->isAssultCause= isAssultCause;
			
			auto& charDesc = character->Get_DescRef();

			dmgDesc->lookVec = character->Get_LookAt();
			dmgDesc->critChancePTG = charDesc.CritRate;
			dmgDesc->critDmgPTG = charDesc.CritDamage;
			dmgDesc->owner = character;
			dmgDesc->target = charDesc.targetLock;
			
			attackReq->damage_desc = dmgDesc;
			reqHolder.lock()->Push(attackReq);

			continue;
		}

		if (fe.type == FrameEvent::FMEVT_VFX_PARTICLE)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			auto vfxDesc = make_shared<VFX_Desc>();
			vfxDesc->IntParamConvertToVFXDesc(fe.intParam, 0);
			vfxDesc->textures.push_back(CFileConvertor::TEXTURE_PREFIX + fe.strParam);
			shared_ptr<CCharacter> character = Get_Owner().lock();
			vfxDesc->owner = character;

			shared_ptr<CGameObject> gameObject = character->Get_Owner().lock();
			auto reqHolder = gameObject->Get_Component<CRequestHolder>();
			if (reqHolder.expired())
				continue;

			auto req = make_shared<Request_Spawn_VFXParticle>();
			req->vfx_desc = vfxDesc;
			reqHolder.lock()->Push(req);

			continue;
		}

		if (fe.type == FrameEvent::FMEVT_VFX_TEXTURE)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			auto vfxDesc = make_shared<VFX_Texture_Desc>();
			vfxDesc->IntParamConvertToVFXDesc(fe.intParam, 0);
			vfxDesc->textures.push_back(CFileConvertor::TEXTURE_PREFIX + fe.strParam);
			shared_ptr<CCharacter> character = Get_Owner().lock();
			vfxDesc->owner = character;

			shared_ptr<CGameObject> gameObject = character->Get_Owner().lock();
			auto reqHolder = gameObject->Get_Component<CRequestHolder>();
			if (reqHolder.expired())
				continue;

			auto req = make_shared<Request_Spawn_VFXTexture>();
			req->vfx_desc = vfxDesc;
			reqHolder.lock()->Push(req);

			continue;
		}

		if (fe.type == FrameEvent::FMEVT_VFX_TRAIL)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			auto vfxTrail = owner->Get_Owner().lock()->Get_Component<CVFX_Trail>();
			if (vfxTrail.expired())
				continue;

			auto game = CGameInstance::GetInstance();
			auto resource = game->Get_Resource();
			auto path = CFileConvertor::TEXTURE_PREFIX + fe.strParam;
			auto texture = resource.lock()->Load_Texture(path, path);
			
			bool isEnable = fe.intParam[0] != 0;
			vfxTrail.lock()->Set_Enable(isEnable);
			if (isEnable)
				vfxTrail.lock()->Set_Texture(texture);
			
			continue;
		}

		if (fe.type == FrameEvent::FMEVT_PRESET)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			if (fe.intParam[0] == 0)
			{

				vector<int> presetIntParam0{
					0, 50, 50 // pivot
					, 0, 100, 0 // position
					, 0, 0, 0, 100, 100, 100 // pos spd, scale
					, 10000, -1000, 0 // scale spd
					, 0 // rotation => [17]
					, 0 // rotation spd
					, 5 // time 0.05
					, 10 // extinct 0.1
				};

				vector<int> presetIntParam1(presetIntParam0);
				presetIntParam1[16] = 9000;
				vector<int> presetIntParam2(presetIntParam0);
				presetIntParam2[16] = 18000;
				vector<int> presetIntParam3(presetIntParam0);
				presetIntParam3[16] = 27000;

				auto vfxDesc0 = make_shared<VFX_Texture_Desc>();
				auto vfxDesc1 = make_shared<VFX_Texture_Desc>();
				auto vfxDesc2 = make_shared<VFX_Texture_Desc>();
				auto vfxDesc3 = make_shared<VFX_Texture_Desc>();
				vfxDesc0->IntParamConvertToVFXDesc(presetIntParam0, 0);
				vfxDesc1->IntParamConvertToVFXDesc(presetIntParam1, 0);
				vfxDesc2->IntParamConvertToVFXDesc(presetIntParam2, 0);
				vfxDesc3->IntParamConvertToVFXDesc(presetIntParam3, 0);

				string textureStr = "Effects/Flares/FlareScreen1.dds";
				vfxDesc0->textures.push_back(CFileConvertor::TEXTURE_PREFIX + textureStr);
				vfxDesc1->textures.push_back(CFileConvertor::TEXTURE_PREFIX + textureStr);
				vfxDesc2->textures.push_back(CFileConvertor::TEXTURE_PREFIX + textureStr);
				vfxDesc3->textures.push_back(CFileConvertor::TEXTURE_PREFIX + textureStr);
				shared_ptr<CCharacter> character = Get_Owner().lock();
				vfxDesc0->owner = character;
				vfxDesc1->owner = character;
				vfxDesc2->owner = character;
				vfxDesc3->owner = character;

				shared_ptr<CGameObject> gameObject = character->Get_Owner().lock();
				auto reqHolder = gameObject->Get_Component<CRequestHolder>();
				if (reqHolder.expired())
					continue;

				auto req0 = make_shared<Request_Spawn_VFXTexture>();
				auto req1 = make_shared<Request_Spawn_VFXTexture>();
				auto req2 = make_shared<Request_Spawn_VFXTexture>();
				auto req3 = make_shared<Request_Spawn_VFXTexture>();
				auto req4 = make_shared<Request_Telegraph>();
				req0->vfx_desc = vfxDesc0;
				req1->vfx_desc = vfxDesc1;
				req2->vfx_desc = vfxDesc2;
				req3->vfx_desc = vfxDesc3;
				req4->owner = character;
				reqHolder.lock()->Push(req0);
				reqHolder.lock()->Push(req1);
				reqHolder.lock()->Push(req2);
				reqHolder.lock()->Push(req3);
				reqHolder.lock()->Push(req4);

				continue;
			}
			else
			{
				// 걸맞은 preset 세팅이 안되어있음.
				assert(0);
			}

			continue;
		}

		if (fe.type == FrameEvent::FMEVT_CAMERA)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			auto& param = fe.intParam;

			vector<bool> isUse;
			vector<float> middleValue;
			isUse.reserve(4);
			middleValue.reserve(4);

			float duration = static_cast<float>(param[0] * 0.01f);
			middleValue.push_back(param[3] * 0.01f); // 2 3 4
			middleValue.push_back(param[6] * 0.01f);
			middleValue.push_back(param[9] * 0.01f);
			middleValue.push_back(param[12] * 0.01f);
			isUse.push_back(param[14]);
			isUse.push_back(param[16]);
			isUse.push_back(param[18]);
			isUse.push_back(param[20]);

			shared_ptr<CCharacter> character = Get_Owner().lock();
			shared_ptr<CGameObject> gameObject = character->Get_Owner().lock();
			auto reqHolder = gameObject->Get_Component<CRequestHolder>();
			if (reqHolder.expired())
				continue;
			auto req = make_shared<Request_CamRotation>();
			req->owner = character;
			req->isUse = isUse;
			req->middleValue = middleValue;
			req->duration = duration;
			reqHolder.lock()->Push(req);

			continue;
		}
		if (fe.type == FrameEvent::FMEVT_SETPOS)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			auto& param = fe.intParam;
			const float x = fe.intParam[0] * 0.01f;
			const float y = fe.intParam[1] * 0.01f;
			const float z = fe.intParam[2] * 0.01f;

			if (owner->Get_Target().expired())
				continue;
			
			shared_ptr<CCharacter> target = owner->Get_Target().lock();
			auto pos = target->Get_Position();
			owner->Set_Position(pos);
			auto trasnform = owner->Get_Owner().lock()->Get_Transform().lock();
			trasnform->Go_Backward(z);
			trasnform->Go_Left(x);
			trasnform->Go_Down(y);

			continue;
		}
		if (fe.type == FrameEvent::FMEVT_PLAY_SFX)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			auto game = CGameInstance::GetInstance();
			shared_ptr<CCharacter> character = Get_Owner().lock();
			game->Play_Sound3D(fe.strParam, character->Get_Position());

			continue;
		}
		if (fe.type == FrameEvent::FMEVT_INC_COST)
		{
			if (ShouldSkipEvent(curTime, fe, m_vFrameExecuted, i, false)) continue;

			const int incValue = fe.intParam[0];
			const int maxValue = fe.intParam[1];

			auto& charDesc = owner->Get_DescRef();
			charDesc.iCost += incValue;
			if (maxValue < charDesc.iCost)
				charDesc.iCost = maxValue;

			continue;
		}
		
	
	}

	if (0.001f < abs(charDesc->fEventSpeed))
	{
		owner->MoveToFront(charDesc->fEventSpeed * deltaTime);
	}

	// perFrameKeyInput은 매 프레임 초기화
	memset(charDesc->perFrameKeyInput, 0, sizeof(charDesc->perFrameKeyInput));

	// curFrame 갱신을 나중에 해야, 0프레임이 실행
	charDesc->fCurTime += deltaTime;
	if (m_pStateEvent->frameLength / 60.0f <= charDesc->fCurTime)
	{
		memset(charDesc->keyInputRegister, 0, sizeof(charDesc->keyInputRegister));
		charDesc->fCurTime = 0;

		std::fill(m_vFrameExecuted.begin(), m_vFrameExecuted.end(), false);
	}

}

void CState::Control(float deltaTime, Character_Desc* charDesc)
{
	// per-frame + 누적 입력 모두 갱신
	RegistKeyInput(charDesc->perFrameKeyInput);
	RegistKeyInput(charDesc->keyInputRegister);

	if (charDesc->bIsMoveable)
	{
		auto game = CGameInstance::GetInstance();
		_float3 localInput = {};
		if (game->Get_MovementKey(localInput))
		{
			auto owner = Get_Owner().lock();
			_vector camVec = owner->Get_CameraLookVec();
			_vector worldUp = XMVectorSet(0, 1, 0, 0);
			_vector camRight = XMVector3Normalize(XMVector3Cross(worldUp, camVec));

			_vector moveVec = camRight * localInput.x + camVec * localInput.z;
			moveVec = XMVector3Normalize(moveVec);

			owner->LookAtLerp(moveVec, deltaTime, 10.0f);
			owner->MoveTo(moveVec * deltaTime);
		}
	}
}

bool CState::Set(Character_Desc* charDesc, string name, _uint _priority)
{
	auto stateManager = CState_Manager::GetInstance();
	auto nextStateEvent = stateManager->Get_StateEvent(name);
	if (!nextStateEvent)
		return false;
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	Get_Owner().lock()->Set_State(nextState, _priority);
	return true;
}

bool CState::Evade(Enemy_Desc _enemy, Character_Desc* charDesc)
{
	auto game = CGameInstance::GetInstance();
	auto stateManager = CState_Manager::GetInstance();
	_float3 moveVec;
	if (game->Get_MovementKey(moveVec))
	{
		auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_Evade_F);
		if (!nextStateEvent)
			return false; 
		auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
		auto nextParam = make_shared<STATE_PARAM>();
		nextParam->bWriteHitable = true;
		nextParam->_bReadHitable = false;
		Get_Owner().lock()->Set_State(nextState, EVADE_PRIORITY, nextParam);
	}
	else
	{
		auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_Evade_B);
		if (!nextStateEvent)
			return false; 
		auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
		auto nextParam = make_shared<STATE_PARAM>();
		nextParam->bWriteHitable = true;
		nextParam->_bReadHitable = false;
		Get_Owner().lock()->Set_State(nextState, EVADE_PRIORITY, nextParam);
	}
	return true;
}

bool CState::SwitchIn(Enemy_Desc _enemy, Character_Desc* charDesc)
{
	auto stateManager = CState_Manager::GetInstance();
	auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_SwitchIn);
	if (!nextStateEvent)
		return false; 
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	Get_Owner().lock()->Set_State(nextState, SWITCH_PRIORITY);
	return true;
}

bool CState::SwitchOut(Character_Desc* charDesc)
{
	auto stateManager = CState_Manager::GetInstance();
	auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_SwitchOut);
	if (!nextStateEvent)
		return false; 
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	Get_Owner().lock()->Set_State(nextState, SWITCH_PRIORITY);
	return true;
}

bool CState::AssultAid(Character_Desc* charDesc)
{
	auto stateManager = CState_Manager::GetInstance();
	auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_AssultAid);
	if (!nextStateEvent)
		return false;
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	auto nextParam = make_shared<STATE_PARAM>();
	nextParam->bWriteHitable = true;
	nextParam->_bReadHitable = false;
	nextParam->bWriteAssulted = true;
	nextParam->_bReadAssulted = true;
	Get_Owner().lock()->Set_State(nextState, ASSULTAID_PRIORITY, nextParam);
	return true;
}

bool CState::Hited(Character_Desc* charDesc, weak_ptr<Damage_Desc> _dmgDesc)
{
	auto stateManager = CState_Manager::GetInstance();
	_vector xmLook = Get_Owner().lock()->Get_Owner().lock()->Get_Transform().lock()->Get_State(CTransform::STATE_LOOK);

	_float3 sourceLook = _dmgDesc.lock()->lookVec;
	_vector xmSourceLook = XMLoadFloat3(&sourceLook);

	_vector dotVec = XMVector3Dot(xmLook, xmSourceLook);
	float dotResult = XMVectorGetX(dotVec);
	StateEvent* nextStateEvent;
	if (0 > dotResult)
		nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_Hit_H_F);
	else
		nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_Hit_H_B);

	if (!nextStateEvent)
		return false; 
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	Get_Owner().lock()->Set_State(nextState);
	return true;
}

bool CState::Parry(Character_Desc* charDesc)
{
	auto stateManager = CState_Manager::GetInstance();
	auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_Parry);
	if (!nextStateEvent)
		return false;
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	auto nextParam = make_shared<STATE_PARAM>();
	nextParam->bWriteHitable = true;
	nextParam->_bReadHitable = false;
	Get_Owner().lock()->Set_State(nextState, PARRY_PRIORITY, nextParam);
	return true;
}

bool CState::Parried(Character_Desc* charDesc)
{
	auto stateManager = CState_Manager::GetInstance();
	auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_Parried);
	if (!nextStateEvent)
		return false;
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	auto nextParam = make_shared<STATE_PARAM>();
	nextParam->bWriteHitable = true;
	nextParam->_bReadHitable = false;
	Get_Owner().lock()->Set_State(nextState, PARRY_PRIORITY, nextParam);
	return true;
}

bool CState::Ultimate(Character_Desc* charDesc)
{
	auto stateManager = CState_Manager::GetInstance();
	auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_Attack_Special);
	if (!nextStateEvent)
		return false;
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	auto nextParam = make_shared<STATE_PARAM>();
	nextParam->bWriteHitable = true;
	nextParam->_bReadHitable = false;
	Get_Owner().lock()->Set_State(nextState, ULTIMATE_PRIORITY, nextParam);
	return true;
}

bool CState::Died(Character_Desc* charDesc)
{
	charDesc->bIsDead = true;
	auto stateManager = CState_Manager::GetInstance();
	auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_Die);
	if (!nextStateEvent)
		return false;
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	Get_Owner().lock()->Set_State(nextState, DIE_PRIORITY);
	return true;
}

bool CState::Groggy(Character_Desc* charDesc)
{
	auto stateManager = CState_Manager::GetInstance();
	auto nextStateEvent = stateManager->Get_StateEvent(charDesc->strState_Hit_Shake);
	if (!nextStateEvent)
		return false;
	auto nextState = CState::Create(Get_Owner(), Get_Model(), nextStateEvent);
	auto nextParam = make_shared<STATE_PARAM>();
	nextParam->bWriteGroggy = true;
	nextParam->_bReadGroggy = true;

	Get_Owner().lock()->Set_StateImmediately(nextState, nextParam);
	return true;
}

weak_ptr<CCharacter> CState::Get_Owner()
{
	return m_pOwner;;
}

weak_ptr<CSkeletalModel> CState::Get_Model()
{
	return m_pModel;
}

void CState::RegistKeyInput(char* keyInputRegister)
{
	auto game = CGameInstance::GetInstance();
	if (game->Get_DIKeyDown(DIK_Q))
		keyInputRegister[static_cast<int>(KEYINPUT::KEY_Q)] = 1;
	if (game->Get_DIKey(DIK_E))
		keyInputRegister[static_cast<int>(KEYINPUT::KEY_E)] = 1;
	if (game->Get_MouseDown(MOUSEKEYSTATE::DIM_LB))
		keyInputRegister[static_cast<int>(KEYINPUT::KEY_LB)] = 1;
	if (game->Get_MouseDown(MOUSEKEYSTATE::DIM_RB))
		keyInputRegister[static_cast<int>(KEYINPUT::KEY_RB)] = 1;
	if (game->Get_MouseDown(MOUSEKEYSTATE::DIM_MB))
		keyInputRegister[static_cast<int>(KEYINPUT::KEY_MB)] = 1;
	_float3 arrow;
	if (game->Get_MovementKey(arrow))
	{
		if (0.01f < arrow.z) keyInputRegister[static_cast<int>(KEYINPUT::KEY_UP)] = 1;
		if (-0.01f > arrow.z) keyInputRegister[static_cast<int>(KEYINPUT::KEY_DOWN)] = 1;
		if (-0.01f > arrow.x) keyInputRegister[static_cast<int>(KEYINPUT::KEY_LEFT)] = 1;
		if (0.01f < arrow.x) keyInputRegister[static_cast<int>(KEYINPUT::KEY_RIGHT)] = 1;
	}
}

bool CState::Set_NextState(string& stateName, _uint priority)
{
	auto stateManager = CState_Manager::GetInstance();
	StateEvent* nextEvent = stateManager->Get_StateEvent(stateName);
	if (nextEvent)
	{
		auto nextState = CState::Create(m_pOwner, m_pModel, nextEvent);
		Get_Owner().lock()->Set_State(nextState, priority);
		return true;
	}

	assert(false);
	return false;
}
