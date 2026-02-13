// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "UI/Widget/FPSUserWidget.h"

void UFPSUserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet(); // Notify the Blueprint that data is ready!
}
