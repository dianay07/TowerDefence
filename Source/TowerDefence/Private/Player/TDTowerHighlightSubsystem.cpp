#include "Player/TDTowerHighlightSubsystem.h"
#include "Tower/TDTowerPawn.h"
#include "Tower/TDTowerBase.h"
#include "TDFL_Utility.h"
#include "Engine/LocalPlayer.h"

void UTDTowerHighlightSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTDTowerHighlightSubsystem::Deinitialize()
{
	// 남은 하이라이트 복구
	if (ATDTowerBase* Tower = HoveredTower.Get())
	{
		ApplyHighlight(Tower, false);
	}
	HoveredTower.Reset();
	SelectedTower.Reset();

	Super::Deinitialize();
}

// ── Tick ─────────────────────────────────────────────────────────────────

bool UTDTowerHighlightSubsystem::IsTickable() const
{
	const ULocalPlayer* LP = GetLocalPlayer<ULocalPlayer>();
	return LP && LP->GetWorld();
}

UWorld* UTDTowerHighlightSubsystem::GetTickableGameObjectWorld() const
{
	if (const ULocalPlayer* LP = GetLocalPlayer<ULocalPlayer>())
	{
		return LP->GetWorld();
	}
	return nullptr;
}

void UTDTowerHighlightSubsystem::Tick(float DeltaTime)
{
	UpdateHover();
}

// ── Hover ────────────────────────────────────────────────────────────────

void UTDTowerHighlightSubsystem::UpdateHover()
{
	ATDTowerBase* NewTower = Cast<ATDTowerBase>(UTDFL_Utility::GetTowerUnderMouse(GetLocalPlayer()->GetWorld()));
	if (NewTower == HoveredTower.Get()) return;

	SetHoveredTower(NewTower);
}

void UTDTowerHighlightSubsystem::SetHoveredTower(ATDTowerBase* NewTower)
{
	ATDTowerBase* OldTower = HoveredTower.Get();
	if (NewTower == OldTower) return;

	if (IsValid(OldTower))
	{
		ApplyHighlight(OldTower, false);
	}

	HoveredTower = NewTower;

	if (IsValid(NewTower))
	{
		ApplyHighlight(NewTower, true);
	}

	OnHoveredTowerChanged.Broadcast(NewTower, OldTower);
}

// ── Select ───────────────────────────────────────────────────────────────

void UTDTowerHighlightSubsystem::SelectTower(ATDTowerBase* Tower)
{
	ATDTowerBase* OldTower = SelectedTower.Get();
	if (Tower == OldTower) return;

	SelectedTower = Tower;
	OnSelectedTowerChanged.Broadcast(Tower, OldTower);
}

void UTDTowerHighlightSubsystem::UnSelectTower()
{
	ATDTowerBase* OldTower = SelectedTower.Get();
	if (!OldTower) return;

	SelectedTower.Reset();
	OnSelectedTowerChanged.Broadcast(nullptr, OldTower);
}

// ── Highlight 적용 ──────────────────────────────────────────────────────

void UTDTowerHighlightSubsystem::ApplyHighlight(ATDTowerBase* Tower, bool bOn)
{
	if (!IsValid(Tower)) return;

	if (ATDTowerBase* Base = Cast<ATDTowerBase>(Tower))
	{
		Base->SetHighlight(bOn);
	}
}
