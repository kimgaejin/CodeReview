
# GameObject Object Pool (Generic & Extensible)

> 일자: 2023.10
> 언어: C#(.NET / Unity)
> 목표: 게임 오브젝트를 위한 범용적인 오브젝트 풀을 설계하고, 래핑하여 상속 구조로 쉽게 확장할 수 있게 한다.

### 개요

유니티에서 GameObject의 Instantiate/Destroy 함수 반복은 GC 스파이크와 프레임 드랍을 유발할 수 있습니다. 이를 줄이기 위해 게임 오브젝트를 위한 오브젝트 풀을 재사용 가능하게 설계했습니다.

- ObjectPoolWrapper<T>: 개별 인스턴스의 상태를 보관하는 래퍼 (수명/경과시간/사용가능 여부 등)
- ObjectPool<T, Q>: 풀 본체. 사전 생성/대여/반납/수명 만료 자동 회수 루프 제공
- DamageFloaterManager & DamageFlaoterWrapper: UI 텍스트 플로터(데미지 수치)를 예시로 한 도메인 특화 구현

> 핵심 아이디어: Wrapper로 인스턴스 데이터를 표준화하고, ObjectPool에서 공통적인 정책을 제공하여, 도메인별 동작은 상속으로 커스터마이징합니다.

### 폴더/구성
```
/ObjectPool
  ├─ ObjectPool.cs # Instance Wrapper와 ObjectPool 본체
  ├─ DamageFloaterManager.cs # 도메인 예시: 데미지 플로터용 풀
  └─ README.md
```

### Unity ObjectPool<T>와의 차이점

- 수명을 기반으로 한 자동 반환 구조
- 대여중인 객체 목록 제공하여 런타임에 집단 연산이 필요할 경우 유연하게 제작
- 루트 게임 오브젝트를 통해 실제 Instance들을 단일 지점에서 통제

### 성능 / 운영 팁

- expectedSize/SupplyPool()로 초기 생성 부하 줄이기
- 수명 조절: 짧은 플로터/이펙트는 TTL을 짧게 유지해 풀 압박 감소
- GC 최소화: 텍스트·머티리얼 변경 시 재할당/박싱 방지
- 업데이트 부하: _working이 커질 경우 단순 루프이므로 비용이 선형 증가 → TTL/연출로 평균 동시 사용량 제어

### 한계 및 개선 여지

- Resources.Load 에 의존하는 객체 생성
- 수동으로 반환할 수 있도록 Release() 함수 추가
- 동시 사용에 대한 대비가 마련되지 않음

### 사용 예시

##### 1. 초기화
``` csharp
// 예: Canvas 밑에 루트를 만들고, Resources 에서 프리팹 로드
_damageFloater = new DamageFloaterManager();
_damageFloater.Init(
    objectKey: "UI/DamageFloaterPrefab",   // Resources.Load 경로
    parent: canvasTransform,               // 루트 부모
    generalLifeTime: 0.8f,                 // 각 플로터 기본 수명
    expectedSize: 20,                      // 예상 동시 사용량
    rootName: "DamageFloaterPool"
);
```

##### 2. Spawn(대여)

``` csharp
// 필요 시 점화: 풀에서 하나 빼와 활성화
var wrapper = _damageFloater.Set();
wrapper.TMP.text = "+120";
wrapper.Instance.anchoredPosition = screenPos; // 시작 위치 지정
// (필요하다면 wrapper의 도메인 속성들(FloatingSpeed 등)도 즉시 변경 가능)
```

##### 3. Update(자동 회수)

``` csharp
void Update()
{
    _damageFloater.OnUpdated(Time.deltaTime);
}
```