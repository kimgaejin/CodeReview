#include "Character.h"

#include "GameObject.h"
#include "State_Manager.h"
#include "Damage_Desc.h"
#include <VFX_Trail.h>
#include <RequestHolder.h>

using namespace Client;

CCharacter::CCharacter(weak_ptr<CGameObject> _owner)
    : CComponent{ _owner }
{
}

CCharacter::~CCharacter()
{
}

weak_ptr<CCharacter> CCharacter::Create(weak_ptr<CGameObject> _owner, weak_ptr<CSkeletalModel> _model, string characterTag)
{
    auto instance = shared_ptr<CCharacter>(new CCharacter(_owner));

    if (auto o = _owner.lock()) {
        o->Add_Component(instance);
        instance->m_pTransform = o->Get_Transform();
        instance->m_pModel = _model;

        auto stateManager = CState_Manager::GetInstance();
        StateEvent* stateEvent = stateManager->Get_StateEvent(characterTag);
        if (stateEvent)
        {
            auto state = CState::Create(instance, _model, stateEvent);
            instance->Set_StateImmediately(state);
        }

        if (FAILED(instance->Initialize()))
        {
            throw runtime_error("CCharacter creating: Initialized Failed");
        }
    }
    else
    {
        throw runtime_error("Component creating: GameObject is missing");
    }

    return instance;
}

void CCharacter::On_Updated(_float fTimeDelta)
{
    if (m_nextState)
    {
        Set_StateImmediately(m_nextState);
        m_nextParam.reset();
    }

    if (m_state)
    {
        m_desc.CurEnergy += fTimeDelta * 200.0f;
        m_desc.fDodgeTimer -= fTimeDelta;
        if (m_desc.bIsGroggy)
        {
            m_desc.iCurImpacted -= fTimeDelta * m_desc.iMaxImpacted / 6.0f;
            if (m_desc.iCurImpacted < 0) 
                m_desc.iCurImpacted = 0;
        }
    }
}

void CCharacter::On_LateUpdated(_float fTimeDelta)
{
    if (m_state)
    {
        m_state->On_Updated(fTimeDelta, &m_desc);
    }
}

HRESULT CCharacter::Initialize()
{
    CGameInstance* game = CGameInstance::GetInstance();
    auto resource = game->Get_Resource().lock();

    wstring texturePath = L"../Bin/Resources/Textures/BattleUI/Status_MainFrame.dds";
    m_desc.IconRoleGeneralTexture = resource->Load_Texture(texturePath, texturePath.c_str());

    return S_OK;
}

void CCharacter::Set_State(shared_ptr<CState> _state, _uint _priority, shared_ptr<STATE_PARAM> _nextParam)
{
    if (m_nextStatePriority <= _priority)
    {
        m_nextState = _state;
        m_nextStatePriority = _priority;
        m_nextParam = _nextParam;
    }
}

void CCharacter::Set_StateImmediately(shared_ptr<CState> _state, shared_ptr<STATE_PARAM> _param)
{
    if (!_state)
        return;

    m_desc.On_ChangedState();

    m_state = _state;
    m_state->Initialize();
    if (_param)
    {
        if (_param->bWriteHitable)
            m_desc.bIsHitable = _param->_bReadHitable;
        // 자신이 그로기가 되었을 때, 해당 캐릭터에게 쓸 수 있는 AssultChance 표기
        if (_param->bWriteGroggy)
        {
            m_desc.bIsGroggy = _param->_bReadGroggy;
            m_desc.iAssultChance = 2;
        }
        // 자신이 Assulat attack을 했을 때, 자신이 AssultAttack을 했음을 저장하여, 연속으로 AssaultAid 하는 것을 방지하는 변수 처리
        if (_param->bWriteAssulted)
        {
            m_desc.bIsAssulted = _param->_bReadAssulted;
        }
    }

    m_nextState.reset();
    m_nextParam.reset();
    m_nextStatePriority = 0;

    auto trail = Get_Owner().lock()->Get_Component<CVFX_Trail>();
    if (!trail.expired())
        trail.lock()->Set_Enable(false);
}

void CCharacter::Set_Visible(bool type)
{
    if (auto m = m_pModel.lock())
        m->Set_Visible(type);
}

void CCharacter::Control(float deltaTime)
{
    if (!m_state) return;

    if (auto owner = Get_Owner().lock())
        m_state->Control(owner->Get_TimeScale() * deltaTime, &m_desc);
}

void CCharacter::MoveTo(_vector worldVec)
{
    worldVec *= m_desc.fMovementSpeed;
    m_pTransform.lock()->MoveTo(worldVec);
}

void CCharacter::MoveToFront(float value)
{
    m_pTransform.lock()->Go_Straight(value);
}

void CCharacter::MoveToBack(float value)
{
    m_pTransform.lock()->Go_Backward(value);
}

void CCharacter::LookAtPos(_fvector vTarget)
{
    LookAtLerpWorld(vTarget, 1.0f, 360.0f);
}

void CCharacter::LookAtLerp(_fvector vTarget, _float fDeltaTime, _float fRotateSpeed)
{
    auto transform = m_pTransform.lock();
    if (!transform) return;

    _vector vPosition = transform->Get_State(CTransform::STATE_POSITION);
    _vector vCurrentLook = transform->Get_State(CTransform::STATE_LOOK);
    _vector vTargetLook = XMVector3Normalize(vTarget);

    // y축 회전만 고려하기 위해 y값 제거
    vCurrentLook.m128_f32[1] = 0.f;
    vTargetLook.m128_f32[1] = 0.f;
    vCurrentLook = XMVector3Normalize(vCurrentLook);
    vTargetLook = XMVector3Normalize(vTargetLook);

    // 0도, 180도 예외 처리
    _float fDot = XMVectorGetX(XMVector3Dot(vCurrentLook, vTargetLook));
    fDot = max(min(fDot, 1.0f), -1.0f);
    _float fAngle = acosf(fDot);

    if (fAngle < 0.001f)
        return;

    _float fDeltaRadian = fRotateSpeed * fDeltaTime;
    if (fDeltaRadian > fAngle)
        fDeltaRadian = fAngle;

    // 회전 방향 결정 (시계 / 반시계)
    // - (acosf 가 절댓값)
    _vector cross = XMVector3Cross(vCurrentLook, vTargetLook);
    _float fSign = XMVectorGetY(cross) >= 0.f ? 1.f : -1.f;

    _matrix RotationMatrix = XMMatrixRotationY(fDeltaRadian * fSign);

    _vector vRight = transform->Get_State(CTransform::STATE_RIGHT);
    _vector vUp = transform->Get_State(CTransform::STATE_UP);
    _vector vLook = transform->Get_State(CTransform::STATE_LOOK);

    transform->Set_State(CTransform::STATE_RIGHT, XMVector3TransformNormal(vRight, RotationMatrix));
    transform->Set_State(CTransform::STATE_UP, XMVector3TransformNormal(vUp, RotationMatrix));
    transform->Set_State(CTransform::STATE_LOOK, XMVector3TransformNormal(vLook, RotationMatrix));
}

void CCharacter::LookAtLerpWorld(_fvector vTarget, _float fDeltaTime, _float fRotateSpeed)
{
    auto transform = m_pTransform.lock();
    if (!transform) return;

    _vector vPosition = transform->Get_State(CTransform::STATE_POSITION);
    _vector vCurrentLook = transform->Get_State(CTransform::STATE_LOOK);
    _vector vTargetLook = XMVector3Normalize(vTarget - vPosition);

    // y축 회전만 고려하기 위해 y값 제거
    vCurrentLook.m128_f32[1] = 0.f;
    vTargetLook.m128_f32[1] = 0.f;
    vCurrentLook = XMVector3Normalize(vCurrentLook);
    vTargetLook = XMVector3Normalize(vTargetLook);

    // 0도, 180도 예외 처리
    _float fDot = XMVectorGetX(XMVector3Dot(vCurrentLook, vTargetLook));
    fDot = max(min(fDot, 1.0f), -1.0f);
    _float fAngle = acosf(fDot);

    if (fAngle < 0.001f)
        return; 

    _float fDeltaRadian = fRotateSpeed * fDeltaTime;
    if (fDeltaRadian > fAngle)
        fDeltaRadian = fAngle;

    // 회전 방향 결정 (시계 / 반시계)
    // - (acosf 가 절댓값)
    _vector cross = XMVector3Cross(vCurrentLook, vTargetLook);
    _float fSign = XMVectorGetY(cross) >= 0.f ? 1.f : -1.f;

    _matrix RotationMatrix = XMMatrixRotationY(fDeltaRadian * fSign);

    _vector vRight = transform->Get_State(CTransform::STATE_RIGHT);
    _vector vUp = transform->Get_State(CTransform::STATE_UP);
    _vector vLook = transform->Get_State(CTransform::STATE_LOOK);

    transform->Set_State(CTransform::STATE_RIGHT, XMVector3TransformNormal(vRight, RotationMatrix));
    transform->Set_State(CTransform::STATE_UP, XMVector3TransformNormal(vUp, RotationMatrix));
    transform->Set_State(CTransform::STATE_LOOK, XMVector3TransformNormal(vLook, RotationMatrix));
}

void CCharacter::Set_CameraLookVec(_vector vec)
{
    m_pCameraLookVec = vec;
}

_vector CCharacter::Get_CameraLookVec()
{
    return m_pCameraLookVec;
}

void CCharacter::Set_Position(_float3 position)
{
    m_pTransform.lock()->Set_Position(position.x, position.y, position.z);
}

_float3 CCharacter::Get_Position()
{
    return m_pTransform.lock()->Get_Position();
}

_float CCharacter::Get_HeadPosition()
{
    return m_desc.CameraHeight * 0.1f;
}

_float3 CCharacter::Get_FocusPosition(float focus)
{
    _float3 pos = m_pTransform.lock()->Get_Position();
    XMFLOAT3 result = {};
    if (0.01f < focus)
    {
        _vector look = m_pTransform.lock()->Get_State(CTransform::STATE_LOOK);
        look = XMVector3Normalize(look) * focus;
        XMStoreFloat3(&result, look);
    }
    float headPos = Get_HeadPosition();
    result.x += pos.x;
    result.y += pos.y + headPos;
    result.z += pos.z;
    return result;
}

_float3 CCharacter::Get_LookAt()
{
    _float3 lookAt;
    XMStoreFloat3(&lookAt, XMVector3Normalize(m_pTransform.lock()->Get_State(CTransform::STATE_LOOK)));
    return lookAt;
}

void CCharacter::Set_Target(weak_ptr<CCharacter> target)
{
    m_desc.targetLock = target;
}

weak_ptr<CCharacter> CCharacter::Get_Target()
{
    return m_desc.targetLock;
}

int CCharacter::Damaged(weak_ptr<Damage_Desc> _dmgDesc)
{
    if (m_desc.bIsDead)
        return 0;

    auto dmgDesc = _dmgDesc.lock();
    auto source = dmgDesc->owner.lock();

    float randomValue = (rand() % 1001 - 500) * 0.01f;
    float damage = dmgDesc->attackPTG * (source->Get_Atk() - m_desc.Defense) * randomValue;

    // 그로기 추가 피해
    if (m_desc.Is_Groggyable())
        damage *= 2.0f;

    if (damage < 0)
    {
        damage = 0;
        return 0; // 데미지가 0인 경우 충격력, 사망 없이 종료
    }
    // HP 감소 처리
    m_desc.CurHp -= damage;
    if (m_desc.CurHp < 0)
        m_desc.CurHp = 0;

    // 충격력 누적 (그로기 중이 아닐 때만)
    if (!m_desc.bIsGroggy)
    {
        int impactedValue = static_cast<int>(dmgDesc->impactPTG * source->Get_Impact());
        m_desc.iCurImpacted += impactedValue;

        if (m_desc.iMaxImpacted <= m_desc.iCurImpacted)
        {
            m_state->Groggy(&m_desc);
        }
    }
   
    // 사망
    if (m_desc.CurHp == 0)
    {
        m_state->Died(&m_desc);
    }

    return damage;
}

void CCharacter::Spawn()
{
    if (m_state)
        m_state->Set(&m_desc, m_desc.strState_Enter, 500);

    m_desc.CurHp = m_desc.MaxHp;
    m_desc.iCurImpacted = 0;
    m_desc.bIsSpawned = true;
    Set_Visible(true);
    Get_Owner().lock()->Set_Enable(true);
}

void CCharacter::Evade(Enemy_Desc enemy)
{
    m_desc.fDodgeTimer = 0.2f;

    m_state->Evade(enemy, &m_desc);
}

void CCharacter::SwitchIn(Enemy_Desc enemy)
{
    m_state->SwitchIn(enemy, &m_desc);
}

void CCharacter::SwitchOut()
{
    m_state->SwitchOut(&m_desc);
}

void CCharacter::AssultAid()
{
    m_state->AssultAid(&m_desc);
}

void CCharacter::Parry()
{
    m_state->Parry(&m_desc);
}

void CCharacter::Parried()
{
    m_state->Parried(&m_desc);
}

void CCharacter::Ultimate()
{
    m_state->Ultimate(&m_desc);
}

void CCharacter::Hit(weak_ptr<Damage_Desc> _dmgDesc, int& damaged)
{
    damaged = 0;
    if (0 < m_desc.fDodgeTimer)
        return;

    if (!m_desc.bIsHitable)
        return;

    damaged = Damaged(_dmgDesc);
    if (0 == damaged)
        return;

    if (m_desc.bIsDead)
        return;

    if (m_desc.Is_Groggyable())
        return;

    m_state->Hited(&m_desc, _dmgDesc);
}

void CCharacter::Wait()
{
    m_state->Set(&m_desc, m_desc.strState_Wait, 0);
}

void CCharacter::Req_EndGame()
{
    m_pModel.lock()->Set_MaterialsEffectParam(3, 1.0f);
}
