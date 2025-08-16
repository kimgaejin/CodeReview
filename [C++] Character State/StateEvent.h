#pragma once

#include <vector>
#include <string>

using namespace std;

struct FrameEvent
{
	enum FrameEventType {
		FMEVT_NONE            
		, FMEVT_PLAYANIM             // 애니메이션 재생 요청 (조건식 포함 가능)
		, FMEVT_CANCEALENAVLE        // [legacy] 현재 상태에서 Cancel(캔슬) 가능 여부 설정
		, FMEVT_SETFOCUS             // Camera Focus value 변경
		, FMEVT_MOVEFRONT            // 전방으로 강제 이동(대시, 전진 등)
		, FMEVT_MOVEABLE             // 이동 가능/불가능 토글
		, FMEVT_CHANGESTATE          // 다른 상태로 전환 요청 (조건식 포함)
		, FMEVT_HITABLE              // 피격 가능 여부 설정
		, FMEVT_TELEGRAPHING         // 공격 예고 동작(텔레그래핑) 시작/종료
		, FMEVT_SHADERPROPERTY       // 셰이더 속성 변경 (argName, value)
		, FMEVT_SHADERPROPERTY_SLOPE // 셰이더 속성 변경(슬로프 적용 버전)
		, FMEVT_SPAWN                // 게임오브젝트 스폰 (pos, asset, name)
		, FMEVT_LOCKON               // 캐릭터 회전을 현재 타겟 방향으로 고정
		, FMEVT_RANDSTATE            // 랜덤으로 상태 전환 요청
		, FMEVT_ATTACKEFFECT         // 히트박스가 있는 공격 오브젝트 생성
		, FMEVT_VFX_TEXTURE          // VfxTexture 컴포넌트 부착 오브젝트 생성
		, FMEVT_VFX_PARTICLE         // VfxParticle 컴포넌트 부착 오브젝트 생성
		, FMEVT_VFX_TRAIL            // VfxTrail 활성화 및 텍스처 설정
		, FMEVT_PRESET               // 사전 정의된 프리셋 요청 (CState 등에서 처리)
		, FMEVT_CAMERA               // 카메라 연출/설정 변경
		, FMEVT_SETPOS               // 캐릭터 위치 강제 설정
		, FMEVT_PLAY_SFX             // 사운드 이펙트 재생
		, FMEVT_INC_COST             // 코스트 증가 (자원 회복/증가)
		, FMEVT_END
	};

	static const char* FrameEventCSTR[FrameEventType::FMEVT_END];

	FrameEventType type;
	int frame;
	string strParam;
	vector<int> intParam;
	float time;

	static int g_debugIdentity;
	int debugIdentity;
	FrameEvent()
	{
		debugIdentity = ++g_debugIdentity;
	}

	void Assign_ParamSize()
	{
		int intParamSize = 0;
		switch (type)
		{
		case FrameEvent::FMEVT_PLAYANIM: intParamSize = 5; break;
		case FrameEvent::FMEVT_CHANGESTATE: intParamSize = 5; break;
		case FrameEvent::FMEVT_CANCEALENAVLE: intParamSize = 1; break;
		case FrameEvent::FMEVT_SETFOCUS: intParamSize = 1; break;
		case FrameEvent::FMEVT_MOVEFRONT:intParamSize = 1; break;
		case FrameEvent::FMEVT_MOVEABLE: intParamSize = 1; break;
		case FrameEvent::FMEVT_SHADERPROPERTY: intParamSize = 4; break;
		case FrameEvent::FMEVT_SHADERPROPERTY_SLOPE: intParamSize = 4; break;
		case FrameEvent::FMEVT_LOCKON:intParamSize = 0; break;
		case FrameEvent::FMEVT_RANDSTATE:intParamSize = 1; break;
		case FrameEvent::FMEVT_ATTACKEFFECT:intParamSize = 14; break;
		case FrameEvent::FMEVT_VFX_TEXTURE:intParamSize = 19; break;
		case FrameEvent::FMEVT_VFX_PARTICLE:intParamSize = 40; break;
		case FrameEvent::FMEVT_VFX_TRAIL:intParamSize = 2; break;
		case FrameEvent::FMEVT_PRESET:intParamSize = 1; break;
		case FrameEvent::FMEVT_CAMERA:intParamSize = 22; break;
		case FrameEvent::FMEVT_SETPOS:intParamSize = 3; break;
		case FrameEvent::FMEVT_PLAY_SFX:intParamSize = 2; break;
		case FrameEvent::FMEVT_INC_COST:intParamSize = 2; break;
		}
		intParam.resize(intParamSize);
	}

	// FrameEvent 타입 설정 및 기본 파라미터 초기화
	void Set_Type(FrameEventType _type)
	{
		type = _type;
		Assign_ParamSize(); // 타입에 맞춰 intParam/strParam 크기 조정

		// 타입별 기본값
		switch (_type)
		{
		case FrameEvent::FMEVT_PLAYANIM: break;
		case FrameEvent::FMEVT_CHANGESTATE: break;
		case FrameEvent::FMEVT_CANCEALENAVLE: break;
		case FrameEvent::FMEVT_SETFOCUS: break;
		case FrameEvent::FMEVT_MOVEFRONT:break;
		case FrameEvent::FMEVT_MOVEABLE: break;
		case FrameEvent::FMEVT_SHADERPROPERTY: break;
		case FrameEvent::FMEVT_SHADERPROPERTY_SLOPE:  break;
		case FrameEvent::FMEVT_LOCKON: break;
		case FrameEvent::FMEVT_RANDSTATE: break;
		case FrameEvent::FMEVT_ATTACKEFFECT:
			// 공격 오브젝트의 히트박스
			intParam[0] = 0;   // bool isTargetPosition
			intParam[1] = 0;   // center X
			intParam[2] = 100; // center Y
			intParam[3] = 100; // center Z
			intParam[4] = 100; // extent X
			intParam[5] = 100; // extent Y
			intParam[6] = 100; // extent Z

			// 데미지 배율 및 타입
			intParam[7] = 100; // Attack %
			intParam[8] = 100; // Impact %
			intParam[9] = 0;   // Anomaly Type
			intParam[10] = 100; // Anomaly %
			intParam[11] = 100; // Knockback power
			break;
		case FrameEvent::FMEVT_VFX_PARTICLE:
			strParam = "Effects/Eff_Flare_108_LZL_01.dds";
			intParam[1] = 1;   // 파티클 개수
			intParam[5] = 100; // 시작 스케일 X
			intParam[6] = 100; // 시작 스케일 Y
			intParam[7] = 100; // 시작 스케일 Z
			intParam[38] = 3;  // Age (30프레임 기준 1프레임)
			break;
		case FrameEvent::FMEVT_VFX_TEXTURE:
			strParam = "Effects/Eff_Flare_108_LZL_01.dds";
			intParam[0] = 50;  // pivot X (0~100, 0.5f)
			intParam[1] = 50;  // pivot Y
			intParam[2] = 50;  // pivot Z
			intParam[9] = 100; // 시작 스케일 X
			intParam[10] = 100; // 시작 스케일 Y
			intParam[11] = 100; // 시작 스케일 Z
			intParam[17] = 50;  // 최대 수명
			intParam[18] = 10;  // 소멸 시간
			break;
		case FrameEvent::FMEVT_VFX_TRAIL:
			strParam = "Effects/Eff_Trail_076_e.dds";
			intParam[0] = 1;  // Enable 토글
			intParam[1] = 50; // legacy. trail이 남는 시간 변수
			break;
		case FrameEvent::FMEVT_PRESET: break;
		case FrameEvent::FMEVT_CAMERA:
			intParam[0] = 20;  // 시작 시간(ms 단위, 예: 0.2초)
			intParam[1] = 20;  // 종료 시간(ms 단위)
			{
				const int defaultDistance = 200; // 카메라 거리 기본값
				intParam[11] = defaultDistance;
				intParam[12] = defaultDistance;
				intParam[13] = defaultDistance;
			}
			for (int i = 0; i < 8; ++i)
				intParam[14 + i] = 0; // 구간값 사용 여부
		case FrameEvent::FMEVT_SETPOS:
			intParam[2] = 100; // Y 좌표 (기본 1.0배)
			break;
		case FrameEvent::FMEVT_INC_COST:
			intParam[0] = 1; // 증가량
			break;
		}

		frame = 0; // 기본 프레임 값 초기화
	}

	string Get_HeaderString()
	{
		return string(FrameEventCSTR[type]);
	}

};

struct StateEvent
{
	string name = { "Default State Name" };

	int frameLength = { 100 };
	vector<FrameEvent> frameEvents;

};