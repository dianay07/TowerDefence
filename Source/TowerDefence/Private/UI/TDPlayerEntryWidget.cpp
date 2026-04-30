#include "UI/TDPlayerEntryWidget.h"
#include "Components/TextBlock.h"

void UTDPlayerEntryWidget::SetPlayerName(const FString& Name)
{
	if (Text_PlayerName)
		Text_PlayerName->SetText(FText::FromString(Name));
}
