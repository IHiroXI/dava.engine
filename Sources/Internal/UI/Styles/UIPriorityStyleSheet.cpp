#include "UI/Styles/UIPriorityStyleSheet.h"

#include "UI/Styles/UIStyleSheet.h"

namespace DAVA
{
UIPriorityStyleSheet::UIPriorityStyleSheet()
{
}

UIPriorityStyleSheet::UIPriorityStyleSheet(UIStyleSheet* aStyleSheet, int32 aPriority)
    : styleSheet(SafeRetain(aStyleSheet))
    , priority(aPriority)
{
}

UIPriorityStyleSheet::UIPriorityStyleSheet(const UIPriorityStyleSheet& other)
    : styleSheet(other.styleSheet)
    , priority(other.priority)
{
}

UIPriorityStyleSheet::~UIPriorityStyleSheet()
{
}

UIPriorityStyleSheet& UIPriorityStyleSheet::operator=(const UIPriorityStyleSheet& other)
{
    styleSheet = other.styleSheet;
    priority = other.priority;
    return *this;
}

bool UIPriorityStyleSheet::operator<(const UIPriorityStyleSheet& other) const
{
    const int32 score1 = styleSheet->GetScore();
    const int32 score2 = other.styleSheet->GetScore();
    return (score1 == score2) ? priority < other.priority : score1 > score2;
}
}
